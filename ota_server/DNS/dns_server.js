const dns2 = require('dns2');
const fs = require("fs");

const { Packet, TCPClient } = dns2;

// const resolve = TCPClient({
//     dns: '1.1.1.1'
// });

const server = dns2.createServer({
    udp: true,
    handle: async (request, send, rinfo) => {
        const config = JSON.parse(fs.readFileSync("./ip_asignement.json"))
        const response = Packet.createResponseFromRequest(request);
        const [question] = request.questions;
        const { name } = question;
        if (Object.keys(config).indexOf(name) != -1) {
            console.log("cache hit:", name )
            response.answers.push({
                name,
                type: Packet.TYPE.A,
                class: Packet.CLASS.IN,
                ttl: 300,
                address: config[name]
            });
        } else {
            // const result = await resolve(name);
            // response.answers = [...response.answers, ...result.answers]
        }
        send(response);


    }
});

// server.on('request', (request, response, rinfo) => {
//     console.log(request.header.id, request.questions[0]);
// });

// server.on('requestError', (error) => {
//     console.log('Client sent an invalid request', error);
// });

server.on('listening', () => {
    console.log(server.addresses());
});

server.on('close', () => {
    console.log('server closed');
});

server.listen({
    // Optionally specify port, address and/or the family of socket() for udp server:
    udp: {
        port: 53,
        address: "0.0.0.0",
        type: "udp4",  // IPv4 or IPv6 (Must be either "udp4" or "udp6")
    },

    // Optionally specify port and/or address for tcp server:
    tcp: {
        port: 53,
        address: "0.0.0.0",
    },
});