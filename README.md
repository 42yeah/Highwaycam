## Highwaycam

![Featured](/Resources/featured.jpg)

Highwaycam is an ongoing attempt (macOS only) to pretend that you are in front of the camera, when you are not. You know, when you are using Zoom (which might not work, because newer versions of zoom excludes "fraud" cameras)? 

In anyway, this stuff is not done. So come back later! 

### Development suspended

Highwaycam development is stopped, since it looks like me (or my friends) doesn't need them anymore. If anyone still needs it, I am glad to back to work though!

Currently implemented features:

- Camera capture
- Shader pipeline (called "passes") to process the camera capture
- You can add your own shader, too! Just add them in `shaders/`
- Take photo
- Show the processed image as camera output in QuickTime and a list of other apps
- Intuitive way to tweak uniforms in camera
- Delete or duplicate passes

### (Kinda) development roadmap:

- [ ] Texture list
- [ ] Record video
- [ ] Custom video as input

### Libraries

Highwaycam is built on top of a few libraries. They are listed below:

- [GLFW](https://www.glfw.org/)
- [glad](https://glad.dav1d.de/)
- [imgui](https://github.com/ocornut/imgui)
- [OpenCV](https://opencv.org/)
- [stb_image and stb_image_write](https://github.com/nothings/stb)
- [GPAC](https://gpac.wp.imt.fr/)
- [coremediaio-dal-minimal-example](https://github.com/johnboiles/coremediaio-dal-minimal-example)
