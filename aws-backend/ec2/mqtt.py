import paho.mqtt.client as mqtt
import paho.mqtt.publish as publish
import requests

from time import sleep
def on_connect(client, userdata, flags, rc):
	print("Connected with result code: " + str(rc))
	client.subscribe("input/#")

def on_message(client, userdata, message):
	print("Received message " + message.payload.decode())
	publish.single("output/echo", "echo: " + message.payload.decode(), hostname="localhost")
	send_to_tele(message.topic, message.payload.decode())
	
def send_to_tele(topic, payload):
	url = "https://byebyebirdie-delta.vercel.app/report"

	data = {
		"topic": topic,
		"payload": payload
	}

	headers = {
		"Content-Type": "application/json"
	}

	print(data)

	try:
		response = requests.post(url, json=data, headers=headers)
		if response.status_code == 200:
			print("Post request send success!")
		else:
			print(f"Post request send failure: {response.status_code}", response.text())
	except requests.exceptions.RequestException as e:
		print("Error:", e)
		return None
	

client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message
client.connect("localhost", 1883, 60)
client.loop_forever()
