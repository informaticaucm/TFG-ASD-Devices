const QRCode = require('qrcode');
const { cli } = require('webpack');

let payload = "";

const segment_size = 40;
const conf_defaults = {
    "thingsboard_url": "https://thingsboard.asd:8080",
    "device_name": "name_here",
    "space_id": 10,
    "mqtt_broker_url": "mqtts://thingsboard.asd:8883",
    "provisioning_device_key": "o7l9pkujk2xgnixqlimv",
    "provisioning_device_secret": "of8htwr0xmh65wjpz7qe",
    "wifi_psw": "1234567890",
    "wifi_ssid": "tfgseguimientodocente",
    "invalidate_backend_auth_auth": "0",
    "invalidate_thingsboard_auth_auth": "0",
};

const transmision_interval = 1000;

const refresh_payload = () => {
    conf = {}
    for (const key in conf_defaults) {
        const input = document.getElementById(key);
        conf[key] = input.value == "true" ? true : input.value == "false" ? false : input.value;
    }

    payload = JSON.stringify(conf);
    text_display.innerHTML = payload;

}


let click = false;

window.addEventListener('load', function () {


    for (const key in conf_defaults) {
        const input = document.getElementById(key);
        input.value = conf_defaults[key];
        input.addEventListener("input", refresh_payload)
        refresh_payload()
    }

    document.getElementById("start_transmision").addEventListener("click", async () => {
        document.getElementById('data_input').style.display = 'none';
        document.getElementById('qr_canvas').style.display = 'block';

        set_text("reconf" + JSON.stringify({ packet_type: "start", segment_size, segment_count: Math.ceil(payload.length / segment_size) }))
        await wait_click();
        click = false;
        console.log("sending segments")
        do {
            for (let i = 0; i < payload.length; i += segment_size) {
                set_text("reconf" + JSON.stringify({ packet_type: "segment", i : i / segment_size, data: payload.slice(i, i + segment_size) }))
                await wait(transmision_interval);
            }
        } while (!click)

        document.getElementById('data_input').style.display = 'block';
        document.getElementById('qr_canvas').style.display = 'none';
    });
})

document.addEventListener('click', () => { console.log("click"); click = true })

function wait(ms) {
    return new Promise(resolve => setTimeout(resolve, ms));
}

function wait_click() {
    click = false;
    return new Promise(resolve => {
        const interval = setInterval(() => {
            if (click) {
                clearInterval(interval);
                resolve();
            }
        }, 100);
    });
}

function set_text(text) {
    const canvas = document.getElementById('qr_canvas')

    QRCode.toCanvas(canvas, text, { width: Math.min(canvas.clientWidth, canvas.clientHeight) }, function (error) {
        if (error) console.error(error)
        console.log('success!');
    })

}