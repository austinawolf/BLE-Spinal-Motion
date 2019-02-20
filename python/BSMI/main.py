from packet import Sample
from packet import Response
from serial_interface import SerialStream
from datetime import datetime 
import math
import opcodes
import keyboard
import sys
import click
import codecs
import error

TIMEOUT_MS = 2000

serial = SerialStream()
com_port = "NULL"

@click.group()
def main():
    pass
 
### Session Command ### 
@main.command()
@click.option('--port', prompt='Port',help='Serial COM Port Connected to Central Device.')
@click.option('--filename',prompt='filename',help='Location to save data.')
def session(port, filename):
    """Starts a data collection session."""
    
    setup_serial(port)  
    start_session(filename)
    
    #print filename
    click.echo('Filename: %s' % filename)

    
    
    
### Custom Command ###  
@main.command()
@click.option('--payload',prompt='Command',help='Opcode to run')
@click.option('--port', prompt='Port',help='Serial COM Port Connected to Central Device')
def custom(payload, port):
    """Sends a custom command."""
    setup_serial(port)
    
    click.echo('Command: %s' % payload)
    
    #run custom command
    send_command(payload)

    
    
    
    
### Stream Command ###      
@main.command()
@click.option('--port', prompt='Port',help='Serial COM Port Connected to Central Device.')
@click.option('--filename',prompt='filename',help='Location to save data.')
def stream(port, filename):
    """Streams any data in saved in flash memory."""  
    
    setup_serial(port)  
    start_stream(filename)
    
    #print filename
    click.echo('Filename: %s' % filename)
    

    
    
    
### General Support Funtions ###
     
def setup_serial(port):
    #check port is correct

    serial.configure(port=port, speed=1000000)
    serial.run()

def send_command1(com):
    #send the command
    trailer = "0d"
    com = com + trailer
    
    serial.writeStream(com.decode("hex"))
    
    while (True):
        bytes = serial.readBytes(1)
        click.echo(bytes)
    
def send_command(com):
    #send the command
    trailer = "0d"
    com = com + trailer
    
    serial.writeStream(codecs.decode(com,"hex"))
 
    #wait for response
    response = Response()
    while (True):
        line = get_line()
        if "Resp" in line:
            try:
                response.parse(line)
                click.echo("Status:" + str(response.status))
                click.echo("Data Length:" + str(response.arg.data_length))
                break
            except Exception as e:
                click.echo(e)
                break  
    click.echo(line)
 

 
def get_line():
    while(True):     
        line = serial.readStream()
        if line != "": return line

        
        
### Session Support Functions ###   
def start_session(filename):
    f = open(filename,'w')

    send_command(opcodes.START_SESSION_BLE_AND_MEMORY)
    click.echo('Session Started.')
    while(True):

        #get line
        line = get_line()

        #parse
        try:
            sample = Sample(line)    
        except Exception as e:
            click.echo(line)
            click.echo(e)
            continue
        
        #check status
        if sample.status != 0:
            continue
        else:
            pass
        
        #print
        click.echo(sample)
        f.write(str(sample) + "\n")

        #save to file
        if keyboard.is_pressed('q'):  # if key 'q' is pressed
            send_command(opcodes.STOP_SESSION)
            f.close()
            click.echo('\r\nSession Ended.')
            sys.exit()

### Streaming Support Functions ###   
def start_stream(filename):
    f = open(filename,'w')

    send_command(opcodes.START_STREAM)
    click.echo('Stream Started.')
    while(True):

        #get line
        line = get_line()
        click.echo(line)
        
        #parse
        try:
            sample = Sample(line)    
        except Exception as e:
            click.echo(line)
            click.echo(e)
            continue
        
        #check status
        if sample.status != 0:
            click.echo("Error: " + str(sample.status))
            continue
        else:
            pass
        
        #print
        click.echo(sample)
        f.write(str(sample) + "\n")
        
        #save to file
        if keyboard.is_pressed('q'):  # if key 'q' is pressed
            send_command(opcodes.STOP_STREAM)
            f.close()
            click.echo('\r\Stream Ended.')
            sys.exit()
            
if __name__ == '__main__':
    main()
    