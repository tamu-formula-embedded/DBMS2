import socket
import select
import threading

class NetCtx:
    def __init__(self, sock=None):
        self.sock = sock or socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    def connect(self, ip: str, port: int):
        self.sock.connect((ip, port))

    def listen(self, port: int):
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.sock.bind(("127.0.0.1", port))
        self.sock.listen()

    def accept(self) -> 'NetCtx':
        client_sock, _ = self.sock.accept()
        return NetCtx(client_sock)

    def send(self, data: bytes):
        total_sent = 0
        while total_sent < len(data):
            sent = self.sock.send(data[total_sent:])
            if sent == 0:
                raise RuntimeError("Socket connection broken")
            total_sent += sent

    def recv(self, max_size: int, timeout_ms: int) -> bytes:
        ready = select.select([self.sock], [], [], timeout_ms / 1000)
        if not ready[0]:
            return b""
        return self.sock.recv(max_size)

    def close(self):
        self.sock.close()


def handle_client(client: NetCtx, callback):
    try:
        while True:
            data = client.recv(1024, 5000)
            if not data:
                print("Client disconnected")
                break
            print("Received:", data.decode(errors="replace"))
            client.send(b"Echo: " + data)
    except Exception as e:
        print("Error with client:", e)
    finally:
        client.close()

def start_server(port, callback):
    server = NetCtx()
    server.listen(port)
    try:
        while True:
            client = server.accept()
            print("Client connected")
            threading.Thread(target=handle_client, args=(client, callback), daemon=True).start()
    except KeyboardInterrupt:
        print("Server shutting down")
        server.close()

if __name__ == "__main__":
    start_server()
