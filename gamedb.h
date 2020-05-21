#pragma once

struct sqlite3;
struct gamedata;

struct unit;
struct faction;
struct region;
struct ship;
struct building;

int db_open(const char * filename, struct sqlite3 **dbp, int version);
int db_close(struct sqlite3 *db);
/*
int db_load(struct sqlite3 *db, struct gamedata *gd);
int db_write_unit(struct sqlite3 *db, const struct unit *u);
int db_write_ship(struct sqlite3 *db, const struct ship *sh);
int db_write_building(struct sqlite3 *db, const struct building *b);
int db_write_region(struct sqlite3 *db, const struct region *r);
*/

int db_write_faction(struct sqlite3 *db, const struct faction *f);
struct faction *db_read_faction(struct sqlite3 *db, int id);
