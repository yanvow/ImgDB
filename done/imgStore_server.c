/**
 * @file imgStore_server.c
 * @brief imgStore Server: imgStore server.
 *
 */

#include <signal.h>
#include <vips/vips.h>
#include "mongoose.h"
#include "imgStore.h"

// Handle interrupts, like Ctrl-C
static int s_signo;
static void signal_handler(int signo) {
  s_signo = signo;
}


// ======================================================================
static const char *s_listening_address = "http://localhost:8000";
static const char* imgstore_filename;
static struct imgst_file myfile;
// ======================================================================
void mg_error_msg(struct mg_connection* nc, int error);
void handle_list_call(struct mg_connection *nc, int ev, struct mg_http_message *hm, void *fn_data);
void handle_read_call(struct mg_connection *nc, int ev, struct mg_http_message *hm, void *fn_data);
void handle_delete_call(struct mg_connection *nc, int ev, struct mg_http_message *hm, void *fn_data);
void handle_insert_call(struct mg_connection *nc, int ev, struct mg_http_message *hm, void *fn_data);

// ======================================================================
/**
 * @brief Handles server events (eg HTTP requests).
 */
static void imgst_event_handler(struct mg_connection *nc,
                          int ev,
                          void *ev_data,
                          void *fn_data
                         )
{
    struct mg_http_message *hm = (struct mg_http_message *) ev_data;

    switch (ev) {
    case MG_EV_HTTP_MSG:
        if(mg_http_match_uri(hm, "/imgStore/list")){
            handle_list_call(nc, ev, hm, fn_data);
        }else if(mg_http_match_uri(hm, "/imgStore/read")){
            handle_read_call(nc, ev, hm, fn_data);
        }else if(mg_http_match_uri(hm, "/imgStore/delete")){
            handle_delete_call(nc, ev, hm, fn_data);
        }else if(mg_http_match_uri(hm, "/imgStore/insert")){
            handle_insert_call(nc, ev, hm, fn_data);
        }else{
            struct mg_http_serve_opts opts = {.root_dir = "tests/data"};
            mg_http_serve_dir(nc, ev_data, &opts);
        }
    }
}

void 
handle_list_call(struct mg_connection *nc,
                          int ev,
                          struct mg_http_message *hm,
                          void *fn_data)
{
    char* list = do_list(&myfile, JSON);

    mg_printf(
                nc,
                "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: %zu\r\n\r\n%s", 
                strlen(list), list);
    free(list);
    nc->is_draining = 1;
}

void 
mg_error_msg(struct mg_connection* nc, int error)
{
    mg_http_reply(nc, 500, "", "Error: %s", ERR_MESSAGES[error]);
}

void
handle_read_call(struct mg_connection *nc,
                          int ev,
                          struct mg_http_message *hm,
                          void *fn_data)
{
    int ret = 0;

    char img_id[MAX_IMG_ID + 1];
    char res[12];
    int img = mg_http_get_var(&hm->query, "img_id", img_id, sizeof(img_id));
    if(img <= 0){
        ret = ERR_INVALID_ARGUMENT;
    }else{
        img_id[img] = '\0';
    }

    img = mg_http_get_var(&hm->query, "res", res, sizeof(res));
    if(img <= 0){
        ret = ERR_INVALID_ARGUMENT;
    }else{
        res[img] = '\0';
    }

    if(ret){
        mg_error_msg(nc, ret);
    }else{
        uint32_t image_size;
        char* image_buffer;

        int resolution = resolution_atoi(res);

        if(resolution == -1){
            ret = ERR_RESOLUTIONS;
        }

        if(ret){
            mg_error_msg(nc, ret);
        }else{
            ret = do_read(img_id, resolution, &image_buffer, &image_size, &myfile);

            if(ret){
                mg_error_msg(nc, ret);
            }else{
                mg_printf(
                        nc,
                        "HTTP/1.1 200 OK\r\nContent-Type: image/jpeg\r\nContent-Length: %" PRIu16 "\r\n\r\n", 
                        image_size);

                mg_send(nc, image_buffer, image_size);

                free(image_buffer);
                image_buffer = NULL;
            }
        }
    }
    nc->is_draining = 1;            
}

