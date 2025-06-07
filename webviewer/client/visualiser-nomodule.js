/**
 * client/visualiser-nomodule.js (refactored to avoid long‐blocking in WebSocket handler)
 *
 * • Parses incoming JSON but defers all heavy scene rebuilding to the animation loop.
 * • Reuses a single dynamic BufferGeometry for all lines (no re‐creation every frame).
 * • You can further optimize glyphs by pooling or using InstancedMesh.
 */

(function () {
    'use strict';

    // =============================================================================
    // 1) HTML ELEMENTS & INITIAL STATE
    // =============================================================================

    const container = document.getElementById('threeContainer');
    const loadingOverlay = document.getElementById('loadingOverlay');

    // We will store the most recent frame here:
    let latestFrame = null;
    // A flag indicating the scene needs to be rebuilt before the next render
    let needsRebuild = false;
    // Track if we've drawn at least one frame
    let firstFrameDrawn = false;

    // =============================================================================
    // 2) THREE.JS SCENE SETUP
    // =============================================================================

    const scene = new THREE.Scene();
    scene.background = new THREE.Color(0x222222);

    const camera = new THREE.PerspectiveCamera(
        60,
        container.clientWidth / container.clientHeight,
        0.1,
        1000
    );
    camera.position.set(0, 0, 50);
    camera.lookAt(0, 0, 0);

    const renderer = new THREE.WebGLRenderer({ antialias: true });
    renderer.setSize(container.clientWidth, container.clientHeight);
    container.appendChild(renderer.domElement);

    // =============================================================================
    // 3) LIGHTING
    // =============================================================================

    const ambientLight = new THREE.AmbientLight(0xffffff, 0.4);
    scene.add(ambientLight);

    const directionalLight = new THREE.DirectionalLight(0xffffff, 0.6);
    directionalLight.position.set(10, 10, 10);
    scene.add(directionalLight);

    // =============================================================================
    // 4) ORBITCONTROLS
    // =============================================================================

    const controls = new THREE.OrbitControls(camera, renderer.domElement);
    controls.enableDamping = true;
    controls.dampingFactor = 0.1;
    controls.enableZoom = true;
    controls.minDistance = 5;
    controls.maxDistance = 200;

    // =============================================================================
    // 5) PROTOTYPE GEOMETRIES FOR GLYPHS (UNCHANGED)
    // =============================================================================

    const sphereGeometry = new THREE.SphereGeometry(0.5, 16, 16);
    const boxGeometry = new THREE.BoxGeometry(0.5, 0.5, 0.5);
    const cylinderGeometry = new THREE.CylinderGeometry(0.1, 0.1, 1.0, 16);

    // =============================================================================
    // 6) ROOT GROUP FOR DYNAMIC OBJECTS
    // =============================================================================

    const rootGroup = new THREE.Group();
    scene.add(rootGroup);

    // We will keep references to:
    //  - a single "linesMesh" (LineSegments + buffer geometry) that we update each frame
    //  - a parent "glyphsGroup" where we either pool glyph meshes or recreate them
    let linesMesh = null;
    let glyphsGroup = new THREE.Group();
    rootGroup.add(glyphsGroup);

    // =============================================================================
    // 7) HELPER: CREATE OR UPDATE THE LINES BufferGeometry
    // =============================================================================

    /**
     * updateLinesMesh(linesArray, pointsArray)
     *
     * Instead of disposing/re‐creating the geometry, we update a single dynamic buffer.
     * On first call (when linesMesh is null), we create a new BufferGeometry/LineSegments.
     * On subsequent calls, we simply overwrite the "position" attribute data and mark it for update.
     */
    function updateLinesMesh(linesArray, pointsArray) {
        const numLines = linesArray.length;
        // Each line has 2 endpoints ⇒ 6 floats
        const expectedLength = numLines * 6;

        if (linesMesh === null) {
            // First frame: create BufferGeometry + LineSegments
            const positions = new Float32Array(expectedLength);
            const geometry = new THREE.BufferGeometry();
            geometry.setAttribute('position', new THREE.BufferAttribute(positions, 3));
            const material = new THREE.LineBasicMaterial({ color: 0xffff00 });
            linesMesh = new THREE.LineSegments(geometry, material);
            rootGroup.add(linesMesh);
        }

        // Now we know linesMesh exists; update its buffer
        const attr = linesMesh.geometry.getAttribute('position');

        // If the current buffer is too small (previous frame had more lines),
        // we need to reallocate a larger Float32Array. Otherwise, reuse.
        if (attr.array.length !== expectedLength) {
            linesMesh.geometry.setAttribute(
                'position',
                new THREE.BufferAttribute(new Float32Array(expectedLength), 3)
            );
        }

        // Write into the buffer
        const positions = linesMesh.geometry.getAttribute('position').array;
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

        // If the new buffer is shorter than the old one, we should Zero‐out the trailing entries
        // to avoid “ghost lines.” The simplest is to just set zeros for any leftover floats:
        if (positions.length > expectedLength) {
            for (let i = expectedLength; i < positions.length; ++i) {
                positions[i] = 0;
            }
        }

        // Tell Three.js the attribute has changed:
        linesMesh.geometry.getAttribute('position').needsUpdate = true;
        // Update the draw range so we only draw exactly numLines*2 vertices:
        linesMesh.geometry.setDrawRange(0, numLines * 2);
    }

    // =============================================================================
    // 8) HELPER: REBUILD GLYPH MESHES (BASIC APPROACH – CAN BE IMPROVED)
    // =============================================================================

    /**
     * rebuildGlyphMeshes(glyphArray)
     *
     * For simplicity, this example just removes all previous glyphs and re‐creates them.
     * In a real production scenario, you’d either:
     *   • Pool a fixed number of Meshes and simply update their position/color/etc.
     *   • Use InstancedMesh so all glyphs share a single geometry/material.
     *
     * Here we do the “clear & re‐create” pattern, but it runs inside requestAnimationFrame,
     * so it won’t block the WebSocket callback.
     */
    function rebuildGlyphMeshes(glyphArray) {
        // Remove old glyphs
        while (glyphsGroup.children.length > 0) {
            const child = glyphsGroup.children[0];
            glyphsGroup.remove(child);
            if (child.geometry) child.geometry.dispose();
            if (child.material) {
                if (Array.isArray(child.material)) {
                    child.material.forEach((m) => m.dispose());
                } else {
                    child.material.dispose();
                }
            }
        }

        // Create new ones
        for (let i = 0; i < glyphArray.length; ++i) {
            const { pos, type, color } = glyphArray[i];
            const [x, y, z] = pos;
            const [r, g, b] = color;

            let geometry;
            switch (type) {
                case 1:
                case 4:
                case 5:
                case 10:
                    geometry = sphereGeometry;
                    break;
                case 2:
                case 7:
                case 8:
                    geometry = cylinderGeometry;
                    break;
                case 3:
                case 9:
                    geometry = boxGeometry;
                    break;
                default:
                    geometry = sphereGeometry;
            }

            const threeColor = new THREE.Color(r / 255, g / 255, b / 255);
            const material = new THREE.MeshPhongMaterial({ color: threeColor });
            const mesh = new THREE.Mesh(geometry, material);
            mesh.position.set(x, y, z);

            glyphsGroup.add(mesh);
        }
    }

    // =============================================================================
    // 9) MAIN: REBUILD SCENE (LINES + GLYPHS) ONCE PER ANIMATION FRAME
    // =============================================================================

    function rebuildScene() {
        if (!latestFrame) {
            return;
        }

        // 9.1) Update lines first
        updateLinesMesh(latestFrame.lines, latestFrame.points);

        // 9.2) Rebuild glyphs
        rebuildGlyphMeshes(latestFrame.glyphs);

        // 9.3) Hide overlay if first draw
        if (!firstFrameDrawn) {
            loadingOverlay.style.display = 'none';
            firstFrameDrawn = true;
        }
    }

    // =============================================================================
    // 10) WEBSOCKET HANDLER: PARSE & FLAG FOR REBUILD (NO HEAVY WORK HERE)
    // =============================================================================

    const WS_URL = 'ws://localhost:9002';
    const socket = new WebSocket(WS_URL);

    socket.onopen = () => {
        console.log('Connected to WebSocket:', WS_URL);
    };

    socket.onmessage = (evt) => {
        // Parse JSON and stash it; do not rebuild here
        try {
            const data = JSON.parse(evt.data);

            // Basic sanity check
            if (
                Array.isArray(data.points) &&
                Array.isArray(data.lines) &&
                Array.isArray(data.glyphs)
            ) {
                latestFrame = data;
                // Signal that we need a rebuild before the next render
                needsRebuild = true;
            } else {
                console.warn('Unexpected frame format', data);
            }
        } catch (err) {
            console.error('JSON parse error:', err);
        }
    };

    socket.onerror = (err) => {
        console.error('WebSocket error:', err);
    };
    socket.onclose = () => {
        console.warn('WebSocket closed.');
        if (!firstFrameDrawn) {
            loadingOverlay.textContent = 'Connection lost.';
        }
    };

    // =============================================================================
    // 11) ANIMATION LOOP: PERFORM REBUILD IF NEEDED, THEN RENDER
    // =============================================================================

    function animate() {
        requestAnimationFrame(animate);

        // Only rebuild when we have a new frame
        if (needsRebuild) {
            rebuildScene();
            needsRebuild = false;
        }

        controls.update();
        renderer.render(scene, camera);
    }
    animate();

    // =============================================================================
    // 12) INITIAL RENDER
    // =============================================================================

    renderer.render(scene, camera);

    // =============================================================================
    // 13) WINDOW RESIZE HANDLER
    // =============================================================================

    window.addEventListener('resize', () => {
        const w = container.clientWidth;
        const h = container.clientHeight;
        camera.aspect = w / h;
        camera.updateProjectionMatrix();
        renderer.setSize(w, h);
        renderer.render(scene, camera);
    });
})();
