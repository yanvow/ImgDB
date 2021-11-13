/**
 * @file imgStore.h
 * @brief Main header file for imgStore core library.
 *
 * Defines the format of the data structures that will be stored on the disk
 * and provides interface functions.
 *
 * The image imgStore starts with exactly one header structure
 * followed by exactly imgst_header.max_files metadata
 * structures. The actual content is not defined by these structures
 * because it should be stored as raw bytes appended at the end of the
 * imgStore file and addressed by offsets in the metadata structure.
 *
 * @author Mia Primorac
 */

#ifndef IMGSTOREPRJ_IMGSTORE_H
#define IMGSTOREPRJ_IMGSTORE_H

#include "error.h" /* not needed in this very file,
                    * but we provide it here, as it is required by
                    * all the functions of this lib.
                    */
#include <stdio.h> // for FILE
#include <stdint.h> // for uint32_t, uint64_t
#include <openssl/sha.h> // for SHA256_DIGEST_LENGTH
#include <json-c/json.h>

#define CAT_TXT "EPFL ImgStore binary"

/* constraints */
#define MAX_IMGST_NAME  31  // max. size of a ImgStore name
#define MAX_IMG_ID     127  // max. size of an image id
#define MAX_MAX_FILES 100000  // will be increased later in the project (is increasted)

/* For is_valid in imgst_metadata */
#define EMPTY 0
#define NON_EMPTY 1

// imgStore library internal codes for different image resolutions.
#define RES_THUMB 0
#define RES_SMALL 1
#define RES_ORIG  2
#define NB_RES    3
#define NB_RES_ORIG  2


#ifdef __cplusplus
extern "C" {
#endif

struct imgst_header {

    char 		    imgst_name[MAX_IMGST_NAME + 1]; // le nom de la base d'images
    uint32_t 		imgst_version; // version de la base de données
    uint32_t 		num_files; // nombre d'images (valides) présentes dans la base
    const uint32_t 	max_files; //nombre maximal d'images possibles dans la base
    const uint16_t 	res_resized[(NB_RES - 1) * 2]; //tableaux des résolutions maximales des images. ORDRE : « thumbnail », « small »
    uint32_t 		unused_32;
    uint64_t 		unused_64;

};

struct img_metadata {

    char 			img_id[MAX_IMG_ID + 1]; // (nom) de l'image
    unsigned char 	SHA[SHA256_DIGEST_LENGTH];  // hash code de l'image
    uint32_t 		res_orig[NB_RES_ORIG]; // résolution de l'image d'origine
    uint32_t 		size[NB_RES]; //  les tailles mémoire (en octets) des images aux différentes résolutions (« thumbnail », « small » et « original »)
    uint64_t 		offset[NB_RES]; // les positions dans le fichier « base de données d'images » des images aux différentes résolutions possibles (« thumbnail », « small » et « original »)
    uint16_t 		is_valid; // si image est encore utilisée (valeur NON_EMPTY / EMPTY)
    uint16_t 		unused_16;

};

struct imgst_file {

    FILE* 					file;
    struct imgst_header 	header;
    struct img_metadata* 	metadata;

};

/**
 * @brief Prints imgStore header informations.
 *
 * @param header The header to be displayed.
 */
void print_header(const struct imgst_header* header);

/**
 * @brief Prints image metadata informations.
 *
 * @param metadata The metadata of one image.
 */
void print_metadata (const struct img_metadata* metadata);

/**
 * @brief Open imgStore file, read the header and all the metadata.
 *
 * @param imgst_filename Path to the imgStore file
 * @param open_mode Mode for fopen(), eg.: "rb", "rb+", etc.
 * @param imgst_file Structure for header, metadata and file pointer.
 */
int do_open (const char* imgst_filename, const char* open_mode, struct imgst_file* imgst_file);

/**
 * @brief Do some clean-up for imgStore file handling.
 *
 * @param imgst_file Structure for header, metadata and file pointer to be freed/closed.
 */
void do_close (struct imgst_file* imgst_file);

/**
 * @brief List of possible output modes for do_list
 *
 * @param imgst_file Structure for header, metadata and file pointer to be freed/closed.
 */
enum do_list_mode {STDOUT, JSON};

/**
 * @brief Displays (on stdout) imgStore metadata.
 *
 * @param imgst_file In memory structure with header and metadata.
 * @param do_list_mode enum with modes STDOUT and JSON
 * @return list of elements in the imgStore.
 */
char* do_list(struct imgst_file* file, enum do_list_mode mode);

/**
 * @brief Creates the imgStore called imgst_filename. Writes the header and the
 *        preallocated empty metadata array to imgStore file.
 * @param imgst_filename Path to the imgStore file
 * @param imgst_file In memory structure with header and metadata.
 */
int do_create(const char* filename, struct imgst_file* imgst_file);

/**
 * @brief Deletes an image from a imgStore imgStore.
 *
 * Effectively, it only invalidates the is_valid field and updates the
 * metadata.  The raw data content is not erased, it stays where it
 * was (and  new content is always appended to the end; no garbage
 * collection).
 *
 * @param img_id The ID of the image to be deleted.
 * @param imgst_file The main in-memory data structure
 * @return Some error code. 0 if no error.
 */
int do_delete(const char * img_id, struct imgst_file* imgst_file);

/**
 * @brief Transforms resolution string to its int value.
 *
 * @param resolution The resolution string. Shall be "original",
 *        "orig", "thumbnail", "thumb" or "small".
 * @return The corresponding value or -1 if error.
 */
 int resolution_atoi(const char* resolution);

/**
 * @brief Reads the content of an image from a imgStore.
 *
 * @param img_id The ID of the image to be read.
 * @param resolution The desired resolution for the image read.
 * @param image_buffer Location of the location of the image content
 * @param image_size Location of the image size variable
 * @param imgst_file The main in-memory data structure
 * @return Some error code. 0 if no error.
 */
 int do_read(const char* img_id, const int resolution, char** image_buffer, uint32_t* image_size, struct imgst_file* imgst_file);

/**
 * @brief Insert image in the imgStore file
 *
 * @param buffer Pointer to the raw image content
 * @param size Image size
 * @param img_id Image ID
 * @param imgst_file The main in-memory data structure
 * @return Some error code. 0 if no error.
 */
 int do_insert(const char* buffer, const size_t size, const char* img_id, struct imgst_file* imgst_file);

/**
 * @brief Removes the deleted images by moving the existing ones
 *
 * @param imgst_path The path to the imgStore file
 * @param imgst_tmp_bkp_path The path to the a (to be created) temporary imgStore backup file
 * @return Some error code. 0 if no error.
 */
int do_gbcollect(const char *imgst_path, const char *imgst_tmp_bkp_path);

#ifdef __cplusplus
}
#endif
#endif
