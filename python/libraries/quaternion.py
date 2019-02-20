import math
from vector import Vector


class Quaternion(object):


    def __init__(self, q0, q1, q2, q3):
		super(Quaternion, self).__init__()
		magnitude = math.sqrt(q0 ** 2 + q1 ** 2 + q2 ** 2 + q3 ** 2)
		self.q0 = float(q0) / magnitude
		self.q1 = float(q1) / magnitude
		self.q2 = float(q2) / magnitude
		self.q3 = float(q3) / magnitude
	
    def magnitude(self):
        """
        Returns the magnitude of the quaternion.
        """

        return math.sqrt(self.q0 ** 2 + self.q1 ** 2 + self.q2 ** 2 + self.q3 ** 2)

    @property
    def roll(self):
        """ Calculates the Roll of the Quaternion. """
        return math.atan2(2*(self.q0*self.q1 + self.q2*self.q3), 1 - 2 * (self.q1*self.q1 + self.q2*self.q2)) * 57.29

    @property
    def pitch(self):
        """ Calculates the Pitch of the Quaternion. """
        return math.asin(2*(self.q0*self.q2 - self.q3*self.q1)) * 57.29

    @property
    def yaw(self):
        """ Calculates the Yaw of the Quaternion. """
        return math.atan2(2*(self.q0*self.q3 + self.q1*self.q2), 1 - 2 * (self.q2*self.q2 + self.q3*self.q3)) * 57.29

