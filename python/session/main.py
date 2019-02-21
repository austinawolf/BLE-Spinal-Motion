#standard libraries
import sys

#pip libraries
import click

#sds python libraries
sys.path.append('..\libraries')
import error
import payloads
from logger import _print, APP, ERROR, WARNING, DEBUG
from serial_interface import SerialInterface
from file import File
from command import Command, Response
from data import Data
from serial_packet import setup_serial

#global instances
global g_file

### Custom Command ###  
@click.command()
@click.option('--port', prompt='Port',help='Serial COM Port Connected to Central Device')
@click.option('--ble',is_flag=True,help='Send session data to central via BLE')
@click.option('--memory',is_flag=True,help='Save session data to memory')
@click.option('--filename',prompt='Filename: ', help='File to save BLE data')
def main(port, ble, memory, filename):
    """Starts a data session."""
        
    # Setup Serial
    setup_serial(port)
    
    # Setup File
    setup_file(filename)
    
    # Send Command
    start_session(ble, memory)
       
    # Get Response
    response = get_response()

    # Wait for data packets
    while (True):
        
        try:
            # Get data
            data = get_data()
            data.save(g_file)
    
        except KeyboardInterrupt:
            _print("Keyboard Interrupt", DEBUG)
            break       
        except error.MyError as e:
            raise e
    
    end_session()
    
    # Wait for response packet
    while (True):
        try:
            # Get Response
            response = get_response()
            break
        except error.MyError:
            continue             
          
    
    # Clean up
    g_file.close()
    
    
    
def setup_file(filename):
    """ Sets up file for data logging """
    global g_file
    g_file = File(filename)
    
def start_session(ble, memory):
    """ Sends a command to the peripheral """
    _print("Staring Session.",DEBUG)   
    Command(payloads.START_SESSION_BLE)
    
    
def get_response():
    """ Gets a response packet from peripheral """
    _print("Getting Response.", DEBUG)  
    return Response()
    
def get_data():
    """ Gets a data packet from peripheral """
    _print("Getting Data.", DEBUG)
    return Data()
    
def end_session():
    """ Ends the session by sending a command and closing file """
    _print("Ending Session.", DEBUG)    
    Command(payloads.STOP_SESSION)  
    pass

    
if __name__ == '__main__':
    main()    