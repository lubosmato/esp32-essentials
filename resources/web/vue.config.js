const webpack = require("webpack");
const CompressionPlugin = require("compression-webpack-plugin");
const OptimizeCSSAssetsPlugin = require("optimize-css-assets-webpack-plugin");
const fs = require("fs");

const deviceName = fs.readFileSync("device-name.txt", "utf8").toString();

module.exports = {
  devServer: {
    proxy: {
      "^/config": {
        target: "http://127.0.0.1:8081/",
        ws: false,
        changeOrigin: true
      }
    }
  },
  pages: {
    index: {
      entry: "src/main.js",
      template: "public/index.html",
      filename: "index.html",
      title: deviceName,
      chunks: ["chunk-vendors", "chunk-common", "index"]
    }
  },
  configureWebpack: {
    plugins: [
      new CompressionPlugin({
        algorithm: "gzip"
      }),
      new webpack.optimize.LimitChunkCountPlugin({
        maxChunks: 1
      })
    ],
    optimization: {
      minimizer: [new OptimizeCSSAssetsPlugin({})]
    },
    output: {
      filename: "app.js"
    }
  },
  css: {
    extract: false
  }
};
