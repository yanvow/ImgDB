/**
 * @file imgst_gccollect.c
 * @brief imgStore library: do_gccollect implementation.
 */

#include "imgStore.h"
#include "image_content.h"
#include "error.h"

#include <stdio.h>
#include <stdlib.h>

int 
do_gbcollect (const char *imgst_path, const char *imgst_tmp_bkp_path)
{
    if (imgst_path == NULL) return ERR_INVALID_ARGUMENT;
    if (imgst_tmp_bkp_path == NULL) return ERR_INVALID_ARGUMENT;

    struct imgst_file myfile;

    int ret = do_open(imgst_path, "rb+", &myfile);

    if(ret) {
        return ret;
    }

    struct imgst_file tmp_imgst = { .header.max_files  = myfile.header.max_files,
                                    .header.res_resized = { myfile.header.res_resized[RES_THUMB * 2], 
                                                            myfile.header.res_resized[(RES_THUMB * 2) + 1], 
                                                            myfile.header.res_resized[RES_SMALL * 2], 
                                                            myfile.header.res_resized[(RES_SMALL * 2) + 1]}};
    tmp_imgst.file = NULL;
    
    ret = do_create(imgst_tmp_bkp_path, &tmp_imgst);

    if(ret) {
        do_close(&myfile);
        do_close(&tmp_imgst);
        return ret;
    }

    uint32_t image_size;
    char* image_buffer;

    for(size_t i = 0; i < myfile.header.max_files; ++i){
        if(myfile.metadata[i].is_valid == NON_EMPTY){
            ret = do_read(myfile.metadata[i].img_id, RES_ORIG, &image_buffer, &image_size, &myfile);
            if(ret) {
                do_close(&myfile);
                do_close(&tmp_imgst);
                return ret;
            }
            ret = do_insert(image_buffer, image_size, myfile.metadata[i].img_id, &tmp_imgst);
            free(image_buffer);
            image_buffer = NULL;
            if(ret) {
                do_close(&myfile);
                do_close(&tmp_imgst);
                return ret;
            }
            if(myfile.metadata[i].offset[RES_THUMB] != 0){
                ret = lazily_resize(RES_THUMB, &tmp_imgst, i);
                if(ret) {
                    do_close(&myfile);
                    do_close(&tmp_imgst);
                    return ret;
                }
            }
            if(myfile.metadata[i].offset[RES_SMALL] != 0){
                ret = lazily_resize(RES_SMALL, &tmp_imgst, i);
                if(ret) {
                    do_close(&myfile);
                    do_close(&tmp_imgst);
                    return ret;
                }
            }
        }
    }

    do_close(&myfile);
    do_close(&tmp_imgst);
    
    ret = remove(imgst_path);
    if(ret){
        return ERR_IO;
    }

    ret = rename(imgst_tmp_bkp_path, imgst_path);
    if(ret){
        return ERR_IO;
    }

    return ERR_NONE;
}