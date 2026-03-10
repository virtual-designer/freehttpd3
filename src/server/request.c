#define FH_LOG_MODULE_NAME "request"

#include <stdint.h>

#include "request.h"
#include "log/log.h"

void
fh_request_print (const struct fh_request *request)
{
    fh_log_debug ("HTTP request [%p]:", (void *) request);
    fh_log_debug ("  Method: %i", request->method);
    fh_log_debug ("  URI: (%zu) |%.*s|", request->uri_len,
                  (int) request->uri_len, request->uri);
    fh_log_debug ("  Version: %i", request->version);
    fh_log_debug ("  Headers:");

    for (struct fh_header *header = request->headers->head; header;
         header = header->next)
    {
        fh_log_debug ("    |> %.*s: %.*s", (int) header->name_len, header->name,
                      (int) header->value_len, header->value);
    }
}
