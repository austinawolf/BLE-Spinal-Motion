from logger import *
import sys
from time import time
import serial

from logger import _print, _print_bytes, APP, ERROR, WARNING, DEBUG


TIMEOUT_S = 2




class SerialInterface:

    def __init__(self):
        self._serial = serial.Serial()

    def is_ports_available(self, port):
        for p in self.port_available:
            if port == p:
                return True
        return False

    def configure(self, port=None, speed=1000000):
        _print("Configuring Serial Port",DEBUG)       
        self._serial.port = port
        self._serial.baudrate = speed
        self._serial.stopbits = serial.STOPBITS_ONE
        self._serial.bytesize = serial.EIGHTBITS
        self._serial.timeout = TIMEOUT_S
        
    def run(self):
        try:
            self._serial.open()
        except serial.SerialException:
            _print('Cannot open port.', ERROR)
            self._serial.close()
            sys.exit()

    def stop(self):
        print('serial stop')
        self._exit.set()

    def read_until(self, bytes):
        read = self._serial.read_until(bytes,None)
        _print_bytes("Read: ",read,DEBUG)
        return read

    def read_bytes(self, num_bytes):
        read = self._serial.read(num_bytes)
        _print_bytes("Read: ",read,DEBUG)
        return read

             
    def write(self, data):
        _print_bytes("Write: ",data,DEBUG)
        return self._serial.write(data)


