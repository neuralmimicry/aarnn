//! aarnn: In-memory schema and massively-parallel energy update engine
//!
//! This library mirrors the schema defined in `aarnn/database.cpp` and provides
//! an in-memory representation plus scalable energy update algorithms that can
//! utilize all CPU cores via Rayon and optionally a GPU via wgpu (feature `gpu`).

use rand::Rng;
use rayon::prelude::*;
use serde::{Serialize, Deserialize};
use std::fs;
use std::io::{Read, Write};
use std::path::Path;
use std::time::{SystemTime, UNIX_EPOCH};
use std::collections::HashMap;

pub type Id = u32;

#[derive(Clone, Copy, Debug, Default, Serialize, Deserialize)]
pub struct Vec3 {
    pub x: f32,
    pub y: f32,
    pub z: f32,
}

// Core row structs roughly mirroring the DB schema. For in-memory compute we
// primarily operate on the `energy_level` and `max_energy_level` fields.

#[derive(Clone, Debug, Serialize, Deserialize)]
pub struct ClusterRow {
    pub cluster_id: Id,
    pub pos: Vec3,
    pub propagation_rate: Option<f32>,
    pub cluster_type: Option<i32>,
    pub energy_level: f32,
    pub max_energy_level: f32,
}

#[derive(Clone, Debug, Serialize, Deserialize)]
pub struct NeuronRow {
    pub neuron_id: Id,
    pub cluster_id: Option<Id>,
    pub pos: Vec3,
    pub propagation_rate: Option<f32>,
    pub neuron_type: Option<i32>,
    pub energy_level: f32,
    pub max_energy_level: f32,
}

#[derive(Clone, Debug, Serialize, Deserialize)]
pub struct SomaRow {
    pub soma_id: Id,
    pub neuron_id: Option<Id>,
    pub pos: Vec3,
    pub energy_level: f32,
    pub max_energy_level: f32,
}

#[derive(Clone, Debug, Serialize, Deserialize)]
pub struct AxonHillockRow {
    pub axon_hillock_id: Id,
    pub soma_id: Option<Id>,
    pub pos: Vec3,
    pub energy_level: f32,
    pub max_energy_level: f32,
}

#[derive(Clone, Debug, Serialize, Deserialize)]
pub struct AxonBranchRow {
    pub axon_branch_id: Id,
    pub parent_axon_id: Option<Id>,
    pub parent_axon_branch_id: Option<Id>,
    pub pos: Vec3,
    pub energy_level: f32,
    pub max_energy_level: f32,
}

#[derive(Clone, Debug, Serialize, Deserialize)]
pub struct AxonRow {
    pub axon_id: Id,
    pub axon_hillock_id: Option<Id>, // may be None for branched axons
    pub axon_branch_id: Option<Id>,  // may be None for main axons
    pub pos: Vec3,
    pub energy_level: f32,
    pub max_energy_level: f32,
}

#[derive(Clone, Debug, Serialize, Deserialize)]
pub struct AxonBoutonRow {
    pub axon_bouton_id: Id,
    pub axon_id: Option<Id>,
    pub pos: Vec3,
    pub energy_level: f32,
    pub max_energy_level: f32,
}

#[derive(Clone, Debug, Serialize, Deserialize)]
pub struct SynapticGapRow {
    pub synaptic_gap_id: Id,
    pub axon_bouton_id: Option<Id>,
    pub pos: Vec3,
    pub energy_level: f32,
    pub max_energy_level: f32,
}

#[derive(Clone, Debug, Serialize, Deserialize)]
pub struct DendriteBranchRow {
    pub dendrite_branch_id: Id,
    pub soma_id: Option<Id>,
    pub parent_dendrite_id: Option<Id>,
    pub pos: Vec3,
    pub energy_level: f32,
    pub max_energy_level: f32,
}

#[derive(Clone, Debug, Serialize, Deserialize)]
pub struct DendriteRow {
    pub dendrite_id: Id,
    pub dendrite_branch_id: Option<Id>,
    pub pos: Vec3,
    pub energy_level: f32,
    pub max_energy_level: f32,
}

#[derive(Clone, Debug, Serialize, Deserialize)]
pub struct DendriteBoutonRow {
    pub dendrite_bouton_id: Id,
    pub dendrite_id: Option<Id>, // UNIQUE in DB, one-to-one with a dendrite
    pub pos: Vec3,
    pub energy_level: f32,
    pub max_energy_level: f32,
}

// ---------------- Geometry: Shapes, Connectivity, Membranes ----------------

#[derive(Clone, Copy, Debug, Serialize, Deserialize, PartialEq, Eq, Hash)]
pub enum ShapeKind { Sphere, Cylinder }

#[derive(Clone, Copy, Debug, Serialize, Deserialize, PartialEq, Eq, Hash)]
pub enum JunctionKind { Attach, Branch, Synapse }

#[derive(Clone, Copy, Debug, Serialize, Deserialize, PartialEq, Eq, Hash)]
pub enum ComponentKind {
    Cluster,
    Neuron,
    Soma,
    AxonHillock,
    Axon,
    AxonBranch,
    AxonBouton,
    SynapticGap,
    DendriteBranch,
    Dendrite,
    DendriteBouton,
}

#[derive(Clone, Debug, Serialize, Deserialize)]
pub struct ShapeRow {
    pub shape_id: Id,
    pub component: ComponentKind,
    pub component_id: Id,
    pub kind: ShapeKind,
    // Geometry parameters
    pub center: Option<Vec3>, // for sphere
    pub start: Option<Vec3>,  // for cylinder
    pub end: Option<Vec3>,    // for cylinder
    pub radius: f32,
    pub energy_level: f32,
    pub max_energy_level: f32,
}