void
handle_delete_call(struct mg_connection *nc,
                          int ev,
                          struct mg_http_message *hm,
                          void *fn_data)
{
    char img_id[MAX_IMG_ID+1];
    int img = mg_http_get_var(&hm->query, "img_id", img_id, MAX_IMG_ID);
    img_id[img] = '\0';

    int ret = do_delete(img_id, &myfile);
    if(ret){
        mg_error_msg(nc, ret);
    }else{
        mg_printf(
                    nc,
                    "HTTP/1.1 302 Found\r\nLocation: %s/index.html\r\n\r\n",
                    s_listening_address);
    }    
    nc->is_draining = 1;
}

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
        free(*buffer);
        *buffer = NULL;
        return ERR_IO;
    }
    return ERR_NONE;
}

void
handle_insert_call(struct mg_connection *nc,
                          int ev,
                          struct mg_http_message *hm,
                          void *fn_data)
{
    char img_id[MAX_IMG_ID+1];
    char size[16];
    char filename[MAX_IMG_ID+5+1] = "/tmp/";
    size_t img_size;
    int ret = 0;
    
    if(mg_vcasecmp(&hm->method, "POST") == 0){

        if(hm->body.len == 0){
            int img = mg_http_get_var(&hm->query, "name", img_id, sizeof(img_id));
            if(img <= 0){
                ret = ERR_INVALID_IMGID;
            }else if(img_id == NULL){
                ret = ERR_INVALID_IMGID;
            }else{
                strcat(filename, img_id);
                filename[img + 5] = '\0';
                img_id[img] = '\0';
            }
            img = mg_http_get_var(&hm->query, "offset", size, sizeof(size));
            if(img <= 0){
                ret = ERR_INVALID_ARGUMENT;
            }else{
                size[img] = '\0';
            }

            if(ret){
                mg_error_msg(nc, ret);
            }else{
                
                img_size = atoi(size);

                FILE* stream = fopen(filename, "rb");

                if(stream == NULL){
                    ret = ERR_IO;
                    mg_error_msg(nc, ret);
                }else{
                    char* buffer;

                    ret = read_disk_image(stream, &buffer, &img_size);

                    fclose(stream);

                    if(ret){
                        mg_error_msg(nc, ret);
                    }else{
                        ret = do_insert(buffer, img_size, img_id, &myfile);

                        free(buffer);
                        buffer = NULL;

                        if(ret){
                            mg_error_msg(nc, ret);
                        }else{
                            mg_printf(
                                        nc,
                                        "HTTP/1.1 302 Found\r\nLocation: %s/index.html\r\n\r\n",
                                        s_listening_address);
                        }
                    }
                }
            }
        }else{
            mg_http_upload(nc, hm, "/tmp");
        }
    }else{
        mg_http_reply(nc, 500, "", "Not found");
    }

    nc->is_draining = 1;
}

// ======================================================================
int main(int argc, char *argv[])
{
    int ret = 0;

    if (argc < 2) {
        ret = ERR_NOT_ENOUGH_ARGUMENTS;
    } else {

        /* Create server */
        struct mg_mgr mgr;
        mg_mgr_init(&mgr);
        if (mg_http_listen(&mgr, s_listening_address, imgst_event_handler, NULL) == NULL) {
            fprintf(stderr, "Error starting server on address %s\n", s_listening_address);
            return 1;
        }

        if (VIPS_INIT (argv[0])) //takes call name
        vips_error_exit ("unable to start VIPS");

        argc--; argv++; // skips command call name

        imgstore_filename = argv[0];

        // Initialise stuff
        signal(SIGINT, signal_handler);
        signal(SIGTERM, signal_handler);

        // Start infinite event loop
        int ret = do_open(imgstore_filename, "rb+", &myfile);
        if(ret){
            return ret;
        }
        printf("Starting imgStore server on http://localhost:8000 \n");
        print_header(&myfile.header);
        
        while (s_signo == 0) mg_mgr_poll(&mgr, 1000);
        mg_mgr_free(&mgr);
        printf("Exiting imgStore server on \n");
        do_close(&myfile);

        vips_shutdown();
    }

    if (ret) {
        fprintf(stderr, "%s\n", ERR_MESSAGES[ret]);
    }

    return ret;
}
