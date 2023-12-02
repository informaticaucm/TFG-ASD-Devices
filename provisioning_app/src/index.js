const QRCode = require('qrcode')

window.addEventListener('load', function () {

    let conf_defaults = { "thingsboard_url":"https://thingsboads.asd:8080","device_name": "name_here", "mqtt_broker_url": "mqtts://thingsboard.asd:8883", "provisioning_device_key": "o7l9pkujk2xgnixqlimv", "provisioning_device_secret": "of8htwr0xmh65wjpz7qe", "wifi_psw": "1234567890", "wifi_ssid": "tfgseguimientodocente" };
    for (const key in conf_defaults) {
        const input = this.document.getElementById(key);
        input.value = conf_defaults[key];

        input.addEventListener("input", () => {
            conf = {}
            for (const key in conf_defaults) {
                const input = this.document.getElementById(key);
                conf[key] = input.value;
            }

            let qr_payload = `reconf` + JSON.stringify(conf)
            set_text(qr_payload);
        })
    }


})

function set_text(text) {
    const canvas = document.getElementById('qr_canvas')

    QRCode.toCanvas(canvas, text, function (error) {
        if (error) console.error(error)
        console.log('success!');
        text_display.innerHTML = text;
    })
}