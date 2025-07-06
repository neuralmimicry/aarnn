// server.js
const express = require('express');
const path = require('path');
const app = express();

// 1) Add CSP header before any routing/static‐serving:
app.use((req, res, next) => {
    res.setHeader(
        'Content-Security-Policy',
        "default-src 'self'; " +
        "script-src 'self'; " + // Removed 'unsafe-eval'
        "style-src 'self'; " +   // Removed 'unsafe-inline'
        "connect-src 'self' ws://visualiser:9002; " +
        "img-src 'self' data:; " + // If you use any base64 encoded images or external images
        "font-src 'self'; " +     // If you use custom fonts
        "object-src 'none'; " +   // Prevents Flash/Java applets
        "base-uri 'self';" +      // Restricts base URL for relative URLs
        "form-action 'self';" +   // Restricts URLs that can be used as the target of form submissions
        "frame-ancestors 'none';" // Prevents clickjacking
    );
    next();
});

// 2) Serve everything under ./client as static:
app.use(express.static(path.join(__dirname, 'pages')));

// 3) Fallback: if someone hits “/” without a filename, serve index.html:
app.get('/', (req, res) => {
    res.sendFile(path.join(__dirname, 'pages', 'index.html'));
});

// 4) Start listening (8180, or whatever you prefer)
const PORT = 8180;
app.listen(PORT, () => {
    console.log(`Listening on http://localhost:${PORT}`);
});
