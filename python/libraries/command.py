from serial_interface import SerialStream
import error
from time import sleep
import click
import codecs

COMMAND_TRAILER = '0d'
RESPONSE_TRAILER = '0xa'
PAYLOAD_PREAMBLE = "0xbb"
RESPONSE_PREAMBLE = "0xcc"
 
class Command():

    def __init__(self, payload):
        self.status = error.SUCCESS
        self.payload = None
        
        # Parse payload
        if payload[0:4] != PAYLOAD_PREAMBLE:
            self.status = PREAMBLE_ERR
            return

        click.echo("Payload: ")
        click.echo("  " + payload)

        payload = payload.replace("0x","")   
        payload = payload + COMMAND_TRAILER    
        payload_hex = codecs.decode(payload,"hex")    
    
        self.payload = payload_hex

    def send(self, serial):      
        serial.writeStream(self.payload)

       
class Response():


    def __init__(self, serial):
        self.status = error.SUCCESS
        self.line = None
        
        #check buffer
        line = serial.readStream()
        if line != b'':            
            self.line = line                   
        else:
            click.echo("Response Timeout.")
            self.status = error.TIMEOUT_ERR
    
    def parse(self):
    
        self.byte_list = []
        
        for byte in self.line:
            self.byte_list.append(hex(byte))

        if self.byte_list[0] != RESPONSE_PREAMBLE:
            self.status = error.PREAMBLE_ERR
            return
        
        if self.byte_list[-1] != RESPONSE_TRAILER:
            click.echo(self.byte_list)
            self.status = error.TRAILER_ERR
            return
            
        self.opcode     = self.byte_list[1]
        self.err_code   = self.byte_list[2]  
        self.arg_len    = int(self.byte_list[3],16)
        self.args       = self.byte_list[4:-1]          
        

    def print(self):
        click.echo("Response:")
        click.echo("  Opcode: " + self.opcode)
        click.echo("  Err Code: " + self.err_code)
        click.echo("  Arg Len: " + str(self.arg_len))
        click.echo("  Args: " + str(self.args))


class Custom(Command):

    pass
        
        

        
        