// server.js

const WebSocket = require('ws');
const http = require('http');

// Create an HTTP server
const server = http.createServer((req, res) => {
  res.writeHead(200, { 'Content-Type': 'text/plain' });
  res.end('WebSocket server is running.');
});

// Create a WebSocket server by passing the HTTP server
const wss = new WebSocket.Server({ server });

// Event handler for WebSocket connections
wss.on('connection', (ws) => {
  console.log('A new client has connected.');

  // Event handler for incoming messages from clients
  ws.on('message', (message) => {
    console.log(`Received: ${message}`);

    // Broadcast the received message to all connected clients
    wss.clients.forEach((client) => {
      if (client !== ws && client.readyState === WebSocket.OPEN) {
        client.send(message);
      }
    });
  });

  // Event handler for WebSocket connection closing
  ws.on('close', () => {
    console.log('A client has disconnected.');
  });
});

// Start the HTTP server on port 3000
const PORT = process.env.PORT || 3000;
server.listen(PORT, () => {
  console.log(`WebSocket server is listening on port ${PORT}`);
});