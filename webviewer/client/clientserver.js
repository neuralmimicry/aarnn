// server.js
const express = require('express');
const path = require('path');
const app = express();

// 1) Add CSP header before any routing/static‐serving:
app.use((req, res, next) => {
    res.setHeader(
        'Content-Security-Policy',
        // match the meta‐tag you had in index.html:
        "default-src 'self'; " +
        "script-src 'self' 'unsafe-eval'; " +
        "style-src 'self' 'unsafe-inline'; " +
        "connect-src 'self' ws://visualiser:9002"
    );
    next();
});

// 2) Serve everything under ./client as static:
app.use(express.static(path.join(__dirname, 'pages')));

// 3) Fallback: if someone hits “/” without a filename, serve index.html:
app.get('/', (req, res) => {
    res.sendFile(path.join(__dirname, 'pages', 'index.html'));
});

// 4) Start listening (8080, or whatever you prefer)
const PORT = 8080;
app.listen(PORT, () => {
    console.log(`Listening on http://localhost:${PORT}`);
});