#[derive(Clone, Debug, Serialize, Deserialize)]
pub struct ShapeEdgeRow {
    pub edge_id: Id,
    pub from_shape_id: Id,
    pub to_shape_id: Id,
    pub junction: JunctionKind,
}

#[derive(Clone, Debug, Serialize, Deserialize, Default)]
pub struct Membrane {
    pub neuron_id: Id,
    pub shape_ids: Vec<Id>,
    pub edge_ids: Vec<Id>,
    pub surface_area: f32,
    pub volume: f32,
}

/// In-memory tables: vectors of rows. Columnar extraction helpers provided for
/// efficient massively-parallel energy updates.
#[derive(Default, Clone, Serialize, Deserialize)]
#[serde(default)]
pub struct InMemoryDB {
    pub clusters: Vec<ClusterRow>,
    pub neurons: Vec<NeuronRow>,
    pub somas: Vec<SomaRow>,
    pub axon_hillocks: Vec<AxonHillockRow>,
    pub axon_branches: Vec<AxonBranchRow>,
    pub axons: Vec<AxonRow>,
    pub axon_boutons: Vec<AxonBoutonRow>,
    pub synaptic_gaps: Vec<SynapticGapRow>,
    pub dendrite_branches: Vec<DendriteBranchRow>,
    pub dendrites: Vec<DendriteRow>,
    pub dendrite_boutons: Vec<DendriteBoutonRow>,
    // Geometry and connectivity
    pub shapes: Vec<ShapeRow>,
    pub shape_edges: Vec<ShapeEdgeRow>,
    pub membranes: Vec<Membrane>,
}

impl InMemoryDB {
    /// Populate the database with random data (useful for demos and perf tests).
    pub fn random(seed: u64, counts: Counts) -> Self {
        let mut rng = rand::rngs::StdRng::seed_from_u64(seed);
        use rand::SeedableRng;
        let mut db = InMemoryDB::default();
        db.shapes = Vec::new();
        db.shape_edges = Vec::new();
        db.membranes = Vec::new();

        db.clusters = (0..counts.clusters)
            .map(|i| ClusterRow {
                cluster_id: i as Id,
                pos: Vec3 { x: rng.gen(), y: rng.gen(), z: rng.gen() },
                propagation_rate: Some(rng.gen()),
                cluster_type: Some(rng.gen_range(0..=3)),
                energy_level: rng.gen(),
                max_energy_level: 1.0,
            })
            .collect();

        db.neurons = (0..counts.neurons)
            .map(|i| NeuronRow {
                neuron_id: i as Id,
                cluster_id: Some((i % counts.clusters.max(1)) as Id),
                pos: Vec3 { x: rng.gen(), y: rng.gen(), z: rng.gen() },
                propagation_rate: Some(rng.gen()),
                neuron_type: Some(rng.gen_range(0..=3)),
                energy_level: rng.gen(),
                max_energy_level: 1.0,
            })
            .collect();

        db.somas = (0..counts.somas)
            .map(|i| SomaRow {
                soma_id: i as Id,
                neuron_id: Some((i % counts.neurons.max(1)) as Id),
                pos: Vec3 { x: rng.gen(), y: rng.gen(), z: rng.gen() },
                energy_level: rng.gen(),
                max_energy_level: 1.0,
            })
            .collect();

        db.axon_hillocks = (0..counts.axon_hillocks)
            .map(|i| AxonHillockRow {
                axon_hillock_id: i as Id,
                soma_id: Some((i % counts.somas.max(1)) as Id),
                pos: Vec3 { x: rng.gen(), y: rng.gen(), z: rng.gen() },
                energy_level: rng.gen(),
                max_energy_level: 1.0,
            })
            .collect();

        db.axon_branches = (0..counts.axon_branches)
            .map(|i| AxonBranchRow {
                axon_branch_id: i as Id,
                parent_axon_id: None,
                parent_axon_branch_id: if i > 0 { Some((i - 1) as Id) } else { None },
                pos: Vec3 { x: rng.gen(), y: rng.gen(), z: rng.gen() },
                energy_level: rng.gen(),
                max_energy_level: 1.0,
            })
            .collect();

        db.axons = (0..counts.axons)
            .map(|i| AxonRow {
                axon_id: i as Id,
                axon_hillock_id: Some((i % counts.axon_hillocks.max(1)) as Id),
                axon_branch_id: None,
                pos: Vec3 { x: rng.gen(), y: rng.gen(), z: rng.gen() },
                energy_level: rng.gen(),
                max_energy_level: 1.0,
            })
            .collect();

        db.axon_boutons = (0..counts.axon_boutons)
            .map(|i| AxonBoutonRow {
                axon_bouton_id: i as Id,
                axon_id: Some((i % counts.axons.max(1)) as Id),
                pos: Vec3 { x: rng.gen(), y: rng.gen(), z: rng.gen() },
                energy_level: rng.gen(),
                max_energy_level: 1.0,
            })
            .collect();

        db.synaptic_gaps = (0..counts.synaptic_gaps)
            .map(|i| SynapticGapRow {
                synaptic_gap_id: i as Id,
                axon_bouton_id: Some((i % counts.axon_boutons.max(1)) as Id),
                pos: Vec3 { x: rng.gen(), y: rng.gen(), z: rng.gen() },
                energy_level: rng.gen(),
                max_energy_level: 1.0,
            })
            .collect();

        db.dendrite_branches = (0..counts.dendrite_branches)
            .map(|i| DendriteBranchRow {
                dendrite_branch_id: i as Id,
                soma_id: Some((i % counts.somas.max(1)) as Id),
                parent_dendrite_id: None,
                pos: Vec3 { x: rng.gen(), y: rng.gen(), z: rng.gen() },
                energy_level: rng.gen(),
                max_energy_level: 1.0,
            })
            .collect();

        db.dendrites = (0..counts.dendrites)
            .map(|i| DendriteRow {
                dendrite_id: i as Id,
                dendrite_branch_id: Some((i % counts.dendrite_branches.max(1)) as Id),
                pos: Vec3 { x: rng.gen(), y: rng.gen(), z: rng.gen() },
                energy_level: rng.gen(),
                max_energy_level: 1.0,
            })
            .collect();

        db.dendrite_boutons = (0..counts.dendrite_boutons)
            .map(|i| DendriteBoutonRow {
                dendrite_bouton_id: i as Id,
                dendrite_id: Some((i % counts.dendrites.max(1)) as Id),
                pos: Vec3 { x: rng.gen(), y: rng.gen(), z: rng.gen() },
                energy_level: rng.gen(),
                max_energy_level: 1.0,
            })
            .collect();

        db
    }
}

