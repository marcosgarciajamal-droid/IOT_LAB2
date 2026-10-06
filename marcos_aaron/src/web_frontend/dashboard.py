import json
import threading
from collections import defaultdict, deque
from datetime import datetime
import argparse

import paho.mqtt.client as mqtt
import plotly.graph_objects as go
from dash import Dash, dcc, html, Input, Output

# Configuration
BROKER = "localhost"
PORT = 1883
TOPIC = "sensor/+"
MAX_POINTS = 200 # points kept per sensor
REFRESH_MS = 2000 # how often the charts update

# Neon theme
BG = "#000000"
FG = "#e6e6e6"
GRID = "#1f1f1f"
# One shade per sensor, cycled if there are more sensors than shades.
TEMP_COLORS = ["#ff1744", "#ff6e40", "#ff4081"]
HUM_COLORS = ["#00b0ff", "#18ffff", "#536dfe"]

# Data store
# data["sensor/1"] = {"t": deque, "temp": deque, "hum": deque}
data = defaultdict(lambda: {
    "t": deque(maxlen=MAX_POINTS),
    "temp": deque(maxlen=MAX_POINTS),
    "hum": deque(maxlen=MAX_POINTS),
})
lock = threading.Lock()


# MQTT
def on_connect(client, userdata, flags, reason_code, properties):
    print(f"Connected to broker (code {reason_code}), subscribed to {TOPIC}")
    client.subscribe(TOPIC)


def on_message(client, userdata, msg):
    try:
        payload = json.loads(msg.payload.decode())
        temp = float(payload["temp"])
        hum = float(payload["hum"])
    except (KeyError, ValueError, TypeError, json.JSONDecodeError) as e:
        print(f"Ignored message on {msg.topic}: {msg.payload!r} ({e})")
        return

    with lock:
        d = data[msg.topic]
        d["t"].append(datetime.now())
        d["temp"].append(temp)
        d["hum"].append(hum)


def start_mqtt(broker, port):
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
    client.on_connect = on_connect
    client.on_message = on_message
    client.connect(broker, port, keepalive=60)
    client.loop_start()
    return client


# Web
app = Dash(__name__)
app.title = "MQTT Sensors"

app.layout = html.Div(
    style={"fontFamily": "system-ui, sans-serif", "maxWidth": "1000px",
           "margin": "0 auto", "padding": "1rem",
           "backgroundColor": BG, "color": FG, "minHeight": "100vh"},
    children=[
        html.H2("Temperature and Humidity",
                style={"color": FG, "marginBottom": "0.2rem"}), # "textShadow": "0 0 8px #ff1744, 0 0 18px #00b0ff"
        html.P("by Aarón Alcalá & Marcos Garcia",
               style={"color": "#9aa0a6", "marginTop": "0", "fontStyle": "italic"}),
        html.P(id="status", style={"color": "#9aa0a6"}),
        dcc.Graph(id="temp-graph"),
        dcc.Graph(id="hum-graph"),
        dcc.Interval(id="interval", interval=REFRESH_MS, n_intervals=0),
    ],
)

# Page background outside the Dash container.
app.index_string = """<!DOCTYPE html>
<html>
    <head>
        {%metas%}
        <title>{%title%}</title>
        {%favicon%}
        {%css%}
        <style>body { background-color: #000000; margin: 0; }</style>
    </head>
    <body>
        {%app_entry%}
        <footer>{%config%}{%scripts%}{%renderer%}</footer>
    </body>
</html>"""


def hex_to_rgba(color, alpha):
    r, g, b = (int(color[i:i + 2], 16) for i in (1, 3, 5))
    return f"rgba({r},{g},{b},{alpha})"


def add_neon_trace(fig, x, y, name, color):
    # Wide, low-opacity copies underneath fake the neon halo.
    for width, alpha in ((14, 0.08), (8, 0.14), (4, 0.25)):
        fig.add_trace(go.Scatter(
            x=x, y=y, mode="lines",
            line=dict(color=hex_to_rgba(color, alpha), width=width),
            hoverinfo="skip", showlegend=False,
        ))
    fig.add_trace(go.Scatter(
        x=x, y=y, mode="lines+markers", name=name,
        line=dict(color=color, width=2),
        marker=dict(color=color, size=7,
                    line=dict(color=hex_to_rgba(color, 0.35), width=6)),
    ))


def build_figure(field, title, unit, palette):
    fig = go.Figure()
    last_lines = []
    with lock:
        for i, (topic, d) in enumerate(sorted(data.items())):
            color = palette[i % len(palette)]
            add_neon_trace(fig, list(d["t"]), list(d[field]), topic, color)
            if d[field]:
                last_lines.append(
                    f"<span style='color:{color}'>{topic}: "
                    f"{d[field][-1]:.1f} {unit}</span>"
                )

    box_text = "<br>".join(last_lines) if last_lines else "no data yet"
    fig.add_annotation(
        text=f"<b>Last value</b><br>{box_text}",
        xref="paper", yref="paper", x=0.99, y=0.99,
        xanchor="right", yanchor="top", align="left",
        showarrow=False, font=dict(color=FG, size=13),
        bgcolor="rgba(0,0,0,0.75)", bordercolor=palette[0], borderwidth=1,
        borderpad=8,
    )
    fig.update_layout(
        title=title, xaxis_title="Time", yaxis_title=unit,
        template="plotly_dark",
        paper_bgcolor=BG, plot_bgcolor=BG,
        font=dict(color=FG),
        margin=dict(l=40, r=20, t=50, b=40),
        uirevision="fixed",   # keeps zoom between updates
        legend=dict(bgcolor="rgba(0,0,0,0)"),
    )
    fig.update_xaxes(gridcolor=GRID, zerolinecolor=GRID)
    fig.update_yaxes(gridcolor=GRID, zerolinecolor=GRID)
    return fig


@app.callback(
    Output("temp-graph", "figure"),
    Output("hum-graph", "figure"),
    Output("status", "children"),
    Input("interval", "n_intervals"),
)
def update(_):
    with lock:
        n_sensors = len(data)
        n_readings = sum(len(d["t"]) for d in data.values())
    status = (f"{n_sensors} sensor(s)"
              if n_sensors else f"Waiting for messages on {TOPIC}…")
    return (
        build_figure("temp", "Temperature", "°C", TEMP_COLORS),
        build_figure("hum", "Humidity", "%", HUM_COLORS),
        status,
    )


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="MQTT temperature/humidity dashboard")
    parser.add_argument("--host", default=BROKER, help="MQTT broker host (default: localhost)")
    parser.add_argument("--port", type=int, default=PORT, help="MQTT broker port (default: 1883)")
    args = parser.parse_args()

    start_mqtt(args.host, args.port)
    app.run(debug=False, host="0.0.0.0", port=8050)
