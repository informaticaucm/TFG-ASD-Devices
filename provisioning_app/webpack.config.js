const path = require('path');
const HtmlWebpackPlugin = require('html-webpack-plugin');
const HtmlInlineScriptPlugin = require('html-inline-script-webpack-plugin');

module.exports = {
    entry: './src/index.js',
    mode: 'development',
    optimization: {
        chunkIds: 'total-size',
        innerGraph: true,
        mangleExports: 'size',
        moduleIds:"size",
        removeAvailableModules: true
    },
    module: {
        rules: [
            {
                test: /\.html$/,
                loader: 'html-loader'
            }],
    },
    plugins: [new HtmlWebpackPlugin({
        template: 'src/index.html'
    }), new HtmlInlineScriptPlugin()],

};