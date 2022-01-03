#include <cstdio>
#include <cstdlib>
#include <cstdbool>
#include <cstring>

#include <id3v2tag.h>
#include <mpegfile.h>
#include <id3v2frame.h>
#include <id3v2header.h>
#include <attachedpictureframe.h>

/**
 * Shamelessly stolen from:
 * https://rajeevandlinux.wordpress.com/2012/04/24/extract-album-art-from-mp3-files-using-taglib-in-c/
 **/

// extern everything because it's a C program after all
extern "C" {
static const char *taglib_id_picture = "APIC";

#include "shared.h"

#include "artwork.h"

bool xava_util_artwork_update_by_audio_file(const char *url,
        struct artwork *artwork) {
    TagLib::MPEG::File                   mpeg_file(&url[strlen(URI_HEADER_MUSIC)]);
    TagLib::ID3v2::Tag                  *id3v2tag = mpeg_file.ID3v2Tag();
    TagLib::ID3v2::FrameList             frame;
    TagLib::ID3v2::AttachedPictureFrame *pic_frame;

    xavaReturnWarnCondition(id3v2tag == nullptr, true,
            "id3v2 not present in '%s'", url);
    
    // picture frame
    frame = id3v2tag->frameListMap()[taglib_id_picture];
    xavaReturnLogCondition(frame.isEmpty() == true, true,
            "'%s' has no artwork information", url);
    
    for(auto it = frame.begin(); it != frame.end(); ++it) {
        pic_frame = (TagLib::ID3v2::AttachedPictureFrame *)(*it);

        //if(pic_frame->type() ==
        //  TagLib::ID3v2::AttachedPictureFrame::FrontCover) {}
        
        // extract image (in it’s compressed form)
        artwork->size = pic_frame->picture().size();
        artwork->file_data = static_cast<unsigned char*>
            (malloc(artwork->size));

        memcpy(artwork->file_data, pic_frame->picture().data(),
                artwork->size);
    }
    return false;
}

}
