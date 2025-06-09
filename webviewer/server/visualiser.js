//------------------------------------------------------------------------------
// server/visualiser.js
//
// Node.js equivalent of visualiser.cpp: reads DB, builds geometry frames, broadcasts via WebSocket.
// Optimized for memory, DB round-trips, and back-pressure.
//------------------------------------------------------------------------------

const fs = require('fs');
const path = require('path');
const { Client } = require('pg');
const WebSocket = require('ws');

// Constants
const MAX_RGB = 255;
const WS_PORT = 9002;
const FRAME_INTERVAL_MS = 5000;

/**
 * Simple file-based logger with timestamped entries.
 */
class Logger {
    constructor(filePath) {
        this.filePath = filePath;
        try {
            fs.appendFileSync(this.filePath, `\n---- Logger initialized ${new Date().toISOString()} ----\n`);
        } catch (err) {
            console.error('Logger init error:', err);
        }
    }

    log(msg) {
        const line = `[${new Date().toISOString()}] ${msg}\n`;
        try {
            fs.appendFileSync(this.filePath, line);
        } catch (err) {
            console.error('Logger write error:', err);
        }
    }
}

/**
 * Load key=value config file (ignores blank lines and comments).
 */
function readConfig(file) {
    const cfg = {};
    const content = fs.readFileSync(file, 'utf8');
    for (const raw of content.split(/\r?\n/)) {
        const line = raw.trim();
        if (!line || line.startsWith('#')) continue;
        const [k, ...rest] = line.split('=');
        if (!rest.length) continue;
        cfg[k.trim()] = rest.join('=').trim();
    }
    return cfg;
}

/**
 * Convert an energy level to an RGB triple.
 */
function energyToColor(energy, maxEnergy) {
    const ratio = (MAX_RGB / maxEnergy) * energy;
    return [Math.floor(ratio), 0, Math.floor(MAX_RGB - ratio)];
}

/**
 * Visualiser class: queries DB and broadcasts JSON geometry frames.
 */
class Visualiser {
    /**
     * @param {Client} pgClient
     * @param {Logger} logger
     * @param {WebSocket.Server} wss
     */
    constructor(pgClient, logger, wss) {
        this.pg = pgClient;
        this.logger = logger;
        this.wss = wss;

        // Reuse buffers to reduce GC
        this.geom = { points: [], lines: [], glyphs: [] };
        this.buildAndBroadcastFrame = this.buildAndBroadcastFrame.bind(this);
    }

    resetGeometry() {
        this.geom.points.length = 0;
        this.geom.lines.length = 0;
        this.geom.glyphs.length = 0;
    }

    /**
     * Send JSON to all open WS clients, with back-pressure check.
     */
    broadcast(json) {
        for (const ws of this.wss.clients) {
            if (ws.readyState === WebSocket.OPEN && ws.bufferedAmount < 1e6) {
                ws.send(json);
            }
        }
    }

    //------------------------------------------------------------------------
    // Inserts dendrite branches, dendrites, and boutons recursively.
    //------------------------------------------------------------------------
    async insertDendriteBranches(tx, parentSomaId, parentDendriteId) {
        let res;
        if (parentDendriteId != null) {
            res = await tx.query(
                `SELECT dendrite_branch_id, dendrite_id, x, y, z, energy_level, max_energy_level
         FROM dendritebranches
         WHERE dendrite_id = $1
         ORDER BY dendrite_branch_id ASC`,
                [parentDendriteId]
            );
        } else if (parentSomaId != null) {
            res = await tx.query(
                `SELECT dendrite_branch_id, dendrite_id, x, y, z, energy_level, max_energy_level
         FROM dendritebranches
         WHERE soma_id = $1
         ORDER BY dendrite_branch_id ASC`,
                [parentSomaId]
            );
        } else {
            return;
        }

        for (const br of res.rows) {
            const { dendrite_branch_id: branchId, dendrite_id, x, y, z, energy_level, max_energy_level } = br;
            const px = +x, py = +y, pz = +z;
            const branchIdx = this.geom.points.length;
            this.geom.points.push([px, py, pz]);
            this.geom.glyphs.push({ pos: [px,py,pz], vec: [0,0,0], type: 3, color: energyToColor(+energy_level, +max_energy_level) });

            const dendRes = await tx.query(
                `SELECT dendrite_id, x, y, z, energy_level, max_energy_level
                 FROM dendrites
                 WHERE dendrite_branch_id = $1
                 ORDER BY dendrite_id ASC`,
                [branchId]
            );

            for (const dn of dendRes.rows) {
                const { dendrite_id: dendId, x: dx, y: dy, z: dz, energy_level: de, max_energy_level: dmax } = dn;
                const fx = +dx, fy = +dy, fz = +dz;
                const dendIdx = this.geom.points.length;
                this.geom.points.push([fx, fy, fz]);
                this.geom.lines.push([branchIdx, dendIdx]);
                this.geom.glyphs.push({ pos:[fx,fy,fz], vec:[fx-px,fy-py,fz-pz], type:2, color: energyToColor(+de, +dmax) });

                const btnRes = await tx.query(
                    `SELECT x, y, z, energy_level, max_energy_level
                     FROM dendriteboutons
                     WHERE dendrite_id = $1
                     ORDER BY dendrite_bouton_id ASC`,
                    [dendId]
                );
                for (const bt of btnRes.rows) {
                    const bx=+bt.x, by=+bt.y, bz=+bt.z;
                    const bIdx = this.geom.points.length;
                    this.geom.points.push([bx,by,bz]);
                    this.geom.lines.push([dendIdx, bIdx]);
                    this.geom.glyphs.push({ pos:[bx,by,bz], vec:[bx-fx,by-fy,bz-fz], type:2, color: energyToColor(+bt.energy_level, +bt.max_energy_level) });
                }

                // Recurse deeper
                if (dendrite_id != null) await this.insertDendriteBranches(tx, null, dendrite_id);
            }
        }
    }

