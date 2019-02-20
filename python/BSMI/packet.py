import click

LINE_PREAMBLE = "Packet,"
MIN_PACKET_LENGTH = 2

#Packet Contents
EVENT_NUM_LOCATION = 0
DATA_FLAGS_LOCATION = 1
DATA_LOCATION = 2

#Error Codes
SUCCESS                 =  0
DATA_UNITIALIZED_ERR    = -1
LINE_LENGTH_ERR         = -2
PREAMBLE_ERR            = -3
DATA_FLAG_ERROR         = -4
ARG_LENGTH_ERROR        = -5

#Data flag masks
QUATERNION_DATA     =   (1<<0)
IMU_DATA 		    =   (1<<1)
COMPASS_DATA 	    =   (1<<2)
TIMESTAMP_DATA 	    =	(1<<3)
SESSION_INFO_DATA   =   (1<<4)
MEMORY_DATA         =   (1<<5)

class Response():
    
    def __init__(self):
        pass
        
    def parse(self, line):
        split = line.replace("\n","").split(",")
        if "Resp" not in line:
            self.status = PREAMBLE_ERR
            return
        elif len(split) < 3 or len(split) > 20:
            self.status = LINE_LENGTH_ERR
            return
        
        self.preamble = split[0]
        self.opcode = int(split[1])
        self.err_code = int(split[2])
        self.arg_len = int(split[3])
        
        if self.opcode == 0x26:
            self.arg = SessionInfo()
            self.status = self.arg.parse(split[4:])
            if self.status != SUCCESS:
                return
        elif self.opcode == 0x28:
            self.arg = SessionInfo()
            self.status = self.arg.parse(split[4:])
            if self.status != SUCCESS:
                return
                
        return SUCCESS



class SessionInfo():

    def __init__(self):
        self.length = 4
        self.data_length = None
        
    def parse(self, arg_list):
        if len(arg_list) != self.length:
            return ARG_LENGTH_ERROR
        
        self.data_length = int(arg_list[0])    
        self.motion_mode = int(arg_list[1])    
        self.motion_rate = int(arg_list[2])    
        self.compass_rate = int(arg_list[3])

        return SUCCESS
        

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

    def __init__(self):
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

    def __init__(self):
        self.line_length = 6
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
            
class Sample():


    def __init__(self, line):
        self.status = SUCCESS
        self.data_flags = 0
        self.data = None
        self.timestamp = 0
        self.event_num = 0
        
        #check for packet header
        if LINE_PREAMBLE not in line:
            self.status = PREAMBLE_ERR
            return
                
        #remove header and split up data into list
        data = line.replace(LINE_PREAMBLE,"").replace("\r","").replace("\n","")
        split = data.split(",")
		     
        #check for list length
        if len(split) < MIN_PACKET_LENGTH:
            self.status = LINE_LENGTH_ERR
            return
        
        #load event number and data flags
        self.event_number = int(split[EVENT_NUM_LOCATION])
        self.data_flags = int(split[DATA_FLAGS_LOCATION])

        #depending on packet type load data
        if (self.data_flags & QUATERNION_DATA):
        
            self.data = Quaternion()        
            if len(split) != self.data.line_length:
                self.status = LINE_LENGTH_ERR
                return               
            self.data.load_quat(split[DATA_LOCATION],split[DATA_LOCATION+1],split[DATA_LOCATION+2],split[DATA_LOCATION+3])            
        
        elif (self.data_flags & IMU_DATA):
        
            self.data = Imu()      
            if len(split) != self.data.line_length:
                self.status = LINE_LENGTH_ERR
                return       
            self.data.load_accel(split[DATA_LOCATION],split[DATA_LOCATION+1],split[DATA_LOCATION+2])
            self.data.load_gyro(split[DATA_LOCATION+3],split[DATA_LOCATION+4],split[DATA_LOCATION+5])

        elif (self.data_flags & COMPASS_DATA):
        
            self.data = Compass()       
            if len(split) != self.data.line_length:
                self.status = LINE_LENGTH_ERR
                return       
            self.data.load_compass(split[DATA_LOCATION],split[DATA_LOCATION+1],split[DATA_LOCATION+2])

        elif (self.data_flags & TIMESTAMP_DATA):
            
            self.data = Timestamp()       
            if len(split) != self.data.line_length:
                self.status = LINE_LENGTH_ERR
                return       
            self.data.load_timestamp(split[DATA_LOCATION])

        else:
            self.status = DATA_FLAG_ERROR
                        
        self.status = SUCCESS
        return
		
    def __str__(self):
        return  str(self.event_num) + "," \
                + str(self.timestamp) + "," \
                + str(self.data) + "," \
                + str(self.status)
                
					