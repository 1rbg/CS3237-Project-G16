import paho.mqtt.client as mqtt
import requests
from time import sleep
from pathlib import Path

MQTTPath = "tray-return/sensor"

def on_connect(client, userdata, flags, rc):
    print("Connected with result code: " + str(rc))
    client.subscribe(MQTTPath)

def on_message(client, userdata, message):
    print("Received message: " + str(message.payload))
    # Prepare the payload data for the HTTP POST request

    payload_message = "Tray System is " + message.payload.decode() + "/8 full"
    data = {
        "topic": "Tray System Capacity",
        "payload": payload_message,  # Decoding bytes to string if needed
        "value": int(message.payload.decode())
    }

    # Send bird count via POST request
    payload = {"value": int(message.payload.decode())}
    try:
        response = requests.post("https://byebyebirdie-delta.vercel.app/updatetray", json=payload)
        if response.status_code == 200:
            print("POST request successful.")
        else:
            print(f"Failed to send POST request. Status code: {response.status_code}")
    except Exception as e:
        print(f"Error sending POST request: {e}")

    # Send HTTP POST request to localhost/data
    try:
        response = requests.post("http://13.250.30.243:80/data", json=data)
        print("POST request sent. Status code:", response.status_code)
    except requests.exceptions.RequestException as e:
        print("Failed to send POST request:", e)

# Create the MQTT client and attach the callbacks
client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

# Connect to the MQTT broker
client.connect("localhost", 1883, 60)

# Start the MQTT loop
client.loop_forever()
