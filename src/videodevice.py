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

from pyv4l2 import *
from videoframesizes import VideoFramesizes
from collections import namedtuple


__all__ = ['v4l2_buf_type']

v4l2_buf_type = {
    V4L2_BUF_TYPE_VIDEO_CAPTURE: 'video capture',
    V4L2_BUF_TYPE_VIDEO_OUTPUT: 'video output',
    V4L2_BUF_TYPE_VIDEO_OVERLAY: 'video overlay',
    V4L2_BUF_TYPE_VBI_CAPTURE: 'vbi capture',
    V4L2_BUF_TYPE_VBI_OUTPUT: 'vbi output',
    V4L2_BUF_TYPE_SLICED_VBI_CAPTURE: 'sliced vbi capture',
    V4L2_BUF_TYPE_SLICED_VBI_OUTPUT: 'sliced vbi output',
    V4L2_BUF_TYPE_VIDEO_OUTPUT_OVERLAY: 'video output overlay',
    V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE: 'video capture mplane',
    V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE: 'video output mplane',
}

v4l2_capability = {
    V4L2_CAP_VIDEO_CAPTURE: 'video capture',
    V4L2_CAP_VIDEO_OUTPUT: 'video output',
    V4L2_CAP_VIDEO_OVERLAY: 'video overlay',
    V4L2_CAP_VBI_CAPTURE: 'vbi capture',
    V4L2_CAP_VBI_OUTPUT: 'vbi output',
    V4L2_CAP_SLICED_VBI_CAPTURE: 'sliced_vbi_capture',
    V4L2_CAP_SLICED_VBI_OUTPUT: 'sliced_vbi_output',
    V4L2_CAP_RDS_CAPTURE: 'rds capture',
    V4L2_CAP_VIDEO_OUTPUT_OVERLAY: 'video_output_overlay',
    V4L2_CAP_HW_FREQ_SEEK: 'hw_freq_seek',
    V4L2_CAP_RDS_OUTPUT: 'rds output',
    V4L2_CAP_VIDEO_CAPTURE_MPLANE: 'video_capture_mplane',
    V4L2_CAP_VIDEO_OUTPUT_MPLANE: 'video_output_mplane',
    V4L2_CAP_TUNER: 'tuner',
    V4L2_CAP_AUDIO: 'audio',
    V4L2_CAP_RADIO: 'radio',
    V4L2_CAP_MODULATOR: 'modulator',
    V4L2_CAP_READWRITE: 'readwrite',
    V4L2_CAP_ASYNCIO: 'asyncio',
    V4L2_CAP_STREAMING: 'streaming',
}

V4L2Fract = namedtuple('V4L2Fract', ['numerator', 'denominator'])
v4l2_parm = ['capability', 'outputmode', 'timeperframe', 'extendedmode',
             'writebuffers']
V4L2OutputParm = namedtuple('V4L2OutputParm', v4l2_parm)
V4L2CaptureParm = namedtuple('V4L2CaptureParm', v4l2_parm)
V4L2StreamParm = namedtuple('V4L2StreamParm', ['type', 'parm'])

# v4l_capture_defparm = V4L2StreamParm(
#     type = pyv4l2.v4l2types.index('video_capture'),
#     parm = V4L2CaptureParm(
#         capability = pyv4l2.capture_cap.index(['timeperframe']),
#         outputmode = 
#     )

class Fourcc(object):
    value = 0

    def __init__(self, value):
        assert(isinstance(value, int))
        self.value = value

    def __str__(self):
        ret = ''
        for i in range(0, 32, 8):
            ret += chr((self.value >> i) & 0xFF)
        return ret

VideoFormatBase = namedtuple('VideoFormatBase', ['type', 'fourcc', 'desc'])
class VideoFormat(VideoFormatBase):
    def __str__(self):
        return str(self.fourcc)

class VideoFormats(object):
    types = None
    desc = None
    fmts = None

    def __init__(self, data):
        self.types = {}
        self.fmts = set()
        self.desc = {}

        for fmt in data:
            fourcc = fmt['fourcc']
            ftype = fmt['type']
            video_fmt = VideoFormat(ftype, Fourcc(fourcc), fmt['desc'])
            if ftype not in self.types:
                self.types[ftype] = set()
            self.types[ftype].add(video_fmt)
            self.fmts.add(video_fmt)

    def __str__(self):
        ret = ''
        for ftype in self.types:
            ret += "%s format: " % v4l2_buf_type[ftype]
            fmts = []
            for fmt in self.types[ftype]:
                fmts.append(str(fmt))
            return ret + ", ".join(fmts)

    def __iter__(self):
        return iter(self.fmts)

    def __getitem__(self, key):
        return self.fmts[key]

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
        self.driver, self.card, _, self.caps = self.video_dev.get_info()
        if not devtype & self.caps:
            raise IOError("%s is not a %s capable device" %
                          (path, v4l2_buf_type[devtype]))

        self.formats = VideoFormats(self.video_dev.get_formats())
        self.framesizes = {}
        for fmt in self.formats:
            framesizes = self.video_dev.get_framesizes(fmt.fourcc.value)
            if not len(framesizes):
                continue
            self.framesizes[fmt.fourcc] = VideoFramesizes(fmt.fourcc,
                                                          framesizes)

    def is_capture_device(self):
        return 'video_capture' & self.caps

    def is_supported_format(self, fmt):
        return fmt in self.formats

    def __str__(self):
        desc = "%s %s\n" % (self.path, self.driver)
        cap_list = []
        for cap in v4l2_capability:
            if cap & self.caps:
                cap_list.append(v4l2_capability[cap])
        desc += "  caps: %s\n" % ", ".join(cap_list)
        desc += "  " + str(self.formats) + "\n"
        desc += "  framesizes: %s" % ", ".join([str(self.framesizes[fourcc])
                                                for fourcc in
                                                self.framesizes])
        return desc


class VideoCaptureDevice(VideoDevice):
    def __init__(self, path):
        super(VideoCaptureDevice, self).__init__(V4L2_CAP_VIDEO_CAPTURE, path)
