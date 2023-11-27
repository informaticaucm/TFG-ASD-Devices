var express = require('express');
// var https = require('https');
var http = require('http');
var fs = require('fs');

const PORT = Number.parseInt(process.argv[0]) || 8888;

// This line is from the Node.js HTTPS documentation.
var options = {
    key: fs.readFileSync('../keys/https/server_key.pem'),
    cert: fs.readFileSync('../keys/https/server_cert.pem')
};

var app = express();

app.get("/ping", (req, res) => {
    console.log("url: ", req.originalUrl);
    console.log("    headers: ", req.headers);
    res.json({ "message": "pong", "epoch": Math.floor(Date.now() / 1000), "ok": true })
})

app.get("/qr", (req, res) => {
    console.log("url: ", req.originalUrl);
    console.log("    headers: ", req.headers);

    res.json({ "ok": true })
})

http.createServer(app).listen(8888);
// https.createServer(options, app).listen(PORT);


console.log("https server started on port", PORT)