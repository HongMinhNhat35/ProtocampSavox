"use strict";

let globalUser = { canvas: {} };
let chartData = {};
let historyData = [];
let settings = {
    lineColor: localStorage.getItem('lineColor') || 'rgba(75, 192, 192, 1)',
    lineStyle: JSON.parse(localStorage.getItem('lineStyle')) || [],
    showGridlines: localStorage.getItem('showGridlines') !== 'false'
};

window.addEventListener('DOMContentLoaded', () => {
    globalUser.canvas.element = document.getElementById('canvas');
    resize(globalUser);

    // Add event listeners for buttons
    document.getElementById('downloadButton').addEventListener('click', downloadChartData);
    document.getElementById('optionsButton').addEventListener('click', openOptions);
    document.getElementById('historyButton').addEventListener('click', showHistory);
    document.getElementById('startNewEndButton').addEventListener('click', clearCanvas);

    // Draw a chart using Chart.js
    fetchChartData(); // Initialize with chart data
});

window.addEventListener('resize', () => resize(globalUser));

function resize(user) {
    let canvas = user.canvas.element;
    let box = canvas.getBoundingClientRect();
    canvas.width = box.width;
    canvas.height = box.height;
    user.canvas.context = canvas.getContext("2d");
}

function downloadChartData() {
    if (!chartData.labels || !chartData.data) {
        alert('No chart data available for download.');
        return;
    }

    let csvContent = "data:text/csv;charset=utf-8,Step,Value\n";
    chartData.labels.forEach((label, index) => {
        csvContent += `${label},${chartData.data[index]}\n`;
    });

    let encodedUri = encodeURI(csvContent);
    let link = document.createElement("a");
    link.setAttribute("href", encodedUri);
    link.setAttribute("download", "chart_data.csv");
    document.body.appendChild(link);
    link.click();
    document.body.removeChild(link);
}

function fetchChartData() {
    fetch('data.json')
        .then(response => response.json())
        .then(data => {
            chartData = {
                labels: data.labels,
                data: data.data
            };

            historyData.push({
                timestamp: new Date().toLocaleString(),
                data: { ...chartData }
            });
            drawChart();
        })
        .catch(error => console.error('Error fetching chart data:', error));
}

function clearCanvas() {
    let ctx = globalUser.canvas.context;
    if (ctx) {
        ctx.clearRect(0, 0, ctx.canvas.width, ctx.canvas.height);
    }
    fetchChartData(); // Redraw the chart after clearing the canvas
}

let myChart;
function drawChart() {
    let ctx = globalUser.canvas.element.getContext('2d');

    if (myChart) {
        myChart.destroy();
    }

    myChart = new Chart(ctx, {
        type: 'line',
        data: {
            labels: chartData.labels,
            datasets: [{
                label: 'Decibels strength',
                data: chartData.data,
                backgroundColor: 'rgba(75, 192, 192, 0.2)',
                borderColor: settings.lineColor,
                borderWidth: 2,
                borderDash: settings.lineStyle
            }]
        },
        options: {
            plugins: {
                legend: {
                    position: 'top',
                    align: 'start'
                },
                zoom: {
                    pan: {
                        enabled: true,
                        mode: 'xy'
                    },
                    zoom: {
                        wheel: {
                            enabled: true
                        },
                        pinch: {
                            enabled: true
                        },
                        mode: 'xy'
                    }
                }
            },
            scales: {
                x: {
                    title: {
                        display: true,
                        text: 'Time'
                    },
                    grid: {
                        display: settings.showGridlines
                    }
                },
                y: {
                    beginAtZero: true,
                    title: {
                        display: true,
                        text: 'Decibels'
                    },
                    grid: {
                        display: settings.showGridlines
                    }
                }
            }
        }
    });

    let countAbove55 = chartData.data.filter(value => value > 55).length;
    let countAbove60 = chartData.data.filter(value => value > 60).length;
    let countAbove65 = chartData.data.filter(value => value > 65).length;
    let countAbove70 = chartData.data.filter(value => value > 70).length;
    let countAbove75 = chartData.data.filter(value => value > 75).length;

    document.getElementById('countInfo').innerHTML = `
    <div class="count-box">Count of values above 55: ${countAbove55}</div>
    <div class="count-box">Count of values above 60: ${countAbove60}</div>
    <div class="count-box">Count of values above 65: ${countAbove65}</div>
    <div class="count-box">Count of values above 70: ${countAbove70}</div>
    <div class="count-box">Count of values above 75: ${countAbove75}</div>`;
}


