//
//  Stream.mm
//  CMIOMinimalSample
//
//  Created by John Boiles  on 4/10/20.
//
//  CMIOMinimalSample is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 2 of the License, or
//  (at your option) any later version.
//
//  CMIOMinimalSample is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with CMIOMinimalSample. If not, see <http://www.gnu.org/licenses/>.

#define PORT 31256

#import "Stream.h"

#import <AppKit/AppKit.h>
#import <mach/mach_time.h>
#include <CoreMediaIO/CMIOSampleBuffer.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#define IPC_IMPLEMENTATION
#include <ipc.h>

#import "Logging.h"

@interface Stream () {
    CMSimpleQueueRef _queue;
    CFTypeRef _clock;
    NSImage *_testImage;
    dispatch_source_t _frameDispatchSource;
    int _sock;
    bool _established;
    int _bufSize;
    int _imgSize;
    char *_buffer;
    NSImage *_image;
}

@property CMIODeviceStreamQueueAlteredProc alteredProc;
@property void * alteredRefCon;
@property (readonly) CMSimpleQueueRef queue;
@property (readonly) CFTypeRef clock;
@property UInt64 sequenceNumber;
@property (readonly) NSImage *testImage;

@end

@implementation Stream

#define FPS 30.0

- (bool) connect {
    if (_sock < 0) {
        _sock = socket(AF_INET, SOCK_STREAM, 0);
    }

    sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_port = htons(PORT);
    sin.sin_addr.s_addr = 16777343; // 127.0.0.1
    int rc = ::connect(_sock, (sockaddr *) &sin, sizeof(sin));
    DLog(@"Attempting to establish connection... %d", rc);
    if (rc >= 0) { DLog(@"Connection established!"); }
    return rc >= 0;
}

- (void) fetchFrame {
    char msg[] = "fetch";
    if (!_established) {
        _established = [self connect];
        if (!_established) { return; }
        return;
    }
    send(_sock, msg, sizeof(msg), 0);

    _imgSize = 0;
    ssize_t len = recv(_sock, &_imgSize, sizeof(_imgSize), 0);
    if (len < 0) {
        _established = false;
        DLog(@"%d: Receive failed: %ld", __LINE__, len);
        return;
    }
    DLog(@"Incoming frame size: %d", _imgSize);
    int received = 0;
    if (_buffer && _imgSize > _bufSize) {
        delete[] _buffer;
        _buffer = nullptr;
    }
    if (_buffer == nullptr) {
        DLog(@"Reallocating buffer: %d", _imgSize);
        _buffer = new char[_imgSize];
        _bufSize = _imgSize;
    }
    while (received < _imgSize) {
        len = recv(_sock, _buffer, _imgSize - received, 0);
        received += len;
        if (len < 0) {
            _established = false;
            DLog(@"%d: Receive failed: %ld", __LINE__, len);
            return;
        }
        DLog(@"Partially received: %ld, tally %d, total %d", len, received, _imgSize);
    }
    DLog(@"Message received. Size: %ld", len);
}

- (instancetype _Nonnull)init {
    self = [super init];
    if (self) {
        _frameDispatchSource = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0,
                                                     dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0));

        dispatch_time_t startTime = dispatch_time(DISPATCH_TIME_NOW, 0);
        uint64_t intervalTime = (int64_t)(NSEC_PER_SEC / FPS);
        dispatch_source_set_timer(_frameDispatchSource, startTime, intervalTime, 0);

        __weak typeof(self) wself = self;
        dispatch_source_set_event_handler(_frameDispatchSource, ^{
            [wself fillFrame];
        });
    }
    
    _buffer = nullptr;
    _sock = -1;
    _imgSize = _bufSize = 0;
    _established = [self connect];
    _image = [NSImage alloc];
    return self;
}

- (void)dealloc {
    DLog(@"Stream Dealloc");
    CMIOStreamClockInvalidate(_clock);
    CFRelease(_clock);
    _clock = NULL;
    CFRelease(_queue);
    _queue = NULL;
    dispatch_suspend(_frameDispatchSource);
}

- (void)startServingFrames {
    dispatch_resume(_frameDispatchSource);
}

- (void)stopServingFrames {
    dispatch_suspend(_frameDispatchSource);
}

