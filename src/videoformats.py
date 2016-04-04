#! /usr/bin/env python

##
# @file videoformat.py
# \brief Python V4L2 video format(s) classes
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

from collections import namedtuple
from videostreamparm import VideoStreamParm


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
            ret += "%s format: " % VideoStreamParm.type_name(ftype)
            fmts = []
            for fmt in self.types[ftype]:
                fmts.append(str(fmt))
            return ret + ", ".join(fmts)

    def __iter__(self):
        return iter(self.fmts)

    def __getitem__(self, key):
        return self.fmts[key]
