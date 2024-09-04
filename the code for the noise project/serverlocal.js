const express = require('express');
const bodyParser = require('body-parser');
const WebSocket = require('ws');
const fs = require('fs');
const path = require('path');

const app = express();
const port = 3000;

let allData = [];
let decibelData = [];
let clients = [];
let lastReceivedTime = Date.now();

app.use(bodyParser.json());

function formatTime(milliseconds) {
    const date = new Date(milliseconds);
    const hours = String(date.getUTCHours() + 3).padStart(2, '0');
    const minutes = String(date.getUTCMinutes()).padStart(2, '0');
    const seconds = String(date.getUTCSeconds()).padStart(2, '0');
    return `${hours}:${minutes}:${seconds}`;
}

app.post('/data', (req, res) => {
    const averages = req.body;
    if (Array.isArray(averages) && averages.length > 0) {
        allData.push(...averages);
        decibelData.push(...averages);

        if (decibelData.length > 50) {
            decibelData = decibelData.slice(-50);
        }

        console.log(`Received decibel data: ${averages.map(a => a.average).join(', ')} dB`);

        clients.forEach(client => {
            if (client.readyState === WebSocket.OPEN) {
                averages.forEach(data => {
                    const formattedTime = formatTime(data.timestamp);
                    client.send(JSON.stringify({ timestamp: formattedTime, average: data.average }));
                });
            }
        });

        lastReceivedTime = Date.now();

        res.send('Data received');
    } else {
        res.status(400).send('Invalid data format');
    }
});

app.get('/', (req, res) => {
    res.send(`
        <!DOCTYPE html>
        <html lang="en">
        <head>
          <meta charset="UTF-8">
          <meta name="viewport" content="width=device-width, initial-scale=1.0">
          <title>Decibel Level Plot</title>
          <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
        </head>
        <body>
          <h2>Decibel Level Data Plot</h2>
          <div id="averagesDisplay" style="font-size: 18px; margin-bottom: 20px;"></div>
          <div id="dataCountDisplay" style="font-size: 18px; margin-bottom: 20px;"></div>
          <canvas id="decibelChart" width="400" height="200"></canvas>
          <button id="downloadBtn">Download All Data</button>
          <script>
            const ctx = document.getElementById('decibelChart').getContext('2d');
            let decibelData = [];
            let labels = [];
            
            const chart = new Chart(ctx, {
              type: 'line',
              data: {
                labels: labels,
                datasets: [{
                  label: 'Decibel Level (dB)',
                  data: decibelData,
                  borderColor: 'rgba(75, 192, 192, 1)',
                  borderWidth: 1,
                  fill: false
                }]
              },
              options: {
                scales: {
                  x: { 
                    title: { display: true, text: 'Time (HH:MM:SS)' },
                    ticks: { autoSkip: true, maxTicksLimit: 10 }
                  },
                  y: { 
                    title: { display: true, text: 'Decibel Level (dB)' },
                    min: 30,
                    max: 90
                  }
                }
              }
            });

            const socket = new WebSocket('ws://' + window.location.host);
            
            socket.onmessage = function(event) {
              const data = JSON.parse(event.data);
              if (data.average !== undefined && data.timestamp !== undefined) {
                const averagesDisplay = document.getElementById('averagesDisplay');
                averagesDisplay.innerHTML = 'Latest Averages: ' + data.average.toFixed(2) + ' dB';

                decibelData.push(data.average);
                labels.push(data.timestamp);
                
                if (decibelData.length > 50) {
                  decibelData.shift();
                  labels.shift();
                }

                chart.update();

                const dataCountDisplay = document.getElementById('dataCountDisplay');
                dataCountDisplay.innerHTML = 'Number of Data Points on Graph: ' + decibelData.length;
              }
            };

            document.getElementById('downloadBtn').addEventListener('click', function() {
              window.location.href = '/download';
            });
          </script>
        </body>
        </html>
    `);
});

app.get('/download', (req, res) => {
    const filePath = path.join(__dirname, 'allDecibelData.json');
    fs.writeFileSync(filePath, JSON.stringify(allData, null, 2));

    res.download(filePath, 'allDecibelData.json', (err) => {
        if (err) {
            console.error('Error downloading the file:', err);
        } else {
            console.log('File downloaded successfully.');
        }
    });
});

const server = app.listen(port, () => {
    console.log(`Server running at http://localhost:${port}`);
});

const wss = new WebSocket.Server({ server });

wss.on('connection', (ws) => {
    clients.push(ws);
    ws.on('close', () => {
        clients = clients.filter(client => client !== ws);
    });
});
