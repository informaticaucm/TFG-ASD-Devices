const DeviceHive = require('devicehive');

// Getting Device model
const Device = DeviceHive.models.Device
// Getting Device list query model
const DeviceListQuery = DeviceHive.models.query.DeviceListQuery;

// Configurating DeviceHive
const myDeviceHive = new DeviceHive({
    login: 'dhadmin',
    password: 'dhadmin_#911',
    mainServiceURL: 'http://localhost/api/rest',
    authServiceURL: 'http://localhost/auth/rest',
    pluginServiceURL: 'http://localhost/plugin/rest'
});

// Configurating Device model
const device = new Device({
    id: 'myTestId',
    name: 'myTestName',
    networkId: 1,
    deviceTypeId: 1,
    blocked: false
});

// Configurating Device List query
const myDeviceListQuery = new DeviceListQuery({
    networkId: 1
});

// Push message handler
myDeviceHive.on(DeviceHive.MESSAGE_EVENT, (message) => {
    console.log(message);
});

// Error handler
myDeviceHive.on(DeviceHive.ERROR_EVENT, (error) => {
    console.error(error);
});

// Connecting and usin API
myDeviceHive.connect()
    .then(() => myDeviceHive.device.list(myDeviceListQuery))
    .then(data => console.log("a:", data))
    .then(() => myDeviceHive.device.add(device))
    .then(data => console.log("b:", data))
    .then(() => myDeviceHive.device.list(myDeviceListQuery))
    .then(data => console.log("c:", data))
    .then(() => myDeviceHive.device.delete(device.id))
    .then(data => console.log("d:", data))
    .then(() => myDeviceHive.device.list(myDeviceListQuery))
    .then(data => console.log("e:", data))
    .catch(error => console.warn(error));