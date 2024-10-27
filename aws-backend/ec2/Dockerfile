FROM python:3-alpine

# ADD mqtt.py .
ADD http_server.py .

# RUN pip install paho-mqtt
RUN pip install requests
RUN pip install boto3

CMD ["python", "http_server.py"]