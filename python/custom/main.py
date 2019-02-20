import sys
sys.path.append('..\libraries')
from serial_interface import SerialStream
from datetime import datetime 
import math
import keyboard
import click
import error
import command
 


serial = SerialStream()
    
### Custom Command ###  
@click.command()
@click.option('--payload',prompt='Command',help='Bytes to send. Start with 0x. Ex: 0xbb2000')
@click.option('--port', prompt='Port',help='Serial COM Port Connected to Central Device')
def main(payload, port):
    """Sends a custom command."""
    
    # Setup Serial
    serial.configure(port=port, speed=1000000)
    serial.run()
      
    # Build Command
    cmd = command.Command(payload)

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

    
if __name__ == '__main__':
    main()    