#include "log.h"

int dpcp_log_level = -1;

inline void sys_hexdump(void* ptr, int buflen)
{
    unsigned char* buf = (unsigned char*)ptr;
    char out_buf[256];
    int ret = 0;
    int out_pos = 0;
    int i, j;

    log_trace("dump data at %p\n", ptr);
    for (i = 0; i < buflen; i += 16) {
        out_pos = 0;
        ret = sprintf(out_buf + out_pos, "%06x: ", i);
        if (ret < 0) {
            return;
        }
        out_pos += ret;
        for (j = 0; j < 16; j++) {
            if (i + j < buflen) {
                ret = sprintf(out_buf + out_pos, "%02x ", buf[i + j]);
            } else {
                ret = sprintf(out_buf + out_pos, "   ");
            }
            if (ret < 0) {
                return;
            }
            out_pos += ret;
        }
        ret = sprintf(out_buf + out_pos, " ");
        if (ret < 0) {
            return;
        }
        out_pos += ret;
        for (j = 0; j < 16; j++)
            if (i + j < buflen) {
                ret = sprintf(out_buf + out_pos, "%c", isprint(buf[i + j]) ? buf[i + j] : '.');
                if (ret < 0) {
                    return;
                }
                out_pos += ret;
            }
        ret = sprintf(out_buf + out_pos, "\n");
        if (ret < 0) {
            return;
        }
        log_trace("%s", out_buf);
    }
}
