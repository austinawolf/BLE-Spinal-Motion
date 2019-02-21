

class SessionInfo:

    def __init__(self, args):
        self.data_size      = args[0]
        self.motion_mode    = args[1]
        self.motion_rate    = args[2]
        self.compass_rate   = args[3]
        
    def __str__(self):
        return "Motion Mode=" + self.motion_mode \
                + ", Motion Rate=" + self.motion_rate \
                + ", Compass Rate=" + self.compass_rate