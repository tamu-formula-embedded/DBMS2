# can.py
import socket
import threading
import time

HOST = "127.0.0.1"
PORT = 4002  # CAN

FRAME_LEN = 12  # 4-byte BE ID + 8 data

# IDs (from your sheet)
TX_HEARTBEAT       = 0x0501
TX_CONSOLE_LOG     = 0x0502
TX_VOLTAGE_MEAS    = 0x0507
TX_TEMP_MEAS       = 0x0508
TX_FATAL_ERROR     = 0x050B
TX_CONFIG_ACK      = 0x0522
TX_CONFIG_RESP     = 0x0523

RX_HEARTBEAT       = 0x0541
RX_SET_CONFIG      = 0x0542
RX_GET_CONFIG      = 0x0543

def pack(can_id: int, data: bytes = b"") -> bytes:
    d = (data + b"\x00"*8)[:8]
    return can_id.to_bytes(4, "big") + d

def u16(hi: int, lo: int) -> int:
    return ((hi & 0xFF) << 8) | (lo & 0xFF)

class CanServer:
    def __init__(self):
        self.srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.srv.bind((HOST, PORT))
        self.srv.listen(1)
        self.conn: socket.socket | None = None
        self.addr = None
        self._stop = threading.Event()

    def start(self):
        print(f"[can] waiting on {HOST}:{PORT} …")
        self.conn, self.addr = self.srv.accept()
        print(f"[can] connected from {self.addr}")
        threading.Thread(target=self._rx_loop, daemon=True).start()
        threading.Thread(target=self._heartbeat_loop, daemon=True).start()

    # ---- transmit (PC -> firmware)
    def send_heartbeat(self):
        self.conn.sendall(pack(RX_HEARTBEAT))

    def get_config(self, index: int):
        self.conn.sendall(pack(RX_GET_CONFIG, bytes([index & 0xFF])))

    def set_config(self, index: int, value: int):
        buf = bytearray(8)
        buf[0] = index & 0xFF    # Data1
        buf[4] = value & 0xFF    # Data5
        self.conn.sendall(pack(RX_SET_CONFIG, buf))

    # ---- receive (firmware -> PC)
    def _rx_loop(self):
        try:
            while not self._stop.is_set():
                pkt = self._recv_exact(FRAME_LEN)
                if not pkt:
                    break
                self._handle(pkt)
        except (OSError, ConnectionError):
            pass
        finally:
            self._stop.set()

    def _recv_exact(self, n: int) -> bytes:
        buf = bytearray()
        while len(buf) < n:
            chunk = self.conn.recv(n - len(buf))
            if not chunk:
                return b""
            buf.extend(chunk)
        return bytes(buf)

    def _handle(self, packet: bytes):
        can_id = int.from_bytes(packet[:4], "big")
        d = packet[4:12]

        if can_id == TX_CONSOLE_LOG:
            s = d.rstrip(b"\x00").decode(errors="replace")
            print(f"[LOG] {s}")
        elif can_id == TX_HEARTBEAT:
            segs, mons, vgrps, therms, crc = d[0], d[1], d[2], d[3], d[7]
            print(f"[HB] segs={segs} mons/seg={mons} vgrps/mon={vgrps} therms/mon={therms} settings_crc=0x{crc:02X}")
        elif can_id == TX_VOLTAGE_MEAS:
            mon, first_idx = d[0], d[1]
            vals = [u16(d[2], d[3]), u16(d[4], d[5]), u16(d[6], d[7])]
            print(f"[V] mon={mon} idx0={first_idx} vals={vals}")
        elif can_id == TX_TEMP_MEAS:
            mon, first_idx = d[0], d[1]
            vals = [u16(d[2], d[3]), u16(d[4], d[5]), u16(d[6], d[7])]
            print(f"[T] mon={mon} idx0={first_idx} vals={vals}")
        elif can_id == TX_FATAL_ERROR:
            fname5 = d[:5].rstrip(b"\x00").decode(errors="replace")
            line   = d[5]
            errno  = d[7]
            print(f"[FATAL] file~='{fname5}' line={line} errno={errno}")
        elif can_id == TX_CONFIG_ACK:
            print(f"[CFG-ACK] index={d[0]}")
        elif can_id == TX_CONFIG_RESP:
            print(f"[CFG-RSP] index={d[0]} value={d[4]}")
        else:
            print(f"[UNK 0x{can_id:03X}] {d.hex()}")

    def _heartbeat_loop(self):
        while not self._stop.is_set():
            try:
                self.send_heartbeat()
            except (OSError, ConnectionError):
                break
            time.sleep(1.0)

    def close(self):
        self._stop.set()
        try:
            if self.conn:
                self.conn.close()
        finally:
            self.srv.close()

if __name__ == "__main__":
    srv = CanServer()
    srv.start()
    try:
        # Example one-shots (optional):
        # time.sleep(0.2); srv.get_config(0x01)
        # time.sleep(0.2); srv.set_config(0x02, 0x7A)
        while not srv._stop.is_set():
            time.sleep(0.25)
    except KeyboardInterrupt:
        pass
    finally:
        srv.close()
