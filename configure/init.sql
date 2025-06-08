-- Create roles if not already existing
DO
$$
BEGIN
    IF NOT EXISTS (SELECT FROM pg_roles WHERE rolname = 'neurons_read') THEN
        CREATE ROLE neurons_read;
    END IF;

    IF NOT EXISTS (SELECT FROM pg_roles WHERE rolname = 'neurons_rw') THEN
        CREATE ROLE neurons_rw;
    END IF;
END
$$;

-- Create users and assign to roles
DO
$$
BEGIN
    IF NOT EXISTS (SELECT FROM pg_roles WHERE rolname = 'aarnn') THEN
        CREATE USER aarnn WITH PASSWORD 'change_this_password';
        GRANT neurons_rw TO aarnn;
        ALTER ROLE aarnn SET search_path TO public;
    END IF;

    IF NOT EXISTS (SELECT FROM pg_roles WHERE rolname = 'audio') THEN
        CREATE USER audio WITH PASSWORD 'change_this_password';
        GRANT neurons_rw TO audio;
        ALTER ROLE audio SET search_path TO public;
    END IF;

    IF NOT EXISTS (SELECT FROM pg_roles WHERE rolname = 'visualiser') THEN
        CREATE USER visualiser WITH PASSWORD 'change_this_password';
        GRANT neurons_read TO visualiser;
        ALTER ROLE visualiser SET search_path TO public;
    END IF;
END
$$;

-- Create tables
CREATE TABLE IF NOT EXISTS clusters (
                                        cluster_id SERIAL PRIMARY KEY,
                                        x REAL NOT NULL,
                                        y REAL NOT NULL,
                                        z REAL NOT NULL,
                                        propagation_rate REAL,
                                        cluster_type INTEGER,
                                        energy_level REAL NOT NULL,
                                        max_energy_level REAL NOT NULL
);

CREATE TABLE IF NOT EXISTS neurons (
                                       neuron_id SERIAL PRIMARY KEY,
                                       x REAL NOT NULL,
                                       y REAL NOT NULL,
                                       z REAL NOT NULL,
                                       propagation_rate REAL,
                                       neuron_type INTEGER,
                                       energy_level REAL NOT NULL,
                                       max_energy_level REAL NOT NULL
);

-- Grant table-level privileges
-- Switch to table owner before granting
SET ROLE neuron;

GRANT SELECT ON TABLE neurons TO neurons_read;
GRANT SELECT ON TABLE clusters TO neurons_read;

GRANT SELECT, INSERT, UPDATE, DELETE ON TABLE neurons TO neurons_rw;
GRANT SELECT, INSERT, UPDATE, DELETE ON TABLE clusters TO neurons_rw;

GRANT USAGE, SELECT ON SEQUENCE neurons_neuron_id_seq TO neurons_rw;
GRANT USAGE, SELECT ON SEQUENCE clusters_cluster_id_seq TO neurons_rw;

RESET ROLE;
