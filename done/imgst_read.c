/**
 * @file imgst_read.c
 * @brief Read the content of an image from a imgStore.
 *
 */

#include "imgStore.h"
#include "error.h"
#include "image_content.h"

#include <vips/vips.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/**
 * Reads the content of an image from a imgStore.
 */
int 
do_read(const char* img_id, const int resolution, char** image_buffer, uint32_t* image_size, struct imgst_file* imgst_file)
{
    if(img_id == NULL)
        return ERR_INVALID_ARGUMENT;
    
    if(imgst_file == NULL)
        return ERR_INVALID_ARGUMENT;
    
    if(imgst_file->file == NULL){
        return ERR_FILE_NOT_FOUND;
    }

    if (imgst_file->header.num_files == 0) {
        return ERR_NONE;
    }

    uint32_t index = 0;
    int ret = 0;

    for(size_t i = 0; i < imgst_file->header.max_files; ++i){
        if(imgst_file->metadata[i].is_valid == NON_EMPTY){
            if(!strcmp(imgst_file->metadata[i].img_id, img_id)){
                index = i;
                break;
            }
        }
        if(i == imgst_file->header.max_files - 1){
            return ERR_FILE_NOT_FOUND;
        }
    }

    if(imgst_file->metadata[index].offset[resolution] == 0){
        ret = lazily_resize(resolution, imgst_file, index);
        if(ret){
            return ret;
        }
    }

    int seek = fseek(imgst_file->file, imgst_file->metadata[index].offset[resolution], SEEK_SET);

    if(seek == -1) {
        return ERR_IO;
    }

    *image_size = imgst_file->metadata[index].size[resolution];

    *image_buffer = calloc(*image_size, sizeof(char));

    if(*image_buffer == NULL)
        return ERR_IO;

    size_t read = fread(*image_buffer, *image_size, 1, imgst_file->file);

    if(read != 1) {
        free(*image_buffer);
        *image_buffer = NULL;
        return ERR_IO;
    }

    return ret;
}
