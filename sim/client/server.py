from flask import Flask, render_template
from flask_socketio import SocketIO, emit
import os
import pty
import threading
import time

from can import can_ipc, monitor_sim
from stack import uart_ipc

app = Flask(__name__)
app.config['SECRET_KEY'] = 'secret!'
socketio = SocketIO(app)

def send_stack_update(update):
    socketio.emit('stackupdate', dict(update))
    print(update)
monitor_sim.packet_sender = send_stack_update

# Web page
@app.route('/')
def index():
    return render_template('can.html', slave_path=can_ipc.get_slave_path())

if __name__ == '__main__':
    print(f'bin/sim {can_ipc.get_slave_path()} {uart_ipc.get_slave_path()}')
    socketio.run(app, host='0.0.0.0', port=1000, debug=False, use_reloader=False)
