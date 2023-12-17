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

const public_domain = "domain-name.com"

app.post("/api/v1/seguimiento", (req, res) => {
    let { fw_version, qr_content } = req.body;

    res.json({
        "text": "qr recibido, contenido era: " + qr_content,
        "duration": 17,
        "icon_id": Math.floor(Math.random() * 2.999)
    });
})

app.post("/api/v1/dispositivos", (req, res) => {
    let { nombre, espacioId, idExternoDispositivo } = req.body;

    res.json({
        // "id": 1,
        // "creadoEn": "2023-12-17T19:09:20.155Z",
        // "actualizadoEn": "2023-12-17T19:09:20.155Z",
        // "creadoPor": 1,
        // "actualizadoPor": 2,
        // "nombre": "Lector QR, BLE, NFC",
        // "espacioId": 1,
        // "idExternoDispositivo": "abcdefg",
        // "endpointSeguimiento": "https://localhost:3000/api/v1/seguimiento",
        "qr_url_template": `https://${public_domain}/submit/?totp=%d&space_id=${espacioId}`, // de momento nos matamos con un %d
        "totpConfig": {
            "t0": 0,
            "secret": "JBSWY3DPEHPK3PXP"
        }
    });
})

app.get("/api/v1/ping", (req, res) => {
    res.json({ "epoch": Math.floor(Date.now() / 1000) })
});

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