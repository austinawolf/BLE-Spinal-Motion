import click
import sys

#Error Codes
SUCCESS                 =  0
DATA_UNITIALIZED_ERR    = -1
LINE_LENGTH_ERR         = -2
PREAMBLE_ERR            = -3
DATA_FLAG_ERR           = -4
ARG_LENGTH_ERR          = -5
TIMEOUT_ERR             = -6
TRAILER_ERR             = -7
DATA_LEN_ERR            = -8

def perror(msg):
    click.echo("Error: " + str(msg))
    sys.exit()

def pwarning(msg):
    click.echo("Warning: " + str(msg))
    
def check(err_code):
    if type(err_code) == int and err_code < 0:
        return True
    else:
        return False
        
def check_warning(err_code):
    if type(err_code) == int and err_code < 0:
        pwarning(err_code) 
        return True
    return False