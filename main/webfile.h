#ifndef __WEBFILE_H__
#define __WEBFILE_H__

typedef struct {
    char *name;
    int datalen;
    unsigned char *data;
} http_file_data_t;

extern int http_file_num;
extern http_file_data_t *http_files[];

#endif