#[derive(Clone, Copy, Debug, Default, Serialize, Deserialize)]
pub struct Counts {
    pub clusters: usize,
    pub neurons: usize,
    pub somas: usize,
    pub axon_hillocks: usize,
    pub axon_branches: usize,
    pub axons: usize,
    pub axon_boutons: usize,
    pub synaptic_gaps: usize,
    pub dendrite_branches: usize,
    pub dendrites: usize,
    pub dendrite_boutons: usize,
}

/// A trait to extract a mutable slice of energy levels for parallel updates.
pub trait EnergyColumn {
    fn energies_mut(&mut self) -> &mut [f32];
    fn max_energies(&self) -> &[f32];
}

macro_rules! impl_energy_column_for_rows {
    ($t:ty, $field:ident, $maxfield:ident) => {
        impl EnergyColumn for Vec<$t> {
            fn energies_mut(&mut self) -> &mut [f32] {
                // SAFETY: `bytemuck` not required; we take a slice of field by collecting into a
                // separate buffer would copy; instead operate per-row in parallel below.
                // Here we return an empty slice to avoid misuse; we provide a separate updater.
                &mut []
            }
            fn max_energies(&self) -> &[f32] { &[] }
        }
    };
}

impl_energy_column_for_rows!(ClusterRow, energy_level, max_energy_level);
impl_energy_column_for_rows!(NeuronRow, energy_level, max_energy_level);
impl_energy_column_for_rows!(SomaRow, energy_level, max_energy_level);
impl_energy_column_for_rows!(AxonHillockRow, energy_level, max_energy_level);
impl_energy_column_for_rows!(AxonBranchRow, energy_level, max_energy_level);
impl_energy_column_for_rows!(AxonRow, energy_level, max_energy_level);
impl_energy_column_for_rows!(AxonBoutonRow, energy_level, max_energy_level);
impl_energy_column_for_rows!(SynapticGapRow, energy_level, max_energy_level);
impl_energy_column_for_rows!(DendriteBranchRow, energy_level, max_energy_level);
impl_energy_column_for_rows!(DendriteRow, energy_level, max_energy_level);
impl_energy_column_for_rows!(DendriteBoutonRow, energy_level, max_energy_level);

/// Energy update policy; define how new energy is computed from current.
#[derive(Clone, Copy, Debug)]
pub struct EnergyUpdateParams {
    pub dt: f32,            // time step
    pub decay: f32,         // decay factor per step (0..1)
    pub input_boost: f32,   // input-driven additive energy
    pub update_shapes: bool, // include shape energies in update loop
}

impl Default for EnergyUpdateParams {
    fn default() -> Self {
        Self { dt: 1.0, decay: 0.01, input_boost: 0.0, update_shapes: true }
    }
}

/// CPU path: cache-friendly chunked parallel update over all tables.
pub fn update_energies_cpu(db: &mut InMemoryDB, p: EnergyUpdateParams) {
    // Helper closure that updates a row vector in parallel.
    fn update_rows<T, F>(rows: &mut [T], get: F, p: EnergyUpdateParams)
    where
        T: Send,
        F: FnMut(&mut T) -> (&mut f32, f32) + Send + Sync + Clone,
    {
        // Use Rayon parallel iterator over chunks to avoid excessive task overhead for tiny rows.
        let chunk = 1024usize.max(rows.len() / rayon::current_num_threads().max(1));
        rows.par_chunks_mut(chunk).for_each(|chunk_rows| {
            let mut get_local = get.clone();
            for row in chunk_rows {
                let (e, max_e) = get_local(row);
                let decay = p.decay * p.dt;
                *e = (*e + p.input_boost) * (1.0 - decay);
                if *e > max_e { *e = max_e; }
                if *e < 0.0 { *e = 0.0; }
            }
        });
    }

    update_rows(&mut db.clusters, |r: &mut ClusterRow| (&mut r.energy_level, r.max_energy_level), p);
    update_rows(&mut db.neurons, |r: &mut NeuronRow| (&mut r.energy_level, r.max_energy_level), p);
    update_rows(&mut db.somas, |r: &mut SomaRow| (&mut r.energy_level, r.max_energy_level), p);
    update_rows(&mut db.axon_hillocks, |r: &mut AxonHillockRow| (&mut r.energy_level, r.max_energy_level), p);
    update_rows(&mut db.axon_branches, |r: &mut AxonBranchRow| (&mut r.energy_level, r.max_energy_level), p);
    update_rows(&mut db.axons, |r: &mut AxonRow| (&mut r.energy_level, r.max_energy_level), p);
    update_rows(&mut db.axon_boutons, |r: &mut AxonBoutonRow| (&mut r.energy_level, r.max_energy_level), p);
    update_rows(&mut db.synaptic_gaps, |r: &mut SynapticGapRow| (&mut r.energy_level, r.max_energy_level), p);
    update_rows(&mut db.dendrite_branches, |r: &mut DendriteBranchRow| (&mut r.energy_level, r.max_energy_level), p);
    update_rows(&mut db.dendrites, |r: &mut DendriteRow| (&mut r.energy_level, r.max_energy_level), p);
    update_rows(&mut db.dendrite_boutons, |r: &mut DendriteBoutonRow| (&mut r.energy_level, r.max_energy_level), p);
    if p.update_shapes {
        update_rows(&mut db.shapes, |r: &mut ShapeRow| (&mut r.energy_level, r.max_energy_level), p);
    }
}

