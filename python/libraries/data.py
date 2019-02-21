import sys
import math

#sds python libraries
import error
from logger import _print, _print_bytes, bytes_to_string, APP, ERROR, WARNING, DEBUG
from notif import Notif
from vector import Vector


#Defs
DATA_PREAMBLE = b"\xaa"

#Data flag masks
QUATERNION_DATA     =   (1<<0)
IMU_DATA 		    =   (1<<1)
COMPASS_DATA 	    =   (1<<2)
TIMESTAMP_DATA 	    =	(1<<3)
SESSION_INFO_DATA   =   (1<<4)
MEMORY_DATA         =   (1<<5)
EULER_DATA          =   (1<<7)

class Euler(object):

    def __init__(self, q0, q1, q2, q3):
        magnitude = math.sqrt(q0 ** 2 + q1 ** 2 + q2 ** 2 + q3 ** 2)
        self.q0 = float(q0) / magnitude
        self.q1 = float(q1) / magnitude
        self.q2 = float(q2) / magnitude
        self.q3 = float(q3) / magnitude

    def magnitude(self):
        return math.sqrt(self.q0 ** 2 + self.q1 ** 2 + self.q2 ** 2 + self.q3 ** 2)

    @property
    def roll(self):
        return math.atan2(2*(self.q0*self.q1 + self.q2*self.q3), 1 - 2 * (self.q1*self.q1 + self.q2*self.q2)) * 57.29

    @property
    def pitch(self):
        return math.asin(2*(self.q0*self.q2 - self.q3*self.q1)) * 57.29

    @property
    def yaw(self):
        return math.atan2(2*(self.q0*self.q3 + self.q1*self.q2), 1 - 2 * (self.q2*self.q2 + self.q3*self.q3)) * 57.29
        
    def __str__(self):   
        return "{0:6.2f},{1:6.2f},{2:6.2f}".format(self.roll, self.pitch, self.yaw)     

    def print(self):   
        _print("\rEuler: x={0:6.2f}, y={1:6.2f}, z={2:6.2f}".format(self.roll, self.pitch, self.yaw), APP)  

class Timestamp():

    def __init__(self, args):
        self.length_bytes = 4
        
        if len(args) != self.length_bytes:
            raise error.DataLenErr
        
        try:
            self.timestamp = int.from_bytes(args,byteorder='little',signed=False)
        except Exception as e:
            raise error.DataParseEr
       
    def __str__(self):
        return str(self.timestamp)
            
class Imu():

    def __init__(self, args):
        self.length_bytes = 12
        if len(args) != self.length_bytes:
            raise error.DataLenErr
        
        try:
            self.ax = int.from_bytes(args[0:2],byteorder='little',signed=True)
            self.ay = int.from_bytes(args[2:4],byteorder='little',signed=True)
            self.az = int.from_bytes(args[4:6],byteorder='little',signed=True)
            self.gx = int.from_bytes(args[6:8],byteorder='little',signed=True)
            self.gy = int.from_bytes(args[8:10],byteorder='little',signed=True)
            self.gz = int.from_bytes(args[10:12],byteorder='little',signed=True)
            
        except Exception as e:
            raise error.DataParseErr
        
    def __str__(self):   
        return str(self.ax) + "," \
            + str(self.ay) + "," \
            + str(self.az) + "," \
            + str(self.gx) + "," \
            + str(self.gy) + "," \
            + str(self.gz)
            
class Compass():

    def __init__(self, args):
        self.length_bytes = 6
        
        if len(args) != self.length_bytes:
            raise error.DataLenErr
        
        try:
            self.cx = int.from_bytes(args[0:2],byteorder='little',signed=True)
            self.cy = int.from_bytes(args[2:4],byteorder='little',signed=True)
            self.cz = int.from_bytes(args[4:6],byteorder='little',signed=True)
        except Exception as e:
            raise error.DataParseError

    def __str__(self):   
        return str(self.cx) + "," \
            + str(self.cy) + "," \
            + str(self.cz)
                
class Quaternion():

    def __init__(self, args):
        self.length_bytes = 16
        
        if len(args) != self.length_bytes:
            raise error.DataLenErr
        
        try:
            self.q0 = int.from_bytes(args[0:4],byteorder='little',signed=True)
            self.q1 = int.from_bytes(args[4:8],byteorder='little',signed=True)
            self.q2 = int.from_bytes(args[8:12],byteorder='little',signed=True)
            self.q3 = int.from_bytes(args[12:16],byteorder='little',signed=True)
        except Exception as e:
            raise error.DataParseErr
        
        self.euler = Euler(self.q0, self.q1, self.q2, self.q2)
        self.euler.print()
        
    def __str__(self):   
        return str(self.euler) + "," \
            + str(self.q0) + "," \
            + str(self.q1) + "," \
            + str(self.q2) + "," \
            + str(self.q3)

class Data():

    def __init__(self):    
        notif = Notif()
        notif.receive()
        self.bytes = notif.get()
        self.motion = None
        
        #check preabmle
        self.preamble = self.bytes[0]
        if (self.preamble != DATA_PREAMBLE[0]):
            _print("Invalid Data Preamble.", ERROR)
            raise error.PreambleErr
            
        #get opcode
        self.data_flags = self.bytes[1]
        
        #get length and check
        self.packet_number = self.bytes[2]
            
        #check arg length       
        self.args = self.bytes[3:]

                 
        self.parse()
        self.print() 
        
    def parse(self):
    
        #depending on packet type load data
        if (self.data_flags & QUATERNION_DATA):          
            self.motion = Quaternion(self.args)                   
        elif (self.data_flags & IMU_DATA):
            self.motion = Imu(self.args)                         
        elif (self.data_flags & COMPASS_DATA):
            self.motion = Compass(self.args)                           
        elif (self.data_flags & TIMESTAMP_DATA):
            self.motion = Timestamp(self.args)                              
        else:
            raise error.DataFlagErr
        
    def print(self):
        _print("Data: ", DEBUG)      
        _print("\tPreamble=" + hex(self.preamble), DEBUG)        
        _print("\tData Flags=" + hex(self.data_flags), DEBUG)
        _print("\tPacket Number=" + str(self.packet_number), DEBUG)        
        _print_bytes("\tArgs=",self.args, DEBUG)
        _print("\tMotion=" + str(self.motion),DEBUG)


    def save(self, _file):
        line = str(self.packet_number) \
            + "," + str(self.data_flags) \
            + "," + str(self.motion)
        _file.write(line)
        
        