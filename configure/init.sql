CREATE TABLE IF NOT EXISTS neurons (
                                       neuron_id SERIAL PRIMARY KEY,
                                       x REAL NOT NULL,
                                       y REAL NOT NULL,
                                       z REAL NOT NULL,
                                       propagation_rate REAL,
                                       neuron_type INTEGER
);