function openOptions() {
    if (document.getElementById('optionsContainer')) {
        return;
    }

    let optionsContainer = document.createElement('div');
    optionsContainer.id = 'optionsContainer';
    optionsContainer.style.position = 'fixed';
    optionsContainer.style.top = '20px';
    optionsContainer.style.right = '20px';
    optionsContainer.style.padding = '10px';
    optionsContainer.style.backgroundColor = 'white';
    optionsContainer.style.border = '1px solid black';
    optionsContainer.style.zIndex = '1000';

    // Line Color Option
    let colorPickerLabel = document.createElement('label');
    colorPickerLabel.innerText = 'Line Color: ';
    let colorPicker = document.createElement('input');
    colorPicker.type = 'color';
    colorPicker.value = settings.lineColor;
    colorPicker.addEventListener('input', (event) => {
        settings.lineColor = event.target.value;
        localStorage.setItem('lineColor', settings.lineColor);
        changeLineColor(settings.lineColor);
    });

    optionsContainer.appendChild(colorPickerLabel);
    optionsContainer.appendChild(colorPicker);
    optionsContainer.appendChild(document.createElement('br'));

    // Line Style Option
    let lineStyleLabel = document.createElement('label');
    lineStyleLabel.innerText = 'Line Style: ';
    let lineStyleSelector = document.createElement('select');
    let lineStyles = {
        Solid: [],
        Dashed: [5, 5],
        Dotted: [1, 5]
    };

    for (let style in lineStyles) {
        let option = document.createElement('option');
        option.value = style;
        option.innerText = style;
        lineStyleSelector.appendChild(option);
    }

    lineStyleSelector.value = Object.keys(lineStyles).find(key => JSON.stringify(lineStyles[key]) === JSON.stringify(settings.lineStyle));

    lineStyleSelector.addEventListener('change', (event) => {
        settings.lineStyle = lineStyles[event.target.value];
        localStorage.setItem('lineStyle', JSON.stringify(settings.lineStyle));
        changeLineStyle(settings.lineStyle);
    });

    optionsContainer.appendChild(lineStyleLabel);
    optionsContainer.appendChild(lineStyleSelector);
    optionsContainer.appendChild(document.createElement('br'));

    // Toggle Gridlines Option
    let gridlineToggleLabel = document.createElement('label');
    gridlineToggleLabel.innerText = 'Toggle Gridlines: ';
    let gridlineToggle = document.createElement('input');
    gridlineToggle.type = 'checkbox';
    gridlineToggle.checked = settings.showGridlines;
    gridlineToggle.addEventListener('change', (event) => {
        settings.showGridlines = event.target.checked;
        localStorage.setItem('showGridlines', settings.showGridlines);
        toggleGridlines(settings.showGridlines);
    });
    // Reset Zoom Button
    let resetZoomButton = document.createElement('button');
    resetZoomButton.innerText = 'Reset Zoom';
        resetZoomButton.addEventListener('click', () => {
        if (myChart) {
            myChart.resetZoom();
        }
    });
    optionsContainer.appendChild(gridlineToggleLabel);
    optionsContainer.appendChild(gridlineToggle);
    optionsContainer.appendChild(document.createElement('br'));

    optionsContainer.appendChild(resetZoomButton);
    optionsContainer.appendChild(document.createElement('br'));
    


    // Close Options Button
    let closeButton = document.createElement('button');
    closeButton.innerText = 'Close Options';
    closeButton.addEventListener('click', () => {
        document.body.removeChild(optionsContainer);
    });

    optionsContainer.appendChild(document.createElement('br'));
    optionsContainer.appendChild(closeButton);

    document.body.appendChild(optionsContainer);
}

function changeLineColor(newColor) {
    if (myChart) {
        myChart.data.datasets[0].borderColor = newColor;
        myChart.update();
    }
}

function changeLineStyle(newStyle) {
    if (myChart) {
        myChart.data.datasets[0].borderDash = newStyle;
        myChart.update();
    }
}

function toggleGridlines(showGridlines) {
    if (myChart) {
        myChart.options.scales.x.grid.display = showGridlines;
        myChart.options.scales.y.grid.display = showGridlines;
        myChart.update();
    }
}

function showHistory() {
    let historyContainer = document.getElementById('historyContainer');
    historyContainer.innerHTML = '';

    if (historyData.length === 0) {
        historyContainer.innerText = 'No history available.';
        return;
    }

    historyData.forEach((entry, index) => {
        let historyItem = document.createElement('div');
        historyItem.innerText = `${entry.timestamp} - Dataset ${index + 1}`;
        historyItem.style.cursor = 'pointer';
        historyItem.addEventListener('click', () => loadFromHistory(index));
        historyContainer.appendChild(historyItem);
    });
}

function loadFromHistory(index) {
    chartData = { ...historyData[index].data };
    drawChart();
}
