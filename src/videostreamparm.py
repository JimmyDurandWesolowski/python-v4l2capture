#! /usr/bin/env python

##
# @file videosteamparm.py
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


__all__ = ['VideoStreamParm']


BUF_TYPE_NAME = {
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

class VideoStreamParm(object):
    type = 0
    parm = None

    TYPE_VIDEO_CAPTURE = V4L2_BUF_TYPE_VIDEO_CAPTURE
    TYPE_VIDEO_CAPTURE_MPLANE = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE
    TYPE_VIDEO_OUTPUT = V4L2_BUF_TYPE_VIDEO_OUTPUT
    TYPE_VIDEO_OUTPUT_MPLANE = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE
    TYPE_VIDEO_OVERLAY = V4L2_BUF_TYPE_VIDEO_OVERLAY
    TYPE_VBI_CAPTURE = V4L2_BUF_TYPE_VBI_CAPTURE
    TYPE_VBI_OUTPUT = V4L2_BUF_TYPE_VBI_OUTPUT
    TYPE_SLICED_VBI_CAPTURE = V4L2_BUF_TYPE_SLICED_VBI_CAPTURE
    TYPE_SLICED_VBI_OUTPUT = V4L2_BUF_TYPE_SLICED_VBI_OUTPUT
    TYPE_VIDEO_OUTPUT_OVERLAY = V4L2_BUF_TYPE_VIDEO_OUTPUT_OVERLAY

    def __init__(self, **kwds):
        self.__dict__.update(kwds)

    @staticmethod
    def type_name(value):
        return BUF_TYPE_NAME[value]
