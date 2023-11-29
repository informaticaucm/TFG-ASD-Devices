var express = require('express');
// var https = require('https');
var http = require('http');
var fs = require('fs');
var bodyparser = require('body-parser');

const PORT = Number.parseInt(process.argv[0]) || 8888;

// This line is from the Node.js HTTPS documentation.
var options = {
    key: fs.readFileSync('../keys/https/server_key.pem'),
    cert: fs.readFileSync('../keys/https/server_cert.pem')
};

var app = express();
app.use(bodyparser.json({ limit: '50mb' }));

const THINGSBOARD_URL = "http://thingsboard.asd:8080";

/*
curl -X POST --header 'Content-Type: application/json' --header 'Accept: application/json' -d '{"username":"tenant@thingsboard.org", "password":"tenant"}' 'http://THINGSBOARD_URL/api/auth/login'

*/
const SERVER_JWT_TOKEN = ""

app.post("/rpc", (req, res) => {
    let { devicename } = req.headers
    let { method, params } = req.body
    console.log("url: ", req.originalUrl, params, method, devicename);

    if (method == "ping") {
        res.json({ "method": "pong", "response": { "epoch": Math.floor(Date.now() / 1000), "ok": true } })
    } else if (method == "qr") {
        http.request({
            host: 'thingsboard.asd',
            port: 8080,
            path: '/api/plugins/rpc/oneway/{deviceId}',
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
                'Content-Length': Buffer.byteLength(JSON.stringify(params)),
                "X-Authorization": SERVER_JWT_TOKEN
            }
        })

        /*
        curl -v -X POST -d @set-gpio-request.json http://localhost:8080/api/plugins/rpc/twoway/$DEVICE_ID \
--header "Content-Type:application/json" \
--header "X-Authorization: $JWT_TOKEN"
*/

        res.json({ "ok": true })
    }
    else {
        res.json({ "ok": true })
    }
})

/* rpc to display text on the device
{
    "method": "display_text",
    "params": {
        "text": "Hello World"
        "duration": 5
    }
}
callType is either oneway or twoway;
send that in a post to http://thingsboard.asd:8080/api/plugins/rpc/{callType}/{deviceId}
*/




http.createServer(app).listen(8888);
// https.createServer(options, app).listen(PORT);


console.log("https server started on port", PORT)