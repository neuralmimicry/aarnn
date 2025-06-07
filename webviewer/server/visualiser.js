//
// server/visualiser.js
//
// “Equivalent” of visualiser.cpp in Node.js + pg + ws.
// - Reads config files (key=value) to build a connection string
// - Connects to PostgreSQL via `pg`
// - Spawns a WebSocket server on port 9002
// - Every 5 seconds, queries exactly the same tables/columns as C++ did
// - Builds a single JSON object containing all points, lines, glyphs, colors, etc.
// - Broadcasts that JSON to all connected WebSocket clients
//

const fs = require('fs');
const path = require('path');
const { Client } = require('pg');
const WebSocket = require('ws');
const maxRGB = 255;

//------------------------------------------------------------------------------
// Simple Logger class, similar to the C++ Logger
//------------------------------------------------------------------------------
class Logger {
    constructor(logFilename) {
        this.logFilename = logFilename;
        // Ensure the log file exists
        try {
            fs.appendFileSync(this.logFilename, `Logger initialized at ${new Date().toISOString()}\n`);
        } catch (err) {
            console.error('Failed to initialize logger:', err);
        }
    }

    log(message) {
        const timestamp = new Date().toISOString();
        const line = `[${timestamp}] ${message}\n`;
        try {
            fs.appendFileSync(this.logFilename, line);
        } catch (err) {
            console.error('Failed to write to log file:', err);
        }
    }

    // Overload << style: logger << "foo\n"
    write(msg) {
        this.log(msg);
    }
}

//------------------------------------------------------------------------------
// readConfig:
// Reads “key=value” lines (ignoring comments/blank) from each filename
// Returns: { [key]: value, ... }
//------------------------------------------------------------------------------
function readConfig(filenames) {
    const config = {};
    filenames.forEach((filename) => {
        let content;
        try {
            content = fs.readFileSync(filename, 'utf8');
        } catch (e) {
            throw new Error(`Failed to open config file: ${filename}`);
        }
        content.split(/\r?\n/).forEach((rawLine) => {
            const line = rawLine.trim();
            if (!line || line.startsWith('#')) return;
            const idx = line.indexOf('=');
            if (idx < 0) return;
            let key = line.slice(0, idx).trim();
            let value = line.slice(idx + 1).trim();
            config[key] = value;
        });
    });
    return config;
}

//------------------------------------------------------------------------------
// buildConnectionString:
// From config map, pick only host, port, user, password, dbname.
// Return a single string like “host=... port=... user=... password=... dbname=…”
//------------------------------------------------------------------------------
function buildConnectionString(configMap) {
    const allowed = new Set(['host', 'port', 'user', 'password', 'dbname']);
    let parts = [];
    for (const [k, v] of Object.entries(configMap)) {
        if (allowed.has(k)) {
            parts.push(`${k}=${v}`);
        }
    }
    return parts.join(' ');
}

//------------------------------------------------------------------------------
// Visualiser:
//   - Holds a shared pg Client, a Logger, and a WebSocket.Server
//   - Has async methods: insertDendriteBranches, insertAxons, buildAndBroadcastFrame
//   - On start(), connects to Postgres, starts WS server, then sets a 5 sec interval
//------------------------------------------------------------------------------
class Visualiser {
    /**
     * @param {Client}          pgClient    - an instance of pg.Client (or pg.Pool client)
     * @param {Logger}          logger      - instance of our Logger
     * @param {WebSocket.Server} wsServer   - a ws.WebSocketServer
     */
    constructor(pgClient, logger, wsServer) {
        this.pgClient = pgClient;
        this.logger = logger;
        this.wsServer = wsServer;

        // Bind methods
        this.insertDendriteBranches = this.insertDendriteBranches.bind(this);
        this.insertAxons = this.insertAxons.bind(this);
        this.buildAndBroadcastFrame = this.buildAndBroadcastFrame.bind(this);
    }

