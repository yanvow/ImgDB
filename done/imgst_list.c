/**
 * @file imgst_list.c
 * @brief Contains the do_list method which print informations about the image store.
 */

#include "imgStore.h"
#include <stdio.h>
#include <json-c/json.h>


/********************************************************************//**
 * List : print the informations about the metadata and the header
 * it prints << empty imgStore >> if the image store does not contain any images.
 */
char* 
do_list(struct imgst_file* file, enum do_list_mode mode)
{
    if(file != NULL) {

        if(mode == STDOUT){
            print_header(&file->header);
            if (file->header.num_files == 0 ) {
                printf("<< empty imgStore >>\n");
            } else {
                for(size_t i = 0; i < file->header.max_files; ++i ) {
                    if(file->metadata[i].is_valid){
                        print_metadata(&file->metadata[i]);
                    }
                }
            }
            return NULL;

        }else if(mode == JSON){

            struct json_object* object = json_object_new_object();
            struct json_object* array = json_object_new_array();

            json_object_object_add(object, "Images", array);

            for(size_t i = 0; i < file->header.max_files; ++i ) {
                if(file->metadata[i].is_valid){
                    struct json_object* string = json_object_new_string(file->metadata[i].img_id);
                    json_object_array_add(array, string);
                }
            }

            char* string2 = json_object_to_json_string(object);
            return string2;
        }else{
            char* error_string = malloc(36);
            strcpy(error_string, "unimplemented do_list output mode");
            return error_string;
        }
    }
    return NULL;
}
