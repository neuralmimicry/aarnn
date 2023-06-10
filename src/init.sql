CREATE TABLE IF NOT EXISTS neurons (
                                       id SERIAL PRIMARY KEY,
                                       propagation_rate REAL NOT NULL,
                                       neuron_type INTEGER NOT NULL,
                                       axon_length REAL NOT NULL
);
