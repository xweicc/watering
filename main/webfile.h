#ifndef __WEBFILE_H__
#define __WEBFILE_H__

#include <esp_http_server.h>

typedef struct {
    char *uri;
    int datalen;
    const unsigned char *data;
} http_file_data_t;

extern int http_file_num;
extern httpd_uri_t *http_files[];
esp_err_t html_handler(httpd_req_t *req);

#endif