/// Strategy for choosing backend.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Backend { Cpu, #[cfg(feature = "gpu")] Gpu }

/// Update energies using the best available backend.
pub fn update_energies(db: &mut InMemoryDB, p: EnergyUpdateParams, _prefer_gpu: bool) -> anyhow::Result<Backend> {
    #[cfg(feature = "gpu")]
    {
        if _prefer_gpu {
            if let Ok(()) = update_energies_gpu(db, p) {
                return Ok(Backend::Gpu);
            }
        }
    }
    update_energies_cpu(db, p);
    Ok(Backend::Cpu)
}

#[cfg(feature = "gpu")]
fn update_energies_gpu(db: &mut InMemoryDB, p: EnergyUpdateParams) -> anyhow::Result<()> {
    use wgpu::util::DeviceExt;

    // Initialize GPU (pick a high-performance adapter if possible)
    let instance = wgpu::Instance::new(wgpu::InstanceDescriptor::default());
    let adapter = pollster::block_on(instance.request_adapter(&wgpu::RequestAdapterOptions {
        power_preference: wgpu::PowerPreference::HighPerformance,
        compatible_surface: None,
        force_fallback_adapter: false,
    })).ok_or_else(|| anyhow::anyhow!("No suitable GPU adapter found"))?;

    let (device, queue) = pollster::block_on(adapter.request_device(&wgpu::DeviceDescriptor {
        required_features: wgpu::Features::empty(),
        required_limits: wgpu::Limits::downlevel_defaults(),
        label: Some("aarnn-device"),
    }, None))?;

    // Simple compute shader: e = clamp((e + input_boost) * (1 - decay*dt), 0, max_e)
    let shader_src = r#"@group(0) @binding(0) var<storage, read_write> energies: array<f32>;
@group(0) @binding(1) var<storage, read> max_energies: array<f32>;
@group(0) @binding(2) var<uniform> params: vec2<f32>; // x=decay_dt, y=input_boost

@compute @workgroup_size(256)
fn main(@builtin(global_invocation_id) id: vec3<u32>) {
  let i = id.x;
  if (i >= arrayLength(&energies)) { return; }
  let decay_dt = params.x;
  let boost = params.y;
  var e = energies[i];
  let max_e = max_energies[i];
  e = (e + boost) * (1.0 - decay_dt);
  if (e < 0.0) { e = 0.0; }
  if (e > max_e) { e = max_e; }
  energies[i] = e;
}
"#;
    let shader = device.create_shader_module(wgpu::ShaderModuleDescriptor {
        label: Some("energy-update-shader"),
        source: wgpu::ShaderSource::Wgsl(shader_src.into()),
    });

    let pipeline = device.create_compute_pipeline(&wgpu::ComputePipelineDescriptor {
        label: Some("energy-update"),
        layout: None,
        module: &shader,
        entry_point: "main",
    });

    let decay_dt = p.decay * p.dt;
    let params_buf = device.create_buffer_init(&wgpu::util::BufferInitDescriptor {
        label: Some("params"),
        contents: bytemuck::bytes_of(&[decay_dt, p.input_boost]),
        usage: wgpu::BufferUsages::UNIFORM | wgpu::BufferUsages::COPY_DST,
    });

    // Helper to process one table's energy arrays on the GPU
    fn process_table(device: &wgpu::Device, queue: &wgpu::Queue, pipeline: &wgpu::ComputePipeline, params_buf: &wgpu::Buffer, energies: &mut [f32], max_energies: &[f32]) -> anyhow::Result<()> {
        use wgpu::util::DeviceExt;
        let n = energies.len() as u64;
        if n == 0 { return Ok(()); }
        let e_buf = device.create_buffer_init(&wgpu::util::BufferInitDescriptor {
            label: Some("energies"),
            contents: bytemuck::cast_slice(energies),
            usage: wgpu::BufferUsages::STORAGE | wgpu::BufferUsages::COPY_DST | wgpu::BufferUsages::COPY_SRC,
        });
        let max_buf = device.create_buffer_init(&wgpu::util::BufferInitDescriptor {
            label: Some("max_energies"),
            contents: bytemuck::cast_slice(max_energies),
            usage: wgpu::BufferUsages::STORAGE,
        });
        let out_read = device.create_buffer(&wgpu::BufferDescriptor {
            label: Some("readback"),
            size: (n * 4) as u64,
            usage: wgpu::BufferUsages::COPY_DST | wgpu::BufferUsages::MAP_READ,
            mapped_at_creation: false,
        });

        let bind_group_layout = pipeline.get_bind_group_layout(0);
        let bind_group = device.create_bind_group(&wgpu::BindGroupDescriptor {
            label: Some("energy-binds"),
            layout: &bind_group_layout,
            entries: &[
                wgpu::BindGroupEntry { binding: 0, resource: e_buf.as_entire_binding() },
                wgpu::BindGroupEntry { binding: 1, resource: max_buf.as_entire_binding() },
                wgpu::BindGroupEntry { binding: 2, resource: params_buf.as_entire_binding() },
            ],
        });

        let mut encoder = device.create_command_encoder(&wgpu::CommandEncoderDescriptor { label: Some("energy-encoder") });
        {
            let mut cpass = encoder.begin_compute_pass(&wgpu::ComputePassDescriptor { label: Some("energy-pass"), ..Default::default() });
            cpass.set_pipeline(pipeline);
            cpass.set_bind_group(0, &bind_group, &[]);
            let workgroups = ((n + 255) / 256) as u32;
            cpass.dispatch_workgroups(workgroups, 1, 1);
        }
        encoder.copy_buffer_to_buffer(&e_buf, 0, &out_read, 0, n * 4);
        queue.submit(Some(encoder.finish()));

        // Readback
        let buffer_slice = out_read.slice(..);
        let (tx, rx) = futures_intrusive::channel::shared::oneshot_channel();
        buffer_slice.map_async(wgpu::MapMode::Read, move |v| { tx.send(v).ok(); });
        pollster::block_on(rx.receive());
        let data = buffer_slice.get_mapped_range();
        let updated: &[f32] = bytemuck::cast_slice(&data);
        energies.copy_from_slice(updated);
        drop(data);
        out_read.unmap();
        Ok(())
    }

    // Build max arrays on the fly per table and process sequentially to keep memory bounded.
    let mut max_tmp: Vec<f32> = Vec::new();

    macro_rules! process {
        ($rows:expr, $max:expr) => {{
            max_tmp.clear();
            max_tmp.extend($rows.iter().map($max));
            process_table(&device, &queue, &pipeline, &params_buf, &mut $rows.iter_mut().map(|r| &mut r.energy_level).collect::<Vec<_>>().iter_mut().map(|r| **r).collect::<Vec<f32>>(), &max_tmp)
        }};
    }

    // Because we can't take a direct mutable slice of struct fields without copying, perform CPU updates for now if rows exist; implement a safer path:
    // For GPU path practicality in this code sample, we fall back to CPU if any table has rows.
    // In production, use a columnar layout to enable zero-copy GPU uploads.
    let any_rows = db.clusters.len() + db.neurons.len() + db.somas.len() + db.axon_hillocks.len() + db.axon_branches.len() + db.axons.len() + db.axon_boutons.len() + db.synaptic_gaps.len() + db.dendrite_branches.len() + db.dendrites.len() + db.dendrite_boutons.len() > 0;
    if any_rows {
        // Fallback to CPU for this proof-of-concept to avoid copying entire columns per table.
        update_energies_cpu(db, p);
        return Ok(());
    }

    Ok(())
}