- (CMSimpleQueueRef)queue {
    if (_queue == NULL) {
        // Allocate a one-second long queue, which we can use our FPS constant for.
        OSStatus err = CMSimpleQueueCreate(kCFAllocatorDefault, FPS, &_queue);
        if (err != noErr) {
            DLog(@"Err %d in CMSimpleQueueCreate", err);
        }
    }
    return _queue;
}

- (CFTypeRef)clock {
    if (_clock == NULL) {
        OSStatus err = CMIOStreamClockCreate(kCFAllocatorDefault, CFSTR("CMIOMinimalSample::Stream::clock"), (__bridge void *)self,  CMTimeMake(1, 10), 100, 10, &_clock);
        if (err != noErr) {
            DLog(@"Error %d from CMIOStreamClockCreate", err);
        }
    }
    return _clock;
}

- (NSImage *)testImage {
    if (_testImage == nil) {
        NSBundle *bundle = [NSBundle bundleForClass:[self class]];
        _testImage = [bundle imageForResource:@"hi"];
    }
    return _testImage;
}

- (CMSimpleQueueRef)copyBufferQueueWithAlteredProc:(CMIODeviceStreamQueueAlteredProc)alteredProc alteredRefCon:(void *)alteredRefCon {
    self.alteredProc = alteredProc;
    self.alteredRefCon = alteredRefCon;

    // Retain this since it's a copy operation
    CFRetain(self.queue);

    return self.queue;
}

- (CVPixelBufferRef)createPixelBufferWithTestAnimation {
    int width = 1280;
    int height = 720;

    NSDictionary *options = [NSDictionary dictionaryWithObjectsAndKeys:
                             [NSNumber numberWithBool:YES], kCVPixelBufferCGImageCompatibilityKey,
                             [NSNumber numberWithBool:YES], kCVPixelBufferCGBitmapContextCompatibilityKey, nil];
    CVPixelBufferRef pxbuffer = NULL;
    CVReturn status = CVPixelBufferCreate(kCFAllocatorDefault, width, height, kCVPixelFormatType_32ARGB, (__bridge CFDictionaryRef) options, &pxbuffer);

    NSParameterAssert(status == kCVReturnSuccess && pxbuffer != NULL);

    CVPixelBufferLockBaseAddress(pxbuffer, 0);
    void *pxdata = CVPixelBufferGetBaseAddress(pxbuffer);
    NSParameterAssert(pxdata != NULL);

    CGColorSpaceRef rgbColorSpace = CGColorSpaceCreateDeviceRGB();
    CGContextRef context = CGBitmapContextCreate(pxdata, width, height, 8, CVPixelBufferGetBytesPerRow(pxbuffer), rgbColorSpace, kCGImageAlphaPremultipliedFirst | kCGImageByteOrder32Big);
    NSParameterAssert(context);

    double time = double(mach_absolute_time()) / NSEC_PER_SEC;
    CGFloat pos = CGFloat(time - floor(time));

    CGColorRef whiteColor = CGColorCreateGenericRGB(1, 1, 1, 1);
    CGColorRef redColor = CGColorCreateGenericRGB(1, 0, 0, 1);

    CGContextSetFillColorWithColor(context, whiteColor);
    CGContextFillRect(context, CGRectMake(0, 0, width, height));

    CGContextSetFillColorWithColor(context, redColor);
    CGContextFillRect(context, CGRectMake(pos * width, 310, 100, 100));

    CGColorRelease(whiteColor);
    CGColorRelease(redColor);

    CGColorSpaceRelease(rgbColorSpace);
    CGContextRelease(context);

    CVPixelBufferUnlockBaseAddress(pxbuffer, 0);

    return pxbuffer;
}

