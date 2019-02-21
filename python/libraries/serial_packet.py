import sys
import error

#sds python libraries
sys.path.append('..\libraries')
from logger import _print, _print_bytes, APP, ERROR, WARNING, DEBUG
from serial_interface import SerialInterface

SERIAL_PREAMBLE = b'\xee'
SERIAL_TRAILER  = b'\x0a\x0d'

g_serial = SerialInterface()
 
class SerialPacket:
    """
    Serial Packet Structure:
        Serial Preamble
        Notification Length
        Notification Data
        Trailer
    """
        
    def __init__(self):

        
        self.bytes = None
    
    def set(self, _bytes):
           
        #calculate length of remaining bytes
        length = len(_bytes)
        length_hex = length.to_bytes(1,byteorder='little')
        
        #build
        self.bytes = SERIAL_PREAMBLE + length_hex + _bytes + SERIAL_TRAILER
        
        _print_bytes("Serial Packet: ",self.bytes, DEBUG)
        
    def get_notif(self):
        return self.notif_bytes
        
    def send(self):
        #send serial packet to central
        g_serial.write(self.bytes)
        _print("Serial Packet Sent.", DEBUG)

           
    def receive(self):
        
        #read up to next preamble, parse, and check
        preamble_byte = g_serial.read_until(SERIAL_PREAMBLE)
        if preamble_byte == b'':
            _print("Serial Read Timeout.", DEBUG)
            raise error.TimeoutErr
        
        #check for misalignment
            
        if preamble_byte != SERIAL_PREAMBLE and preamble_byte[-1] != SERIAL_PREAMBLE[0]:
            _print("Serial Packet Preamble Error: " + hex(preamble_byte), ERROR)
            raise error.PreambleErr
            
        #read length byte, parse, and check
        length_byte = g_serial.read_bytes(1)
        length = int.from_bytes(length_byte, byteorder='little', signed=False)
        
        #read notif bytes and trailer byte
        self.notif_bytes = g_serial.read_bytes(length)
        trailer_bytes = g_serial.read_bytes(len(SERIAL_TRAILER))
                
        #build bytes for debugging
        _print("Serial Packet Recieved.", DEBUG)
        
def setup_serial(port):
    g_serial.configure(port)
    g_serial.run()
    