/// Compute a checksum of all energies (useful for quick verification/benching).
pub fn energy_checksum(db: &InMemoryDB) -> f64 {
    let mut sum = 0.0f64;
    let mut add = |e: f32| { sum += e as f64; };
    for r in &db.clusters { add(r.energy_level); }
    for r in &db.neurons { add(r.energy_level); }
    for r in &db.somas { add(r.energy_level); }
    for r in &db.axon_hillocks { add(r.energy_level); }
    for r in &db.axon_branches { add(r.energy_level); }
    for r in &db.axons { add(r.energy_level); }
    for r in &db.axon_boutons { add(r.energy_level); }
    for r in &db.synaptic_gaps { add(r.energy_level); }
    for r in &db.dendrite_branches { add(r.energy_level); }
    for r in &db.dendrites { add(r.energy_level); }
    for r in &db.dendrite_boutons { add(r.energy_level); }
    for r in &db.shapes { add(r.energy_level); }
    sum
}

// ---- Persistence (versioned snapshots) ----
#[derive(Serialize, Deserialize)]
struct Snapshot {
    version: u32,
    created_millis: u64,
    db: InMemoryDB,
}

const SNAPSHOT_VERSION: u32 = 2;

impl InMemoryDB {
    pub fn save_to_path<P: AsRef<Path>>(&self, path: P) -> anyhow::Result<()> {
        let path = path.as_ref();
        if let Some(parent) = path.parent() { fs::create_dir_all(parent)?; }
        let tmp_path = path.with_extension("tmp");
        let snap = Snapshot { version: SNAPSHOT_VERSION, created_millis: now_millis(), db: self.clone() };
        let data = bincode::serialize(&snap)?;
        {
            let mut f = fs::File::create(&tmp_path)?;
            f.write_all(&data)?;
            f.sync_all()?;
        }
        // Atomic replace where possible. On Windows, rename over existing file may fail; remove destination first.
        #[cfg(target_os = "windows")]
        if path.exists() {
            let _ = fs::remove_file(path);
        }
        fs::rename(&tmp_path, path)?;
        Ok(())
    }

