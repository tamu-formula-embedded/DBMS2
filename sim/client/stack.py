
import os
import pty
import threading
import time
from enum import Enum

from ipc import SerialIpcReader



FRAME_WAKE = [
    b'\x00',
    b'\x90\x00\x03\t \x13\x95'
]


FRAME_SHUTDOWN = [
    b'\x01',
    b'\x90\x00\x03\t \x13\x95'
]

        
class State(Enum):
    SHUTDOWN = 1
    AWAKE = 2
    
class StackSim:
   
    def __init__(self):
        self.state = State.SHUTDOWN
        self.recv_frames = []

    def check(self, n, val):
        if len(self.recv_frames) < n:
            return False
        return self.recv_frames[-n] == val
            
    def checkseq(self, seq):
        return all([self.check(len(seq) - i, v) for i, v in enumerate(seq)])

    def handle_uart_frame(self, packet):
        self.recv_frames.append(packet)
        print('RX ', packet)
        
        if self.state == State.SHUTDOWN and self.checkseq(FRAME_WAKE):
            self.state = State.AWAKE
            
        if self.state == State.AWAKE and self.checkseq(FRAME_SHUTDOWN):
            self.state = State.SHUTDOWN
            
        print(self.state)
        
stack = StackSim()

uart_ipc = SerialIpcReader(packet_callback=stack.handle_uart_frame)
uart_ipc.start("UART")
