const QRCode = require('qrcode');

let payload = "";

const segment_size = 100;
const conf_defaults = {
    "thingsboard_url": { default_value: "https://tbm-asistencia.dev.fdi.ucm.es", type: "text" },
    "device_name": { default_value: "name_here", type: "text" },
    "space_id": { default_value: 10, type: "number" },
    "mqtt_broker_url": { default_value: "mqtts://mqtt.dev.fdi.ucm.es:443", type: "text" },
    "provisioning_device_key": { default_value: "o7l9pkujk2xgnixqlimv", type: "text" },
    "provisioning_device_secret": { default_value: "of8htwr0xmh65wjpz7qe", type: "text" },
    "wifi_psw": { default_value: "1234567890", type: "text" },
    "wifi_ssid": { default_value: "tfgseguimientodocente", type: "text" },
    "totp_form_base_url" : { default_value: "https://tbm-asistencia.dev.fdi.ucm.es/api/auth/login", type: "text" },
    "invalidate_backend_auth": { default_value: false, type: "checkbox" },
    "invalidate_thingsboard_auth": { default_value: false, type: "checkbox" },
};

const transmision_interval = 1000;

const refresh_payload = () => {
    conf = {}
    for (const key in conf_defaults) {
        let { type } = conf_defaults[key];
        const input = document.getElementById(key);
        if (type == "number") {
            if (document.getElementById("checkbox_for_" + key).checked)
                conf[key] = parseInt(input.value);
        } else if (type == "checkbox") {

            conf[key] = input.checked;
        } else {
            if (document.getElementById("checkbox_for_" + key).checked)
                conf[key] = input.value;
        }
    }

    payload = JSON.stringify(conf);
    text_display.innerHTML = payload;

}

function field_html(field, default_value, type) {
    if (type == "text") {
        return `
            <div class="field m-1">
                <input type="checkbox" id="checkbox_for_${field}" checked>
                <label class="label" for="checkbox_for_${field}">${field}</label>
                <div class="control">
                    <input class="input form-control" type="text" id="${field}" value="${default_value}">
                </div>
            </div>
        `;
    } else if (type == "number") {
        return `
            <div class="field m-1" >
                <input type="checkbox" id="checkbox_for_${field}" checked>
                <label class="label" for="checkbox_for_${field}">${field}</label>
                <div class="control">
                    <input class="input form-control" type="number" id="${field}" value="${default_value}">
                </div>
            </div>
        `;
    } else if (type == "checkbox") {
        return `
            <div class="field m-1">
                <input type="checkbox" id="${field}" ${default_value ? "checked" : ""}>
                <label class="label" for="${field}">${field}</label>
            </div>
        `;
    }
}


let click = false;

window.addEventListener('load', function () {


    for (const key in conf_defaults) {
        let { default_value, type } = conf_defaults[key];
        document.getElementById("form").innerHTML += field_html(key, default_value, type);
    }


    for (const key in conf_defaults) {
        let { default_value, type } = conf_defaults[key];

        console.log(document.getElementById(key))
        document.getElementById(key).addEventListener("input", refresh_payload);
        if (type != "checkbox") {
            document.getElementById("checkbox_for_" + key).addEventListener("input", refresh_payload);
        }
    }

    refresh_payload()

    document.getElementById("start_transmision").addEventListener("click", async () => {
        document.getElementById('data_input').style.display = 'none';
        document.getElementById('qr_canvas').style.display = 'block';
        await wait(1);

        set_text("reconf" + JSON.stringify({ packet_type: "start", segment_size, segment_count: Math.ceil(payload.length / segment_size) }))
        await wait_click();
        click = false;
        console.log("sending segments")
        do {
            for (let i = 0; i < payload.length; i += segment_size) {
                set_text("reconf" + JSON.stringify({ packet_type: "segment", i: i / segment_size, data: payload.slice(i, i + segment_size) }))
                await wait(transmision_interval);
                if (click) break;
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