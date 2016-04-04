#! /usr/bin/env python

##
# @file videodevice.py
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
from videoframesizes import VideoFramesizes
from videotimeperframe import VideoTimePerFrame
from videostreamparm import VideoStreamParm
from videocapabilities import VideoCapabilities
from videoformats import VideoFormats
from captureparm import CaptureParm



capture_defparm = VideoStreamParm(
    type = VideoStreamParm.TYPE_VIDEO_CAPTURE,
    parm = CaptureParm(capture_mode = CaptureParm.CAPTURE_MODE_DEFAULT,
                       timeperframe = VideoTimePerFrame(1, 30))
)

class VideoDevice(object):
    video_dev = None
    path = None
    driver = None
    card = None
    caps = None
    formats = None
    framesizes = None

    def __init__(self, devtype, path):
        self.path = path
        self.video_dev = V4L2VideoDevice(devtype, path)
        self.video_dev.open()
        self.driver, self.card, self.bus_info, caps = self.video_dev.get_info()
        self.caps = VideoCapabilities(caps)
        if not self.caps.compatible(devtype):
            raise IOError("%s is not a %s capable device" %
                          (path, VideoCapabilities.name(devtype)))

        self.formats = VideoFormats(self.video_dev.get_formats())
        self.framesizes = {}
        for fmt in self.formats:
            framesizes = self.video_dev.get_framesizes(fmt.fourcc.value)
            if not len(framesizes):
                continue
            self.framesizes[fmt.fourcc] = VideoFramesizes(fmt.fourcc,
                                                          framesizes)

    def is_capture_device(self):
        return self.caps.compatible()

    def is_supported_format(self, fmt):
        return fmt in self.formats

    def __str__(self):
        desc = "%s %s\n" % (self.path, self.driver)
        desc += "  caps: %s\n" % self.caps
        desc += "  %s\n" % self.formats
        desc += "  framesizes %s" % ", ".join([str(self.framesizes[fourcc])
                                               for fourcc in
                                               self.framesizes])
        return desc


class VideoCaptureDevice(VideoDevice):
    def __init__(self, path):
        super(VideoCaptureDevice,
              self).__init__(VideoCapabilities.VIDEO_CAPTURE, path)