    //--------------------------------------------------------------------------
    // insertDendriteBranches:
    //   Recursively query dendritebranches → dendrites → dendriteboutons,
    //   append to “geometry” object arrays: points, lines, glyphs, colors, types
    //
    // Params:
    //   txn           - a client with an open transaction (pgClient)
    //   geom          - an object with arrays:
    //                     points:  [ [x,y,z], ... ]
    //                     lines:   [ [pID0, pID1], ... ]          (0-based point indices)
    //                     glyphs:  [ { pos: [x,y,z], vec: [vx,vy,vz], type: int, color: [r,g,b] }, ... ]
    //   parentSomaId  - int or null
    //   parentDendId  - int or null
    //
    // Note: We keep exactly the same logic as C++:
    //   1) Query “dendritebranches” by parent_dendrite_id or parent_soma_id
    //   2) For each branch row, insert a point (x,y,z), insert a glyph of type=3,
    //      compute color from energy_level
    //   3) Then query all “dendrites” under that branch, insert each dendrite point + line from branch to dendrite, glyph type=2, color from energy_level
    //   4) For each dendrite, query all “dendriteboutons,” insert bouton points, lines, glyph type=2 (same as dendrite in C++), color
    //   5) Recurse into nested branches by calling insertDendriteBranches(txn, geom, null, dendriteIdNew)
    //--------------------------------------------------------------------------
    async insertDendriteBranches(txn, geom, parentSomaId, parentDendId) {
        let resBranches;
        if (parentDendId !== null) {
            // Dendrite branches attached to another dendrite
            resBranches = await txn.query(
                `SELECT dendrite_branch_id, dendrite_id, x, y, z, energy_level, max_energy_level
         FROM dendritebranches
         WHERE dendrite_id = $1
         ORDER BY dendrite_branch_id ASC`,
                [parentDendId]
            );
        } else if (parentSomaId !== null) {
            // Dendrite branches attached to a soma
            resBranches = await txn.query(
                `SELECT dendrite_branch_id, dendrite_id, x, y, z, energy_level, max_energy_level
         FROM dendritebranches
         WHERE soma_id = $1
         ORDER BY dendrite_branch_id ASC`,
                [parentSomaId]
            );
        } else {
            // No valid parent
            return;
        }

        for (const row of resBranches.rows) {
            const dendriteBranchId = row.dendrite_branch_id;
            const dendriteId       = row.dendrite_id;                    // might be null
            const x                = parseFloat(row.x);
            const y                = parseFloat(row.y);
            const z                = parseFloat(row.z);
            const energyLevel      = parseFloat(row.energy_level);
            const maxEnergyLevel   = parseFloat(row.max_energy_level);

            // 1) Insert branch endpoint into 'points' array
            const branchAnchor = geom.points.length;
            geom.points.push([x, y, z]);

            // 2) Insert glyph for the branch itself (type=3)
            {
                const speedCalculation = (maxRGB / maxEnergyLevel) * energyLevel;
                const R = Math.floor(speedCalculation);
                const G = 0;
                const B = Math.floor(maxRGB - speedCalculation);
                geom.glyphs.push({
                    pos: [x, y, z],
                    vec: [0.0, 0.0, 0.0],
                    type: 3,
                    color: [R, G, B],
                });
            }

            // 3) Now fetch all dendrites under this branch
            const resDendrites = await txn.query(
                `SELECT dendrite_id, x, y, z, energy_level, max_energy_level
         FROM dendrites
         WHERE dendrite_branch_id = $1
         ORDER BY dendrite_id ASC`,
                [dendriteBranchId]
            );

            for (const dRow of resDendrites.rows) {
                const dendriteIdNew = dRow.dendrite_id;
                const dx            = parseFloat(dRow.x);
                const dy            = parseFloat(dRow.y);
                const dz            = parseFloat(dRow.z);
                const dEnergyLevel  = parseFloat(dRow.energy_level);
                const dMaxEnergyLevel = parseFloat(dRow.max_energy_level);

                // Insert dendrite endpoint
                const dendriteAnchor = geom.points.length;
                geom.points.push([dx, dy, dz]);

                // Create line from branch to dendrite
                geom.lines.push([branchAnchor, dendriteAnchor]);

                // Insert glyph for dendrite (type=2)
                {
                    const speedCalculation = (maxRGB / dMaxEnergyLevel) * dEnergyLevel;
                    const R_d = Math.floor(speedCalculation);
                    const G_d = 0;
                    const B_d = Math.floor(maxRGB - speedCalculation);
                    geom.glyphs.push({
                        pos: [dx, dy, dz],
                        vec: [dx - x, dy - y, dz - z],
                        type: 2,
                        color: [R_d, G_d, B_d],
                    });
                }

                // 4) Fetch all dendrite boutons under this dendrite
                const resBoutons = await txn.query(
                    `SELECT dendrite_bouton_id, x, y, z, energy_level, max_energy_level
           FROM dendriteboutons
           WHERE dendrite_id = $1
           ORDER BY dendrite_bouton_id ASC`,
                    [dendriteIdNew]
                );

                for (const bRow of resBoutons.rows) {
                    const boutonId       = bRow.dendrite_bouton_id;
                    const bx             = parseFloat(bRow.x);
                    const by             = parseFloat(bRow.y);
                    const bz             = parseFloat(bRow.z);
                    const bEnergyLevel   = parseFloat(bRow.energy_level);
                    const bMaxEnergyLevel = parseFloat(bRow.max_energy_level);

                    // Insert bouton point
                    const boutonAnchor = geom.points.length;
                    geom.points.push([bx, by, bz]);

                    // Line from dendrite to bouton
                    geom.lines.push([dendriteAnchor, boutonAnchor]);

                    // Insert glyph for bouton (type=2, same as dendrite in C++)
                    {
                        const speedCalculation = (maxRGB / bMaxEnergyLevel) * bEnergyLevel;
                        const R_b = Math.floor(speedCalculation);
                        const G_b = 0;
                        const B_b = Math.floor(maxRGB - speedCalculation);
                        geom.glyphs.push({
                            pos: [bx, by, bz],
                            vec: [bx - dx, by - dy, bz - dz],
                            type: 2,
                            color: [R_b, G_b, B_b],
                        });
                    }
                }

                // 5) Recursively insert nested dendrite branches
                await this.insertDendriteBranches(txn, geom, null, dendriteIdNew);
            }
        }
    }

