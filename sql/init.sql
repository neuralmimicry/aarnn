
SELECT session_user, current_database();

\c postgres;
DROP DATABASE IF EXISTS neurons;

CREATE DATABASE neurons;

CREATE TABLE IF NOT EXISTS
    neurons (
        neuron_id SERIAL PRIMARY KEY,
        x REAL NOT NULL,
        y REAL NOT NULL,
        z REAL NOT NULL,
        propagation_rate REAL,
        neuron_type INTEGER,
        axon_length REAL
    );

CREATE TABLE IF NOT EXISTS
    somas (
        soma_id SERIAL PRIMARY KEY,
        neuron_id INTEGER REFERENCES neurons (neuron_id),
        x REAL NOT NULL,
        y REAL NOT NULL,
        z REAL NOT NULL
    );

CREATE TABLE IF NOT EXISTS
    axonhillocks (
        axon_hillock_id SERIAL PRIMARY KEY,
        soma_id INTEGER REFERENCES somas (soma_id),
        x REAL NOT NULL,
        y REAL NOT NULL,
        z REAL NOT NULL
    );

CREATE TABLE IF NOT EXISTS
    axons_hillock (
        axon_id SERIAL PRIMARY KEY,
        axon_hillock_id INTEGER,
        x REAL NOT NULL,
        y REAL NOT NULL,
        z REAL NOT NULL
    );

CREATE TABLE IF NOT EXISTS
    axons (
        axon_id SERIAL PRIMARY KEY,
        axon_hillock_id INTEGER REFERENCES axonhillocks (axon_hillock_id),
        x REAL NOT NULL,
        y REAL NOT NULL,
        z REAL NOT NULL
    );

ALTER TABLE axons_hillock ADD CONSTRAINT fk_axons_hillock_axon_id FOREIGN KEY (axon_id) REFERENCES axons (axon_id);

CREATE TABLE IF NOT EXISTS
    axonboutons (
        axon_bouton_id SERIAL PRIMARY KEY,
        axon_id INTEGER REFERENCES axons (axon_id),
        x REAL NOT NULL,
        y REAL NOT NULL,
        z REAL NOT NULL
    );

CREATE TABLE IF NOT EXISTS
    synapticgaps (
        synaptic_gap_id SERIAL PRIMARY KEY,
        axon_bouton_id INTEGER REFERENCES axonboutons (axon_bouton_id),
        x REAL NOT NULL,
        y REAL NOT NULL,
        z REAL NOT NULL
    );

CREATE TABLE IF NOT EXISTS
    axonbranches (
        axon_branch_id SERIAL PRIMARY KEY,
        axon_id INTEGER REFERENCES axons (axon_id),
        x REAL NOT NULL,
        y REAL NOT NULL,
        z REAL NOT NULL
    );

CREATE TABLE IF NOT EXISTS
    dendritebranches_soma (
        dendrite_branch_id SERIAL PRIMARY KEY,
        soma_id INTEGER REFERENCES somas (soma_id),
        x REAL NOT NULL,
        y REAL NOT NULL,
        z REAL NOT NULL
    );

CREATE TABLE IF NOT EXISTS
    dendrites (
        dendrite_id SERIAL PRIMARY KEY,
        dendrite_branch_id INTEGER,
        x REAL NOT NULL,
        y REAL NOT NULL,
        z REAL NOT NULL
    );

CREATE TABLE IF NOT EXISTS
    dendritebranches (
        dendrite_branch_id SERIAL PRIMARY KEY,
        dendrite_id INTEGER REFERENCES dendrites (dendrite_id),
        x REAL NOT NULL,
        y REAL NOT NULL,
        z REAL NOT NULL
    );

ALTER TABLE dendritebranches ADD CONSTRAINT fk_dendritebranches_dendrite_id FOREIGN KEY (dendrite_id) REFERENCES dendrites (dendrite_id);

CREATE TABLE IF NOT EXISTS
    dendriteboutons (
        dendrite_bouton_id SERIAL PRIMARY KEY,
        dendrite_id INTEGER REFERENCES dendrites (dendrite_id),
        x REAL NOT NULL,
        y REAL NOT NULL,
        z REAL NOT NULL
    )
    
