DROP TABLE IF EXISTS config;
CREATE TABLE config (
    key TEXT PRIMARY KEY NOT NULL,
    value TEXT
);

DROP TABLE IF EXISTS regions;
CREATE TABLE regions (
    x INT NOT NULL,
    y INT NOT NULL,
    p INT NOT NULL,
    id INT,
    name TEXT,
    terrain TEXT,
    data BLOB
);
CREATE UNIQUE INDEX IF NOT EXISTS regions_xyp ON regions (x, y, p);

DROP TABLE IF EXISTS factions;
CREATE TABLE factions (
    id INT PRIMARY KEY NOT NULL,
    name TEXT,
    email TEXT,
    data BLOB
);

DROP TABLE IF EXISTS ships;
CREATE TABLE ships (
    id INT PRIMARY KEY NOT NULL,
    region_id INT NOT NULL,
    name TEXT,
    data BLOB,
    FOREIGN KEY(region_id) REFERENCES regions(id)
);

DROP TABLE IF EXISTS buildings;
CREATE TABLE buildings (
    id INT PRIMARY KEY NOT NULL,
    region_id INT NOT NULL,
    name TEXT,
    data BLOB,
    FOREIGN KEY(region_id) REFERENCES regions(id)
);

DROP TABLE IF EXISTS units;
CREATE TABLE units (
    id INT PRIMARY KEY NOT NULL,
    region_id INT NOT NULL,
    faction_id INT,
    ship_id INT,
    building_id INT,
    name TEXT,
    orders TEXT,
    data BLOB,
    FOREIGN KEY(ship_id) REFERENCES ships(id),
    FOREIGN KEY(faction_id) REFERENCES factions(id),
    FOREIGN KEY(building_id) REFERENCES buildings(id),
    FOREIGN KEY(region_id) REFERENCES regions(id)
);

PRAGMA user_version = 1;
