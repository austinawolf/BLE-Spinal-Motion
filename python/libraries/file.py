
#standard libraries
import sys
import time

#pip libraries

#sds python libraries
sys.path.append('..\libraries')
from logger import _print, APP, ERROR, WARNING, DEBUG

HEADER = "Session Data:\n"

class File:

    def __init__(self,filename):
        _print("Creating File: " + str(filename), DEBUG)
        
        self.filename = filename
        
        try:
            self.f = open(filename, 'w')
        except Exception as e:
            _print("Could not open file.", ERROR)
            sys.exit()
            
        self.f.write(HEADER)
        
    def write(self, line):
        self.f.write(str(get_unix_time_ms()) + "," + str(line) + "\n")
        
    def close(self):
        _print("Closing File.", DEBUG)    
        self.f.close()
        
        
def get_unix_time_ms():
    return int(time.time() * 1000)