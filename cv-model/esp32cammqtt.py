import paho.mqtt.client as mqtt
from time import sleep
from pathlib import Path
import os
import base64
import cv2
from ultralytics import YOLO
import requests

# Load the YOLOv8 model
model = YOLO('yolov8n.pt')

folderPath = Path.cwd().joinpath('images')
MQTTPath = "esp32/image/#"
birdDetectedTopic = "tray-return/actuate" 

def countNumOfFiles(filepath):
    count = 0
    for path in os.scandir(filepath):
        if path.is_file():
            count += 1
    return count

def on_connect(client, userdata, flags, rc):
    print("Connected with result code: " + str(rc))
    client.subscribe(MQTTPath)

def on_message(client, userdata, message):
    #print("Received message: " + str(message.payload))
    imageDecoded = base64.b64decode(message.payload)

    # Check if the folder exists, if not create it
    if not folderPath.exists():
        folderPath.mkdir(parents=True)

    # Count existing image files and create a new filename
    imageNum = countNumOfFiles(folderPath) + 1
    imagePath = folderPath.joinpath(f'images{imageNum}.jpg')  # Changed to use f-string for clarity

    with open(imagePath, 'wb') as f:  # Use 'with' statement for file handling
        f.write(imageDecoded)
    
    print(f"Image saved as {imagePath.name}")

    detect_bird_in_image(imagePath)


# Function to detect birds in a single image
def detect_bird_in_image(image_path):
    # Load the image from the given path
    img = cv2.imread(image_path)

    if img is None:
        print(f"Error: Could not load image from {image_path}")
        return

    # Run YOLOv8 on the image
    results = model(img)

    # Initialize bird detection flag
    bird_count = 0
    bird_detected = False

    # YOLOv8 returns a list of results; iterate through each result
    for result in results:
        # Iterate through detected objects (each object has a class index)
        for cls_idx in result.boxes.cls:
            # Convert class index to int and get the class name
            class_name = model.names[int(cls_idx)]
            if class_name == 'bird':
                bird_count += 1
                bird_detected = True
                
    
    if bird_detected:
        print("Bird detected!")
        client.publish(birdDetectedTopic, "bird")
    else:
        client.publish(birdDetectedTopic, "idle")
        print("No bird detected in the image.")
        
    # Send bird count via POST request
    payload = {"birdcount": bird_count}
    try:
        response = requests.post("https://byebyebirdie-delta.vercel.app/updatebird", json=payload)
        if response.status_code == 200:
            print("POST request successful.")
        else:
            print(f"Failed to send POST request. Status code: {response.status_code}")
    except Exception as e:
        print(f"Error sending POST request: {e}")


# Create the MQTT client and attach the callbacks
client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

# Connect to the MQTT broker
client.connect("localhost", 1883, 60)

# Start the MQTT loop
client.loop_forever()
