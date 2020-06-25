//
//  main.cpp
//  RecordTest
//
//  Created by Hao Zhou on 25/06/2020.
//  Copyright © 2020 John Boiles . All rights reserved.
//

#include <iostream>
#define GPAC_CONFIG_DARWIN
#include <gpac/isomedia.h>
#include <gpac/tools.h>
#include <gpac/compositor.h>


static GF_ISOSample *extracted() {
    GF_ISOSample *iso_sample;
    return iso_sample;
}

int main(int argc, const char * argv[]) {
    /* The ISO progressive reader */
    GF_ISOFile *movie;
    /* Error indicator */
    GF_Err e;
    /* Number of bytes required to finish the current ISO Box reading */
    u64 missing_bytes;
    /* Return value for the program */
    int ret = 0;
    u32 track_id = 1;
    u32 track_number;
    u32 sample_count; 
    u32 sample_index;

    e = gf_isom_open_progressive("Attachments/test.mp4", 0, 0, GF_FALSE, &movie, &missing_bytes);
    if ((e != GF_OK && e != GF_ISOM_INCOMPLETE_FILE) || movie == NULL) {
        fprintf(stdout, "Could not open file %s for reading (%s).\n", argv[1], gf_error_to_string(e));
        return 1;
    }

    track_number = gf_isom_get_track_by_id(movie, track_id);
    if (track_number == 0) {
        fprintf(stdout, "Could not find track ID=%u of file %s.\n", track_id, argv[1]);
        ret = 1;
        goto exit;
    }

    sample_count = gf_isom_get_sample_count(movie, track_number);
    sample_index = 1;
    while (sample_index <= sample_count) {
        GF_ISOSample * iso_sample = extracted();
        u32 sample_description_index;
        iso_sample = gf_isom_get_sample(movie, track_number, sample_index, &sample_description_index);
        if (iso_sample) {
            fprintf(stdout, "Found sample #%5d/%5d of length %8d, RAP: %d, DTS: " LLD ", CTS: " LLD "\n", sample_index, sample_count, iso_sample->dataLength, iso_sample->IsRAP, iso_sample->DTS, iso_sample->DTS+iso_sample->CTS_Offset);
            sample_index++;
            GF_SceneGraph *gf = gf_sg_new();
            gf_sg_get_scene_size_info(<#GF_SceneGraph *sg#>, <#u32 *width#>, <#u32 *height#>)
            /*release the sample data, once you're done with it*/
            gf_isom_sample_del(&iso_sample);
        } else {
            e = gf_isom_last_error(movie);
            if (e == GF_ISOM_INCOMPLETE_FILE) {
                missing_bytes = gf_isom_get_missing_bytes(movie, track_number);
                fprintf(stdout, "Missing " LLU " bytes on input file\n", missing_bytes);
                gf_sleep(1000);
            }
        }
    }

exit:
    gf_isom_close(movie);

    return ret;
}
