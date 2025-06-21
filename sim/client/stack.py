
import os
import pty
import threading
import time

from ipc import SerialIpcReader


def handle_uart_frame(packet):
    print(packet)
    print("\n")

uart_ipc = SerialIpcReader(packet_callback=handle_uart_frame)
uart_ipc.start("UART")
