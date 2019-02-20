import sys
sys.path.append('..\libraries')
from serial_interface import SerialStream
from datetime import datetime 
import math
import keyboard
import click
import error
import command
import opcodes
from time import sleep

TIMEOUT_S = 2
WAIT_TIME_S = 0.1

serial = SerialStream()
    
### Custom Command ###  
@click.command()
@click.option('--port', prompt='Port',help='Serial COM Port Connected to Central Device')
@click.option('--ble',is_flag=True,help='Send session data to central via BLE')
@click.option('--memory',is_flag=True,help='Save session data to memory')
@click.option('--filename',prompt='Filename: ', help='File to save BLE data')
def main(port, ble, memory, filename):
    """Starts a data session."""
        
    # Setup Serial
    serial.configure(port=port, speed=1000000)
    serial.run()    
      
    # Build Command
    if (ble == True and memory == True):
        cmd = command.Command(opcodes.START_SESSION_BLE_AND_MEMORY)
    elif (ble == True):
        cmd = command.Command(opcodes.START_SESSION_MEMORY)
    elif (memory == True):
        cmd = command.Command(opcodes.START_SESSION_BLE)
    else:    
        error.perror("Select a data destination (Use --ble or --memory. Type --help for more info).")
        sys.exit()

    # Setup File

    
    # Send Command
    cmd.send(serial)
    
    # Check command for error
    if error.check(cmd.status):
        click.echo("Command Error:" + str(cmd.status))
        sys.exit()

    # Wait for response
    resp = command.Response(serial)

    # Parse response
    resp.parse()
        
    # Check response status code 
    if error.check(resp.status):
        click.echo("Response Error:" + str(resp.status))
        sys.exit()
    
    # Print Response
    resp.print()
    
    # Check response error code
    if resp.err_code != "0x0":
        sys.exit()
        
    # Collect Data
    while (True):
    
        #check buffer
        line = serial.readStream()
        if line == b'':
            continue
        
        click.echo(line)
                
        #parse line
        data = Data(line)
        
        #check status
        if error.check(data.status):
            click.echo("Response Error:" + str(resp.status))
            continue   
        
        #parse raw data
        data.parse()
        
        #check status
        if error.check(data.status):
            click.echo("Response Error:" + str(resp.status))
            continue         
        
        #if quat print and return
    
        
        #save to file
        
        
        #check for keyboard interrupt
            
            



    
if __name__ == '__main__':
    main()    