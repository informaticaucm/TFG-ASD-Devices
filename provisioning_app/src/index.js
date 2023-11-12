const QRCode = require('qrcode')

window.addEventListener('load', function () {

    const input = this.document.getElementById("name_input");

    input.addEventListener("blur", () => {
        let qr_payload = `reconf{"device_name":"${input.value}","mqtt_broker_url":"mqtts://thingsboard.asd:8883","provisioning_device_key":"o7l9pkujk2xgnixqlimv","provisioning_device_secret":"of8htwr0xmh65wjpz7qe","wifi_psw":"1234567890","wifi_ssid":"tfgseguimientodocente"}`
        set_text(qr_payload);
    })

})

function set_text(text) {
    const canvas = document.getElementById('qr_canvas')

    QRCode.toCanvas(canvas, text, function (error) {
        if (error) console.error(error)
        console.log('success!');
        text_display.innerHTML = text;
    })
}