import click

# defs
DEBUG   = 4
WARNING = 3
ERROR   = 2
APP     = 1

PRINT_LEVEL = WARNING

def _print(string,level):

    if not level <= PRINT_LEVEL:
        pass
    else:
        if (level == DEBUG):
            click.echo("[DEBUG] " + str(string))
        elif (level == WARNING):
            click.echo("[WARNING] " + str(string))
        elif (level == ERROR):
            click.echo("[ERROR] " + str(string))
        elif (level == APP):
            click.echo("[APP] " + str(string))
        else:
            pass
            
def _print_bytes(string,bytes,level):

    if not level <= PRINT_LEVEL:
        pass
    else:
        if (level == DEBUG):
            click.echo("[DEBUG] " + str(string) + bytes_to_string(bytes))
        elif (level == WARNING):
            click.echo("[WARNING] " + str(string) + bytes_to_string(bytes))
        elif (level == ERROR):
            click.echo("[ERROR] " + str(string) + bytes_to_string(bytes))
        elif (level == APP):
            click.echo("[APP] " + str(string) + bytes_to_string(bytes))
        else:
            pass
            
def bytes_to_string(bytes):
    string = ""
    for byte in bytes:
        string += hex(byte) + " "
    return string