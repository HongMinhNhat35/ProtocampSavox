let process = require("process");
let os = require("os");
let fs = require("fs");
let http = require("http");
let ws = require("ws");

let homedir = os.homedir();
let path = `${homedir}/api/ws_htmlcssjs_backend.sock`;

function wsSendIfOpen(client, data) {
  if (client.readyState === ws.WebSocket.OPEN && typeof data === "string") {
      client.send(data);
  }
}

let hs = http.createServer({ keepAlive: true, requestTimeout: 16000, keepAliveInitialDelay: 4000 });
let wss = new ws.WebSocketServer({ backlog: 128, clientTracking: true, maxPayload: 1024 * 1024 *10, server: hs });

wss.on("connection", (client, req) => {
  console.log("open: " + req.headers["x-real-ip"]);

  // Listener for messages
  client.on("message", (message) => {
    console.log("Received message: ", message.toString());

    // Try to parse JSON data
    try {
      let jsonData = JSON.parse(message);

      // Process JSON data (e.g., save to a file)
      let filePath = `${homedir}/public/data.json`;
      fs.writeFileSync(filePath, JSON.stringify(jsonData, null, 2)); // Pretty-print JSON

      // Send confirmation back to the client
      wsSendIfOpen(client, "JSON data received and saved successfully.");

    } catch (error) {
      console.error("Error parsing JSON: ", error);
      wsSendIfOpen(client, "Failed to parse JSON data.");
    }
  });

  // Close the connection after 1.5 minutes (90,000 milliseconds)
  setTimeout(() => client.close(), 90000);

  client.on("close", () => {
    console.log("close: " + req.headers["x-real-ip"]);
  });
});

try {
  fs.unlinkSync(path);
} catch (error) {}

hs.listen(path, () => {
  try {
      fs.chmodSync(hs.address(), 666);
  } catch (error) {
      console.log(error);
      process.exit(1);
  }
});

console.log("ready");
