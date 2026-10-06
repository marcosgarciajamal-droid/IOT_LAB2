let sensorChart = null;

async function loadReadings() {
    const status = document.getElementById("status");
    const refreshButton = document.getElementById("refreshButton");

    status.textContent = "Loading data...";
    refreshButton.disabled = true;

    try {
        const response = await fetch("/api/measurements");

        if (!response.ok) {
            throw new Error(`HTTP error: ${response.status}`);
        }

        const readings = await response.json();

        if (!Array.isArray(readings)) {
            throw new Error("Invalid API response.");
        }

        // Keep only one reading per minute
        const minuteReadings = aggregateByMinute(readings);

        updateChart(minuteReadings);

        status.textContent =
            `${minuteReadings.length} readings displayed (${readings.length} total).`;

    } catch (error) {
        console.error("Error loading sensor data:", error);

        status.textContent = "Unable to load sensor data.";
    } finally {
        refreshButton.disabled = false;
    }
}


/*
 * Keep one measurement per minute.
 *
 * If several measurements exist within the same minute,
 * the last measurement from that minute is kept.
 */
function aggregateByMinute(readings) {
    const readingsByMinute = new Map();

    readings.forEach(reading => {
        const date = parseTimestamp(reading.timestamp);

        if (Number.isNaN(date.getTime())) {
            return;
        }

        // Example:
        // 2026-09-24 14:35
        const minuteKey =
            `${date.getFullYear()}-` +
            `${String(date.getMonth() + 1).padStart(2, "0")}-` +
            `${String(date.getDate()).padStart(2, "0")} ` +
            `${String(date.getHours()).padStart(2, "0")}:` +
            `${String(date.getMinutes()).padStart(2, "0")}`;

        // If another measurement exists in the same minute,
        // replace it with this later measurement.
        readingsByMinute.set(minuteKey, {
            timestamp: date,
            temperature: Number(reading.temperature),
            humidity: Number(reading.humidity)
        });
    });

    return Array.from(readingsByMinute.values())
        .sort((a, b) => a.timestamp - b.timestamp);
}


/*
 * Create the graph.
 */
function updateChart(readings) {
    const context = document
        .getElementById("sensorChart")
        .getContext("2d");

    if (sensorChart !== null) {
        sensorChart.destroy();
    }

    /*
     * Create labels for every reading.
     *
     * The graph contains one point per minute,
     * but we only display a label every 10 minutes.
     */
    const labels = readings.map(reading => {
        return formatTimestamp(reading.timestamp);
    });

    const temperatures = readings.map(reading => {
        return reading.temperature;
    });

    const humidities = readings.map(reading => {
        return reading.humidity;
    });

    sensorChart = new Chart(context, {
        type: "line",

        data: {
            labels: labels,

            datasets: [
                {
                    label: "Temperature (°C)",
                    data: temperatures,

                    yAxisID: "temperature",

                    borderWidth: 2,
                    pointRadius: 3,
                    pointHoverRadius: 5,

                    tension: 0.2,
                    fill: false
                },

                {
                    label: "Humidity (%)",
                    data: humidities,

                    yAxisID: "humidity",

                    borderWidth: 2,
                    pointRadius: 3,
                    pointHoverRadius: 5,

                    tension: 0.2,
                    fill: false
                }
            ]
        },

        options: {
            responsive: true,
            maintainAspectRatio: false,

            interaction: {
                mode: "index",
                intersect: false
            },

            plugins: {
                legend: {
                    position: "top"
                }
            },

            scales: {
                x: {
                    title: {
                        display: true,
                        text: "Time"
                    },

                    /*
                     * Only show an X-axis label every 10 readings.
                     *
                     * Because we have one reading per minute,
                     * this corresponds to 10-minute intervals.
                     */
                    ticks: {
                        autoSkip: false,

                        callback: function(value, index) {
                            if (index % 10 === 0) {
                                return this.getLabelForValue(value);
                            }

                            return "";
                        }
                    }
                },

                temperature: {
                    type: "linear",
                    position: "left",

                    title: {
                        display: true,
                        text: "Temperature (°C)"
                    }
                },

                humidity: {
                    type: "linear",
                    position: "right",

                    title: {
                        display: true,
                        text: "Humidity (%)"
                    },

                    grid: {
                        drawOnChartArea: false
                    }
                }
            }
        }
    });
}


/*
 * Convert:
 *
 * "2026-09-24 14:35:12"
 *
 * into a JavaScript Date.
 */
function parseTimestamp(timestamp) {
    return new Date(timestamp.replace(" ", "T"));
}


/*
 * Display only hour and minute.
 *
 * Example:
 * "14:35"
 */
function formatTimestamp(date) {
    if (!(date instanceof Date) || Number.isNaN(date.getTime())) {
        return "";
    }

    return date.toLocaleTimeString([], {
        hour: "2-digit",
        minute: "2-digit"
    });
}


document
    .getElementById("refreshButton")
    .addEventListener("click", loadReadings);

loadReadings();

