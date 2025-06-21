import os
import pty
import threading
import time

MESSAGE_LEN_CAN = 12
MESSAGE_LEN_NONE = -1

class SerialIpcReader:
    def __init__(self, packet_callback, fixed_length=MESSAGE_LEN_NONE):
        self.packet_callback = packet_callback
        self.master_fd, self.slave_fd = pty.openpty()
        self.slave_path = os.ttyname(self.slave_fd)
        self._stop = False
        self.thread = threading.Thread(target=self._reader, daemon=True)
        self.fixed_length = fixed_length 
        self.name = None

    def start(self, name):
        # print(f"\u2705 Connect to {name} on {self.slave_path}")
        self.name = name
        self.thread.start()
        return self.slave_path

    def _reader(self):
        while not self._stop:
            try:
                if self.fixed_length != MESSAGE_LEN_NONE:
                    msg = os.read(self.master_fd, self.fixed_length)
                    if len(msg) == self.fixed_length and self.packet_callback:
                        self.packet_callback(msg)
                else:
                    msg = os.read(self.master_fd, 1024)
                    if msg and self.packet_callback:
                        self.packet_callback(msg)
            except OSError as e:
                print("Read error:", e)
                time.sleep(1)

    def stop(self):
        self._stop = True
        self.thread.join()

    def get_slave_path(self):
        return self.slave_path