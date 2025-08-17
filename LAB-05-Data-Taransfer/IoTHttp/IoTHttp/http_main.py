from flask import Flask, request, jsonify
import threading
import asyncio

# ---------------------------
# REST using Flask
# ---------------------------
app = Flask(__name__)

@app.route('/rest', methods=['GET', 'POST'])
def rest_endpoint():
    if request.method == 'GET':
        return jsonify({"message": "GET request received FROM JOBAYER FAISAL FAHIM With his all friends"})
    else:
        data = request.json
        return jsonify({"message": "POST request received FROM JOBAYER FAISAL FAHIM", "data": data})



# ---------------------------
# Main entry point: start all servers
# ---------------------------
if __name__ == '__main__':
    # Start the CoAP server in a separate thread
    #coap_thread = threading.Thread(target=start_coap_server, daemon=True)
    #coap_thread.start()

    # Start the MQTT client in a separate thread
    #mqtt_thread = threading.Thread(target=start_mqtt_client, daemon=True)
    #mqtt_thread.start()

    # Start the Flask (REST) server on port 5000
    app.run(host='0.0.0.0', port=5000)
