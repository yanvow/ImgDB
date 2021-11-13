/**
 * @file imgStoreMgr.c
 * @brief imgStore Manager: command line interpretor for imgStore core commands.
 *
 * Image Database Management Tool
 *
 * @author Mia Primorac
 */

#include "util.h" // for _unused
#include "imgStore.h"

#include <stdlib.h>
#include <string.h>
#include <vips/vips.h>

#define NB_COMMANDS 7
#define LENGTH_OPTIONAL_CREATE_CMD 10

static const uint16_t max_res_thumb = 128;
static const uint16_t max_res_small = 512;


int do_list_cmd (int args, char* argv[]);
int do_create_cmd (int args, char* argv[]);
int help (int args, char* argv[]);
int do_delete_cmd (int args, char* argv[]);
int do_read_cmd (int args, char* argv[]);
int do_insert_cmd (int args, char* argv[]);
int do_gc_cmd(int args, char* argv[]);

typedef int (*command) (int args, char* argv[]);

struct command_mapping {
    const char* string;
    command command;
};

struct command_mapping commands[NB_COMMANDS] = {
    {"list", do_list_cmd}, 
    {"create", do_create_cmd},
    {"help", help},
    {"delete", do_delete_cmd},
    {"read", do_read_cmd},
    {"insert", do_insert_cmd},
    {"gc", do_gc_cmd}
};

///args = nb d'arguments
///argv[] = tableau de chaine de char [list, filemane]
int do_COMMAND_cmd(int args, char* argv[]);

int help(int args, char* argv[]);

/********************************************************************//**
 * Opens imgStore file and calls do_list command.
 ********************************************************************** */
int
do_list_cmd (int args, char* argv[])
//(const char* imgstore_filename)
{
    if(args < 2){
        return ERR_NOT_ENOUGH_ARGUMENTS;
    }

    const char* imgstore_filename = argv[1];

    struct imgst_file myfile;
    do_open(imgstore_filename, "rb", &myfile);
    char* ls = do_list(&myfile, STDOUT);
    free(ls);
    do_close(&myfile);

    return ERR_NONE;
}

/**********************************************************************
 * Prepares and calls do_create command.
********************************************************************** */
int
do_create_cmd (int args, char* argv[])
//(const char* imgstore_filename)
{
    // Default
    uint32_t max_files   =  10;///MAGIC NB define dans h
    uint16_t thumb_res_x =  64;
    uint16_t thumb_res_y =  64;
    uint16_t small_res_x = 256;
    uint16_t small_res_y = 256;


    const char* imgstore_filename = argv[1];
    for(size_t i = 2; i < args; i++){

        if(!strcmp("-max_files", argv[i])){
            if(args <= i + 1){
                return ERR_NOT_ENOUGH_ARGUMENTS; 
            }
            max_files = atouint32(argv[i + 1]);
            i += 1; 
            if(max_files <= 0 || max_files > MAX_MAX_FILES){
                return ERR_MAX_FILES;
            }
        }else if(!strcmp("-thumb_res", argv[i])){
            if(args <= i + 2){
                return ERR_NOT_ENOUGH_ARGUMENTS;
            }
            thumb_res_x = atouint16(argv[i + 1]);
            thumb_res_y = atouint16(argv[i + 2]);
            i += 2;
            if(thumb_res_x <= 0 || thumb_res_x > max_res_thumb ||
                thumb_res_y <= 0 || thumb_res_y > max_res_thumb){
                return ERR_RESOLUTIONS;
            }

        }else if(!strcmp("-small_res", argv[i])){
            if(args <= i + 2){
                return ERR_NOT_ENOUGH_ARGUMENTS;
            }
            small_res_x = atouint16(argv[i + 1]);
            small_res_y = atouint16(argv[i + 2]);
            i += 2;
            if(small_res_x <= 0 || small_res_x > max_res_small ||
                small_res_y <= 0 || small_res_y > max_res_small){
                return ERR_RESOLUTIONS;
            }
        }else{
            return ERR_INVALID_ARGUMENT;
        }
    }

    puts("Create");

    // Local var myfile  // Initialization of the header of the imgst_file
    struct imgst_file myfile = { .header.max_files  = max_files,
                                 .header.res_resized = {thumb_res_x, thumb_res_y, small_res_x, small_res_y}};
    myfile.file = NULL;
    
    if (imgstore_filename == NULL) return ERR_INVALID_ARGUMENT;
    if (strlen(imgstore_filename) > MAX_IMGST_NAME) return ERR_INVALID_ARGUMENT;
    
    //Call to do_create
    int ret = do_create(imgstore_filename, &myfile);
    if(ret == ERR_NONE) {
        print_header(&myfile.header);
    }

    //Free the allocation and return
    do_close(&myfile);

    return ret;
}

