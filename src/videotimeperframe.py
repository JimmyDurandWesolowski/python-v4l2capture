#! /usr/bin/env python

##
# @file videotimeperframe.py
# \brief Python time per frame class for the V4L2 abstraction framework
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

class VideoTimePerFrame(object):
    numerator = 0
    denominator = 0

    def __init__(self, numerator = 1, denominator = 30):
        self.numerator = numerator
        self.denominator = denominator
