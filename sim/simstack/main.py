
from stack_conn import StackConnection 
from cmd_hdlr import Command
from battery import Battery, Monitor

def on_uart_rx(data: bytes):
    print(f"[UART RX] {len(data)}B:", data.hex(" "))
    # Example: send 8 bytes back if your firmware is waiting for 8
    # srv.send_uart(b"\xAA\x55\xAA\x55\xAA\x55\xAA\x55")

srv = StackConnection(on_rx=on_uart_rx)
srv.start()

battery = Battery()

print("UART server listening on 127.0.0.1:4001")
try:
    while True:
        command = Command(input('> '))
        if command.valid:
            battery.proc_battery_cmd(command)
            
except KeyboardInterrupt:
    srv.close()