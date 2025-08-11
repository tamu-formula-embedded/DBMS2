import socket
import threading
from typing import Callable, Optional

class StackConnection:
    """
    Minimal TCP server for your SIM UART (C side connects to 127.0.0.1:4001).
    - Calls on_rx(bytes) whenever data arrives.
    - Use send_uart(bytes) to deliver bytes to HAL_UART_Receive on the C side.
    """
    def __init__(self, host: str = "127.0.0.1", port: int = 4001,
                 on_rx: Optional[Callable[[bytes], None]] = None):
        self.host = host
        self.port = port
        self.on_rx = on_rx or (lambda b: None)
        self._srv: Optional[socket.socket] = None
        self._conn: Optional[socket.socket] = None
        self._stop = threading.Event()

    def start(self) -> None:
        self._srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self._srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self._srv.bind((self.host, self.port))
        self._srv.listen(1)
        threading.Thread(target=self._accept_loop, daemon=True).start()

    def _accept_loop(self):
        try:
            self._conn, _ = self._srv.accept()
            self._rx_loop()
        finally:
            self.close()

    def _rx_loop(self):
        try:
            while not self._stop.is_set():
                data = self._conn.recv(4096)
                if not data:
                    break
                self.on_rx(data)
        except OSError:
            pass

    def send_uart(self, data: bytes) -> None:
        if self._conn:
            self._conn.sendall(data)

    def close(self) -> None:
        self._stop.set()
        try:
            if self._conn:
                self._conn.close()
        finally:
            if self._srv:
                self._srv.close()