/********************************************************************//**
 * Displays some explanations.
 ********************************************************************** */
int
help (int args, char* argv[])
//(void)
{
    fprintf(stdout, "imgStoreMgr [COMMAND] [ARGUMENTS]\n");
    fprintf(stdout, "   help: displays this help.\n");
    fprintf(stdout, "   list <imgstore_filename>: list imgStore content.\n");
    fprintf(stdout, "   create <imgstore_filename> [options]: create a new imgStore.\n");
    fprintf(stdout, "       options are: \n");
    fprintf(stdout, "           -max_files <MAX_FILES>: maximum number of files. \n");
    fprintf(stdout, "                                   default value is 10 \n");
    fprintf(stdout, "                                   maximum value is 100000 \n");
    fprintf(stdout, "           -thumb_res <X_RES> <Y_RES>: resolution for thumbnail images. \n");
    fprintf(stdout, "                                       default value is 64x64 \n");
    fprintf(stdout, "                                       maximum value is 128x128 \n");
    fprintf(stdout, "           -small_res <X_RES> <Y_RES>: resolution for small images. \n");
    fprintf(stdout, "                                       default value is 256x256 \n");
    fprintf(stdout, "                                       maximum value is 512x512 \n");
    fprintf(stdout, "   read   <imgstore_filename> <imgID> [original|orig|thumbnail|thumb|small]: \n");
    fprintf(stdout, "       read an image from the imgStore and save it to a file. \n");
    fprintf(stdout, "       default resolution is \"original\". \n");
    fprintf(stdout, "   insert <imgstore_filename> <imgID> <filename>: insert a new image in the imgStore. \n");
    fprintf(stdout, "   delete <imgstore_filename> <imgID>: delete image imgID from imgStore.\n");
    fprintf(stdout, "   gc <imgstore_filename> <tmp imgstore_filename>: performs garbage collecting on imgStore. Requires a temporary filename for copying the imgStore.\n");
    return 0;
}

/********************************************************************//**
 * Deletes an image from the imgStore.
 */
int
do_delete_cmd (int args, char* argv[])
//(const char* imgstore_filename, const char* imgID)
{
    if(args < 3){
        return ERR_NOT_ENOUGH_ARGUMENTS;
    }

    const char* imgstore_filename = argv[1];
    const char* imgID = argv[2];

    if(imgID == NULL || strlen(imgID) > MAX_IMG_ID) {
        return ERR_INVALID_IMGID;
    }

    struct imgst_file myfile;
    int ret = do_open(imgstore_filename,"rb+", &myfile);
    if(ret){
        return ret;
    }
    ret = do_delete(imgID, &myfile);
    do_close(&myfile);
    return ret;
}

// ======================================================================
/**
 * @brief Creates a new name in the image_id + resolution_suffix +'.jpg' format
 *
 */
char* 
generate_name(const char* imgID, uint32_t resolution)
{
    char* resolution_suffix;
    char* dot = ".jpg";

    if(resolution == RES_ORIG)
        resolution_suffix = "_orig";
    else if(resolution == RES_SMALL)
        resolution_suffix = "_small";
    else if(resolution == RES_THUMB)
        resolution_suffix = "_thumb";

    char* newname = calloc(sizeof(char),
                            strlen(imgID) + strlen(resolution_suffix) + strlen(dot) + 1);

    strcat(newname, imgID);
    strcat(newname, resolution_suffix);
    strcat(newname, dot);

    return newname;
}

/********************************************************************//**
 * Read an image from disk
 */
int 
read_disk_image(FILE* stream, char** buffer, size_t* image_size)
{
    fseek(stream, 0, SEEK_END);
    *image_size = ftell(stream);
    fseek(stream, 0, SEEK_SET);

    *buffer = calloc(*image_size, sizeof(char));

    if(*buffer == NULL)
        return ERR_IO;

    size_t read = fread(*buffer, *image_size, 1, stream);

    if(read != 1) {
        fclose(stream);
        free(*buffer);
        *buffer = NULL;
        return ERR_IO;
    }

    fclose(stream);
    return ERR_NONE;
}
/********************************************************************//**
 * Write an image in disk
 */
