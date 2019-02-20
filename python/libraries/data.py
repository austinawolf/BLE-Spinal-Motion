from serial_interface import SerialStream
import error
from time import sleep
import click
import codecs

TIMEOUT_S = 2
WAIT_TIME_S = 0.1
DATA_TRAILER = '0d'
DATA_PREAMBLE = "0xaa"
MIN_DATA_LEN = 4



#Data flag masks
QUATERNION_DATA     =   (1<<0)
IMU_DATA 		    =   (1<<1)
COMPASS_DATA 	    =   (1<<2)
TIMESTAMP_DATA 	    =	(1<<3)
SESSION_INFO_DATA   =   (1<<4)
MEMORY_DATA         =   (1<<5)

class Timestamp():

    def __init__(self):
        self.line_length = 3
        self.timestamp = None
    
    def load_timestamp(self, timestamp):
        self.timestamp = timestamp
       
    def __str__(self):
        return str(TIMESTAMP_DATA) + "," \
            + str(self.timestamp)
            
class Imu():

    def __init__(self, args):
        self.line_length = 8
        self.ax = None
        self.ay = None
        self.az = None
        self.gx = None
        self.gy = None
        self.gz = None
                
    def load_accel(self, x, y, z):
        self.ax = int(x)
        self.ay = int(y)
        self.az = int(z)
        
    def load_gyro(self, x, y, z):
        self.gx = int(x)
        self.gy = int(y)
        self.gz = int(z)
        
    def __str__(self):   
        return str(IMU_DATA) + "," \
            + str(self.ax) + "," \
            + str(self.ay) + "," \
            + str(self.az) + "," \
            + str(self.gx) + "," \
            + str(self.gy) + "," \
            + str(self.gz)
            
class Compass():

    def __init__(self):
        self.line_length = 5
        self.cx = None
        self.cy = None
        self.cz = None
        
    def load_compass(self, x, y, z):
        self.cx = int(x)
        self.cy = int(y)
        self.cz = int(z)

    def __str__(self):   
        return str(COMPASS_DATA) + "," \
            + str(self.cx) + "," \
            + str(self.cy) + "," \
            + str(self.cz)
                
class Quaternion():

    def __init__(self, args):
        self.raw_arg_length = 6
        if len(args) != self.line_length
        
        self.q0 = None
        self.q1 = None
        self.q2 = None
        self.q3 = None
        
    def load_quat(self, q0, q1, q2, q3):
        self.q0 = int(q0)
        self.q1 = int(q1)
        self.q2 = int(q2)
        self.q3 = int(q3)

    def __str__(self):   
        return str(QUATERNION_DATA) + "," \
            + str(self.q0) + "," \
            + str(self.q1) + "," \
            + str(self.q2) + "," \
            + str(self.q3)

class Data():

    def __init__(self, line):
    
        self.byte_list = []
        self.status = error.SUCCESS
        self.sample = None 
        
        for byte in line:
            self.byte_list.append(hex(byte))
            return

        if self.byte_list[0] == DATA_PREAMBLE:
            self.status = error.PREAMBLE_ERR
            return
        
        if self.byte_list[-1] != DATA_TRAILER:
            self.status = error.TRAILER_ERR
            return

        if len(self.byte_list) < MIN_DATA_LEN:
            self.status = error.DATA_LEN_ERR
            return

            
        self.data_flags         = self.byte_list[1]
        self.packet_num         = self.byte_list[2]  
        self.raw_data           = self.byte_list[3:]
    
    def parse(self):
        click.echo(self.args)
    
        #depending on packet type load data
        if (self.data_flags & QUATERNION_DATA):          
            self.sample = Quaternion(self.args)                   
        elif (self.data_flags & IMU_DATA):
            self.sample = Imu(self.args)                         
        elif (self.data_flags & COMPASS_DATA):
            self.sample = Compass(self.args)                           
        elif (self.data_flags & TIMESTAMP_DATA):
            self.sample = Timestamp(self.args)                              
        else:
            self.status = DATA_FLAG_ERROR
        

    def print(self):
        click.echo("Response")
        click.echo("  Opcode: " + self.opcode)
        click.echo("  Err Code: " + self.err_code)
        click.echo("  Arg Len: " + str(self.arg_len))
        click.echo("  Args: " + str(self.args))



        
        