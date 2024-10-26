CREATE TABLE IF NOT EXISTS clusters (
                    cluster_id SERIAL PRIMARY KEY,
                    x REAL NOT NULL,
                    y REAL NOT NULL,
                    z REAL NOT NULL,
                    propagation_rate REAL,
                    cluster_type INTEGER,
                    energy_level REAL NOT NULL
                    );
CREATE TABLE IF NOT EXISTS neurons (
                    neuron_id SERIAL PRIMARY KEY,
                    x REAL NOT NULL,
                    y REAL NOT NULL,
                    z REAL NOT NULL,
                    propagation_rate REAL,
                    neuron_type INTEGER,
                    energy_level REAL NOT NULL
                    );
