FROM python:3-alpine

ADD mqtt.py .

RUN pip install paho-mqtt
RUN pip install requests

CMD ["python", "mqtt.py"]