
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

FRAME_POLL_VOLTS = [
    b'\xc0\x05t\x13J\xe8'
]

FRAME_START_ADC = [
    b'\xd0\x03\r\x06Lv'
]
        
class StackSim:
   
    def __init__(self):
        self.alive = False
        self.polling_volts = True
        self.recv_frames = []

    def check(self, n, val):
        if len(self.recv_frames) < n:
            return False
        return self.recv_frames[-n] == val
            
    def checkseq(self, seq):
        return all([self.check(len(seq) - i, v) for i, v in enumerate(seq)])

    def handle_uart_frame(self, packet):
        self.recv_frames.append(packet)
        print(time.time(), '', packet)
        
        if not self.alive and self.checkseq(FRAME_WAKE):
            self.alive = True
            
        if self.alive and self.checkseq(FRAME_SHUTDOWN):
            self.alive = False
            self.polling_volts = False
            
        if self.alive:
                
            if not self.polling_volts and self.checkseq(FRAME_START_ADC):
                self.polling_volts = True
                
            if self.polling_volts and self.checkseq(FRAME_POLL_VOLTS):
                print("Should respond with voltage levels!")
            
        
stack = StackSim()

uart_ipc = SerialIpcReader(packet_callback=stack.handle_uart_frame)
uart_ipc.start("UART")