// Thanks @Thomas Zoechling! https://stackoverflow.com/questions/17481254/how-to-convert-nsdata-object-with-jpeg-data-into-cvpixelbufferref-in-os-x
- (CVPixelBufferRef)newPixelBufferFromNSImage:(NSImage*)image
{
    CGColorSpaceRef colorSpace = CGColorSpaceCreateWithName(kCGColorSpaceGenericRGB);
    NSDictionary* pixelBufferProperties = @{(id)kCVPixelBufferCGImageCompatibilityKey:@YES, (id)kCVPixelBufferCGBitmapContextCompatibilityKey:@YES};
    CVPixelBufferRef pixelBuffer = NULL;
    CVPixelBufferCreate(kCFAllocatorDefault, [image size].width, [image size].height, k32ARGBPixelFormat, (__bridge CFDictionaryRef)pixelBufferProperties, &pixelBuffer);
    CVPixelBufferLockBaseAddress(pixelBuffer, 0);
    void* baseAddress = CVPixelBufferGetBaseAddress(pixelBuffer);
    size_t bytesPerRow = CVPixelBufferGetBytesPerRow(pixelBuffer);
    CGContextRef context = CGBitmapContextCreate(baseAddress, [image size].width, [image size].height, 8, bytesPerRow, colorSpace, kCGImageAlphaNoneSkipFirst);
    NSGraphicsContext* imageContext = [NSGraphicsContext graphicsContextWithGraphicsPort:context flipped:NO];
    [NSGraphicsContext saveGraphicsState];
    [NSGraphicsContext setCurrentContext:imageContext];
    [image compositeToPoint:NSMakePoint(0.0, 0.0) operation:NSCompositeCopy];
    [NSGraphicsContext restoreGraphicsState];
    CVPixelBufferUnlockBaseAddress(pixelBuffer, 0);
    CFRelease(context);
    CGColorSpaceRelease(colorSpace);
    return pixelBuffer;
}

- (void)fillFrame {
    if (CMSimpleQueueGetFullness(self.queue) >= 1.0) {
        DLog(@"Queue is full, bailing out");
        return;
    }
    
    [self fetchFrame];

//    CVPixelBufferRef pixelBuffer = [self createPixelBufferWithTestAnimation];
//    + (nullable UIImage *)imageWithData:(NSData *)data;
    NSData *data = [NSData dataWithBytes:_buffer length:_imgSize];
    _image = [_image initWithData:data];
    CVPixelBufferRef pixelBuffer = [self newPixelBufferFromNSImage:_image];
    // The timing here is quite important. For frames to be delivered correctly and successfully be recorded by apps
    // like QuickTime Player, we need to be accurate in both our timestamps _and_ have a sensible scale. Using large
    // timestamps and scales like mach_absolute_time() and NSEC_PER_SEC will work for display, but will error out
    // when trying to record.
    //
    // Instead, we start our presentation times from zero (using the sequence number as a base), and use a scale that's
    // a multiple of our framerate. This has been observed in parts of AVFoundation and lets us be frame-accurate even
    // on non-round framerates (i.e., we can use a scale of 2997 for 29,97 fps content if we want to).
    CMTimeScale scale = FPS * 100;
    CMTime frameDuration = CMTimeMake(scale / FPS, scale);
    CMTime pts = CMTimeMake(frameDuration.value * self.sequenceNumber, scale);
    CMSampleTimingInfo timing;
    timing.duration = frameDuration;
    timing.presentationTimeStamp = pts;
    timing.decodeTimeStamp = pts;
    OSStatus err = CMIOStreamClockPostTimingEvent(pts, mach_absolute_time(), true, self.clock);
    if (err != noErr) {
        DLog(@"CMIOStreamClockPostTimingEvent err %d", err);
    }

    CMFormatDescriptionRef format;
    CMVideoFormatDescriptionCreateForImageBuffer(kCFAllocatorDefault, pixelBuffer, &format);

    self.sequenceNumber = CMIOGetNextSequenceNumber(self.sequenceNumber);

    CMSampleBufferRef buffer;
    err = CMIOSampleBufferCreateForImageBuffer(
        kCFAllocatorDefault,
        pixelBuffer,
        format,
        &timing,
        self.sequenceNumber,
        kCMIOSampleBufferNoDiscontinuities,
        &buffer
    );
    CFRelease(pixelBuffer);
    CFRelease(format);
    if (err != noErr) {
        DLog(@"CMIOSampleBufferCreateForImageBuffer err %d", err);
    }

    CMSimpleQueueEnqueue(self.queue, buffer);

    // Inform the clients that the queue has been altered
    if (self.alteredProc != NULL) {
        (self.alteredProc)(self.objectId, buffer, self.alteredRefCon);
    }
}

