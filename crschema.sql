PRAGMA user_version = 1;

DROP TABLE IF EXISTS region;
CREATE TABLE region (
    id INT PRIMARY KEY NOT NULL,
    x INT NOT NULL,
    y INT NOT NULL,
    p INT NOT NULL,
    name TEXT,
    terrain TEXT,
    data BLOB
);

DROP TABLE IF EXISTS faction;
CREATE TABLE region (
    id INT PRIMARY KEY NOT NULL,
    name TEXT,
    email TEXT,
    data BLOB
);

DROP TABLE IF EXISTS ship;
CREATE TABLE ship (
    id INT PRIMARY KEY NOT NULL,
    region_id INT NOT NULL,
    name TEXT,
    data BLOB,
    FOREIGN KEY(region_id) REFERENCES region(id)
);

DROP TABLE IF EXISTS building;
CREATE TABLE building (
    id INT PRIMARY KEY NOT NULL,
    region_id INT NOT NULL,
    name TEXT,
    data BLOB,
    FOREIGN KEY(region_id) REFERENCES region(id)
);

DROP TABLE IF EXISTS unit;
CREATE TABLE unit (
    id INT PRIMARY KEY NOT NULL,
    region_id INT NOT NULL,
    faction_id INT,
    ship_id INT,
    building_id INT,
    name TEXT,
    orders TEXT,
    data BLOB,
    FOREIGN KEY(ship_id) REFERENCES ship(id),
    FOREIGN KEY(faction_id) REFERENCES faction(id),
    FOREIGN KEY(building_id) REFERENCES building(id),
    FOREIGN KEY(region_id) REFERENCES region(id)
);
