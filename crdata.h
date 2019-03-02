#pragma once

struct crdata;
struct cJSON;

struct crdata *crdata_get(struct crdata *cr, int no);
struct cJSON * crdata_get_faction(struct crdata *cr, int no);
struct cJSON * crdata_get_region(struct crdata *cr, int no);
struct cJSON * crdata_get_region_at(struct crdata *cr, int x, int y, int z);

struct cJSON * crdata_get_unit(struct crdata *cr, int no, struct cJSON *region);
struct cJSON * crdata_get_ship(struct crdata *cr, int no, struct cJSON *region);
struct cJSON * crdata_get_building(struct crdata *cr, int no, struct cJSON *region);

char *int_to_id(int no);
char *itoa_base(int value, char * buffer, int radix);
