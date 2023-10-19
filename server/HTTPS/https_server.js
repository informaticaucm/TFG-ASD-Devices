var express = require('express');
var https = require('https');
// var http = require('http');
var fs = require('fs');
var morgan = require('morgan')

const PORT = Number.parseInt(process.argv[0]) || 443;

// This line is from the Node.js HTTPS documentation.
var options = {
    key: fs.readFileSync('../keys/https/server_key.pem'),
    cert: fs.readFileSync('../keys/https/server_cert.pem')
};

var app = express();
app.use(morgan('tiny')) // https://expressjs.com/en/resources/middleware/morgan.html
app.use(express.static("../public/"))

// http.createServer(app).listen(80);
https.createServer(options, app).listen(PORT);

console.log("https server started on port", PORT)