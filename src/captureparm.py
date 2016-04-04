#! /usr/bin/env python

##
# @file captureparm.py
# \brief Python capture parm class for the V4L2 abstraction framework
#
# 2016 Jimmy Durand Wesolowski
#
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.

# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

from pyv4l2 import *
from videotimeperframe import VideoTimePerFrame


class CaptureParm(object):
    capability = 0
    capturemode = 0
    timerperframe = None
    extendedmode = 0
    readbuffer = 0

    CAPTURE_MODE_DEFAULT = 0
    CAPTURE_MODE_HIGH_QUALITY = V4L2_MODE_HIGHQUALITY
    # def __init__(self, capture_mode = None, timeperframe = None,
    #              extendedmode = None, readbuffer = 0):
    #     if timeperframe:
    #         self.capability |= V4L2_CAP_TIMEPERFRAME
    #         self.timeperframe = timeperframe
    #     else:
    #         self.timeperframe = VideoTimePerFrame()
    #         self.capture_mode = capture_mode
    #     self.timeperframe = timerperframe
    #     self.extendedmode = extendedmode
    #     self.readbuffer = readbuffer

    def __init__(self, **kwds):
        self.__dict__.update(kwds)
        if self.timerperframe:
            self.capability |= V4L2_CAP_TIMEPERFRAME
        else:
            self.timeperframe = VideoTimePerFrame()
