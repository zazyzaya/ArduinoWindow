class Color:
    MAX_H = 255+42
    MIN_S = 90
    MAX_V = 254

    def __init__(self, h, s=255, v=30):
        self.h = h; self.s = s; self.v = v 
    
    def incriment(self):
        ret = False 
        
        if self.h < self.MAX_H: 
            self.h += 1; ret = True 
        if self.s > self.MAX_S:
            self.s -= 1; ret = True 
        if self.v < self.MAX_V: 
            self.v += 1; ret = True 
        
        return ret 
    
    def get_cycle_len(self):
        return max(
            self.get_individual_lens()    
        )
    
    def get_individual_lens(self):
        return [
            self.MAX_H - self.h, 
            self.s - self.MAX_S, 
            self.MAX_V - self.v 
        ]

colors = [
  Color(177), # Blue
  Color(187), # Purple
  Color(197), # Pink 
  Color(227)  # Red 
]

'''
>>> [c.get_cycle_len() for c in colors]
[224, 224, 224, 224]
>>>
>>> [c.get_individual_lens() for c in colors]
[[120, 165, 224], [110, 165, 224], [100, 165, 224], [70, 165, 224]]
'''