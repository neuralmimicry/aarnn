/**
 * client/visualiser.js (ES Module)
 *
 * • Connects to ws://visualiser:9002
 * • Receives JSON frames with: { points: [...], lines: [...], glyphs: [...] }
 * (NOTE: glyphs now expected to have a unique 'id' field for merging)
 * • Renders them via Three.js + OrbitControls in a single canvas,
 * performing cumulative updates instead of full scene rebuilds.
 *
 * Dependencies (relative to client/):
 * • ./jsm/three.module.js
 * • ./jsm/controls/OrbitControls.js
 */

import * as THREE from './jsm/three.module.js';
import { OrbitControls } from './jsm/controls/OrbitControls.js';

/**
 * Checks for WebGL support in the browser.
 * @returns {boolean} True if WebGL is supported, false otherwise.
 */
function webgl_support() {
    try {
        const canvas = document.createElement('canvas');
        return !!window.WebGLRenderingContext &&
            (canvas.getContext('webgl') || canvas.getContext('experimental-webgl'));
    } catch (e) {
        return false;
    }
}

(async function () {
    'use strict';

    // ===========================================================================
    // 1) DOM ELEMENTS & INITIAL STATE
    // ===========================================================================
    const container         = document.getElementById('threeContainer');
    const loadingOverlay    = document.getElementById('loadingOverlay');
    const loadingTextEl     = document.getElementById('loadingText');
    const progressContainer = document.getElementById('progressContainer');
    const dataProgress      = document.getElementById('dataProgress');
    const progressTextEl    = document.getElementById('progressText');
    const statusMessageEl   = document.getElementById('statusMessage');

    // Create a dedicated div for error messages within the loading overlay
    const errorMessageEl = document.createElement('div');
    errorMessageEl.classList.add('error-message-text'); // Apply the CSS class
    errorMessageEl.classList.add('d-none'); // Hidden by default
    loadingOverlay.appendChild(errorMessageEl); // Add it to the loading overlay

    // Flag to track if the first full frame has been drawn, used to hide loading overlay.
    let firstFrameDrawn = false;

    // Stores references to active Three.js glyph meshes, keyed by their unique ID.
    // This allows for efficient updates, additions, and removals.
    const activeGlyphMeshes = new Map(); // Map<glyph_id, THREE.Mesh>

    // Reference to the current lines mesh in the scene.
    // Used to dispose of the old one when new line data arrives.
    let currentLinesMesh = null;

    // Accumulated data received from the server.
    // We maintain this client-side store to represent the current state of the visualization.
    let accumulatedPoints = [];  // Array of [x, y, z] point coordinates
    let accumulatedLines  = [];  // Array of [point_idx0, point_idx1] for line segments
    // Note: accumulatedGlyphs is implicitly managed by activeGlyphMeshes for live updates.

    // ===========================================================================
    // 2) CHECK WEBGL SUPPORT
    // ===========================================================================
    if (!webgl_support()) {
        // Ensure loading overlay is visible and set error message using a CSS class
        loadingOverlay.classList.remove('d-none');
        loadingOverlay.classList.add('d-flex');

        // Hide other elements and show error message
        loadingTextEl.classList.add('d-none');
        progressContainer.classList.add('d-none');
        errorMessageEl.classList.remove('d-none');
        errorMessageEl.classList.add('d-flex');
        errorMessageEl.textContent = 'WebGL not supported in your browser. Please try a modern browser or update your graphics drivers.';

        console.error('WebGL not supported');
        return;
    }

    // ===========================================================================
    // 3) THREE.JS SCENE SETUP
    // ===========================================================================
    const scene = new THREE.Scene();
    scene.background = new THREE.Color(0x222222); // Dark background for the scene

    // Camera setup: Perspective camera for 3D viewing.
    const camera = new THREE.PerspectiveCamera(
        60, // Field of view (vertical, in degrees)
        container.clientWidth / container.clientHeight, // Aspect ratio
        0.1, // Near clipping plane
        1000 // Far clipping plane
    );
    camera.position.set(0, 0, 50); // Initial camera position

    // Renderer setup: WebGL renderer with anti-aliasing for smoother edges.
    const renderer = new THREE.WebGLRenderer({ antialias: true });
    renderer.setPixelRatio(window.devicePixelRatio); // Adjust for high-DPI screens
    renderer.setSize(container.clientWidth, container.clientHeight); // Set renderer size to container
    container.appendChild(renderer.domElement); // Add the renderer's canvas to the DOM

    // Add lights to the scene for better visibility of 3D objects.
    const ambientLight     = new THREE.AmbientLight(0xffffff, 0.4); // Soft global light
    const directionalLight = new THREE.DirectionalLight(0xffffff, 0.6); // Positional light
    directionalLight.position.set(10, 10, 10); // Position the directional light
    scene.add(ambientLight, directionalLight);

    // ===========================================================================
    // 4) ORBITCONTROLS SETUP
    // ===========================================================================
    // OrbitControls allow camera manipulation (rotate, zoom, pan) with mouse/touch.
    const controls = new OrbitControls(camera, renderer.domElement);
    controls.enableDamping   = true; // Enable smooth camera movement
    controls.dampingFactor   = 0.1; // Damping intensity
    controls.enableZoom      = true; // Enable zooming
    controls.minDistance     = 5;    // Minimum zoom distance
    controls.maxDistance     = 200;  // Maximum zoom distance

    // ===========================================================================
    // 5) PROTOTYPE GEOMETRIES FOR GLYPHS
    //
    // These are reusable geometries for different types of neural components.
    // ===========================================================================
    const sphereGeometry   = new THREE.SphereGeometry(0.5, 16, 16); // For somas, axon hillocks, synaptic gaps
    const boxGeometry      = new THREE.BoxGeometry(0.5, 0.5, 0.5); // For dendrite/axon branches/boutons
    const cylinderGeometry = new THREE.CylinderGeometry(0.1, 0.1, 1.0, 16); // For dendrites, axons

    // ===========================================================================
    // 6) ROOT GROUP FOR DYNAMIC GEOMETRY
    //
    // All dynamically loaded/updated 3D objects (points, lines, glyphs) will be
    // added as children to this group, making it easy to manage them collectively.
    // ===========================================================================
    const rootGroup = new THREE.Group();
    scene.add(rootGroup);

    // ===========================================================================
    // 7) UTIL: DISPOSE MESH RESOURCES
    //
    // Helper function to safely remove a mesh and dispose of its geometry and material
    // to prevent memory leaks in Three.js when objects are no longer needed.
    // ===========================================================================
    function disposeMesh(mesh) {
        if (mesh.geometry) mesh.geometry.dispose();
        if (mesh.material) {
            if (Array.isArray(mesh.material)) {
                mesh.material.forEach((m) => m.dispose());
            } else {
                mesh.material.dispose();
            }
        }
        if (mesh.parent) { // Ensure the mesh is removed from its parent group
            mesh.parent.remove(mesh);
        }
    }

    // ===========================================================================
    // 8) UTIL: BUILD LINESEGMENTS FOR “lines” + “points”
    //
    // Creates a single LineSegments object from the accumulated line data.
    // ===========================================================================
    function buildLines(linesArray, pointsArray) {
        if (!Array.isArray(linesArray) || linesArray.length === 0 || !Array.isArray(pointsArray) || pointsArray.length === 0) {
            return null; // Return null if no valid data to build lines
        }

        const numLines  = linesArray.length;
        // Each line has two points, each point has 3 coordinates (x, y, z)
        const positions = new Float32Array(numLines * 2 * 3);

        for (let i = 0; i < numLines; ++i) {
            const [idx0, idx1] = linesArray[i];
            const p0 = pointsArray[idx0];
            const p1 = pointsArray[idx1];

            // Ensure points exist before trying to access them
            if (!p0 || !p1) {
                console.warn(`Skipping invalid line: point indices ${idx0} or ${idx1} not found.`);
                continue;
            }

            const base = i * 6; // 6 floats per line (2 points * 3 components)
            positions[base + 0] = p0[0];
            positions[base + 1] = p0[1];
            positions[base + 2] = p0[2];
            positions[base + 3] = p1[0];
            positions[base + 4] = p1[1];
            positions[base + 5] = p1[2];
        }

        const geometry = new THREE.BufferGeometry();
        geometry.setAttribute('position', new THREE.BufferAttribute(positions, 3)); // 3 components per vertex
        const material = new THREE.LineBasicMaterial({ color: 0xffff00, linewidth: 2 }); // Yellow lines
        return new THREE.LineSegments(geometry, material);
    }

    // ===========================================================================
    // 9) UTIL: BUILD A SINGLE GLYPH MESH
    //
    // Creates a Three.js Mesh for a single glyph entry based on its type and properties.
    // ===========================================================================
    function buildSingleGlyph(entry) {
        const { pos, type, color } = entry;
        const [x, y, z] = pos;
        const [r, g, b] = color.map((c) => c / 255); // Normalize color components to [0,1]

        let geometry;
        switch (type) {
            case 1:  // Neuron Center
            case 4:  // Soma
            case 5:  // Axon Hillock
            case 10: // Synaptic Gap
                geometry = sphereGeometry;
                break;
            case 2:  // Dendrite / Bouton (assuming this refers to main dendrite structure)
            case 7:  // Axon Branch
            case 8:  // Axon
                geometry = cylinderGeometry;
                break;
            case 3:  // Dendrite Branch
            case 9:  // Axon Bouton
                geometry = boxGeometry;
                break;
            default: // Default to sphere if type is unknown
                geometry = sphereGeometry;
        }

        const material = new THREE.MeshPhongMaterial({ color: new THREE.Color(r, g, b) });
        const mesh     = new THREE.Mesh(geometry, material);
        mesh.position.set(x, y, z); // Set initial position
        return mesh;
    }

    // ===========================================================================
    // 10) MAIN: UPDATE SCENE WITH INCOMING JSON DATA
    //
    // This function incrementally updates the Three.js scene based on new data.
    // It identifies additions, updates, and removals of glyphs.
    // ===========================================================================
    async function updateSceneWithData(incomingData) {
        // Assume incomingData.points, .lines, .glyphs are full arrays from the server.

        // --- 10.1) Process POINTS and LINES ---
        // For simplicity and because lines are a single mesh, we replace them entirely.
        // If granular point updates were needed, a Map<id, point_data> would be used.
        accumulatedPoints = incomingData.points || [];
        accumulatedLines  = incomingData.lines || [];

        // Dispose of the old lines mesh if it exists
        if (currentLinesMesh) {
            disposeMesh(currentLinesMesh);
        }

        // Create and add the new lines mesh
        if (accumulatedLines.length > 0 && accumulatedPoints.length > 0) {
            const newLinesMesh = buildLines(accumulatedLines, accumulatedPoints);
            if (newLinesMesh) { // Only add if lines were successfully built
                rootGroup.add(newLinesMesh);
                currentLinesMesh = newLinesMesh;
            }
        } else {
            currentLinesMesh = null; // No lines to display
        }

        // --- 10.2) Process GLYPHS (Add, Update, Remove) ---
        // Create a temporary map for quick lookup of incoming glyphs
        const incomingGlyphMap = new Map();
        if (Array.isArray(incomingData.glyphs)) {
            incomingData.glyphs.forEach(glyph => {
                // Ensure glyphs have a unique 'id' for merging! This is critical.
                if (glyph.id !== undefined && glyph.id !== null) {
                    incomingGlyphMap.set(glyph.id, glyph);
                } else {
                    console.warn('Incoming glyph missing unique ID:', glyph);
                }
            });
        }

        // Identify and REMOVE glyphs that are no longer in the incoming data
        const glyphsToRemove = [];
        for (const [id, mesh] of activeGlyphMeshes) {
            if (!incomingGlyphMap.has(id)) {
                glyphsToRemove.push({ id, mesh });
            }
        }

        glyphsToRemove.forEach(({ id, mesh }) => {
            disposeMesh(mesh);
            activeGlyphMeshes.delete(id);
        });

        // Add or UPDATE glyphs present in the incoming data
        for (const [id, incomingGlyph] of incomingGlyphMap) {
            const existingMesh = activeGlyphMeshes.get(id);

            // Normalize color components to [0,1] for Three.js Color object
            const [r, g, b] = incomingGlyph.color.map((c) => c / 255);

            if (existingMesh) {
                // UPDATE existing glyph's properties
                existingMesh.position.set(incomingGlyph.pos[0], incomingGlyph.pos[1], incomingGlyph.pos[2]);
                existingMesh.material.color.setRGB(r, g, b);

                // If glyph type changes, we must replace the mesh entirely as geometry changes.
                // This assumes buildSingleGlyph returns a mesh with the correct geometry type.
                // (Advanced: you could store `existingMesh.userData.type = entry.type;` during creation
                // and compare `existingMesh.userData.type !== incomingGlyph.type` here to trigger recreation)
            } else {
                // ADD new glyph
                const newMesh = buildSingleGlyph(incomingGlyph);
                rootGroup.add(newMesh);
                activeGlyphMeshes.set(id, newMesh);
            }
        }

        // --- 10.3) First-frame housekeeping & Status Update ---
        if (!firstFrameDrawn) {
            // Hide loading overlay and progress container on first frame
            loadingOverlay.classList.remove('d-flex');
            loadingOverlay.classList.add('d-none');
            progressContainer.classList.remove('d-flex');
            progressContainer.classList.add('d-none');
            firstFrameDrawn                  = true;
            statusMessageEl.textContent      = `Status: Rendering initial data (${activeGlyphMeshes.size} glyphs)`;
        } else {
            statusMessageEl.textContent      = `Status: Rendering updates (${activeGlyphMeshes.size} glyphs)`;
            // Ensure progress bar is hidden after initial load
            progressContainer.classList.remove('d-flex');
            progressContainer.classList.add('d-none');
        }

        // --- 10.4) Render Scene ---
        renderer.render(scene, camera);
    }

    // ===========================================================================
    // 11) WEBSOCKET: CONNECT & HANDLE MESSAGES
    // ===========================================================================
    const WS_URL = 'ws://visualiser:9002';
    let socket;

    try {
        socket = new WebSocket(WS_URL);
        statusMessageEl.textContent = 'Status: Connecting…';

        socket.addEventListener('open', () => {
            console.log('WebSocket: connected to', WS_URL);
            statusMessageEl.textContent = 'Status: Connected';

            // Only show loading progress on initial connection if no data yet
            if (!firstFrameDrawn) {
                loadingTextEl.textContent = 'Loading data…';
                progressContainer.classList.remove('d-none');
                progressContainer.classList.add('d-flex');
                dataProgress.value        = 0;
                progressTextEl.textContent = '0%';
            }
        });

        socket.addEventListener('message', async (evt) => {
            try {
                const data = JSON.parse(evt.data);

                // Ensure data has expected array structures and glyphs have IDs
                if (
                    Array.isArray(data.points) &&
                    Array.isArray(data.lines) &&
                    Array.isArray(data.glyphs) &&
                    data.glyphs.every(g => g.id !== undefined && g.id !== null) // Ensure all glyphs have an ID
                ) {
                    await updateSceneWithData(data); // Call the updated scene update function
                } else {
                    console.warn('Received unexpected frame format or missing glyph IDs:', data);
                    statusMessageEl.textContent = 'Status: Invalid data received';
                }
            } catch (err) {
                console.error('JSON parse error:', err);
                statusMessageEl.textContent = 'Status: Data parse error';
            }
        });

        socket.addEventListener('error', (err) => {
            console.error('WebSocket error:', err);
            statusMessageEl.textContent = 'Status: WebSocket error';
        });

        socket.addEventListener('close', () => {
            console.warn('WebSocket closed');
            statusMessageEl.textContent = 'Status: Connection closed';
            if (!firstFrameDrawn) {
                // Show loading overlay with error message if connection closes before first frame
                loadingOverlay.classList.remove('d-none');
                loadingOverlay.classList.add('d-flex');

                // Hide other elements and show error message
                loadingTextEl.classList.add('d-none');
                progressContainer.classList.add('d-none');
                errorMessageEl.classList.remove('d-none');
                errorMessageEl.classList.add('d-flex');
                errorMessageEl.textContent = 'Connection lost. Please refresh the page.';
            }
        });
    } catch (err) {
        console.error('Failed to open WebSocket:', err);
        // Show loading overlay with error message if WebSocket connection fails
        loadingOverlay.classList.remove('d-none');
        loadingOverlay.classList.add('d-flex');

        // Hide other elements and show error message
        loadingTextEl.classList.add('d-none');
        progressContainer.classList.add('d-none');
        errorMessageEl.classList.remove('d-none');
        errorMessageEl.classList.add('d-flex');
        errorMessageEl.textContent = 'Could not connect to server. Check that ws://visualiser:9002 is running.';
    }

    // ===========================================================================
    // 12) ANIMATION LOOP (for OrbitControls damping)
    //
    // This function continuously renders the scene, allowing for smooth camera
    // controls and any Three.js animations.
    // ===========================================================================
    function animate() {
        requestAnimationFrame(animate); // Request the next frame
        controls.update(); // Update OrbitControls (for damping and interactivity)
        renderer.render(scene, camera); // Render the scene from the camera's perspective
    }
    animate(); // Start the animation loop

    // ===========================================================================
    // 13) HANDLE RESIZE
    //
    // Adjusts the camera aspect ratio and renderer size when the browser window is resized,
    // ensuring the visualization remains correctly proportioned.
    // ===========================================================================
    window.addEventListener('resize', () => {
        const w = container.clientWidth;
        const h = container.clientHeight;
        camera.aspect = w / h; // Update camera aspect ratio
        camera.updateProjectionMatrix(); // Recalculate projection matrix
        renderer.setSize(w, h); // Resize the renderer
        renderer.render(scene, camera); // Re-render the scene
    });

    // ===========================================================================
    // 14) INITIAL RENDER
    //
    // Performs an initial render of the empty scene once the script loads.
    // This ensures something is displayed before data arrives.
    // ===========================================================================
    renderer.render(scene, camera);
})();
