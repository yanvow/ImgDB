/**
 * @file dedup.c
 * @brief Manage what happen when there is a duplication of an image int
 * the data base  
 */

#include "dedup.h"


/**
 * Compares two values ​​of SHA to determine if the value is the same
 */
int
equals_SHA(const unsigned char* SHA1, const unsigned char* SHA2)
{
    if (SHA1 == NULL || SHA2 == NULL) {
        return ERR_INVALID_ARGUMENT;
    }
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        if(SHA1[i] != SHA2[i])
            return 1;
    }
    return 0;
}


/**
 * Check if there exist an image in the Image base which has the same content
 *  (by comparing their SHA's ID) that the one at index "index".
 */
int 
do_name_and_content_dedup(struct imgst_file* imgst_file, uint32_t index)
{
    if(imgst_file == NULL){
        return ERR_INVALID_ARGUMENT;
    }

    if(index < 0 || index >= imgst_file->header.max_files){
        return ERR_INVALID_ARGUMENT;
    }

    int content_dedup = 0;
    uint32_t found_copy_index = 0;
    for(uint32_t i = 0; i < imgst_file->header.max_files; ++i){
        if(imgst_file->metadata[i].is_valid == NON_EMPTY){
            if(i != index){
                if(!strcmp(imgst_file->metadata[i].img_id, imgst_file->metadata[index].img_id)){
                    return ERR_DUPLICATE_ID;
                }
                if(!equals_SHA(imgst_file->metadata[i].SHA, imgst_file->metadata[index].SHA)){
                    content_dedup = 1;
                    found_copy_index = i;
                }
            }
        }
    }
    
    if(content_dedup == 0){
        imgst_file->metadata[index].offset[RES_ORIG] = 0;
    }else{
        imgst_file->metadata[index].offset[RES_ORIG] = imgst_file->metadata[found_copy_index].offset[RES_ORIG];
        imgst_file->metadata[index].offset[RES_SMALL] = imgst_file->metadata[found_copy_index].offset[RES_SMALL];
        imgst_file->metadata[index].offset[RES_THUMB] = imgst_file->metadata[found_copy_index].offset[RES_THUMB];
        imgst_file->metadata[index].size[RES_SMALL] = imgst_file->metadata[found_copy_index].size[RES_SMALL];
        imgst_file->metadata[index].size[RES_THUMB] = imgst_file->metadata[found_copy_index].size[RES_THUMB];
    }
    return ERR_NONE;
}