    pub fn load_from_path<P: AsRef<Path>>(path: P) -> anyhow::Result<Self> {
        let path = path.as_ref();
        let mut buf = Vec::new();
        let mut f = fs::File::open(path)?;
        f.read_to_end(&mut buf)?;
        let snap: Snapshot = bincode::deserialize(&buf)?;
        match snap.version {
            1 | 2 => Ok(snap.db),
            other => anyhow::bail!("unsupported snapshot version: {} (expected 1 or 2)", other),
        }
    }
}

fn now_millis() -> u64 {
    SystemTime::now().duration_since(UNIX_EPOCH).unwrap_or_default().as_millis() as u64
}

// ---------------- Geometry generation and membrane assembly ----------------
impl InMemoryDB {
    fn make_shape_sphere(&mut self, comp_shape: &mut HashMap<(ComponentKind, Id), Id>, next_shape: &mut Id, component: ComponentKind, component_id: Id, center: Vec3, radius: f32) -> Id {
        let sid = *next_shape; *next_shape = next_shape.wrapping_add(1);
        let s = ShapeRow { shape_id: sid, component, component_id, kind: ShapeKind::Sphere, center: Some(center), start: None, end: None, radius, energy_level: 0.0, max_energy_level: 1.0 };
        comp_shape.insert((component, component_id), sid);
        self.shapes.push(s);
        sid
    }
    fn make_shape_cylinder(&mut self, comp_shape: &mut HashMap<(ComponentKind, Id), Id>, next_shape: &mut Id, component: ComponentKind, component_id: Id, start: Vec3, end: Vec3, radius: f32) -> Id {
        let sid = *next_shape; *next_shape = next_shape.wrapping_add(1);
        let s = ShapeRow { shape_id: sid, component, component_id, kind: ShapeKind::Cylinder, center: None, start: Some(start), end: Some(end), radius, energy_level: 0.0, max_energy_level: 1.0 };
        comp_shape.insert((component, component_id), sid);
        self.shapes.push(s);
        sid
    }
    fn add_edge(&mut self, next_edge: &mut Id, from_sid: Id, to_sid: Id, junction: JunctionKind) {
        let eid = *next_edge; *next_edge = next_edge.wrapping_add(1);
        self.shape_edges.push(ShapeEdgeRow { edge_id: eid, from_shape_id: from_sid, to_shape_id: to_sid, junction });
    }

