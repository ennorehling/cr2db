-- sqlite
PRAGMA foreign_keys = ON;
PRAGMA user_version = 1;

DROP TABLE IF EXISTS config;
DROP TABLE IF EXISTS terrains;
DROP TABLE IF EXISTS races;
DROP TABLE IF EXISTS building_types;
DROP TABLE IF EXISTS ship_types;
DROP TABLE IF EXISTS regions;
DROP TABLE IF EXISTS factions;
DROP TABLE IF EXISTS ships;
DROP TABLE IF EXISTS buildings;
DROP TABLE IF EXISTS units;
DROP TABLE IF EXISTS messages;
DROP TABLE IF EXISTS region_messages;
DROP TABLE IF EXISTS faction_messages;
DROP TABLE IF EXISTS battles;

CREATE TABLE config (
    key TEXT NOT NULL PRIMARY KEY,
    value TEXT
);

CREATE TABLE terrains (
    id INTEGER NOT NULL PRIMARY KEY,
    name TEXT NOT NULL
);

INSERT INTO terrains (id, name) VALUES (0, "Ozean");

CREATE TABLE races (
    id INTEGER NOT NULL PRIMARY KEY,
    name TEXT NOT NULL
);

CREATE TABLE building_types (
    id INTEGER NOT NULL PRIMARY KEY,
    name TEXT
);

CREATE TABLE ship_types (
    id INTEGER NOT NULL PRIMARY KEY,
    name TEXT
);

CREATE TABLE regions (
    id INTEGER UNIQUE,
    terrain_id INTEGER NOT NULL DEFAULT 0,
    x INTEGER NOT NULL,
    y INTEGER NOT NULL,
    z INTEGER NOT NULL,
    name TEXT,
    data BLOB,
    PRIMARY KEY (x, y, z),
    FOREIGN KEY(terrain_id) REFERENCES terrains(id)
) WITHOUT ROWID;

DROP INDEX IF EXISTS regions_xyp;
DROP INDEX IF EXISTS regions_xyz;

CREATE TABLE factions (
    id INTEGER NOT NULL PRIMARY KEY,
    name TEXT,
    email TEXT,
    data BLOB
);

CREATE TABLE ships (
    id INTEGER NOT NULL PRIMARY KEY,
    region_id INTEGER NOT NULL,
    type INTEGER NOT NULL,
    name TEXT,
    data BLOB,
    FOREIGN KEY(region_id) REFERENCES regions(id),
    FOREIGN KEY(type) REFERENCES ship_types(id)
);

CREATE TABLE buildings (
    id INTEGER NOT NULL PRIMARY KEY,
    region_id INTEGER NOT NULL,
    type INTEGER NOT NULL,
    name TEXT,
    data BLOB,
    FOREIGN KEY(region_id) REFERENCES regions(id),
    FOREIGN KEY(type) REFERENCES building_types(id)
);

CREATE TABLE units (
    id INTEGER NOT NULL PRIMARY KEY,
    region_id INTEGER NOT NULL,
    faction_id INTEGER,
    ship_id INTEGER,
    building_id INTEGER,
    race INTEGER,
    name TEXT,
    orders TEXT,
    data BLOB,
    FOREIGN KEY(race) REFERENCES races(id),
    FOREIGN KEY(ship_id) REFERENCES ships(id),
    FOREIGN KEY(faction_id) REFERENCES factions(id),
    FOREIGN KEY(building_id) REFERENCES buildings(id),
    FOREIGN KEY(region_id) REFERENCES regions(id)
);

CREATE TABLE messages (
    id INTEGER NOT NULL PRIMARY KEY,
    type INTEGER,
    text TEXT NOT NULL
);

DROP INDEX IF EXISTS messages_region_id;
DROP INDEX IF EXISTS messages_faction_id;

CREATE TABLE region_messages (
    message_id INTEGER NOT NULL,
    region_id INTEGER NOT NULL,
    FOREIGN KEY(message_id) REFERENCES messages(id),
    FOREIGN KEY(region_id) REFERENCES regions(id)
);

CREATE TABLE faction_messages (
    message_id INTEGER NOT NULL,
    faction_id INTEGER NOT NULL,
    FOREIGN KEY(message_id) REFERENCES messages(id),
    FOREIGN KEY(faction_id) REFERENCES factions(id)
);

CREATE TABLE battles (
    faction_id INTEGER NOT NULL,
    x INTEGER NOT NULL,
    y INTEGER NOT NULL,
    z INTEGER NOT NULL,
    report TEXT NOT NULL,
    FOREIGN KEY(faction_id) REFERENCES factions(id)
    PRIMARY KEY (faction_id, x, y, z)
);
