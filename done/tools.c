/* * NOTE: undocumented in Doxygen
 * @file tools.c
 * @brief implementation of several tool functions for imgStore
 *
 * @author Mia Primorac
 */

#include "imgStore.h"

#include <stdint.h> // for uint8_t
#include <stdio.h> // for sprintf
#include <openssl/sha.h> // for SHA256_DIGEST_LENGTH
#include <stdlib.h>

/********************************************************************//**
 * Human-readable SHA
 */
static void
sha_to_string (const unsigned char* SHA, char* sha_string)
{
    if (SHA == NULL) {
        return;
    }
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        sprintf(&sha_string[i*2], "%02x", SHA[i]);
    }

    sha_string[2*SHA256_DIGEST_LENGTH] = '\0';
}

/********************************************************************//**
 * imgStore header display.
 */
void
print_header(const struct imgst_header* header)
{

    printf("*****************************************\n");
    printf("**********IMGSTORE HEADER START**********\n");
    printf("TYPE: %31s \n",
           header->imgst_name);
    printf("VERSION: %" PRIu32 " \n",
           header->imgst_version);
    printf("IMAGE COUNT: %" PRIu32 " \t\t",
           header->num_files);
    printf("MAX IMAGES: %" PRIu32 " \n",
           header->max_files);
    printf("THUMBNAIL: %" PRIu16 " x %" PRIu16 " \t",
           header->res_resized[RES_THUMB * 2], header->res_resized[(RES_THUMB * 2) + 1]);
    printf("SMALL: %" PRIu16 " x %" PRIu16 " \n",
           header->res_resized[RES_SMALL * 2], header->res_resized[(RES_SMALL * 2) + 1]);
    printf("***********IMGSTORE HEADER END***********\n");
    printf("*****************************************\n");
}


/********************************************************************//**
 * Metadata display.
 */
void
print_metadata (const struct img_metadata* metadata)
{
    char sha_printable[2*SHA256_DIGEST_LENGTH+1];
    sha_to_string(metadata->SHA, sha_printable);

    printf("IMAGE ID: %s \n",
           metadata->img_id);
    printf("SHA: %s \n",
           sha_printable);
    printf("VALID: %" PRIu16 " \n",
           metadata->is_valid);
    printf("UNUSED: %" PRIu16 " \n",
           metadata->unused_16);
    printf("OFFSET ORIG. : %" PRIu64 " \t\t",
           metadata->offset[RES_ORIG]);
    printf("SIZE ORIG. : %" PRIu32 "\n",
           metadata->size[RES_ORIG]);
    printf("OFFSET THUMB.: %" PRIu64 " \t\t",
           metadata->offset[RES_THUMB]);
    printf("SIZE THUMB.: %" PRIu16 "\n",
           metadata->size[RES_THUMB]);
    printf("OFFSET SMALL : %" PRIu64 " \t\t",
           metadata->offset[RES_SMALL]);
    printf("SIZE SMALL : %" PRIu16 "\n",
           metadata->size[RES_SMALL]);
    printf("ORIGINAL: %" PRIu32 " x %" PRIu32 "\n",
           metadata->res_orig[0], metadata->res_orig[1]);
    printf("*****************************************\n");
}

/**********************************************************************
 * Open a file which contain an imgst_file
 * Read the header and the metadatas
 */
int
do_open (const char* imgst_filename, const char* open_mode, struct imgst_file* imgst_file)
{

    if (imgst_filename == NULL) return ERR_INVALID_ARGUMENT;
    if (open_mode == NULL) return ERR_INVALID_ARGUMENT;
    if (imgst_file == NULL) return ERR_INVALID_ARGUMENT;


    // open the file
    FILE * fileptr = fopen(imgst_filename, open_mode);
    if(fileptr == NULL) {
       return ERR_IO;
    }
    imgst_file->file = fileptr;

    // Put the content of the file(the header part in imgst_file)
    size_t r1 = fread(&imgst_file->header, sizeof(struct imgst_header),1, imgst_file->file);
    if (r1 != 1) return ERR_IO;
    

    // Allocation for the metadata
    struct img_metadata* ptr = NULL;
    ptr = calloc(imgst_file->header.max_files, sizeof(struct img_metadata));
    if (ptr == NULL) return ERR_OUT_OF_MEMORY;
    imgst_file->metadata = ptr;

    // Put the content of the file(the hmetadataeader part in imgst_file)
    size_t r2 = fread(imgst_file->metadata, sizeof(struct img_metadata), imgst_file->header.max_files,imgst_file->file);
    if (r2 != imgst_file->header.max_files) return ERR_IO;

    return ERR_NONE;
}

/**********************************************************************
 * Close a file
 */
void
do_close (struct imgst_file* imgst_file)
{
    if (imgst_file->metadata != NULL) {
       free(imgst_file->metadata);
       imgst_file->metadata = NULL;
    }
    
    if(imgst_file->file != NULL) {
       fclose(imgst_file->file);
       imgst_file->file = NULL;

    }
}

/**
 * Transforms resolution string to its int value.
 */
int 
resolution_atoi(const char* resolution)
{
	if(resolution == NULL)
			return -1;
	if(!strcmp("thumb", resolution) || !strcmp("thumbnail", resolution) ){
			return RES_THUMB;
    }else if(!strcmp("small", resolution)){
        return RES_SMALL;
    }else if(!strcmp("orig", resolution) || !strcmp("original", resolution)){
        return RES_ORIG;
    }
    return -1;
}
