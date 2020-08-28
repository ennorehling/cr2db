#include "message.h"

#include "stb_ds.h"

void message_free(message *msg) {
    int i;
    for (i = 0; i != stbds_shlen(msg->attr); ++i) {
        free(msg->attr[i].value.valuestring);
        free(msg->attr[i].value.valuexyz);
        free(msg->attr[i].key);
    }
    stbds_shfree(msg->attr);
    free(msg->text);
}
