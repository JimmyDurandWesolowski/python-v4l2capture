#! /usr/bin/env python

##
# @file videocapabilities.py
# \brief Python V4L2 capability class
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

__all__ = ['VideoCapabilities']

class VideoCapabilities(object):
    VIDEO_CAPTURE = V4L2_CAP_VIDEO_CAPTURE
    VIDEO_OUTPUT = V4L2_CAP_VIDEO_OUTPUT
    VIDEO_OVERLAY = V4L2_CAP_VIDEO_OVERLAY
    VBI_CAPTURE = V4L2_CAP_VBI_CAPTURE
    VBI_OUTPUT = V4L2_CAP_VBI_OUTPUT
    SLICED_VBI_CAPTURE = V4L2_CAP_SLICED_VBI_CAPTURE
    SLICED_VBI_OUTPUT = V4L2_CAP_SLICED_VBI_OUTPUT
    RDS_CAPTURE = V4L2_CAP_RDS_CAPTURE
    VIDEO_OUTPUT_OVERLAY = V4L2_CAP_VIDEO_OUTPUT_OVERLAY
    HW_FREQ_SEEK = V4L2_CAP_HW_FREQ_SEEK
    RDS_OUTPUT = V4L2_CAP_RDS_OUTPUT
    VIDEO_CAPTURE_MPLANE = V4L2_CAP_VIDEO_CAPTURE_MPLANE
    VIDEO_OUTPUT_MPLANE = V4L2_CAP_VIDEO_OUTPUT_MPLANE
    TUNER = V4L2_CAP_TUNER
    AUDIO = V4L2_CAP_AUDIO
    RADIO = V4L2_CAP_RADIO
    MODULATOR = V4L2_CAP_MODULATOR
    READWRITE = V4L2_CAP_READWRITE
    ASYNCIO = V4L2_CAP_ASYNCIO
    STREAMING = V4L2_CAP_STREAMING

    CAPABILITY_NAME = {
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

    capabilities = 0

    def __init__(self, data):
        self.capabilities = data

    def compatible(self, caps):
        return self.capabilities & caps

    def __str__(self):
        cap_list = []
        for cap in self.CAPABILITY_NAME:
            if cap & self.capabilities:
                cap_list.append(self.CAPABILITY_NAME[cap])
        return ", ".join(cap_list)

    @classmethod
    def name(cls, type):
        return cls.CAPABILITY_NAME[type]
