/**
 * @file dedup.h
 * @brief Main header file for file dedup.c
 * 
 * Contais two methods : one which compare two SHA's to
 * determine if they are the same and one and one which use the latter one
 * to check if their is a duplicate image in the base.
 * 
 */
#pragma once

#include "imgStore.h"
#include "error.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <openssl/sha.h>


/**
 * @brief Compares two values ​​of SHA to determine if the value is the same
 *
 * @param SHA1 SHA number one to compare
 * @param SHA2 SHA number two to compare
 * @return 1 if same SHA. 0 if not.
 */
int equals_SHA(const unsigned char* SHA1, const unsigned char* SHA2);


/**
 * @brief Check if there exist an image in the Image base which has the same content
 *  (by comparing their SHA's ID) that the one at index "index".
 *
 * @param imgst_file In memory structure with header and metadata.
 * @param index Image at index index
 * @return Some error code. 0 if no error.
 */
int do_name_and_content_dedup(struct imgst_file* imgst_file, uint32_t index);