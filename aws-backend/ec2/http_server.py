import boto3
import json
import requests
import datetime
from urllib.parse import urlparse, parse_qs
from http.server import BaseHTTPRequestHandler, HTTPServer

s3_client = boto3.client("s3")
bucket_name = "cs3237-g16-b1"
data_buffer = []
current_hour = None
# gmt = 8
# gmt_string = "+08"
# timezone_delta = datetime.timedelta(hours=gmt)

class MyHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        parsed_url = urlparse(self.path)
        
        if parsed_url.path == "/getdata":
            # Parse query parameters
            query_params = parse_qs(parsed_url.query)
            
            # Extract year, month, and day if they are provided in the query parameters
            year = query_params.get("year", [None])[0]
            month = query_params.get("month", [None])[0]
            day = query_params.get("day", [None])[0]
            hour = query_params.get("hour", [None])[0]
            
            # Call getdata with extracted parameters
            self.handle_getdata(year, month, day, hour)
        else:
            # If path does not match, send a 404 response
            self.send_response(404)
            self.end_headers()
            self.wfile.write(b"Not Found")

    def do_POST(self):
        # Handle different POST endpoints
        if self.path == "/data":
            self.handle_data()
        else:
            self.handle_404()

    def handle_data(self):
        # Get content length to read the incoming data
        content_length = int(self.headers['Content-Length'])
        post_data = self.rfile.read(content_length)  # Read the POST body data
        
        try:
            # Parse the POST data assuming it is in JSON format
            json_data = json.loads(post_data)
            print(f"Received JSON data: {json_data}")

            # Check if both "topic" and "payload" are present in the JSON body
            if 'topic' in json_data and 'payload' in json_data:
                topic = json_data['topic']
                payload = json_data['payload']

                # Example of handling based on topic
                response_message = f"Received topic: {topic}, with payload: {payload}"

                # Send response
                self.send_response(200)
                self.send_header('Content-type', 'application/json')
                self.end_headers()

                # Respond with JSON
                response = {
                    'message': response_message,
                    'received': {
                        'topic': topic,
                        'payload': payload
                    }
                }
                self.wfile.write(json.dumps(response).encode('utf-8'))
                send_to_tele(topic, payload)
                add_data({"topic": topic, "payload": payload})
            else:
                # Missing fields, return 400 Bad Request
                self.send_response(400)
                self.send_header('Content-type', 'application/json')
                self.end_headers()
                error_response = {'error': 'Missing "topic" or "payload" in the JSON body'}
                self.wfile.write(json.dumps(error_response).encode('utf-8'))

        except json.JSONDecodeError:
            # If JSON is invalid, send a 400 Bad Request response
            self.send_response(400)
            self.send_header('Content-type', 'application/json')
            self.end_headers()
            error_response = {'error': 'Invalid JSON'}
            self.wfile.write(json.dumps(error_response).encode('utf-8'))

    def handle_getdata(self, year=None, month=None, day=None, hour=None):
        if year and month and day and hour:
            data = get_data_by_date(int(year), int(month), int(day), int(hour))
            self.send_response(200)
            self.end_headers()
            response = f"Data for date: {year}-{month}-{day}-{hour}\n" + "\n".join(json.dumps(record) for record in data)
            self.wfile.write(response.encode())
        elif year and month and day:
            data = get_data_by_date(int(year), int(month), int(day))
            self.send_response(200)
            self.end_headers()
            response = f"Data for date: {year}-{month}-{day}\n" + "\n".join(json.dumps(record) for record in data)
            self.wfile.write(response.encode())
        elif year and month:
            data = get_data_by_date(int(year), int(month))
            self.send_response(200)
            self.end_headers()
            response = f"Data for date: {year}-{month}\n" + "\n".join(json.dumps(record) for record in data)
            self.wfile.write(response.encode())
        elif year:
            data = get_data_by_date(int(year))
            self.send_response(200)
            self.end_headers()
            response = f"Data for date: {year}\n" + "\n".join(json.dumps(record) for record in data)
            self.wfile.write(response.encode())
        else:
            self.send_response(400)
            self.end_headers()
            self.wfile.write(b"Bad Request: Invalid date parameters")

    def handle_404(self):
        self.send_response(404)
        self.send_header('Content-type', 'text/html')
        self.end_headers()
        self.wfile.write(b"<h1>404 Not Found</h1><p>The page you requested does not exist.</p>")

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

def save_to_s3(data_buffer):
    # Generate file path based on the current time
    now = datetime.datetime.now()# + timezone_delta
    s3_key = f"data/{now.strftime('%Y/%m/%d/%H')}/{now.strftime('%Y%m%d %HH%MM%SS')}.jsonl"

    # Convert data to JSON lines
    json_lines = "\n".join(json.dumps(record) for record in data_buffer)
    
    # Upload file to S3
    s3_client.put_object(Body=json_lines, Bucket=bucket_name, Key=s3_key)
    print(json_lines)
    print(f"Uploaded to {s3_key}")

def add_data(data):
    global current_hour
    now = datetime.datetime.now()# + timezone_delta
    data_buffer.append({"timestamp": now.strftime('%Y%m%d %HH%MM%SS'), "data": data})

    entry_hour = now.hour

    # Check if buffer threshold is met (e.g., every 100 entries)
    if entry_hour != current_hour or len(data_buffer) >= 5:
        save_to_s3(data_buffer)
        data_buffer.clear()  # Clear buffer after upload
        current_hour = entry_hour

def list_files_by_date(prefix):
    # List all files in the S3 bucket with the specified prefix
    response = s3_client.list_objects_v2(Bucket=bucket_name, Prefix=prefix)
    if 'Contents' in response:
        return [obj['Key'] for obj in response['Contents']]
    else:
        return []

def get_data_by_date(year, month=None, day=None, hour=None):
    # Construct prefix based on known date parts
    prefix = f"data/{year}/"
    if month:
        prefix += f"{month:02d}/"
    if day:
        prefix += f"{day:02d}/"
    if hour:
        prefix += f"{hour:02d}/"

    # List files based on the prefix
    file_keys = list_files_by_date(prefix)
    
    # Download and process each file
    data = []
    for file_key in file_keys:
        obj = s3_client.get_object(Bucket=bucket_name, Key=file_key)
        content = obj['Body'].read().decode('utf-8')
        
        # Assuming JSON lines format, parse each line
        for line in content.splitlines():
            data.append(json.loads(line))
    
    return data

# Set up the server
def run(server_class=HTTPServer, handler_class=MyHandler, port=80):
    global current_hour
    now = datetime.datetime.now()# + timezone_delta
    current_hour = now.hour
    server_address = ('', port)
    httpd = server_class(server_address, handler_class)
    print(f"Serving HTTP on port {port}")
    httpd.serve_forever()

if __name__ == "__main__":
    run()