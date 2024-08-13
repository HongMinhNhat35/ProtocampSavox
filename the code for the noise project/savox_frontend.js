"use strict";

let globalUser = { canvas: {} }; // Initialize globalUser object
let chartData = {}; // Global variable to store chart data

window.addEventListener('DOMContentLoaded', () => {
    globalUser.canvas.element = document.getElementById('canvas');
    resize(globalUser);

    // Add event listeners for buttons
    document.getElementById('downloadButton').addEventListener('click', () => {
        drawRectangle(); // If you want to keep the original behavior
        downloadChartData(); // Export chart data
    });
        document.getElementById('optionsButton').addEventListener('click', drawCircle);
    document.getElementById('historyButton').addEventListener('click', drawTriangle);
    document.getElementById('startNewEndButton').addEventListener('click', clearCanvas);
    // Draw a chart using Chart.js
    drawChart();


});

window.addEventListener('resize', () => resize(globalUser));

function resize(user) {
    let canvas = user.canvas.element;
    let box = canvas.getBoundingClientRect();
    canvas.width = box.width;
    canvas.height = box.height;
    user.canvas.context = canvas.getContext("2d");
    //drawRectangle(); // Draw default shape to ensure canvas is not blank
}

function drawRectangle() {
    console.log('Drawing Rectangle');
    let ctx = globalUser.canvas.context;
    if (ctx) {
        ctx.clearRect(0, 0, ctx.canvas.width, ctx.canvas.height);
        ctx.fillStyle = "red"; // Red color
        ctx.fillRect(10, 10, 100, 100); // Draw a rectangle at (10,10) with a width and height of 100
    }

}
function downloadChartData() {
    if (!chartData.labels || !chartData.data) {
        alert('No chart data available for download.');
        return;
    }

    // Convert chart data to CSV format
    let csvContent = "data:text/csv;charset=utf-8,Step,Value\n";
    chartData.labels.forEach((label, index) => {
        csvContent += `${label},${chartData.data[index]}\n`;
    });

    // Create a link element and trigger the download
    let encodedUri = encodeURI(csvContent);
    let link = document.createElement("a");
    link.setAttribute("href", encodedUri);
    link.setAttribute("download", "chart_data.csv");
    document.body.appendChild(link);
    link.click();
    document.body.removeChild(link);
}


function drawCircle() {
    console.log('Drawing Circle');
    let ctx = globalUser.canvas.context;
    if (ctx) {
        ctx.clearRect(0, 0, ctx.canvas.width, ctx.canvas.height);
        ctx.fillStyle = "blue"; // Blue color
        ctx.beginPath();
        ctx.arc(100, 100, 50, 0, 2 * Math.PI);
        ctx.fill();
    }
    
}

function drawTriangle() {
    let ctx = globalUser.canvas.context;
    if (ctx) {
        ctx.clearRect(0, 0, ctx.canvas.width, ctx.canvas.height);
        ctx.fillStyle = "green"; // Green color
        ctx.beginPath();
        ctx.moveTo(50, 50);
        ctx.lineTo(150, 50);
        ctx.lineTo(100, 150);
        ctx.closePath();
        ctx.fill();
    }
}



function clearCanvas() {
    let ctx = globalUser.canvas.context;
    if (ctx) {
        ctx.clearRect(0, 0, ctx.canvas.width, ctx.canvas.height);
    }
    drawChart();
}
let myChart; // Declare the chart variable in a scope accessible to both the drawChart function and other parts of your script

// Function to draw a chart using Chart.js
function drawChart() {
    let ctx = globalUser.canvas.element.getContext('2d');

    // Function to compute the sequence for a given starting number
    function collatzSequence(start) {
        let sequence = [];
        let n = start;
        while (n !== 1) {
            sequence.push(n);
            if (n % 2 === 0) {
                n = n / 2;
            } else {
                n = 3 * n + 1;
            }
        }
        sequence.push(1); // Add the last 1 to the sequence
        return sequence;
    }

    // Example: Generate the sequence for a starting number, e.g., 5
    let startNumber = 33; // You can change this to any other starting number
    let sequence = collatzSequence(startNumber);
    let labels = Array.from({ length: sequence.length }, (_, i) => `Step ${i + 1}`);

    // Destroy the previous chart instance if it exists
    if (myChart) {
        myChart.destroy();
    }

    // Create a new chart instance
    myChart = new Chart(ctx, {
        type: 'line',
        data: {
            labels: labels, // Steps as labels
            datasets: [{
                label: `Collatz Sequence for ${startNumber}`,
                data: sequence,
                backgroundColor: 'rgba(75, 192, 192, 0.2)',
                borderColor: 'rgba(75, 192, 192, 1)',
                borderWidth: 2
            }]
        },
        options: {
            scales: {
                x: {
                    title: {
                        display: true,
                        text: 'Steps'
                    }
                },
                y: {
                    beginAtZero: true,
                    title: {
                        display: true,
                        text: 'Value'
                    }
                }
            }
        }
    });
    chartData = {
        labels: labels,
        data: sequence
    };

    // Calculate the count of values above certain limits
    let countAbove1 = sequence.filter(value => value > 85).length;
    let countAbove2 = sequence.filter(value => value > 88).length;
    let countAbove3 = sequence.filter(value => value > 91).length;
    let countAbove4 = sequence.filter(value => value > 94).length;
    let countAbove5 = sequence.filter(value => value > 100).length;

  
    // Display the counts in the new div
    document.getElementById('countInfo').innerText = `Count of values above 85: ${countAbove1}\nCount of values above 88: ${countAbove2}\nCount of values above 91: ${countAbove3}\nCount of values above 94: ${countAbove4}\nCount of values above 100: ${countAbove5}`;
}