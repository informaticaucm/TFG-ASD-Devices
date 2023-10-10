var express = require('express');
var app = express();

app.get('/ping', function (req, res) {
    console.log("man pineao", req.originalUrl, req.query);
    res.send("Hello world!");
});

app.listen(3000);