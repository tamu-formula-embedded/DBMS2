import time
from can import can_ipc
from stack import uart_ipc

if __name__ == '__main__':
    print(f'bin/sim {can_ipc.get_slave_path()} {uart_ipc.get_slave_path()}')
    
    while True:
        time.sleep(1)
    