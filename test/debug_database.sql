-- Filename: debug_database.sql

-- *************************************
-- ** 1. List All Tables in the Schema **
-- *************************************
SELECT table_name
FROM information_schema.tables
WHERE table_schema = 'public'
ORDER BY table_name;

-- ************************************
-- ** 2. Describe Table Structures **
-- ************************************

-- Describe each table using information_schema.columns

-- Describe 'neurons' table
SELECT column_name, data_type, is_nullable
FROM information_schema.columns
WHERE table_name = 'neurons' AND table_schema = 'public'
ORDER BY ordinal_position;

-- Describe 'somas' table
SELECT column_name, data_type, is_nullable
FROM information_schema.columns
WHERE table_name = 'somas' AND table_schema = 'public'
ORDER BY ordinal_position;

-- Describe 'axonhillocks' table
SELECT column_name, data_type, is_nullable
FROM information_schema.columns
WHERE table_name = 'axonhillocks' AND table_schema = 'public'
ORDER BY ordinal_position;

-- Describe 'axons' table
SELECT column_name, data_type, is_nullable
FROM information_schema.columns
WHERE table_name = 'axons' AND table_schema = 'public'
ORDER BY ordinal_position;

-- Describe 'axonbranches' table
SELECT column_name, data_type, is_nullable
FROM information_schema.columns
WHERE table_name = 'axonbranches' AND table_schema = 'public'
ORDER BY ordinal_position;

-- Describe 'axonboutons' table
SELECT column_name, data_type, is_nullable
FROM information_schema.columns
WHERE table_name = 'axonboutons' AND table_schema = 'public'
ORDER BY ordinal_position;

-- Describe 'synapticgaps' table
SELECT column_name, data_type, is_nullable
FROM information_schema.columns
WHERE table_name = 'synapticgaps' AND table_schema = 'public'
ORDER BY ordinal_position;

-- Describe 'dendritebranches' table
SELECT column_name, data_type, is_nullable
FROM information_schema.columns
WHERE table_name = 'dendritebranches' AND table_schema = 'public'
ORDER BY ordinal_position;

-- Describe 'dendrites' table
SELECT column_name, data_type, is_nullable
FROM information_schema.columns
WHERE table_name = 'dendrites' AND table_schema = 'public'
ORDER BY ordinal_position;

-- Describe 'dendriteboutons' table
SELECT column_name, data_type, is_nullable
FROM information_schema.columns
WHERE table_name = 'dendriteboutons' AND table_schema = 'public'
ORDER BY ordinal_position;

-- ****************************************
-- ** 3. Check Foreign Key Constraints **
-- ****************************************
SELECT
    tc.table_name,
    kcu.column_name,
    ccu.table_name AS foreign_table_name,
    ccu.column_name AS foreign_column_name,
    tc.constraint_name
FROM
    information_schema.table_constraints AS tc
        JOIN information_schema.key_column_usage AS kcu
             ON tc.constraint_name = kcu.constraint_name AND tc.table_schema = kcu.table_schema
        JOIN information_schema.constraint_column_usage AS ccu
             ON ccu.constraint_name = tc.constraint_name AND ccu.table_schema = tc.table_schema
WHERE
    tc.constraint_type = 'FOREIGN KEY'
  AND tc.table_schema = 'public'
ORDER BY tc.table_name;

-- *********************************
-- ** 4. View Table Contents **
-- *********************************

-- View sample data from each table
SELECT * FROM neurons LIMIT 2000;
SELECT * FROM somas LIMIT 10;
SELECT * FROM axonhillocks LIMIT 10;
SELECT * FROM axons LIMIT 10;
SELECT * FROM axonbranches LIMIT 10;
SELECT * FROM axonboutons LIMIT 10;
SELECT * FROM synapticgaps LIMIT 10;
SELECT * FROM dendritebranches LIMIT 10;
SELECT * FROM dendrites LIMIT 10;
SELECT * FROM dendriteboutons LIMIT 10;

-- ***********************************************
-- ** 5. Verify Data Integrity Between Tables **
-- ***********************************************

-- Neurons and Somas
SELECT n.neuron_id, n.x AS neuron_x, s.soma_id, s.x AS soma_x
FROM neurons n
         JOIN somas s ON n.neuron_id = s.neuron_id
LIMIT 10;

-- Axons and Axon Hillocks
SELECT a.axon_id, a.x AS axon_x, ah.axon_hillock_id, ah.x AS hillock_x
FROM axons a
         JOIN axonhillocks ah ON a.axon_hillock_id = ah.axon_hillock_id
LIMIT 10;

-- Axons and Axon Branches
SELECT a.axon_id, ab.axon_branch_id, a.x AS axon_x, ab.x AS branch_x
FROM axons a
         LEFT JOIN axonbranches ab ON a.axon_branch_id = ab.axon_branch_id
LIMIT 10;

-- ****************************************
-- ** 6. Check for Orphaned Records **
-- ****************************************

