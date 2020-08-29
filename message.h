#pragma once

typedef struct message_attr {
    char *key;
    enum attr_type { ATTR_TEXT, ATTR_NUMBER, ATTR_MULTI } type;
    union {
        char *text;
        int number;
    } value;
} message_attr;

typedef struct message {
    char *text;
    int type;
    int id;
    struct message_attr *args; /* stb_arr */
} message;

void message_free(struct message *msg);
void create_attribute(message_attr *attr, const char *key, enum attr_type type, const char *text, int number);