- (CMVideoFormatDescriptionRef)getFormatDescription {
    CMVideoFormatDescriptionRef formatDescription;
    OSStatus err = CMVideoFormatDescriptionCreate(kCFAllocatorDefault, kCMVideoCodecType_422YpCbCr8, 1280, 720, NULL, &formatDescription);
    if (err != noErr) {
        DLog(@"Error %d from CMVideoFormatDescriptionCreate", err);
    }
    return formatDescription;
}

#pragma mark - CMIOObject

- (UInt32)getPropertyDataSizeWithAddress:(CMIOObjectPropertyAddress)address qualifierDataSize:(UInt32)qualifierDataSize qualifierData:(nonnull const void *)qualifierData {
    switch (address.mSelector) {
        case kCMIOStreamPropertyInitialPresentationTimeStampForLinkedAndSyncedAudio:
            return sizeof(CMTime);
        case kCMIOStreamPropertyOutputBuffersNeededForThrottledPlayback:
            return sizeof(UInt32);
        case kCMIOObjectPropertyName:
            return sizeof(CFStringRef);
        case kCMIOObjectPropertyManufacturer:
            return sizeof(CFStringRef);
        case kCMIOObjectPropertyElementName:
            return sizeof(CFStringRef);
        case kCMIOObjectPropertyElementCategoryName:
            return sizeof(CFStringRef);
        case kCMIOObjectPropertyElementNumberName:
            return sizeof(CFStringRef);
        case kCMIOStreamPropertyDirection:
            return sizeof(UInt32);
        case kCMIOStreamPropertyTerminalType:
            return sizeof(UInt32);
        case kCMIOStreamPropertyStartingChannel:
            return sizeof(UInt32);
        case kCMIOStreamPropertyLatency:
            return sizeof(UInt32);
        case kCMIOStreamPropertyFormatDescriptions:
            return sizeof(CFArrayRef);
        case kCMIOStreamPropertyFormatDescription:
            return sizeof(CMFormatDescriptionRef);
        case kCMIOStreamPropertyFrameRateRanges:
            return sizeof(AudioValueRange);
        case kCMIOStreamPropertyFrameRate:
        case kCMIOStreamPropertyFrameRates:
            return sizeof(Float64);
        case kCMIOStreamPropertyMinimumFrameRate:
            return sizeof(Float64);
        case kCMIOStreamPropertyClock:
            return sizeof(CFTypeRef);
        default:
            DLog(@"Stream unhandled getPropertyDataSizeWithAddress for %@", [ObjectStore StringFromPropertySelector:address.mSelector]);
            return 0;
    };
}

