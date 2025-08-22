

class Command:
    def __init__(self, inp):
        self.valid = False
        self.parts = []
        self.i = 0
        try:
            self.parts = inp.split(' ')
            if len(self.parts) > 0:
                self.valid = True
        except Exception as _:
            pass
    
    def consume(self, tk):
        if self.i < len(self.parts):
            if self.parts[self.i] == tk:
                self.i += 1
                return True
            # print(f'Invalid command, expected {tk}, got {self.parts[self.i]}')
            return False
        else:
            print(f'Invalid command, expected {tk}, got nothing')
    
    def next(self):
        if self.i < len(self.parts):
            a = self.parts[self.i]
            self.i += 1
            return a
        print('Invalid command, expected something, got nothing')
        
    def next_range(self, l=0, h=1):
        if self.i < len(self.parts):
            a = self.parts[self.i]
            self.i += 1
            if a == '*':
                return range(l, h)
            else:
                return range(int(a), int(a)+1)
        print('Invalid command, expected something, got nothing') 
 