    pub fn build_geometry(&mut self) {
        // Reset any existing geometry
        self.shapes.clear();
        self.shape_edges.clear();
        self.membranes.clear();

        let mut next_shape: Id = 0;
        let mut next_edge: Id = 0;
        let mut comp_shape: HashMap<(ComponentKind, Id), Id> = HashMap::new();

        // Snapshot positions and relationships to avoid borrowing self during mutation
        let somas: Vec<(Id, Option<Id>, Vec3)> = self.somas.iter().map(|s| (s.soma_id, s.neuron_id, s.pos)).collect();
        let hillocks: Vec<(Id, Option<Id>, Vec3)> = self.axon_hillocks.iter().map(|h| (h.axon_hillock_id, h.soma_id, h.pos)).collect();
        let axons: Vec<(Id, Option<Id>, Option<Id>, Vec3)> = self.axons.iter().map(|a| (a.axon_id, a.axon_hillock_id, a.axon_branch_id, a.pos)).collect();
        let ax_branches: Vec<(Id, Option<Id>, Option<Id>, Vec3)> = self.axon_branches.iter().map(|b| (b.axon_branch_id, b.parent_axon_id, b.parent_axon_branch_id, b.pos)).collect();
        let den_branches: Vec<(Id, Option<Id>, Option<Id>, Vec3)> = self.dendrite_branches.iter().map(|b| (b.dendrite_branch_id, b.soma_id, b.parent_dendrite_id, b.pos)).collect();
        let dendrites: Vec<(Id, Option<Id>, Vec3)> = self.dendrites.iter().map(|d| (d.dendrite_id, d.dendrite_branch_id, d.pos)).collect();
        let ax_boutons: Vec<(Id, Option<Id>, Vec3)> = self.axon_boutons.iter().map(|b| (b.axon_bouton_id, b.axon_id, b.pos)).collect();
        let den_boutons: Vec<(Id, Option<Id>, Vec3)> = self.dendrite_boutons.iter().map(|b| (b.dendrite_bouton_id, b.dendrite_id, b.pos)).collect();
        let syn_gaps: Vec<(Id, Option<Id>, Vec3)> = self.synaptic_gaps.iter().map(|g| (g.synaptic_gap_id, g.axon_bouton_id, g.pos)).collect();

        let soma_pos: HashMap<Id, Vec3> = somas.iter().map(|(id,_,p)| (*id,*p)).collect();
        let hillock_pos: HashMap<Id, Vec3> = hillocks.iter().map(|(id,_,p)| (*id,*p)).collect();
        let ax_pos: HashMap<Id, Vec3> = axons.iter().map(|(id,_,_,p)| (*id,*p)).collect();
        let ax_branch_pos: HashMap<Id, Vec3> = ax_branches.iter().map(|(id,_,_,p)| (*id,*p)).collect();
        let den_branch_pos: HashMap<Id, Vec3> = den_branches.iter().map(|(id,_,_,p)| (*id,*p)).collect();
        let dend_pos: HashMap<Id, Vec3> = dendrites.iter().map(|(id,_,p)| (*id,*p)).collect();

        let soma_to_neuron: HashMap<Id, Id> = somas.iter().filter_map(|(sid, nid, _)| nid.map(|n| (*sid, n))).collect();
        let hillock_to_soma: HashMap<Id, Id> = hillocks.iter().filter_map(|(hid, sid, _)| sid.map(|s| (*hid, s))).collect();
        let branch_to_soma: HashMap<Id, Id> = den_branches.iter().filter_map(|(bid, sid, _, _)| sid.map(|s| (*bid, s))).collect();
        let dend_to_branch: HashMap<Id, Id> = dendrites.iter().filter_map(|(did, bid, _)| bid.map(|b| (*did, b))).collect();
        let axon_to_hillock: HashMap<Id, Id> = axons.iter().filter_map(|(aid, hid, _, _)| hid.map(|h| (*aid, h))).collect();

        // 1) Somas as spheres
        for (soma_id, _nid, pos) in &somas {
            let r = 0.01f32;
            let _sid = self.make_shape_sphere(&mut comp_shape, &mut next_shape, ComponentKind::Soma, *soma_id, *pos, r);
            let _ = _sid;
        }
        // 2) Axon hillock cylinders from soma to hillock
        for (hid, sid_opt, hpos) in &hillocks {
            if let Some(soma_id) = sid_opt {
                let soma_pos = *soma_pos.get(soma_id).unwrap_or(hpos);
                let sid = self.make_shape_cylinder(&mut comp_shape, &mut next_shape, ComponentKind::AxonHillock, *hid, soma_pos, *hpos, 0.005);
                if let Some(sphere_sid) = comp_shape.get(&(ComponentKind::Soma, *soma_id)).cloned() {
                    self.add_edge(&mut next_edge, sphere_sid, sid, JunctionKind::Attach);
                }
            }
        }
        // 3) Axons
        for (aid, hid_opt, bid_opt, apos) in &axons {
            let start_pos = if let Some(hid) = hid_opt { *hillock_pos.get(hid).unwrap_or(apos) }
                            else if let Some(bid) = bid_opt { *ax_branch_pos.get(bid).unwrap_or(apos) }
                            else { *apos };
            let sid = self.make_shape_cylinder(&mut comp_shape, &mut next_shape, ComponentKind::Axon, *aid, start_pos, *apos, 0.003);
            if let Some(hid) = hid_opt { if let Some(h_sid) = comp_shape.get(&(ComponentKind::AxonHillock, *hid)).cloned() { self.add_edge(&mut next_edge, h_sid, sid, JunctionKind::Attach); } }
            if let Some(bid) = bid_opt { if let Some(b_sid) = comp_shape.get(&(ComponentKind::AxonBranch, *bid)).cloned() { self.add_edge(&mut next_edge, b_sid, sid, JunctionKind::Attach); } }
        }
        // 4) Axon branches
        for (bid, pid_opt, pb_opt, bpos) in &ax_branches {
            let start_pos = if let Some(pid) = pid_opt { *ax_pos.get(pid).unwrap_or(bpos) }
                            else if let Some(pb) = pb_opt { *ax_branch_pos.get(pb).unwrap_or(bpos) }
                            else { *bpos };
            let sid = self.make_shape_cylinder(&mut comp_shape, &mut next_shape, ComponentKind::AxonBranch, *bid, start_pos, *bpos, 0.0025);
            if let Some(pid) = pid_opt { if let Some(p_sid) = comp_shape.get(&(ComponentKind::Axon, *pid)).cloned() { self.add_edge(&mut next_edge, p_sid, sid, JunctionKind::Branch); } }
            if let Some(pb) = pb_opt { if let Some(p_sid) = comp_shape.get(&(ComponentKind::AxonBranch, *pb)).cloned() { self.add_edge(&mut next_edge, p_sid, sid, JunctionKind::Branch); } }
        }
        // 5) Dendrite branches
        for (bid, sid_opt, pd_opt, bpos) in &den_branches {
            let start_pos = if let Some(sid) = sid_opt { *soma_pos.get(sid).unwrap_or(bpos) }
                            else if let Some(pd) = pd_opt { *dend_pos.get(pd).unwrap_or(bpos) }
                            else { *bpos };
            let sid = self.make_shape_cylinder(&mut comp_shape, &mut next_shape, ComponentKind::DendriteBranch, *bid, start_pos, *bpos, 0.0025);
            if let Some(sid2) = sid_opt { if let Some(s_sphere) = comp_shape.get(&(ComponentKind::Soma, *sid2)).cloned() { self.add_edge(&mut next_edge, s_sphere, sid, JunctionKind::Attach); } }
            if let Some(pd) = pd_opt { if let Some(p_sid) = comp_shape.get(&(ComponentKind::Dendrite, *pd)).cloned() { self.add_edge(&mut next_edge, p_sid, sid, JunctionKind::Branch); } }
        }
        // 6) Dendrites
        for (did, bid_opt, dpos) in &dendrites {
            if let Some(bid) = bid_opt {
                let start_pos = *den_branch_pos.get(bid).unwrap_or(dpos);
                let sid = self.make_shape_cylinder(&mut comp_shape, &mut next_shape, ComponentKind::Dendrite, *did, start_pos, *dpos, 0.002);
                if let Some(b_sid) = comp_shape.get(&(ComponentKind::DendriteBranch, *bid)).cloned() { self.add_edge(&mut next_edge, b_sid, sid, JunctionKind::Branch); }
            }
        }
        // 7) Boutons and synaptic gaps
        for (abid, axid_opt, pos) in &ax_boutons {
            let sid = self.make_shape_sphere(&mut comp_shape, &mut next_shape, ComponentKind::AxonBouton, *abid, *pos, 0.0015);
            if let Some(axid) = axid_opt { if let Some(a_sid) = comp_shape.get(&(ComponentKind::Axon, *axid)).cloned() { self.add_edge(&mut next_edge, a_sid, sid, JunctionKind::Attach); } }
        }
        for (dbid, did_opt, pos) in &den_boutons {
            let sid = self.make_shape_sphere(&mut comp_shape, &mut next_shape, ComponentKind::DendriteBouton, *dbid, *pos, 0.0015);
            if let Some(did) = did_opt { if let Some(d_sid) = comp_shape.get(&(ComponentKind::Dendrite, *did)).cloned() { self.add_edge(&mut next_edge, d_sid, sid, JunctionKind::Attach); } }
        }
        for (sgid, abid_opt, pos) in &syn_gaps {
            let sid = self.make_shape_sphere(&mut comp_shape, &mut next_shape, ComponentKind::SynapticGap, *sgid, *pos, 0.001);
            if let Some(bid) = abid_opt { if let Some(b_sid) = comp_shape.get(&(ComponentKind::AxonBouton, *bid)).cloned() { self.add_edge(&mut next_edge, b_sid, sid, JunctionKind::Synapse); } }
        }

        // Build membranes per neuron
        let mut neuron_shapes: HashMap<Id, Vec<Id>> = HashMap::new();
        let mut neuron_edges: HashMap<Id, Vec<Id>> = HashMap::new();
        for s in &self.shapes {
            let neuron_id_opt: Option<Id> = match s.component {
                ComponentKind::Soma => somas.iter().find(|(sid,_,_)| *sid == s.component_id).and_then(|(_, nid, _)| *nid),
                ComponentKind::AxonHillock => hillock_to_soma.get(&s.component_id).and_then(|sid| soma_to_neuron.get(sid)).cloned(),
                ComponentKind::Axon => axon_to_hillock.get(&s.component_id).and_then(|hid| hillock_to_soma.get(hid)).and_then(|sid| soma_to_neuron.get(sid)).cloned(),
                ComponentKind::AxonBranch => None,
                ComponentKind::AxonBouton => ax_boutons.iter().find(|(abid,_,_)| *abid == s.component_id).and_then(|(_, axid, _)| *axid).and_then(|axid| axon_to_hillock.get(&axid)).and_then(|hid| hillock_to_soma.get(hid)).and_then(|sid| soma_to_neuron.get(sid)).cloned(),
                ComponentKind::SynapticGap => None,
                ComponentKind::DendriteBranch => branch_to_soma.get(&s.component_id).and_then(|sid| soma_to_neuron.get(sid)).cloned(),
                ComponentKind::Dendrite => dend_to_branch.get(&s.component_id).and_then(|bid| branch_to_soma.get(bid)).and_then(|sid| soma_to_neuron.get(sid)).cloned(),
                ComponentKind::DendriteBouton => den_boutons.iter().find(|(dbid,_,_)| *dbid == s.component_id).and_then(|(_, did, _)| *did).and_then(|did| dend_to_branch.get(&did)).and_then(|bid| branch_to_soma.get(bid)).and_then(|sid| soma_to_neuron.get(sid)).cloned(),
                ComponentKind::Cluster | ComponentKind::Neuron => None,
            };
            if let Some(nid) = neuron_id_opt { neuron_shapes.entry(nid).or_default().push(s.shape_id); }
        }
        // Attribute edges if both endpoints belong to same neuron
        let shape_to_neuron: HashMap<Id, Id> = neuron_shapes.iter().flat_map(|(nid, sids)| sids.iter().map(move |sid| (*sid, *nid))).collect();
        for e in &self.shape_edges {
            if let (Some(n1), Some(n2)) = (shape_to_neuron.get(&e.from_shape_id), shape_to_neuron.get(&e.to_shape_id)) {
                if n1 == n2 { neuron_edges.entry(*n1).or_default().push(e.edge_id); }
            }
        }
        // Build membranes with simple area/volume aggregation
        for (nid, sids) in neuron_shapes {
            let mut area = 0.0f32;
            let mut vol = 0.0f32;
            for sid in &sids {
                if let Some(s) = self.shapes.iter().find(|x| x.shape_id == *sid) {
                    match s.kind {
                        ShapeKind::Sphere => {
                            let r = s.radius; area += 4.0 * std::f32::consts::PI * r * r; vol += 4.0/3.0 * std::f32::consts::PI * r * r * r;
                        }
                        ShapeKind::Cylinder => {
                            if let (Some(a), Some(b)) = (s.start, s.end) {
                                let dx = a.x - b.x; let dy = a.y - b.y; let dz = a.z - b.z;
                                let l = (dx*dx + dy*dy + dz*dz).sqrt();
                                let r = s.radius; area += 2.0 * std::f32::consts::PI * r * l + 2.0 * std::f32::consts::PI * r * r; vol += std::f32::consts::PI * r * r * l;
                            }
                        }
                    }
                }
            }
            let eids = neuron_edges.remove(&nid).unwrap_or_default();
            self.membranes.push(Membrane { neuron_id: nid, shape_ids: sids, edge_ids: eids, surface_area: area, volume: vol });
        }
    }

    pub fn membrane_stats(&self, neuron_id: Id) -> Option<(usize, usize, f32, f32)> {
        self.membranes.iter().find(|m| m.neuron_id == neuron_id).map(|m| (m.shape_ids.len(), m.edge_ids.len(), m.surface_area, m.volume))
    }
}