    //--------------------------------------------------------------------------
    // insertAxons:
    //   Recursively query axonbranches → axons → axonboutons → synapticgaps,
    //   append to the same “geom” object.
    //
    // Params:
    //   txn                - a pg client in an open transaction
    //   geom               - { points:[], lines:[], glyphs:[] }
    //   parentAxonId       - int or null
    //   parentAxonBranchId - int or null
    //   parentAxonHillockId- int or null
    //--------------------------------------------------------------------------
    async insertAxons(txn, geom, parentAxonId, parentAxonBranchId, parentAxonHillockId) {
        let resBranches;

        if (parentAxonBranchId !== null) {
            // Axon branches under another axon branch
            resBranches = await txn.query(
                `SELECT axon_branch_id, x, y, z, energy_level, max_energy_level
         FROM axonbranches
         WHERE parent_axon_branch_id = $1
         ORDER BY axon_branch_id ASC`,
                [parentAxonBranchId]
            );
        } else if (parentAxonId !== null) {
            // Axon branches directly under an axon
            resBranches = await txn.query(
                `SELECT axon_branch_id, x, y, z, energy_level, max_energy_level
         FROM axonbranches
         WHERE parent_axon_id = $1
         ORDER BY axon_branch_id ASC`,
                [parentAxonId]
            );
        } else if (parentAxonHillockId !== null) {
            // Axon branches under an axon hillock
            resBranches = await txn.query(
                `SELECT ab.axon_branch_id, ab.x, ab.y, ab.z, ab.energy_level, ab.max_energy_level
         FROM axonbranches AS ab
         JOIN axons ON ab.parent_axon_id = axons.axon_id
         WHERE axons.axon_hillock_id = $1
         ORDER BY ab.axon_branch_id ASC`,
                [parentAxonHillockId]
            );
        } else {
            // No valid parent
            return;
        }

        for (const row of resBranches.rows) {
            const axonBranchId = row.axon_branch_id;
            const x            = parseFloat(row.x);
            const y            = parseFloat(row.y);
            const z            = parseFloat(row.z);
            const energyLevel  = parseFloat(row.energy_level);
            const maxEnergyLevel = parseFloat(row.max_energy_level);

            // 1) Insert branch point
            const branchAnchor = geom.points.length;
            geom.points.push([x, y, z]);

            // 2) Insert glyph for the branch (type=7)
            {
                const speedCalculation = (maxRGB / maxEnergyLevel) * energyLevel;
                const R = Math.floor(speedCalculation);
                const G = 0;
                const B = Math.floor(maxRGB - speedCalculation);
                geom.glyphs.push({
                    pos: [x, y, z],
                    vec: [0.0, 0.0, 0.0],
                    type: 7,
                    color: [R, G, B],
                });
            }

            // 3) Query all axons under this branch
            const resAxons = await txn.query(
                `SELECT axon_id, x, y, z, energy_level, max_energy_level
         FROM axons
         WHERE axon_branch_id = $1
         ORDER BY axon_id ASC`,
                [axonBranchId]
            );

            for (const aRow of resAxons.rows) {
                const axonIdNew      = aRow.axon_id;
                const ax             = parseFloat(aRow.x);
                const ay             = parseFloat(aRow.y);
                const az             = parseFloat(aRow.z);
                const aEnergyLevel   = parseFloat(aRow.energy_level);
                const aMaxEnergyLevel = parseFloat(aRow.max_energy_level);

                // Insert axon endpoint
                const axonAnchor = geom.points.length;
                geom.points.push([ax, ay, az]);

                // Create line from branch to axon
                geom.lines.push([branchAnchor, axonAnchor]);

                // Insert glyph for axon (type=8)
                {
                    const speedCalculation = (maxRGB / aMaxEnergyLevel) * aEnergyLevel;
                    const R_a = Math.floor(speedCalculation);
                    const G_a = 0;
                    const B_a = Math.floor(maxRGB - speedCalculation);
                    geom.glyphs.push({
                        pos: [ax, ay, az],
                        vec: [ax - x, ay - y, az - z],
                        type: 8,
                        color: [R_a, G_a, B_a],
                    });
                }

                // 4) Query all axon boutons under this axon
                const resBoutons = await txn.query(
                    `SELECT axon_bouton_id, x, y, z, energy_level, max_energy_level
           FROM axonboutons
           WHERE axon_id = $1
           ORDER BY axon_bouton_id ASC`,
                    [axonIdNew]
                );

                for (const bRow of resBoutons.rows) {
                    const boutonId       = bRow.axon_bouton_id;
                    const bx             = parseFloat(bRow.x);
                    const by             = parseFloat(bRow.y);
                    const bz             = parseFloat(bRow.z);
                    const bEnergyLevel   = parseFloat(bRow.energy_level);
                    const bMaxEnergyLevel = parseFloat(bRow.max_energy_level);

                    // Insert bouton point
                    const boutonAnchor = geom.points.length;
                    geom.points.push([bx, by, bz]);

                    // Create line from axon to bouton
                    geom.lines.push([axonAnchor, boutonAnchor]);

                    // Insert glyph for bouton (type=9)
                    {
                        const speedCalculation = (maxRGB / bMaxEnergyLevel) * bEnergyLevel;
                        const R_b = Math.floor(speedCalculation);
                        const G_b = 0;
                        const B_b = Math.floor(maxRGB - speedCalculation);
                        geom.glyphs.push({
                            pos: [bx, by, bz],
                            vec: [bx - ax, by - ay, bz - az],
                            type: 9,
                            color: [R_b, G_b, B_b],
                        });
                    }

                    // 5) Query all synaptic gaps under this bouton
                    const resGaps = await txn.query(
                        `SELECT synaptic_gap_id, x, y, z, energy_level, max_energy_level
             FROM synapticgaps
             WHERE axon_bouton_id = $1
             ORDER BY synaptic_gap_id ASC`,
                        [boutonId]
                    );

                    for (const gRow of resGaps.rows) {
                        const synGapId       = gRow.synaptic_gap_id;
                        const gx             = parseFloat(gRow.x);
                        const gy             = parseFloat(gRow.y);
                        const gz             = parseFloat(gRow.z);
                        const gEnergyLevel   = parseFloat(gRow.energy_level);
                        const gMaxEnergyLevel = parseFloat(gRow.max_energy_level);

                        // Insert synaptic gap point
                        const gapAnchor = geom.points.length;
                        geom.points.push([gx, gy, gz]);

                        // Create line from bouton to synaptic gap
                        geom.lines.push([boutonAnchor, gapAnchor]);

                        // Insert glyph for synaptic gap (type=10)
                        {
                            const speedCalculation = (maxRGB / gMaxEnergyLevel) * gEnergyLevel;
                            const R_g = Math.floor(speedCalculation);
                            const G_g = 0;
                            const B_g = Math.floor(maxRGB - speedCalculation);
                            geom.glyphs.push({
                                pos: [gx, gy, gz],
                                vec: [0.0, 0.0, 0.0],
                                type: 10,
                                color: [R_g, G_g, B_g],
                            });
                        }
                    }
                }

                // 6) Recursively insert nested axon branches
                await this.insertAxons(txn, geom, axonIdNew, null, null);
            }
        }
    }