    //------------------------------------------------------------------------
    // Inserts axon branches, axons, boutons, synaptic gaps recursively.
    //------------------------------------------------------------------------
    async insertAxons(tx, parentAxonId, parentBranchId, parentHillockId) {
        let res;
        if (parentBranchId != null) {
            res = await tx.query(
                `SELECT axon_branch_id, x, y, z, energy_level, max_energy_level
         FROM axonbranches
         WHERE parent_axon_branch_id = $1
         ORDER BY axon_branch_id ASC`,
                [parentBranchId]
            );
        } else if (parentAxonId != null) {
            res = await tx.query(
                `SELECT axon_branch_id, x, y, z, energy_level, max_energy_level
         FROM axonbranches
         WHERE parent_axon_id = $1
         ORDER BY axon_branch_id ASC`,
                [parentAxonId]
            );
        } else if (parentHillockId != null) {
            res = await tx.query(
                `SELECT ab.axon_branch_id, ab.x, ab.y, ab.z, ab.energy_level, ab.max_energy_level
         FROM axonbranches ab
         JOIN axons a ON ab.parent_axon_id = a.axon_id
         WHERE a.axon_hillock_id = $1
         ORDER BY ab.axon_branch_id ASC`,
                [parentHillockId]
            );
        } else {
            return;
        }

        for (const br of res.rows) {
            const { axon_branch_id: branchId, x, y, z, energy_level, max_energy_level } = br;
            const px=+x, py=+y, pz=+z;
            const branchIdx = this.geom.points.length;
            this.geom.points.push([px,py,pz]);
            this.geom.glyphs.push({ pos:[px,py,pz], vec:[0,0,0], type:7, color: energyToColor(+energy_level, +max_energy_level) });

            const axRes = await tx.query(
                `SELECT axon_id, x, y, z, energy_level, max_energy_level
                 FROM axons
                 WHERE axon_branch_id = $1
                 ORDER BY axon_id ASC`,
                [branchId]
            );

            for (const ax of axRes.rows) {
                const { axon_id, x: axx, y: ayy, z: azz, energy_level: ae, max_energy_level: amax } = ax;
                const fx=+axx, fy=+ayy, fz=+azz;
                const axIdx = this.geom.points.length;
                this.geom.points.push([fx,fy,fz]);
                this.geom.lines.push([branchIdx, axIdx]);
                this.geom.glyphs.push({ pos:[fx,fy,fz], vec:[fx-px,fy-py,fz-pz], type:8, color: energyToColor(+ae, +amax) });

                const btnRes = await tx.query(
                    `SELECT axon_bouton_id, x, y, z, energy_level, max_energy_level
                     FROM axonboutons
                     WHERE axon_id = $1
                     ORDER BY axon_bouton_id ASC`,
                    [axon_id]
                );
                for (const bt of btnRes.rows) {
                    const bx=+bt.x, by=+bt.y, bz=+bt.z;
                    const bIdx = this.geom.points.length;
                    this.geom.points.push([bx,by,bz]);
                    this.geom.lines.push([axIdx, bIdx]);
                    this.geom.glyphs.push({ pos:[bx,by,bz], vec:[bx-fx,by-fy,bz-fz], type:9, color: energyToColor(+bt.energy_level, +bt.max_energy_level) });

                    const gapRes = await tx.query(
                        `SELECT x, y, z, energy_level, max_energy_level
                         FROM synapticgaps
                         WHERE axon_bouton_id = $1
                         ORDER BY synaptic_gap_id ASC`,
                        [bt.axon_bouton_id]
                    );
                    for (const gp of gapRes.rows) {
                        const gx=+gp.x, gy=+gp.y, gz=+gp.z;
                        const gIdx = this.geom.points.length;
                        this.geom.points.push([gx,gy,gz]);
                        this.geom.lines.push([bIdx, gIdx]);
                        this.geom.glyphs.push({ pos:[gx,gy,gz], vec:[0,0,0], type:10, color: energyToColor(+gp.energy_level, +gp.max_energy_level) });
                    }
                }

                // Recurse under this axon for deeper branches
                await this.insertAxons(tx, axon_id, null, null);
            }

            // Also recurse under branch-of-branch if needed
            await this.insertAxons(tx, null, branchId, null);
        }
    }

