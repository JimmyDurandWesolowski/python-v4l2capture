#! /usr/bin/env python

##
# @file videodevice
# \brief Python v4l2 device class
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

from pyv4l2 import V4L2VideoDevice
from collections import namedtuple


VideoFormat = namedtuple('VideoFormat', 'type fourcc desc')

class VideoFrame:
    def __init__(self, data):
        print(data)
        assert(data is list)
        if 1 == len(data):
            print("Discrete")
        else:
            print("Stepwise")


class VideoDevice:
    video_dev = None
    path = None
    driver = None
    card = None
    caps = None
    formats = None
    framesize = None

    def __init__(self, path):
        self.path = path
        self.video_dev = V4L2VideoDevice(path)
        self.video_dev.open()
        self.driver, self.card, _, self.caps = self.video_dev.get_info()
        self.formats = [VideoFormat(**fmt) for fmt in \
                        self.video_dev.get_formats()]
        self.framesize = {}
        for fmt in self.formats:
            fourcc = fmt.fourcc
            data = self.video_dev.get_framesizes(fourcc)
            print data
            self.framesize[fourcc] = data

    def is_capture_device(self):
        return 'video_capture' in self.caps

    def is_supported_format(self, fmt):
        return fmt in self.formats

    def __str__(self):
        desc = "%s %s\n" % (self.path, self.driver)
        desc += "  caps: " + ", ".join(self.caps) + "\n"
        for fmt in self.formats:
            desc += "  format: %s, %s (%s)\n" % (fmt.type, fmt.fourcc,
                                                 fmt.desc)
        desc += str(self.framesize)
        return desc
