/**
 * client/visualiser.js (ES Module)
 *
 * • Connects to ws://localhost:9003
 * • Receives JSON frames with: { points: [...], lines: [...], glyphs: [...] }
 * • Renders them via Three.js + OrbitControls in a single canvas.
 *
 * Dependencies (relative to client/):
 *   • ./jsm/three.module.js
 *   • ./jsm/controls/OrbitControls.js
 */

import * as THREE from './jsm/three.module.js';
import { OrbitControls } from './jsm/controls/OrbitControls.js';

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

    let firstFrameDrawn = false;

    // ===========================================================================
    // 2) CHECK WEBGL SUPPORT
    // ===========================================================================
    if (!webgl_support()) {
        loadingOverlay.style.display = 'flex';
        loadingOverlay.innerHTML = `
      <div style="color: #FF6666; font-size:1.2rem;">
        WebGL not supported in your browser.<br>
        Please try a modern browser or update your graphics drivers.
      </div>
    `;
        console.error('WebGL not supported');
        return;
    }

    // ===========================================================================
    // 3) THREE.JS SCENE SETUP
    // ===========================================================================
    const scene = new THREE.Scene();
    scene.background = new THREE.Color(0x222222);

    // Camera
    const camera = new THREE.PerspectiveCamera(
        60,
        container.clientWidth / container.clientHeight,
        0.1,
        1000
    );
    camera.position.set(0, 0, 50);

    // Renderer
    const renderer = new THREE.WebGLRenderer({ antialias: true });
    renderer.setPixelRatio(window.devicePixelRatio);
    renderer.setSize(container.clientWidth, container.clientHeight);
    container.appendChild(renderer.domElement);

    // Add lights
    const ambientLight     = new THREE.AmbientLight(0xffffff, 0.4);
    const directionalLight = new THREE.DirectionalLight(0xffffff, 0.6);
    directionalLight.position.set(10, 10, 10);
    scene.add(ambientLight, directionalLight);

    // ===========================================================================
    // 4) ORBITCONTROLS SETUP
    // ===========================================================================
    const controls = new OrbitControls(camera, renderer.domElement);
    controls.enableDamping   = true;
    controls.dampingFactor   = 0.1;
    controls.enableZoom      = true;
    controls.minDistance     = 5;
    controls.maxDistance     = 200;

    // ===========================================================================
    // 5) PROTOTYPE GEOMETRIES FOR GLYPHS
    // ===========================================================================
    const sphereGeometry   = new THREE.SphereGeometry(0.5, 16, 16);
    const boxGeometry      = new THREE.BoxGeometry(0.5, 0.5, 0.5);
    const cylinderGeometry = new THREE.CylinderGeometry(0.1, 0.1, 1.0, 16);

    // ===========================================================================
    // 6) ROOT GROUP FOR DYNAMIC GEOMETRY
    // ===========================================================================
    const rootGroup = new THREE.Group();
    scene.add(rootGroup);

    // Optional: keep a reference to the last lines mesh so we can dispose its geometry/material
    let currentLinesMesh = null;

    // ===========================================================================
    // 7) UTIL: CLEAR A GROUP (dispose all children)
    // ===========================================================================
    function clearGroup(group) {
        while (group.children.length > 0) {
            const child = group.children[0];
            group.remove(child);
            if (child.geometry) child.geometry.dispose();
            if (child.material) {
                if (Array.isArray(child.material)) {
                    child.material.forEach((m) => m.dispose());
                } else {
                    child.material.dispose();
                }
            }
        }
    }

    // ===========================================================================
    // 8) UTIL: BUILD LINESEGMENTS FOR “lines” + “points”
    //    → We treat all lines as one “step” for progress
    // ===========================================================================
    function buildLines(linesArray, pointsArray) {
        const numLines  = linesArray.length;
        const positions = new Float32Array(numLines * 6);

        for (let i = 0; i < numLines; ++i) {
            const [idx0, idx1] = linesArray[i];
            const p0 = pointsArray[idx0];
            const p1 = pointsArray[idx1];
            const base = i * 6;
            positions[base + 0] = p0[0];
            positions[base + 1] = p0[1];
            positions[base + 2] = p0[2];
            positions[base + 3] = p1[0];
            positions[base + 4] = p1[1];
            positions[base + 5] = p1[2];
        }

        const geometry = new THREE.BufferGeometry();
        geometry.setAttribute('position', new THREE.BufferAttribute(positions, 3));
        const material = new THREE.LineBasicMaterial({ color: 0xffff00, linewidth: 2 });
        return new THREE.LineSegments(geometry, material);
    }

    // ===========================================================================
    // 9) UTIL: BUILD A SINGLE GLYPH MESH (so we can increment progress per glyph)
    // ===========================================================================
    function buildSingleGlyph(entry) {
        const { pos, type, color } = entry;
        const [x, y, z] = pos;
        const [r, g, b] = color.map((c) => c / 255); // normalize to [0,1]

        // Choose geometry by type
        let geometry;
        switch (type) {
            case 1:  // neuron center
            case 4:  // soma
            case 5:  // axon hillock
            case 10: // synaptic gap
                geometry = sphereGeometry;
                break;
            case 2:  // dendrite / bouton
            case 7:  // axon branch
            case 8:  // axon
                geometry = cylinderGeometry;
                break;
            case 3:  // dendrite branch
            case 9:  // axon bouton
                geometry = boxGeometry;
                break;
            default:
                geometry = sphereGeometry;
        }

        const material = new THREE.MeshPhongMaterial({ color: new THREE.Color(r, g, b) });
        const mesh     = new THREE.Mesh(geometry, material);
        mesh.position.set(x, y, z);
        return mesh;
    }

    // ===========================================================================
    // 10) MAIN: REBUILD SCENE WHEN JSON ARRIVES
    //
    //     This is now async so we can chunk “build glyph by glyph” and yield
    //     to the browser between steps (via requestAnimationFrame) to update the progress bar.
    // ===========================================================================
    async function rebuildSceneFromJSON(data) {
        // 10.1) Clear previous dynamic geometry
        clearGroup(rootGroup);

        // 10.2) Count total steps: (1 for all lines, if any) + (1 per glyph)
        const totalLines  = Array.isArray(data.lines) && Array.isArray(data.points)
            ? data.lines.length
            : 0;
        const totalGlyphs = Array.isArray(data.glyphs) ? data.glyphs.length : 0;
        const lineSteps   = totalLines > 0 ? 1 : 0;
        const totalSteps  = lineSteps + totalGlyphs;
        let completedSteps = 0;

        // 10.3) If there is any data to load, show + reset the progress UI
        if (totalSteps > 0) {
            loadingOverlay.style.display     = 'flex';
            progressContainer.style.display  = 'flex';
            loadingTextEl.textContent        = 'Loading data…';
            dataProgress.value               = 0;
            progressTextEl.textContent       = '0%';

            // Give the browser one frame to paint “Loading data…” + the empty progress bar
            await new Promise((resolve) => requestAnimationFrame(resolve));
        }

        // Helper to update the progress bar + text
        function updateProgress() {
            const pct = Math.floor((completedSteps / totalSteps) * 100);
            dataProgress.value        = pct;
            progressTextEl.textContent = `${pct}%`;
        }

        // 10.4) Build all lines in a single step (if any)
        if (totalLines > 0 && Array.isArray(data.points)) {
            const linesMesh = buildLines(data.lines, data.points);
            rootGroup.add(linesMesh);
            currentLinesMesh = linesMesh;
            completedSteps += 1;
            updateProgress();
            // Yield so the progress bar can actually redraw
            await new Promise((resolve) => requestAnimationFrame(resolve));
        }

        // 10.5) Build each glyph one at a time (updating progress each time)
        if (totalGlyphs > 0) {
            for (let i = 0; i < totalGlyphs; i++) {
                const glyphEntry = data.glyphs[i];
                const mesh       = buildSingleGlyph(glyphEntry);
                rootGroup.add(mesh);

                completedSteps += 1;
                updateProgress();
                // Yield so the progress bar can repaint
                await new Promise((resolve) => requestAnimationFrame(resolve));
            }
        }

        // 10.6) First‐frame housekeeping (hide overlay once everything’s in place)
        if (!firstFrameDrawn) {
            loadingOverlay.style.display     = 'none';
            progressContainer.style.display  = 'none';
            firstFrameDrawn                  = true;
            statusMessageEl.textContent      = 'Status: Rendering data';
        }

        // 10.7) Render one final time (OrbitControls will keep animating)
        renderer.render(scene, camera);
    }

    // ===========================================================================
    // 11) WEBSOCKET: CONNECT & HANDLE MESSAGES
    // ===========================================================================
    const WS_URL = 'ws://localhost:9003';
    let socket;

    try {
        socket = new WebSocket(WS_URL);
        statusMessageEl.textContent = 'Status: Connecting…';

        socket.addEventListener('open', () => {
            console.log('WebSocket: connected to', WS_URL);
            statusMessageEl.textContent = 'Status: Connected';

            // Switch from “Connecting…” to “Loading data…”
            loadingTextEl.textContent       = 'Loading data…';
            progressContainer.style.display = 'flex';
            // Ensure the progress bar is visible before we begin
            dataProgress.value        = 0;
            progressTextEl.textContent = '0%';
        });

        socket.addEventListener('message', async (evt) => {
            try {
                const data = JSON.parse(evt.data);

                if (
                    Array.isArray(data.points) &&
                    Array.isArray(data.lines) &&
                    Array.isArray(data.glyphs)
                ) {
                    await rebuildSceneFromJSON(data);
                } else {
                    console.warn('Received unexpected frame format:', data);
                }
            } catch (err) {
                console.error('JSON parse error:', err);
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
                loadingOverlay.style.display = 'flex';
                loadingOverlay.innerHTML = `
          <div style="color: #FF6666; font-size:1.2rem;">
            Connection lost.<br>
            Please refresh the page.
          </div>
        `;
            }
        });
    } catch (err) {
        console.error('Failed to open WebSocket:', err);
        loadingOverlay.style.display = 'flex';
        loadingOverlay.innerHTML = `
      <div style="color: #FF6666; font-size:1.2rem;">
        Could not connect to server.<br>
        Check that ws://localhost:9003 is running.
      </div>
    `;
    }

    // ===========================================================================
    // 12) ANIMATION LOOP (for OrbitControls damping)
    // ===========================================================================
    function animate() {
        requestAnimationFrame(animate);
        controls.update();
        renderer.render(scene, camera);
    }
    animate();

    // ===========================================================================
    // 13) HANDLE RESIZE
    // ===========================================================================
    window.addEventListener('resize', () => {
        const w = container.clientWidth;
        const h = container.clientHeight;
        camera.aspect = w / h;
        camera.updateProjectionMatrix();
        renderer.setSize(w, h);
        renderer.render(scene, camera);
    });

    // ===========================================================================
    // 14) INITIAL RENDER
    // ===========================================================================
    renderer.render(scene, camera);
})();
