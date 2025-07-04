
import os
import pty
import threading
import time

from ipc import SerialIpcReader

class MonitorSim:
    def __init__(self):
        self.packet_sender = None

    def handle_can_packet(self, packet):
        id_bytes = packet[:4]
        packet_id = int.from_bytes(id_bytes, byteorder='big')
        data = packet[4:]
        
        
        if packet_id == 0x5F2:
            if self.packet_sender is not None:
                self.packet_sender(data)
        
        if packet_id == 0x5F4 or packet_id == 0x5F5:
            if self.packet_sender is not None:
                self.packet_sender({
                    'type': 'v' if packet_id == 0x5F4 else 't',
                    'segment': int(data[0]),
                    'monitor': int(data[1]),
                    'item': int(data[2]),
                    'val': ((int(data[6]) << 8) + int(data[7]))
                })
                
                
monitor_sim = MonitorSim()

can_ipc = SerialIpcReader(packet_callback=monitor_sim.handle_can_packet, fixed_length=12)
can_path = can_ipc.start("CAN")
