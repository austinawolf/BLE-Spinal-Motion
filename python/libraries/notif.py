import sys
from serial_packet import SerialPacket

MINIMUM_NOTIF_LENGTH = 3


#sds python libraries
sys.path.append('..\libraries')
from logger import _print, _print_bytes, APP, ERROR, WARNING, DEBUG
 
class Notif:

    def __init__(self):
        self.bytes = None
    
    def set(self, _bytes):
        self.bytes = _bytes
        
    def send(self):
        _print_bytes("Sent Notif: ",self.bytes, DEBUG)
        serial_packet = SerialPacket()
        serial_packet.set(self.bytes)
        serial_packet.send()

        
    def get(self):
        return self.bytes
        
    def receive(self):
        serial_packet = SerialPacket()
        serial_packet.receive()
        self.bytes = serial_packet.get_notif()
        _print_bytes("Recieved Notif: ",self.bytes, DEBUG)

