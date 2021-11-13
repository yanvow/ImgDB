#pragma once

/**
 * @file image_content.h
 * @brief Contain the lazily_resize method
 *
 */


#include "imgStore.h"
#include "error.h"

#include <vips/vips.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/**
 * @brief Create a new image resized with the lazy strategy
 * @param res_code internal code of the resized imaged : THUMB or SMALL
 * @param imgst_file the structure which is given in parameter
 * @param index position/ index of the image to resized
 */
int lazily_resize(int res_code, struct imgst_file* imgst_file, size_t index);


/**
 * @brief Recover the resolution of a JPEG image
 * @param height pointer of the height of the image
 * @param width pointer of the width of the image
 * @param image_buffer memory buffer area to load. 
 * @param image_size image size in imemory
 */
int get_resolution(uint32_t* height, uint32_t* width, const char* image_buffer, size_t image_size);