    //--------------------------------------------------------------------------
    // buildAndBroadcastFrame:
    //   - Starts a DB transaction
    //   - SELECT top-level neurons (LIMIT 10)
    //   - For each neuron: insert glyph (type=1, color from nenergy_level)
    //                    → SELECT somas for that neuron
    //                      → insert soma glyph (type=4)
    //                      → insert dendrite branches (recursively)
    //                      → SELECT axonhillocks → insert hillock glyph (type=5)
    //                         → SELECT axons under hillock → insert axon glyph (type=8)
    //                            → insert nested axons (recursively)
    //   - Commit transaction
    //   - Broadcast the entire “geom” object over WebSocket (JSON.stringify)
    //
    // Note: We buffer everything into a single in-memory “geom” object with:
    //    {
    //      points:  [ [x,y,z], ... ],
    //      lines:   [ [idx0, idx1], ... ],
    //      glyphs:  [ { pos:[x,y,z], vec:[vx,vy,vz], type:int, color:[r,g,b] }, ... ]
    //    }
    //
    // This parallels exactly the data arrays in C++ (vtkPoints, vtkCellArray, vtkFloatArray, vtkUnsignedCharArray, etc.).
    //--------------------------------------------------------------------------
    async buildAndBroadcastFrame() {
        // 1) Reset geometry arrays
        const geom = {
            points: [],
            lines: [],
            glyphs: [],
        };

        try {
            // 2) Start a new DB transaction
            await this.pgClient.query('BEGIN');

            // 3) Query top-level neurons (limit 10)
            const resNeurons = await this.pgClient.query(
                `SELECT neuron_id, x, y, z, propagation_rate, neuron_type, energy_level, max_energy_level
         FROM neurons
         ORDER BY neuron_id ASC
         LIMIT 8000`
            );

            for (const nRow of resNeurons.rows) {
                if (
                    nRow.neuron_id === null ||
                    nRow.x === null        ||
                    nRow.y === null        ||
                    nRow.z === null        ||
                    nRow.propagation_rate === null ||
                    nRow.neuron_type === null      ||
                    nRow.energy_level === null ||
                    nRow.max_energy_level === null
                ) {
                    continue;
                }

                const neuronId       = nRow.neuron_id;
                const nx             = parseFloat(nRow.x);
                const ny             = parseFloat(nRow.y);
                const nz             = parseFloat(nRow.z);
                const nEnergyLevel   = parseFloat(nRow.energy_level);
                const nMaxEnergyLevel = parseFloat(nRow.max_energy_level);

                // 1) Insert the neuron’s center as a glyph (type=1)
                {
                    const speedCalculation = (maxRGB / nMaxEnergyLevel) * nEnergyLevel;
                    const R = Math.floor(speedCalculation);
                    const G = 0;
                    const B = Math.floor(maxRGB - speedCalculation);
                    geom.glyphs.push({
                        pos: [nx, ny, nz],
                        vec: [0.0, 0.0, 0.0],
                        type: 1,
                        color: [R, G, B],
                    });
                }

                // 2) Fetch all somas for this neuron
                const resSomas = await this.pgClient.query(
                    `SELECT soma_id, x, y, z, energy_level, max_energy_level
           FROM somas
           WHERE neuron_id = $1
           ORDER BY soma_id ASC`,
                    [neuronId]
                );

                for (const sRow of resSomas.rows) {
                    if (
                        sRow.soma_id === null ||
                        sRow.x === null       ||
                        sRow.y === null       ||
                        sRow.z === null       ||
                        sRow.energy_level === null ||
                        sRow.max_energy_level === null
                    ) {
                        continue;
                    }

                    const somaId     = sRow.soma_id;
                    const sx         = parseFloat(sRow.x);
                    const sy         = parseFloat(sRow.y);
                    const sz         = parseFloat(sRow.z);
                    const sEnergy    = parseFloat(sRow.energy_level);
                    const sMaxEnergy = parseFloat(sRow.max_energy_level);

                    // Insert soma glyph (type=4)
                    {
                        const speedCalculation = (maxRGB / sMaxEnergy) * sEnergy;
                        const R_s = Math.floor(speedCalculation);
                        const G_s = 0;
                        const B_s = Math.floor(maxRGB - speedCalculation);
                        geom.glyphs.push({
                            pos: [sx, sy, sz],
                            vec: [0.0, 0.0, 0.0],
                            type: 4,
                            color: [R_s, G_s, B_s],
                        });
                    }

                    // 3) Recursively insert dendrite branches under this soma
                    await this.insertDendriteBranches(this.pgClient, geom, somaId, null);

                    // 4) Fetch all axon hillocks for this soma
                    const resHillocks = await this.pgClient.query(
                        `SELECT axon_hillock_id, x, y, z, energy_level, max_energy_level
             FROM axonhillocks
             WHERE soma_id = $1
             ORDER BY axon_hillock_id ASC`,
                        [somaId]
                    );

                    for (const hRow of resHillocks.rows) {
                        if (
                            hRow.axon_hillock_id === null ||
                            hRow.x === null          ||
                            hRow.y === null          ||
                            hRow.z === null          ||
                            hRow.energy_level === null ||
                            hRow.max_energy_level === null
                        ) {
                            continue;
                        }

                        const hillockId   = hRow.axon_hillock_id;
                        const ahx         = parseFloat(hRow.x);
                        const ahy         = parseFloat(hRow.y);
                        const ahz         = parseFloat(hRow.z);
                        const ahEnergy    = parseFloat(hRow.energy_level);
                        const ahMaxEnergy = parseFloat(hRow.max_energy_level);

                        // Insert axon hillock glyph (type=5)
                        {
                            const speedCalculation = (maxRGB / ahMaxEnergy) * ahEnergy;
                            const R_ah = Math.floor(speedCalculation);
                            const G_ah = 0;
                            const B_ah = Math.floor(maxRGB - speedCalculation);
                            geom.glyphs.push({
                                pos: [ahx, ahy, ahz],
                                vec: [0.0, 0.0, 0.0],
                                type: 5,
                                color: [R_ah, G_ah, B_ah],
                            });
                        }

                        // 5) Query all axons under this hillock
                        const resAxonsH = await this.pgClient.query(
                            `SELECT axon_id, x, y, z, energy_level, max_energy_level
               FROM axons
               WHERE axon_hillock_id = $1
               ORDER BY axon_id ASC`,
                            [hillockId]
                        );

                        for (const axRow of resAxonsH.rows) {
                            if (
                                axRow.axon_id === null ||
                                axRow.x === null       ||
                                axRow.y === null       ||
                                axRow.z === null       ||
                                axRow.energy_level === null ||
                                axRow.max_energy_level === null
                            ) {
                                continue;
                            }

                            const axonId   = axRow.axon_id;
                            const ax       = parseFloat(axRow.x);
                            const ay       = parseFloat(axRow.y);
                            const az       = parseFloat(axRow.z);
                            const aEnergy  = parseFloat(axRow.energy_level);
                            const aMaxEnergy = parseFloat(axRow.max_energy_level);

                            // Insert axon endpoint
                            const axonAnchor = geom.points.length;
                            geom.points.push([ax, ay, az]);

                            // Line from hillock to axon
                            // Note: In C++, they did:
                            //   points_->InsertNextPoint(ahx, ahy, ahz) earlier,
                            //   then used that index as “hillock” point.
                            //   Here, we did not push the hillock as a “point” into geom.points.
                            //   But in C++ they never directly inserted the hillock point into points_;
                            //   they only inserted hillock into glyphPoints_.
                            //   So to mimic exactly, we’ll first insert hillock into geom.points_ as well:
                            const hillockPointIndex = geom.points.length - 1; // just added axon, oops
                            // Actually, we must insert the hillock point prior to inserting the axon point.
                            // To keep the same ordering as C++, we’ll push the hillock point explicitly now:
                            geom.points.splice(geom.points.length - 1, 0, [ahx, ahy, ahz]);
                            // That pushes the hillock point at (length-1), shifting the axon we just added to length-0 index + 1.
                            // Fix indices accordingly:
                            const newAxonAnchor = geom.points.length - 1; // axon is now last
                            const newHillockIdx = geom.points.length - 2; // the one just before it

                            // Create line from hillock (newHillockIdx) to axon (newAxonAnchor)
                            geom.lines.push([newHillockIdx, newAxonAnchor]);

                            // Insert glyph for axon (type=8)
                            {
                                const speedCalculation = (maxRGB / aMaxEnergy) * aEnergy;
                                const R_a = Math.floor(speedCalculation);
                                const G_a = 0;
                                const B_a = Math.floor(maxRGB - speedCalculation);
                                geom.glyphs.push({
                                    pos: [ax, ay, az],
                                    vec: [ax - ahx, ay - ahy, az - ahz],
                                    type: 8,
                                    color: [R_a, G_a, B_a],
                                });
                            }

                            // Recursively insert nested axons, boutons, synaptic gaps under this axon
                            await this.insertAxons(this.pgClient, geom, axonId, null, null);
                        }
                    }
                }
            }

            await this.pgClient.query('COMMIT');
        } catch (err) {
            this.logger.log(`Error building frame: ${err.message}`);
            console.error('Error building frame:', err);
            try {
                await this.pgClient.query('ROLLBACK');
            } catch (rollbackErr) {
                console.error('Failed to rollback:', rollbackErr);
            }
        }

        // 6) Broadcast geom to all WebSocket clients
        const payload = JSON.stringify(geom);
        this.wsServer.clients.forEach((ws) => {
            if (ws.readyState === WebSocket.OPEN) {
                ws.send(payload);
            }
        });
    }

