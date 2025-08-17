from random import randint

from cmd_hdlr import Command

# USER DEFINED UNIQUE TO EACH BATTERY
N_SEGMENTS = 1
N_MONITORS_PER_SEG = 4
N_GROUPS = 14
N_TEMPS = 10
# END USER DEFINED
N_MONITORS = N_SEGMENTS * N_MONITORS_PER_SEG


class Monitor:
    def __init__(self, addr):
        self.addr = addr
        self.volts = [0] * N_GROUPS
        self.temps = [0] * N_TEMPS

    def get_volts_frame(self):
        # todo: fix incorrect frames
        vresp = b'\xb0' + self.addr.to_bytes(1, byteorder='big') + b'\x00\x00'
        for v in self.volts:
            vresp += (v).to_bytes(2, byteorder='big')
        return vresp + b'\x00\x00' # <-- crc go here

class Battery:
    def __init__(self):
        self.monitors = [Monitor(i+1) for i in range(N_MONITORS)]
        
    def set_volts(self, ir, jr, provider):
        for i in ir:
            if i >= N_MONITORS or i < 0:
                print(f'Monitor {i} out of range (check stats)')
                return
            for j in jr:
                if j >= N_GROUPS or j < 0:
                    print(f'Voltage group {j} out of range (check stats)')
                    return
                self.monitors[i].volts[j] = provider()
                
    def get_volts_frames(self):
        resp = b''
        for mon in self.monitors:
            resp += mon.get_volts_frame()
        return resp
                
    def proc_battery_cmd(self, cmd: Command):
        if cmd.consume('set'):
            metric = cmd.next()
            if metric == 'v':
                mon_n = cmd.next_range(h=N_MONITORS)
                item_n = cmd.next_range(h=N_GROUPS)
                val = int(cmd.next())
                def setter(): return val
                self.set_volts(mon_n, item_n, setter)
                    
        elif cmd.consume('view'):
            metric = cmd.next()
            for i, monitor in enumerate(self.monitors):
                if metric == 'v':
                    print(' '.join(map(str, monitor.volts))) 
                elif metric == 't':
                    print(' '.join(map(str, monitor.temps)))
                
            
            
            
    
    


    
   
 
    
        