    /**
     * Construct and broadcast a full frame.
     */
    async buildAndBroadcastFrame() {
        this.resetGeometry();
        const tx = this.pg;
        try {
            await tx.query('BEGIN');

            // Top-level neurons
            const nRes = await tx.query(
                `SELECT neuron_id, x, y, z, propagation_rate, neuron_type, energy_level, max_energy_level
                 FROM neurons
                 ORDER BY neuron_id ASC
                     LIMIT 8000`
            );

            for (const nr of nRes.rows) {
                if ([nr.x, nr.y, nr.z, nr.energy_level, nr.max_energy_level].some(v => v == null)) continue;
                const nx=+nr.x, ny=+nr.y, nz=+nr.z;
                this.geom.glyphs.push({ pos:[nx,ny,nz], vec:[0,0,0], type:1, color: energyToColor(+nr.energy_level,+nr.max_energy_level) });

                // Somas
                const sRes = await tx.query(
                    `SELECT soma_id, x, y, z, energy_level, max_energy_level
                     FROM somas
                     WHERE neuron_id=$1
                     ORDER BY soma_id ASC`,
                    [nr.neuron_id]
                );
                for (const sm of sRes.rows) {
                    if ([sm.x,sm.y,sm.z,sm.energy_level,sm.max_energy_level].some(v=>v==null)) continue;
                    const sx=+sm.x, sy=+sm.y, sz=+sm.z;
                    this.geom.glyphs.push({ pos:[sx,sy,sz], vec:[0,0,0], type:4, color: energyToColor(+sm.energy_level,+sm.max_energy_level) });

                    await this.insertDendriteBranches(tx, sm.soma_id, null);

                    // Axon hillocks
                    const hRes = await tx.query(
                        `SELECT axon_hillock_id, x, y, z, energy_level, max_energy_level
                         FROM axonhillocks
                         WHERE soma_id=$1
                         ORDER BY axon_hillock_id ASC`,
                        [sm.soma_id]
                    );
                    for (const hl of hRes.rows) {
                        if ([hl.x,hl.y,hl.z,hl.energy_level,hl.max_energy_level].some(v=>v==null)) continue;
                        const ahx=+hl.x, ahy=+hl.y, ahz=+hl.z;
                        const hillIdx = this.geom.points.length;
                        this.geom.points.push([ahx,ahy,ahz]);
                        this.geom.glyphs.push({ pos:[ahx,ahy,ahz], vec:[0,0,0], type:5, color: energyToColor(+hl.energy_level,+hl.max_energy_level) });

                        // Branch under hillock
                        await this.insertAxons(tx, null, null, hl.axon_hillock_id);
                    }
                }
            }

            await tx.query('COMMIT');
        } catch (err) {
            this.logger.log(`Frame build error: ${err.message}`);
            console.error('Build error:', err);
            try { await tx.query('ROLLBACK'); } catch {};
        }

        const payload = JSON.stringify(this.geom);
        this.broadcast(payload);
        if (global.gc) global.gc();
    }

    /**
     * Initialize DB connection and WS server, then start frames.
     */
    async start() {
        try {
            await this.pg.connect();
            this.logger.log('Connected to PostgreSQL');
            console.log('Connected to PostgreSQL');

            this.wss.on('connection', ws => {
                this.logger.log('WS client connected');
                ws.on('close', () => this.logger.log('WS client disconnected'));
            });
            this.logger.log(`WebSocket listening on port ${WS_PORT}`);
            console.log(`WebSocket listening on ws://localhost:${WS_PORT}`);

            // Immediate and interval frames
            await this.buildAndBroadcastFrame();
            setInterval(this.buildAndBroadcastFrame, FRAME_INTERVAL_MS);
        } catch (err) {
            console.error('Fatal start error:', err);
            process.exit(1);
        }
    }
}

//------------------------------------------------------------------------------
// Main entrypoint
//------------------------------------------------------------------------------
(async () => {
    const logger = new Logger(path.resolve(__dirname, 'errors_visualiser.log'));
    const dbCfg = readConfig(path.resolve(__dirname, 'Visualiser.conf'));
    const connOpts = {
        host: dbCfg.host,
        port: parseInt(dbCfg.port, 10),
        user: dbCfg.user,
        password: dbCfg.password,
        database: dbCfg.dbname
    };
    console.log('Connecting with options:', connOpts);

    const pgClient = new Client(connOpts);
    const wss = new WebSocket.Server({ port: WS_PORT });

    const vis = new Visualiser(pgClient, logger, wss);
    await vis.start();
})();
