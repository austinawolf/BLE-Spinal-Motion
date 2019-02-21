import click
import sys

from logger import _print, APP, ERROR, WARNING, DEBUG

class MyError(Exception):
    """ Base Class """
    pass
    
    def __str__(self):
        return self.__class__.__name__
    
class PreambleErr(MyError):
    pass
    
class ArgLenErr(MyError):
    pass

class TrailerErr(MyError):
    pass

class DataFlagErr(MyError):
    pass

class TimeoutErr(MyError):
    pass

class DataParseErr(MyError):
    pass

class DataLenErr(MyError):
    pass