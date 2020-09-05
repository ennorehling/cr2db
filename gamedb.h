#pragma once

struct sqlite3;
struct gamedata;

struct faction;
struct region;
struct unit;
struct building;
struct ship;
struct factions;
struct regions;
struct terrains;

int db_open(const char * filename, struct sqlite3 **dbp, int version);
int db_close(struct sqlite3 *db);
int db_execute(struct sqlite3 *db, const char *sql);
/*
int db_write_unit(struct sqlite3 *db, const struct unit *u);
int db_write_ship(struct sqlite3 *db, const struct ship *sh);
int db_write_building(struct sqlite3 *db, const struct building *b);
*/

int db_read_terrains(struct sqlite3 *db, struct terrains *list);
int db_write_terrains(struct sqlite3 *db, const struct terrains *list);
int db_write_terrain(struct sqlite3 *db, unsigned int id, const struct terrain *t);

int db_write_faction(struct sqlite3 *db, const struct faction *f);
int db_read_faction(struct sqlite3 *db, struct faction *f);

int db_write_region(struct sqlite3 *db, const struct region *r);
int db_read_region(struct sqlite3 *db, struct region *r);
int db_region_delete_objects(struct sqlite3 *db, unsigned int region_id);

int db_factions_walk(struct sqlite3 *db, int(*callback)(struct faction *, void *), void *arg);
int db_regions_walk(struct sqlite3 *db, int(*callback)(struct region *, void *), void *arg);

int db_load_map(struct sqlite3 *db, struct regions *list);
int db_load_factions(struct sqlite3 *db, struct factions *list);
