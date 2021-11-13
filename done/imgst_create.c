/**
 * @file imgst_create.c
 * @brief imgStore library: do_create implementation.
 *
 * Finishes to initialize the structure imgst_file and
 * writes it on the disk: it creates the file and put the data of the
 * header and of the metadatas.
 */

#include "imgStore.h"
#include "error.h"

#include <stdio.h>
#include <stdlib.h>


/**
 * @brief Creates the imgStore called imgst_filename. Writes the header and the
 *        preallocated empty metadata array to imgStore file.
 * @param imgst_filename Path to the imgStore file
 * @param imgst_file In memory structure with header and metadata.
 */
int
do_create(const char* filename, struct imgst_file* imgst_file)
{
    
    if (filename == NULL) return ERR_INVALID_ARGUMENT;
    if (imgst_file == NULL) return ERR_INVALID_ARGUMENT;
    

    // Sets the DB header name
    strncpy(imgst_file -> header.imgst_name, CAT_TXT,  MAX_IMGST_NAME);
    imgst_file -> header.imgst_name[MAX_IMGST_NAME] = '\0';

    // initalise the number of files and the version of the base
    imgst_file->header.num_files = 0;
    imgst_file->header.imgst_version = 0;

    //Create a file with filename and overwrite it (if it already exists).
    if(!strncmp("/tmp/", filename, 5)){
        imgst_file->file = fopen(filename, "wb+");
    }else{
        imgst_file->file = fopen(filename, "wb");
    }

    if(imgst_file->file == NULL) {
        return ERR_IO;
    }

    //Allocation for the metadatas, preallocated empty metadata array to imgStore file.
    struct img_metadata* ptr = calloc(imgst_file->header.max_files, sizeof(struct img_metadata));
    if(ptr == NULL) return ERR_OUT_OF_MEMORY;
    memset(ptr,0,sizeof(struct img_metadata)* imgst_file->header.max_files );
    imgst_file->metadata = ptr;
    

    //Ready to write on the file prevously created the header and the metadatas
    size_t written = 0;
    //header:
    size_t x;
    x = fwrite(&imgst_file->header, sizeof(struct imgst_header), 1, imgst_file->file);
    if ( x!= 1) {
        fclose(imgst_file->file);
        return ERR_IO;
    }
    ++written;
    //metadata:
    size_t y;
    y = fwrite(imgst_file->metadata, sizeof(struct img_metadata), imgst_file->header.max_files, imgst_file->file);
    if(y != imgst_file->header.max_files) {
        fclose(imgst_file->file);
        return ERR_IO;
    }
    written += imgst_file->header.max_files;

    // do_close(imgst_file);
    fprintf(stdout, "%zu item(s) written \n", written);

    return ERR_NONE;
}
