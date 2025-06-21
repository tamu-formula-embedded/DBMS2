
import os
import pty
import threading
import time

from ipc import SerialIpcReader

def handle_can_packet(packet):
    id_bytes = packet[:4]
    packet_id = int.from_bytes(id_bytes, byteorder='big')
    
    if packet_id == 0x5F2:
        data_bytes = packet[4:]  # 8-byte data
        print("CAN | ", data_bytes.decode('utf-8', errors='replace'))

can_ipc = SerialIpcReader(packet_callback=handle_can_packet, fixed_length=12)
can_path = can_ipc.start("CAN")
