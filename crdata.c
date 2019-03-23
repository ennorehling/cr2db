#ifdef _MSC_VER
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#endif

#include "crdata.h"
#include "log.h"
#include "crdata.h"

#include "crfile.h"

#include <cJSON.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

typedef struct crdata {
    int no;
    struct cJSON *data;
    struct crdata *next;
} crdata;

#define CRHASHSIZE 31

static crdata * cr_reports[CRHASHSIZE];
static int cr_turn;

char *itoab_r(int value, char *buffer, int radix, size_t len)
{
    char *dst;

    assert(len > 2);
    dst = buffer + len - 2;
    dst[1] = 0;
    if (value != 0) {
        int neg = 0;

        if (value < 0) {
            value = -value;
            neg = 1;
        }
        while (value && dst >= buffer) {
            int x = value % radix;
            value = value / radix;
            if (x < 10) {
                *(dst--) = (char)('0' + x);
            }
            else if ('a' + x - 10 == 'l') {
                *(dst--) = 'L';
            }
            else {
                *(dst--) = (char)('a' + (x - 10));
            }
        }
        if (dst > buffer) {
            if (neg) {
                *(dst) = '-';
            }
            else {
                ++dst;
            }
        }
        else {
            log_error(NULL, "static buffer exhausted, itoab(%d, %d)", value, radix);
            assert(value == 0 || !"itoab: static buffer exhausted");
        }
    }
    else {
        dst[0] = '0';
    }

    return dst;
}

char *int_to_id(int no) {
    static char sbuf[12];
    char *s;
    int i;
#ifdef _MSC_VER
    s = _itoa(no, sbuf, 36);
#else
    s = itoab_r(no, sbuf, 36, sizeof(sbuf));
#endif
    for (i = 0; s[i]; ++i) {
        if (s[i] == 'l') s[i] = 'L';
    }
    return s;
}

crdata *crdata_get(int no) {
    int h = no % CRHASHSIZE;
    crdata *cr = cr_reports[h];
    while (cr && cr->no != no) {
        cr = cr->next;
    }
    if (cr == NULL) {
        FILE * F;
        cJSON *data;
        char buffer[16];

        snprintf(buffer, sizeof(buffer), "%d-%s.cr",
            cr_turn, int_to_id(no));
        F = fopen(buffer, "rt");
        if (!F) {
            return NULL;
        }
        data = crfile_read(F, buffer);
        fclose(F);
        cr = calloc(1, sizeof(crdata));
        cr->next = cr_reports[h];
        cr->no = no;
        cr->data = data;
        cr_reports[h] = cr;
    }
    return cr;
}

static cJSON *crdata_get_child_id(cJSON *arr, int no) {
    cJSON *child;
    if (arr != NULL) {
        assert(arr->type == cJSON_Array);
        for (child = arr->child; child; child = child->next) {
            cJSON *item;
            assert(child->type == cJSON_Object);
            item = cJSON_GetObjectItem(child, "id");
            if (item && item->valueint == no) {
                return child;
            }
        }
    }
    return NULL;
}

struct cJSON * crdata_get_faction(struct crdata *cr, int no)
{
    cJSON *arr = cJSON_GetObjectItem(cr->data, "PARTEI");

    return crdata_get_child_id(arr, no);
}

struct cJSON * crdata_get_region(struct crdata *cr, int no) {
    cJSON *arr = cJSON_GetObjectItem(cr->data, "REGION");
    return crdata_get_child_id(arr, no);
}

struct cJSON * crdata_get_region_at(struct crdata *cr, int x, int y, int z) {
    cJSON *child, *arr = cJSON_GetObjectItem(cr->data, "REGION");

    assert(arr->type == cJSON_Array);
    for (child = arr->child; child; child = child->next) {
        cJSON *item;
        assert(child->type == cJSON_Object);
        if (z != 0) {
            item = cJSON_GetObjectItem(item, "z");
            if (item == NULL || item->valueint != z) {
                continue;
            }
        }
        item = cJSON_GetObjectItem(item, "x");
        if (item == NULL || item->valueint != x) {
            continue;
        }
        item = cJSON_GetObjectItem(item, "y");
        if (item == NULL || item->valueint != y) {
            continue;
        }
        return child;
    }
    return NULL;
}

static struct cJSON * crdata_get_region_child(struct crdata *cr, const char *type, int no, struct cJSON *region) {
    cJSON *arr, *child;
    if (region) {
        assert(region->type == cJSON_Object);
        arr = cJSON_GetObjectItem(region, type);
        return crdata_get_child_id(arr, no);
    }
    arr = cJSON_GetObjectItem(region, "REGION");
    for (child = arr->child; child; child = child->next) {
        cJSON *unit = crdata_get_region_child(cr, type, no, child);
        if (unit) {
            return unit;
        }
    }
    return NULL;
}

struct cJSON * crdata_get_unit(struct crdata *cr, int no, struct cJSON *region)
{
    return crdata_get_region_child(cr, "EINHEIT", no, region);
}

struct cJSON * crdata_get_ship(struct crdata *cr, int no, struct cJSON *region) {
    return crdata_get_region_child(cr, "SCHIFF", no, region);
}

struct cJSON * crdata_get_building(struct crdata *cr, int no, struct cJSON *region) {
    return crdata_get_region_child(cr, "BURG", no, region);
}
