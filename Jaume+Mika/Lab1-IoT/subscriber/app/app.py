import json
import paho.mqtt.client as mqtt
import mysql.connector as mysql
import time 
from flask import Flask, jsonify 
from threading import Thread

# MQTT connection configuration
MQTT_BROKER = "mosquitto"
MQTT_PORT = 1883
MQTT_TOPIC = "weatherstation/data"

#MYSQL connection configuration
MYSQL_HOST = "mysql"
MYSQL_PORT = 3306
MYSQL_DATABASE = "lab1"
MYSQL_USER = "lab1_user"
MYSQL_PASSWORD = "lab1_password"

web = Flask(__name__)

@web.route("/api/measurements", methods=["GET"])
def get_measurements():
    cursor.execute("""
        SELECT timestamp, temperature, humidity
        FROM measurements
        ORDER BY timestamp ASC
    """)

    rows = cursor.fetchall()

    measurements = []

    for timestamp, temperature, humidity in rows:
        measurements.append({
            "timestamp": timestamp.strftime("%Y-%m-%d %H:%M:%S"),
            "temperature": temperature,
            "humidity": humidity
        })

    return jsonify(measurements)



def connect_to_database():
    while True:
        try:
            print("Connecting to MySQL...")

            db = mysql.connect(
                host=MYSQL_HOST,
                port=MYSQL_PORT,
                database=MYSQL_DATABASE,
                user=MYSQL_USER,
                password=MYSQL_PASSWORD
            )

            print("Connected to MySQL.")
            return db

        except mysql.Error as error:
            print(f"MySQL is not ready: {error}")
            print("Retrying in 5 seconds...")
            time.sleep(5)

def on_connect(client, userdata, flags, reason_code, properties):
    print(f"Connected to MQTT broker with code {reason_code}.")

    client.subscribe(MQTT_TOPIC)
    print(f"Subscribed to topic: {MQTT_TOPIC}.")

def on_message(client, userdata, message):
    payload = message.payload.decode()

    print(f"Message received: {payload}.")

    try:
        data = json.loads(payload)

        temperature = data["temperature"]
        humidity = data["humidity"]

        print(f"Temperature: {temperature} °C")
        print(f"Humidity: {humidity} %")

        sql = """
            INSERT INTO measurements (temperature, humidity)
            VALUES (%s, %s)
        """

        cursor.execute(sql, (temperature, humidity))
        db.commit()

    except(json.JSONDecodeError, KeyError) as error:
        print(f"Invalid message: {error}.")

    except mysql.connector.Error as error:
        print(f"MySQL error: {error}")


def run_web_server():
    web.run(host="0.0.0.0", port=5000)



db = connect_to_database()
cursor = db.cursor()

client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, client_id="subscriber-app")

client.on_connect = on_connect
client.on_message = on_message

print("Connecting to MQTT broker")

client.connect(MQTT_BROKER, MQTT_PORT, 60)

web_thread = Thread(target=run_web_server, daemon=True)
web_thread.start()

client.loop_forever()

