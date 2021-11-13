/**
 * @file lazily_resize.c
 * @brief image_content library: lazily_resize implementation.
 */

#include "image_content.h"

/**
 * @brief Computes the shrinking factor (keeping aspect ratio)
 *
 * @param image The image to be resized.
 * @param max_thumbnail_width The maximum width allowed for resized image creation.
 * @param max_thumbnail_height The maximum height allowed for resized image creation.
 */
double
shrink_value(const VipsImage *image,
             int max_width,
             int max_height)
{
    const double h_shrink =  max_width / (double) image->Xsize ;
    const double v_shrink = max_height / (double) image->Ysize ;
    return h_shrink > v_shrink ? v_shrink : h_shrink ;
}

/**
 * Create a resized image
 * 
*/
int
lazily_resize(int res_code, struct imgst_file* imgst_file, size_t index)
{
    if(imgst_file == NULL) return ERR_INVALID_ARGUMENT;
    if(imgst_file->file == NULL) return ERR_INVALID_ARGUMENT;

    if(res_code == RES_ORIG) return ERR_NONE;

    if(index < 0 || index >= imgst_file->header.max_files || (res_code != RES_SMALL && res_code != RES_THUMB)){
        return ERR_INVALID_ARGUMENT;
    }

    if(imgst_file->metadata[index].offset[res_code] == 0){

        VipsImage* parent = vips_image_new();

        VipsImage** t = (VipsImage**)vips_object_local_array(VIPS_OBJECT(parent), 2);

        VipsImage* original = t[0];
        VipsImage* resized = t[1];

        uint32_t original_size = imgst_file->metadata[index].size[RES_ORIG];

        void* buf = calloc(original_size, sizeof(char));

        if(buf == NULL) {
            g_object_unref(parent);
            return ERR_OUT_OF_MEMORY;
        }

        int seek = fseek(imgst_file->file, imgst_file->metadata[index].offset[RES_ORIG], SEEK_SET);

        if(seek != 0) {
            free(buf);
            buf = NULL;
            g_object_unref(parent);
            return ERR_IO;
        }

        size_t read = fread(buf, original_size, 1, imgst_file->file);

        if(read != 1) {
            free(buf);
            buf = NULL;
            g_object_unref(parent);
            return ERR_IO;
        }

        int load = vips_jpegload_buffer(buf, original_size, &original, NULL);

        if(load == -1) {
            g_object_unref(parent);
            free(buf);
            buf = NULL;
            return ERR_IMGLIB;
        }

        const double ratio = shrink_value(original,
                                          imgst_file->header.res_resized[(res_code * 2)],
                                          imgst_file->header.res_resized[(res_code * 2) + 1]);

        vips_resize(original, &resized, ratio, NULL);

        size_t len = 0;

        void* buffer;

        int save = vips_jpegsave_buffer(resized, &buffer, &len, NULL);

        if(save == -1) {
            g_object_unref(parent);
            free(buffer);
            free(buf);
            buffer = NULL;
            buf = NULL;
            return ERR_IMGLIB;
        }

        seek = fseek(imgst_file->file, 0, SEEK_END);

        if(seek != 0){
            g_object_unref(parent);
            return ERR_IO;
        }

        imgst_file->metadata[index].size[res_code] = len;
        imgst_file->metadata[index].offset[res_code] = ftell(imgst_file->file);

        size_t write = fwrite(buffer, len, 1, imgst_file->file);

        if(write != 1) {
            g_object_unref(parent);
            free(buffer);
            free(buf);
            buffer = NULL;
            buf = NULL;
            return ERR_IO;
        }

        g_object_unref(parent);

        long int offset = sizeof(struct img_metadata) * (index) + sizeof(struct imgst_header);

        if(fseek(imgst_file->file, offset, SEEK_SET) != 0){
            return ERR_IO;
        }

        int w = fwrite(&imgst_file->metadata[index], sizeof(struct img_metadata), 1, imgst_file->file);

        if(w != 1) {
            return ERR_IO;
        }

        free(buffer);
        free(buf);
        buffer = NULL;
        buf = NULL;
    }
    return ERR_NONE;
}


/**
 * Recover the resolution of a JPEG image
 */
int 
get_resolution(uint32_t* height, uint32_t* width, const char* image_buffer, size_t image_size)
{
    VipsObject* parent = VIPS_OBJECT(vips_image_new());

    if(parent == NULL){
        return ERR_IMGLIB;
    }

    VipsImage** t = (VipsImage**)vips_object_local_array(parent, 1);

    if(t == NULL){
        return ERR_IMGLIB;
    }

    VipsImage* image = t[0];

    int load = vips_jpegload_buffer((void*) image_buffer, image_size, &image, NULL);

    if(load == -1){
        return ERR_IMGLIB;
    }

    *width =  (uint32_t)image->Xsize;
    *height = (uint32_t)image->Ysize;
    
    g_object_unref(parent);
    
    return ERR_NONE;
}
