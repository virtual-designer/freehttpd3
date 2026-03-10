#ifndef FH_REQUEST_H
#define FH_REQUEST_H

#include <stddef.h>

struct fh_header
{
    const char *name;
    const char *value;
    size_t name_len;
    size_t value_len;
    struct fh_header *next;
};

struct fh_headers
{
    struct fh_header *head;
    struct fh_header *tail;
};

enum fh_method
{
    HTTP_GET,
    HTTP_POST,
    HTTP_PUT,
    HTTP_PATCH,
    HTTP_DELETE,
    HTTP_HEAD,
    HTTP_TRACE,
};

enum fh_http_version
{
    HTTP_VERSION_1_0 = 10,
    HTTP_VERSION_1_1 = 11,
    HTTP_VERSION_2_0 = 20,
};

struct fh_request
{
    enum fh_method method;
    size_t uri_len;
    const char *uri;
    enum fh_http_version version;
    struct fh_headers *headers;
};

void fh_request_print (const struct fh_request *request);

#endif /* FH_REQUEST_H */
