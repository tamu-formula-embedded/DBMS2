from flask import Flask, render_template
from flask_socketio import SocketIO, emit
import os
import pty
import threading
import time

from sim.client.ipc import SerialIpcReader

app = Flask(__name__)
app.config['SECRET_KEY'] = 'secret!'
socketio = SocketIO(app)

# Callback to handle packets and push updates via WebSocket
def handle_can_packet(packet):
    id_bytes = packet[:4]
    packet_id = int.from_bytes(id_bytes, byteorder='big')
    
    if packet_id == 0x5F2:
        data_bytes = packet[4:]  # 8-byte data
        socketio.emit('data', {'ascii': data_bytes.decode('utf-8', errors='replace')})

# Start the IPC reader
can_ipc = SerialIpcReader(packet_callback=handle_can_packet, fixed_length=12)
can_ipc.start()

# Web page
@app.route('/')
def index():
    return render_template('index.html', slave_path=can_ipc.get_slave_path())

if __name__ == '__main__':
    socketio.run(app, host='0.0.0.0', port=1000, debug=False, use_reloader=False)