    //--------------------------------------------------------------------------
    // start():
    //   - Connect to DB
    //   - When DB is ready, spawn WS server on port 9002
    //   - Every 5 seconds, call buildAndBroadcastFrame()
    //--------------------------------------------------------------------------
    async start() {
        try {
            await this.pgClient.connect();
            this.logger.log('Connected to PostgreSQL.');
            console.log('Connected to PostgreSQL.');

            // Start WebSocket server
            this.wsServer.on('connection', (ws) => {
                console.log('Client connected via WebSocket.');
                this.logger.log('Client connected to WS.');

                ws.on('close', () => {
                    console.log('WebSocket client disconnected.');
                    this.logger.log('WebSocket client disconnected.');
                });
            });

            console.log('WebSocket server listening on ws://localhost:9002');
            this.logger.log('WebSocket server listening on port 9002.');

            // Immediately build & broadcast an initial frame:
            await this.buildAndBroadcastFrame();

            // Then set up a 5 sec interval (5000 ms)
            setInterval(this.buildAndBroadcastFrame, 5000);
        } catch (err) {
            console.error('Fatal error in Visualiser.start():', err);
            process.exit(1);
        }
    }
}

//------------------------------------------------------------------------------
// main():
//   - Read config files: Visualiser.conf, simulation.conf
//   - Build connection string
//   - Create pg.Client
//   - Create Logger
//   - Create WebSocket.Server
//   - Instantiate Visualiser and start it.
//------------------------------------------------------------------------------

async function main() {
    try {
        // 1) Initialise Logger
        const logger = new Logger(path.resolve(__dirname, 'errors_visualiser.log'));

        // 2) Read config files & build connection string
        const configFiles = [
            path.resolve(__dirname, 'Visualiser.conf'),
            path.resolve(__dirname, 'simulation.conf'),
        ];
        const configMap = readConfig(configFiles);

        // ==== DEBUG: print out the raw configMap and final connection string ====
        console.log('––– debug: raw configMap object ───');
        console.log(configMap);
        // ==========================================================================

        // 3) Create pg.Client
        const pgClient = new Client({
            user:     configMap.user,
            host:     configMap.host,
            database: configMap.dbname,
            password: configMap.password,
            port:     parseInt(configMap.port, 10),
        });

        // 4) Launch WebSocket server on port 9002
        const wsServer = new WebSocket.Server({ port: 9002 });

        // 5) Create & run Visualiser
        const vis = new Visualiser(pgClient, logger, wsServer);
        await vis.start();
    } catch (ex) {
        console.error('Fatal error in main():', ex);
        process.exit(1);
    }
}

main();
