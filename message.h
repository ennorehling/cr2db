#pragma once

typedef struct message {
    struct message *next;
    unsigned int type;
    int id;
    char *text;
} message;
