const fs = require("fs");
const { encode } = require('urlencode');
var ncp = require("copy-paste");
const webpack = require("webpack");


webpack(require("./webpack.config"), (err, stats) => {
    if (err || stats.hasErrors()) {
        // Handle errors here
    }

    let web = fs.readFileSync("dist/index.html");

    // data:,Hello%2C World!
    const url = "data:text/html," + encode(web)

    ncp.copy(url);
});
