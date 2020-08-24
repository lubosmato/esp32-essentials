const config = {
  ssid: "esp32",
  pass: "12345678",
  version: "0.0.1"
};

const express = require("express");
const app = express();
const bodyParser = require("body-parser");

app.use(bodyParser.text());

app.get("/config/:key", (req, res) => {
  const value = config[req.params.key];
  if (value === undefined) {
    res.status(404);
    res.end("ERROR");
    return;
  }

  res.status(200);
  res.end(value);
});

app.post("/config/:key", (req, res) => {
  const value = config[req.params.key];
  if (value === undefined) {
    res.status(404);
    res.end("ERROR");
    return;
  }

  config[req.params.key] = req.body;

  res.status(200);
  res.end(config[req.params.key]);
});

const port = 8081;
app.listen(port);
console.log(`Listening at http://localhost:${port}`);
