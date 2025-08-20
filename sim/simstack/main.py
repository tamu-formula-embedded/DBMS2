
from stack_conn import StackConnection 
from cmd_hdlr import Command
from battery import Battery, Monitor

from queue import Queue

# FRAME_POLL_VOLTS = [ b'\xc0\x05t\x13J\xe8' ]

FRAME_POLL_VOLTS = [0xc0, 0x05, 0x6c, 0x1b, 0x41, 0x2e]


class Responder:
    def __init__(self, battery):
        self.past_rx_data = [] #make q
        self.battery = battery
        self.log = Queue()
        
    def plog(self, s): 
        self.log.put(s)

    def check(self, n, val):
        if len(self.past_rx_data) < n:
            return False
        return self.past_rx_data[-n] == val
            
    def checkseq(self, seq):
        return all([self.check(len(seq) - i, v) for i, v in enumerate(seq)])

    def on_uart_rx(self, data: bytes):
        self.plog(f"[UART RX] {len(data)}B: {data.hex(' ')}")
        
        self.past_rx_data += data
        if self.checkseq(FRAME_POLL_VOLTS):
            srv.send_uart(battery.get_volts_frames())
        
        # Example: send 8 bytes back if your firmware is waiting for 8
        # srv.send_uart(b"\xAA\x55\xAA\x55\xAA\x55\xAA\x55")

battery = Battery()
resp = Responder(battery)

srv = StackConnection(on_rx=resp.on_uart_rx)
srv.start()


print("UART server listening on 127.0.0.1:4001")
try:
    while True:
        command = Command(input('> '))
        if command.valid:
            if command.consume('exit'):
                break
            if command.consume("log"):
                try:
                    while True:
                        while resp.log.not_empty:
                            print(resp.log.get())
                except KeyboardInterrupt:
                    pass
                
            battery.proc_battery_cmd(command)
            
except KeyboardInterrupt:
    print('Use exit to leave')

srv.close()
