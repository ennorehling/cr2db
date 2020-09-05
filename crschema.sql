-- sqlite
DROP TABLE IF EXISTS config;
CREATE TABLE config (
    key TEXT NOT NULL PRIMARY KEY,
    value TEXT
);

DROP TABLE IF EXISTS terrains;
CREATE TABLE terrains (
    id INTEGER NOT NULL PRIMARY KEY,
    name TEXT,
    crname TEXT
);

INSERT INTO terrains (id, name, crname) VALUES (0, 'unknown', 'Unbekannt');
INSERT INTO terrains (id, name, crname) VALUES (1, 'ocean', 'Ozean');
INSERT INTO terrains (id, name, crname) VALUES (2, 'firewall', 'Feuerwand');
INSERT INTO terrains (id, name, crname) VALUES (3, 'glacier', 'Gletscher');
INSERT INTO terrains (id, name, crname) VALUES (4, 'plain', 'Ebene');
INSERT INTO terrains (id, name, crname) VALUES (5, 'swamp', 'Sumpf');
INSERT INTO terrains (id, name, crname) VALUES (6, 'desert', 'Wüste');
INSERT INTO terrains (id, name, crname) VALUES (7, 'packice', 'Packeis');
INSERT INTO terrains (id, name, crname) VALUES (8, 'highland', 'Hochland');
INSERT INTO terrains (id, name, crname) VALUES (9, 'mountain', 'Berge');
INSERT INTO terrains (id, name, crname) VALUES (10, 'volcano', 'Vulkan');
INSERT INTO terrains (id, name, crname) VALUES (11, 'volcano_active', 'Aktiver Vulkan');
INSERT INTO terrains (id, name, crname) VALUES (12, 'iceberg', 'Eisberg');
INSERT INTO terrains (id, name, crname) VALUES (13, 'fog', 'Nebel');
INSERT INTO terrains (id, name, crname) VALUES (14, 'fog_thick', 'Dichter Nebel');

DROP TABLE IF EXISTS building_types;
CREATE TABLE building_types (
    id INTEGER NOT NULL PRIMARY KEY,
    crname TEXT
);

INSERT INTO building_types (id, crname) VALUES (0, 'Unbekannt');

DROP TABLE IF EXISTS ship_types;
CREATE TABLE ship_types (
    id INTEGER NOT NULL PRIMARY KEY,
    crname TEXT
);

INSERT INTO ship_types (id, crname) VALUES (0, 'Unbekannt');

DROP TABLE IF EXISTS regions;
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

DROP TABLE IF EXISTS factions;
CREATE TABLE factions (
    id INTEGER NOT NULL PRIMARY KEY,
    name TEXT,
    email TEXT,
    data BLOB
);

DROP TABLE IF EXISTS ships;
CREATE TABLE ships (
    id INTEGER NOT NULL PRIMARY KEY,
    region_id INTEGER NOT NULL,
    type INTEGER NOT NULL,
    name TEXT,
    data BLOB,
    FOREIGN KEY(region_id) REFERENCES regions(id),
    FOREIGN KEY(type) REFERENCES ship_types(id)
);

DROP TABLE IF EXISTS buildings;
CREATE TABLE buildings (
    id INTEGER NOT NULL PRIMARY KEY,
    region_id INTEGER NOT NULL,
    type INTEGER NOT NULL,
    name TEXT,
    data BLOB,
    FOREIGN KEY(region_id) REFERENCES regions(id),
    FOREIGN KEY(type) REFERENCES building_types(id)
);

DROP TABLE IF EXISTS units;
CREATE TABLE units (
    id INTEGER NOT NULL PRIMARY KEY,
    region_id INTEGER NOT NULL,
    faction_id INTEGER,
    ship_id INTEGER,
    building_id INTEGER,
    name TEXT,
    orders TEXT,
    data BLOB,
    FOREIGN KEY(ship_id) REFERENCES ships(id),
    FOREIGN KEY(faction_id) REFERENCES factions(id),
    FOREIGN KEY(building_id) REFERENCES buildings(id),
    FOREIGN KEY(region_id) REFERENCES regions(id)
);

DROP TABLE IF EXISTS messages;
CREATE TABLE messages (
    id INTEGER NOT NULL PRIMARY KEY,
    type INTEGER,
    region_id INTEGER,
    faction_id INTEGER,
    text TEXT,
    data BLOB,
    FOREIGN KEY(region_id) REFERENCES regions(id)
    FOREIGN KEY(faction_id) REFERENCES factions(id)
);

DROP INDEX IF EXISTS messages_region_id;
CREATE INDEX messages_region_id ON messages (region_id);
DROP INDEX IF EXISTS messages_faction_id;
CREATE INDEX messages_faction_id ON messages (faction_id);

PRAGMA user_version = 2;