-- Somas without Neurons
SELECT s.*
FROM somas s
         LEFT JOIN neurons n ON s.neuron_id = n.neuron_id
WHERE n.neuron_id IS NULL;

-- Axons without Axon Hillocks
SELECT a.*
FROM axons a
         LEFT JOIN axonhillocks ah ON a.axon_hillock_id = ah.axon_hillock_id
WHERE ah.axon_hillock_id IS NULL;

-- *******************************************
-- ** 7. Examine Recursive Relationships **
-- *******************************************

-- Hierarchy of Axon Branches
WITH RECURSIVE axon_branch_tree AS (
    SELECT
        ab.axon_branch_id,
        ab.parent_axon_id,
        ab.parent_axon_branch_id,
        ab.x,
        ab.y,
        ab.z,
        1 AS level
    FROM axonbranches ab
    WHERE ab.parent_axon_branch_id IS NULL

    UNION ALL

    SELECT
        ab_child.axon_branch_id,
        ab_child.parent_axon_id,
        ab_child.parent_axon_branch_id,
        ab_child.x,
        ab_child.y,
        ab_child.z,
        ab_parent.level + 1 AS level
    FROM axonbranches ab_child
             INNER JOIN axon_branch_tree ab_parent ON ab_child.parent_axon_branch_id = ab_parent.axon_branch_id
)
SELECT *
FROM axon_branch_tree
ORDER BY level, axon_branch_id;

-- **********************************************
-- ** 8. Check Constraint Deferrability **
-- **********************************************

SELECT
    con.conname AS constraint_name,
    con.conrelid::regclass AS table_name,
    att.attname AS column_name,
    con.confrelid::regclass AS foreign_table_name,
    confkey[1] AS foreign_column_position,
    att2.attname AS foreign_column_name,
    CASE con.condeferrable
        WHEN TRUE THEN 'YES'
        ELSE 'NO'
        END AS is_deferrable,
    CASE con.condeferred
        WHEN TRUE THEN 'YES'
        ELSE 'NO'
        END AS initially_deferred
FROM
    pg_constraint con
        JOIN pg_class cl ON cl.oid = con.conrelid
        JOIN pg_namespace ns ON ns.oid = cl.relnamespace
        JOIN pg_attribute att ON att.attrelid = con.conrelid AND att.attnum = ANY(con.conkey)
        LEFT JOIN pg_attribute att2 ON att2.attrelid = con.confrelid AND att2.attnum = ANY(con.confkey)
WHERE
    con.contype = 'f'
  AND ns.nspname = 'public'
ORDER BY
    con.conrelid::regclass::text, con.conname;

-- *********************************************
-- ** 9. Inspect Indexes and Constraints **
-- *********************************************

-- List indexes
SELECT
    schemaname,
    tablename,
    indexname,
    indexdef
FROM
    pg_indexes
WHERE
    schemaname = 'public'
ORDER BY
    tablename;

-- List constraints
SELECT
    conrelid::regclass AS table_name,
    conname AS constraint_name,
    pg_get_constraintdef(c.oid) AS constraint_definition
FROM
    pg_constraint c
WHERE
    connamespace = 'public'::regnamespace
ORDER BY
    conrelid::regclass::text, conname;

-- *********************************************************
-- ** 10. Test Data Insertion and Constraint Enforcement **
-- *********************************************************

-- Attempt to insert data that violates foreign key constraints
BEGIN;
-- This should fail due to foreign key constraint on axon_hillock_id
INSERT INTO axons (axon_hillock_id, x, y, z) VALUES (9999, 0.0, 0.0, 0.0);
ROLLBACK;

-- ***************************************************
-- ** 11. Verify Data After Batch Insertions **
-- ***************************************************

-- Retrieve Neurons with their associated data
SELECT
    n.neuron_id, n.x AS neuron_x, n.y AS neuron_y, n.z AS neuron_z,
    s.soma_id, s.x AS soma_x, s.y AS soma_y, s.z AS soma_z,
    ah.axon_hillock_id, ah.x AS hillock_x, ah.y AS hillock_y, ah.z AS hillock_z
FROM neurons n
         JOIN somas s ON n.neuron_id = s.neuron_id
         JOIN axonhillocks ah ON s.soma_id = ah.soma_id
LIMIT 10;

-- **********************************************
-- ** 12. Clean Up Test Data (Use with Caution) **
-- **********************************************

-- Uncomment the following lines if you wish to delete all data from the tables.
-- Be careful as this will remove all records.

-- DELETE FROM synapticgaps;
-- DELETE FROM axonboutons;
-- DELETE FROM axons;
-- DELETE FROM axonbranches;
-- DELETE FROM axonhillocks;
-- DELETE FROM dendriteboutons;
-- DELETE FROM dendrites;
-- DELETE FROM dendritebranches;
-- DELETE FROM somas;
-- DELETE FROM neurons;

-- ************************************
-- ** End of debug_database.sql **
-- ************************************
