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

INSERT INTO terrains (name, crname) VALUES ('unknown', 'Unbekannt');
INSERT INTO terrains (name, crname) VALUES ('ocean', 'Ozean');
INSERT INTO terrains (name, crname) VALUES ('firewall', 'Feuerwand');
INSERT INTO terrains (name, crname) VALUES ('glacier', 'Gletscher');
INSERT INTO terrains (name, crname) VALUES ('plain', 'Ebene');
INSERT INTO terrains (name, crname) VALUES ('swamp', 'Sumpf');
INSERT INTO terrains (name, crname) VALUES ('desert', 'Wüste');
INSERT INTO terrains (name, crname) VALUES ('packice', 'Packeis');
INSERT INTO terrains (name, crname) VALUES ('highland', 'Hochland');
INSERT INTO terrains (name, crname) VALUES ('mountain', 'Berge');
INSERT INTO terrains (name, crname) VALUES ('volcano', 'Vulkan');
INSERT INTO terrains (name, crname) VALUES ('volcano_active', 'Aktiver Vulkan');
INSERT INTO terrains (name, crname) VALUES ('iceberg', 'Eisberg');
INSERT INTO terrains (name, crname) VALUES ('fog', 'Nebel');
INSERT INTO terrains (name, crname) VALUES ('fog_thick', 'Dichter Nebel');

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
    name TEXT,
    data BLOB,
    FOREIGN KEY(region_id) REFERENCES regions(id)
);

DROP TABLE IF EXISTS buildings;
CREATE TABLE buildings (
    id INTEGER NOT NULL PRIMARY KEY,
    region_id INTEGER NOT NULL,
    name TEXT,
    data BLOB,
    FOREIGN KEY(region_id) REFERENCES regions(id)
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
