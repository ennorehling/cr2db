#include "message.h"

#include "stb_ds.h"

#include <strings.h>

void message_free(message *msg) {
    unsigned int i, len;
    for (i = 0, len = stbds_arrlen(msg->args); i != len; ++i) {
        struct message_attr *a = msg->args + i;
        if (a->type != ATTR_NUMBER) {
            free(a->value.text);
        }
        free(a->key);
    }
    stbds_arrfree(msg->args);
    free(msg->text);
}

void create_attribute(message_attr *attr, const char *key, enum attr_type type, const char *text, int number)
{
    attr->key = str_strdup(key);
    attr->type = type;
    if (type == ATTR_NUMBER) {
        attr->value.number = number;
    }
    else {
        attr->value.text = str_strdup(text);
    }
}
