/**
 * @file imgst_insert.c
 * @brief Insert an image in the imgStore file
 *
 */

#include "imgStore.h"
#include "error.h"
#include "image_content.h"
#include "dedup.h"

#include <vips/vips.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <openssl/sha.h>

/**
 * Insert an image in the imgStore file
 */
int 
do_insert(const char* buffer, const size_t size, const char* img_id, struct imgst_file* imgst_file)
{
    if(buffer == NULL){
        return ERR_NONE;
    }
    if(size < 0){
        return ERR_NONE;
    }

    if(img_id == NULL){
        return ERR_INVALID_ARGUMENT;
    }
    
    if(imgst_file == NULL)
        return ERR_INVALID_ARGUMENT;

    if(strlen(img_id) > MAX_IMG_ID)
        return ERR_INVALID_IMGID;
    
    if(imgst_file->file == NULL){
        return ERR_FILE_NOT_FOUND;
    }

    if(imgst_file->header.num_files >= imgst_file->header.max_files)
        return ERR_FULL_IMGSTORE;

    uint32_t index = 0;

    for(uint32_t i = 0; i < imgst_file->header.max_files; ++i){
        if(imgst_file->metadata[i].is_valid == EMPTY){
            index = i;
            break;
        }
    }
    
    SHA256((const unsigned char *)buffer, size, imgst_file->metadata[index].SHA);

    strncpy(imgst_file->metadata[index].img_id, img_id, MAX_IMG_ID);
    imgst_file->metadata[index].img_id[MAX_IMG_ID] = '\0';

    imgst_file->metadata[index].is_valid = NON_EMPTY;

    imgst_file->metadata[index].size[RES_ORIG] = (uint32_t) size;
    imgst_file->metadata[index].size[RES_THUMB] = 0;
    imgst_file->metadata[index].size[RES_SMALL] = 0;

    imgst_file->metadata[index].offset[RES_THUMB] = 0;
    imgst_file->metadata[index].offset[RES_SMALL] = 0;

    int ret = do_name_and_content_dedup(imgst_file, index);

    if(ret){
        return ret;
    }

    if(imgst_file->metadata[index].offset[RES_ORIG] == 0){
        if(fseek(imgst_file->file, 0, SEEK_END) != 0)
            return ERR_IO;

        imgst_file->metadata[index].offset[RES_ORIG] = ftell(imgst_file->file);

        size_t write = fwrite(buffer, size, 1, imgst_file->file);

        if(write != 1){
            return ERR_IO;
        }
    }
    
    int reso = get_resolution(&imgst_file->metadata[index].res_orig[1],
        &imgst_file->metadata[index].res_orig[0], buffer, size);

    if(reso != 0){
        return reso;
    }
    
    imgst_file->header.imgst_version += 1;
    imgst_file->header.num_files += 1;

    if(fseek(imgst_file->file, 0, SEEK_SET) != 0) 
        return ERR_IO;

    size_t write = fwrite(&imgst_file->header, sizeof(struct imgst_header), 1, imgst_file->file);

    if(write != 1) {
        return ERR_IO;
    }

    long int offset = sizeof(struct img_metadata) * (index) + sizeof(struct imgst_header);

    if(fseek(imgst_file->file, offset, SEEK_SET) != 0) 
        return ERR_IO;

    write = fwrite(&imgst_file->metadata[index], sizeof(struct img_metadata), 1, imgst_file->file);

    if(write != 1) {
        return ERR_IO;
    }
    return ret;
}
