/**
 * @file imgst_delete.c
 * @brief imgStore library: do_delete implementation.
 */

#include "imgStore.h"
#include "error.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

/********************************************************************//**
 * Delete an image from a file
 */
int
do_delete(const char* img_id, struct imgst_file* imgst_file)
{
    if(img_id == NULL)return ERR_INVALID_ARGUMENT;
    if (imgst_file == NULL) return ERR_INVALID_ARGUMENT;
    if (imgst_file->metadata == NULL)return ERR_FILE_NOT_FOUND;

    bool found = false;
    struct img_metadata metadata;
    size_t i;
    
    for(i = 0; i < imgst_file->header.max_files && !found; ++i ) {
        if (imgst_file->metadata[i].is_valid == NON_EMPTY){
            if(!strcmp(imgst_file->metadata[i].img_id, img_id)) {
                found = true;
                imgst_file->metadata[i].is_valid = EMPTY;
                metadata = imgst_file->metadata[i];
            }
        }
    }
    if (!found) return ERR_FILE_NOT_FOUND;

    if(imgst_file->file == NULL) {
        return ERR_IO;
    }

    long int offset = sizeof(struct img_metadata) * (i-1) + sizeof(struct imgst_header);

    if (fseek(imgst_file->file,offset, SEEK_SET) != 0) return ERR_IO;

    size_t w1 = fwrite(&metadata, sizeof(struct img_metadata), 1, imgst_file->file);
    if (w1!= 1) return ERR_IO;
    imgst_file->header.num_files -=1;
    imgst_file->header.imgst_version +=1;


    if (fseek(imgst_file->file,0, SEEK_SET)!= 0) return ERR_IO;

    size_t w2 = fwrite(&imgst_file->header, sizeof(struct imgst_header), 1, imgst_file->file);
    if (w2!= 1) return ERR_IO;

    return ERR_NONE;
}