- (void)getPropertyDataWithAddress:(CMIOObjectPropertyAddress)address qualifierDataSize:(UInt32)qualifierDataSize qualifierData:(nonnull const void *)qualifierData dataSize:(UInt32)dataSize dataUsed:(nonnull UInt32 *)dataUsed data:(nonnull void *)data {
    switch (address.mSelector) {
        case kCMIOObjectPropertyName:
            *static_cast<CFStringRef*>(data) = CFSTR("CMIOMinimalSample Stream");
            *dataUsed = sizeof(CFStringRef);
            break;
        case kCMIOObjectPropertyElementName:
            *static_cast<CFStringRef*>(data) = CFSTR("CMIOMinimalSample Stream Element");
            *dataUsed = sizeof(CFStringRef);
            break;
        case kCMIOObjectPropertyManufacturer:
        case kCMIOObjectPropertyElementCategoryName:
        case kCMIOObjectPropertyElementNumberName:
        case kCMIOStreamPropertyTerminalType:
        case kCMIOStreamPropertyStartingChannel:
        case kCMIOStreamPropertyLatency:
        case kCMIOStreamPropertyInitialPresentationTimeStampForLinkedAndSyncedAudio:
        case kCMIOStreamPropertyOutputBuffersNeededForThrottledPlayback:
            DLog(@"TODO: %@", [ObjectStore StringFromPropertySelector:address.mSelector]);
            break;
        case kCMIOStreamPropertyDirection:
            *static_cast<UInt32*>(data) = 1;
            *dataUsed = sizeof(UInt32);
            break;
        case kCMIOStreamPropertyFormatDescriptions:
            *static_cast<CFArrayRef*>(data) = (__bridge_retained CFArrayRef)[NSArray arrayWithObject:(__bridge_transfer NSObject *)[self getFormatDescription]];
            *dataUsed = sizeof(CFArrayRef);
            break;
        case kCMIOStreamPropertyFormatDescription:
            *static_cast<CMVideoFormatDescriptionRef*>(data) = [self getFormatDescription];
            *dataUsed = sizeof(CMVideoFormatDescriptionRef);
            break;
        case kCMIOStreamPropertyFrameRateRanges:
            AudioValueRange range;
            range.mMinimum = FPS;
            range.mMaximum = FPS;
            *static_cast<AudioValueRange*>(data) = range;
            *dataUsed = sizeof(AudioValueRange);
            break;
        case kCMIOStreamPropertyFrameRate:
        case kCMIOStreamPropertyFrameRates:
            *static_cast<Float64*>(data) = FPS;
            *dataUsed = sizeof(Float64);
            break;
        case kCMIOStreamPropertyMinimumFrameRate:
            *static_cast<Float64*>(data) = FPS;
            *dataUsed = sizeof(Float64);
            break;
        case kCMIOStreamPropertyClock:
            *static_cast<CFTypeRef*>(data) = self.clock;
            // This one was incredibly tricky and cost me many hours to find. It seems that DAL expects
            // the clock to be retained when returned. It's unclear why, and that seems inconsistent
            // with other properties that don't have the same behavior. But this is what Apple's sample
            // code does.
            // https://github.com/lvsti/CoreMediaIO-DAL-Example/blob/0392cb/Sources/Extras/CoreMediaIO/DeviceAbstractionLayer/Devices/DP/Properties/CMIO_DP_Property_Clock.cpp#L75
            CFRetain(*static_cast<CFTypeRef*>(data));
            *dataUsed = sizeof(CFTypeRef);
            break;
        default:
            DLog(@"Stream unhandled getPropertyDataWithAddress for %@", [ObjectStore StringFromPropertySelector:address.mSelector]);
            *dataUsed = 0;
    };
}

- (BOOL)hasPropertyWithAddress:(CMIOObjectPropertyAddress)address {
    switch (address.mSelector){
        case kCMIOObjectPropertyName:
        case kCMIOObjectPropertyElementName:
        case kCMIOStreamPropertyFormatDescriptions:
        case kCMIOStreamPropertyFormatDescription:
        case kCMIOStreamPropertyFrameRateRanges:
        case kCMIOStreamPropertyFrameRate:
        case kCMIOStreamPropertyFrameRates:
        case kCMIOStreamPropertyMinimumFrameRate:
        case kCMIOStreamPropertyClock:
            return true;
        case kCMIOObjectPropertyManufacturer:
        case kCMIOObjectPropertyElementCategoryName:
        case kCMIOObjectPropertyElementNumberName:
        case kCMIOStreamPropertyDirection:
        case kCMIOStreamPropertyTerminalType:
        case kCMIOStreamPropertyStartingChannel:
        case kCMIOStreamPropertyLatency:
        case kCMIOStreamPropertyInitialPresentationTimeStampForLinkedAndSyncedAudio:
        case kCMIOStreamPropertyOutputBuffersNeededForThrottledPlayback:
            DLog(@"TODO: %@", [ObjectStore StringFromPropertySelector:address.mSelector]);
            return false;
        default:
            DLog(@"Stream unhandled hasPropertyWithAddress for %@", [ObjectStore StringFromPropertySelector:address.mSelector]);
            return false;
    };
}

- (BOOL)isPropertySettableWithAddress:(CMIOObjectPropertyAddress)address {
    DLog(@"Stream unhandled isPropertySettableWithAddress for %@", [ObjectStore StringFromPropertySelector:address.mSelector]);
    return false;
}

- (void)setPropertyDataWithAddress:(CMIOObjectPropertyAddress)address qualifierDataSize:(UInt32)qualifierDataSize qualifierData:(nonnull const void *)qualifierData dataSize:(UInt32)dataSize data:(nonnull const void *)data {
    DLog(@"Stream unhandled setPropertyDataWithAddress for %@", [ObjectStore StringFromPropertySelector:address.mSelector]);
}

@end
