#pragma once

typedef struct message_attr {
    char *key;
    struct attr_value {
        char *valuestring;
        char *valuexyz;
        int valueint;
    } value;
} message_attr;

typedef struct message {
    char *text;
    int type;
    int id;
    struct message_attr *attr; /* stb_ds string hash */
} message;

void message_free(struct message *msg);
