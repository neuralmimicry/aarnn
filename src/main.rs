use aarnn::{Counts, InMemoryDB, EnergyUpdateParams, update_energies, energy_checksum};
use std::env;
use std::path::PathBuf;
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::Arc;

fn default_state_path() -> PathBuf {
    if let Ok(p) = env::var("AARNN_STATE") { return PathBuf::from(p); }
    PathBuf::from("aarnn_state.bin")
}

fn parse_args() -> anyhow::Result<(PathBuf, bool, bool, usize, bool, bool, Option<u32>, bool)> {
    // returns: (state_path, resume, autosave, steps, prefer_gpu, build_membranes, inspect_neuron, update_shapes)
    let mut state_path = default_state_path();
    let mut resume = true;
    let mut autosave = true;
    let mut steps: usize = 1;
    let mut prefer_gpu = true;
    let mut build_membranes = false;
    let mut inspect_neuron: Option<u32> = None;
    let mut update_shapes = true;
    let mut args = env::args().skip(1);
    while let Some(arg) = args.next() {
        match arg.as_str() {
            "--state" => { if let Some(p) = args.next() { state_path = PathBuf::from(p); } },
            "--resume" => resume = true,
            "--no-resume" => resume = false,
            "--autosave" => autosave = true,
            "--no-autosave" => autosave = false,
            "--steps" => { if let Some(n) = args.next() { steps = n.parse()?; } },
            "--prefer-gpu" => prefer_gpu = true,
            "--no-prefer-gpu" => prefer_gpu = false,
            "--build-membranes" => build_membranes = true,
            "--inspect-neuron" => { if let Some(n) = args.next() { inspect_neuron = Some(n.parse()?); } },
            "--no-shape-update" => update_shapes = false,
            "-h" | "--help" => {
                println!("aarnn demo with persistence + geometry\n\nOptions:\n  --state PATH         Path to snapshot file (default: $AARNN_STATE or ./aarnn_state.bin)\n  --resume | --no-resume   Enable/disable loading snapshot on start (default: resume)\n  --autosave | --no-autosave  Enable/disable saving snapshot (default: autosave)\n  --steps N            Number of update steps to run before exit (default: 1)\n  --prefer-gpu | --no-prefer-gpu  Prefer GPU backend when available (default: prefer)\n  --build-membranes    Rebuild shapes/connectivity and per-neuron membranes from current components\n  --inspect-neuron N   Print membrane stats for neuron with id N (after generation/loading)\n  --no-shape-update    Exclude shape energies from update loop (default: include)\n");
                std::process::exit(0);
            }
            _ => {
                eprintln!("Unknown arg: {}", arg);
                std::process::exit(2);
            }
        }
    }
    Ok((state_path, resume, autosave, steps, prefer_gpu, build_membranes, inspect_neuron, update_shapes))
}

fn main() -> anyhow::Result<()> {
    let (state_path, resume, autosave, steps, prefer_gpu, build_membranes, inspect_neuron, update_shapes) = parse_args()?;

    // Build or load the in-memory database
    let mut db = if resume && state_path.exists() {
        println!("[aarnn] Loading state from {}", state_path.display());
        match InMemoryDB::load_from_path(&state_path) {
            Ok(db) => db,
            Err(e) => {
                eprintln!("[aarnn] Failed to load state: {e}; creating a new random network instead.");
                let counts = Counts {
                    clusters: 10_000,
                    neurons: 50_000,
                    somas: 50_000,
                    axon_hillocks: 50_000,
                    axon_branches: 100_000,
                    axons: 50_000,
                    axon_boutons: 200_000,
                    synaptic_gaps: 200_000,
                    dendrite_branches: 100_000,
                    dendrites: 100_000,
                    dendrite_boutons: 100_000,
                };
                InMemoryDB::random(42, counts)
            }
        }
    } else {
        println!("[aarnn] No existing state or resume disabled; creating random network");
        let counts = Counts {
            clusters: 10_000,
            neurons: 50_000,
            somas: 50_000,
            axon_hillocks: 50_000,
            axon_branches: 100_000,
            axons: 50_000,
            axon_boutons: 200_000,
            synaptic_gaps: 200_000,
            dendrite_branches: 100_000,
            dendrites: 100_000,
            dendrite_boutons: 100_000,
        };
        InMemoryDB::random(42, counts)
    };

    // Geometry: build membranes if requested or missing (fresh v1 snapshot load will have empty via serde default)
    if build_membranes || db.shapes.is_empty() {
        println!("[aarnn] Building shapes/connectivity and membranes ...");
        db.build_geometry();
        println!("[aarnn] Geometry built: shapes={}, edges={}, membranes={}", db.shapes.len(), db.shape_edges.len(), db.membranes.len());
    } else {
        println!("[aarnn] Using existing geometry: shapes={}, edges={}, membranes={}", db.shapes.len(), db.shape_edges.len(), db.membranes.len());
    }

    if let Some(nid) = inspect_neuron {
        if let Some((sc, ec, area, vol)) = db.membrane_stats(nid) {
            println!("[aarnn] Neuron {} membrane: shapes={}, edges={}, area={:.6}, volume={:.6}", nid, sc, ec, area, vol);
        } else {
            println!("[aarnn] Neuron {} membrane not found.", nid);
        }
    }

    let before = energy_checksum(&db);
    println!("Initial energy checksum: {:.6}", before);

    // Ctrl-C handling for graceful autosave
    let interrupted = Arc::new(AtomicBool::new(false));
    {
        let flag = interrupted.clone();
        ctrlc::set_handler(move || { flag.store(true, Ordering::SeqCst); }).expect("failed to set Ctrl-C handler");
    }

    let mut last_backend = None;
    for step in 0..steps {
        if interrupted.load(Ordering::SeqCst) { println!("[aarnn] Interrupted; exiting..."); break; }
        let backend = update_energies(&mut db, EnergyUpdateParams { dt: 1.0, decay: 0.02, input_boost: 0.005, update_shapes }, prefer_gpu)?;
        last_backend = Some(backend);
        if autosave {
            db.save_to_path(&state_path)?;
        }
        println!("[aarnn] Step {} complete (backend: {:?})", step + 1, backend);
    }

    if autosave {
        println!("[aarnn] Saving final state to {}", state_path.display());
        db.save_to_path(&state_path)?;
    }

    let after = energy_checksum(&db);
    println!("Updated energy checksum:  {:.6} (backend: {:?})", after, last_backend.unwrap_or(aarnn::Backend::Cpu));

    Ok(())
}
