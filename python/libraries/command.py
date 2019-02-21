import sys

#sds python libraries
sys.path.append('..\libraries')
from logger import _print, _print_bytes, bytes_to_string, APP, ERROR, WARNING, DEBUG
from notif import Notif
import error

COMMAND_PREAMBLE = b"\xbb"
RESPONSE_PREAMBLE = b"\xcc"
DATA_PREAMBLE = b"\xaa"

# Payloads
START_SESSION_BLE = b"\x26\x01\x01"
START_SESSION_MEMORY = b"\x26\x01\x02"
START_SESSION_BLE_AND_MEMORY = b"\x26\x01\x03"
STOP_SESSION = b"\x27\x00"
START_STREAM = b"\x27\x00"
STOP_STREAM = b"\x27\x00"
GET_FW_VERSION = b"\x20\x00"


 
class Command():

    def __init__(self, payload):
        _print("Sending Command, Payload=: " + str(payload), DEBUG)
    
        self.bytes = COMMAND_PREAMBLE + payload
        
        #check preabmle
        self.preamble = self.bytes[0]
  
        #get opcode
        self.opcode = self.bytes[1]
        
        #get length and check
        self.length = self.bytes[2]
            
        #check arg length       
        self.args = self.bytes[3:3+self.length]
              
        self.print()
        
        notif = Notif()
        notif.set(self.bytes)
        notif.send()        
        
        
    def print(self):
        _print("Command: ", APP)      
        _print("\tPreamble=" + hex(self.preamble), APP)        
        _print("\tOpcode=" + hex(self.opcode), APP)
        _print("\tLength=" + str(self.length), APP)        
        _print_bytes("\tArgs=",self.args, APP)        
 
class Response():

    def __init__(self):    
        notif = Notif()
        notif.receive()
        self.bytes = notif.get()
        
        #check preabmle
        self.preamble = self.bytes[0]
        if (self.preamble != RESPONSE_PREAMBLE[0]):
            _print("Invalid Response Preamble.", WARNING)
            raise error.PreambleErr
            
        #get opcode
        self.opcode = self.bytes[1]
        
        #get length and check
        self.length = self.bytes[2]
            
        #check arg length       
        self.args = self.bytes[3:3+self.length]
        
        self.print() 
        
    def print(self):
        _print("Response: ", APP)      
        _print("\tPreamble=" + hex(self.preamble), APP)        
        _print("\tOpcode=" + hex(self.opcode), APP)
        _print("\tLength=" + str(self.length), APP)        
        _print_bytes("\tArgs=",self.args, APP)        
        
        
      
        