int 
write_disk_image(FILE* stream, char* image_buffer, uint32_t image_size)
{
    size_t write = fwrite(image_buffer, image_size, 1, stream);

    if(write != 1) {
        fclose(stream);
        return ERR_IO;
    }

    fclose(stream);
    return ERR_NONE;
}
/********************************************************************//**
 * Insert an image in the imgStore.
 */
int
do_insert_cmd (int args, char* argv[])
//(char* imgstore_filename, char* imgID, char* filename)
{
    if(args < 4){
        return ERR_NOT_ENOUGH_ARGUMENTS;
    }

    struct imgst_file myfile;

    char* imgstore_filename = argv[1];
    char* imgID = argv[2];
    char* filename = argv[3];

    int ret = do_open(imgstore_filename, "rb+", &myfile);
    if(ret){
        return ret;
    }

    if(myfile.header.num_files >= myfile.header.max_files){
        return ERR_FULL_IMGSTORE;
    }

    size_t image_size;
    char* buffer;

    FILE* stream = fopen(filename, "rb");

    if(stream == NULL) {
        return ERR_IO;
    }

    ret = read_disk_image(stream, &buffer, &image_size);
    if(ret){
        return ret;
    }

    ret = do_insert(buffer, image_size, imgID, &myfile);

    do_close(&myfile);

    free(buffer);
    buffer = NULL;

    return ret;
}

/********************************************************************//**
 * Read an image from the imgStore.
 */
int
do_read_cmd (int args, char* argv[])
//(char* imgstore_filename, char* imgID, uint32_t resolution)
{
    if(args < 3){
        return ERR_NOT_ENOUGH_ARGUMENTS;
    }

    uint32_t resolution = RES_ORIG;

    char* imgstore_filename = argv[1];
    char* imgID = argv[2];

    if(args == 4){
        resolution = resolution_atoi(argv[3]);
        if(resolution == -1){
            return ERR_RESOLUTIONS;
        }
    }
   
    char* newname =  generate_name(imgID, resolution);

    if(newname == NULL){
        return ERR_OUT_OF_MEMORY;
    }

    struct imgst_file myfile;

    int ret = do_open(imgstore_filename, "rb+", &myfile);
    if(ret){
        return ret;
    }

    uint32_t image_size;   
    char* image_buffer;

    ret = do_read(imgID, resolution, &image_buffer, &image_size, &myfile);
    do_close(&myfile);

    if(ret){
        return ret;
    }

    FILE* stream = fopen(newname, "wb");

    if(stream == NULL) {
        free(image_buffer);
        image_buffer = NULL;
        return ERR_IO;
    }

    int write_disk = write_disk_image(stream, image_buffer, image_size);

    free(image_buffer);
    image_buffer = NULL;
    
    free(newname);
    newname = NULL;

    return write_disk;
}

/********************************************************************//**
 * Does the garbage collecting in the imgStore.
 */
int 
do_gc_cmd(int args, char* argv[])
//(char* imgstore_filename, char* tmp_filename)
{
    if(args < 3){
        return ERR_NOT_ENOUGH_ARGUMENTS;
    }

    const char* imgstore_filename = argv[1];
    const char* tmp_filename = argv[2];

    int ret = do_gbcollect (imgstore_filename, tmp_filename);

    return ret;
}

/********************************************************************//**
 * MAIN
 */
int main (int argc, char* argv[])
{
    int ret = 0;

    if (argc < 2) {
        ret = ERR_NOT_ENOUGH_ARGUMENTS;
    } else {
        if (VIPS_INIT (argv[0])) //takes call name
            vips_error_exit ("unable to start VIPS");

        argc--; argv++; // skips command call name

        int found = 1;
        
        for(size_t i = 0; i < NB_COMMANDS; i++){
            if (!strcmp(commands[i].string, argv[0])) {
                found = 0;
                if (argc < 2 && strcmp("help", argv[0])) {
                    ret = ERR_NOT_ENOUGH_ARGUMENTS;
                } else {
                    ret = commands[i].command(argc, argv);
                }
            }
        }
        if(found == 1){
            ret = ERR_INVALID_COMMAND;
        }
        vips_shutdown();
    }
    if (ret) {
        fprintf(stderr, "ERROR: %s\n", ERR_MESSAGES[ret]);
        help(argc, argv);
    }

    return ret;
}
