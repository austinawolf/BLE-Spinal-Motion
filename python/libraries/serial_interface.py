"""
Real-Time Graph: local process
    Role: data computation for Sine Wave graph
    References:
        https://docs.python.org/2/library/multiprocessing.html
    Credit for:
        https://github.com/ssepulveda/RTGraph
"""

from time import time
import serial

TIMEOUT_S = 1

class SerialStream:

    # Constructor
    def __init__(self):
        self._serial = serial.Serial()

    def is_ports_available(self, port):
        for p in self.port_available:
            if port == p:
                return True
        return False

    def run(self):
        try:
            self._serial.open()
        except serial.SerialException:
            print('Cannot open port')
            self._serial.close()



    def configure(self, port="COM4", speed=1000000):
       
        self._serial.port = port
        self._serial.baudrate = speed
        self._serial.stopbits = serial.STOPBITS_ONE
        self._serial.bytesize = serial.EIGHTBITS
        self._serial.timeout = TIMEOUT_S

    def stop(self):
        print('serial stop')
        self._exit.set()

    def readStream(self):
        return self._serial.readline()

    def readUntil(self):
        return self._serial.read_until()
        
    def readBytes(self,bytes):
        return self._serial.read(bytes)
        
    def writeStream(self, data):
        return self._serial.write(data)

    # @staticmethod
    # def get_os_type():
    #     os_name = platform.platform()
    #     if 'Darwin' in os_name:
    #         os_type = 0
    #     elif 'Window' in os_name:
    #         os_type = 1
    #     elif 'Linux' in os_name:
    #         os_type = 2
    #     else:
    #         os_type = 4
    #     return os_type
    #
    # # Scan available serial port
    # def scan_serial_port(self, os):
    #     ports_available = []
    #     if os == 0:
    #         return glob.glob('/dev/tty.*')
    #     else:
    #         for p in list(list_ports.comports()):
    #             ports_available.append(p.device)
    #         return ports_available
