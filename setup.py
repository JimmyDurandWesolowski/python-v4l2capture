#! /usr/bin/python

##
# python-v4l2capture
#
# 2009, 2010, 2011 Fredrik Portstrom
#   http://fredrik.jemla.eu/v4l2capture
# 2016 Jimmy Durand Wesolowski
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

from distutils.core import Extension, setup
from os import path, getenv

setup(
    name = "v4l2capture",
    version = "1.0",
    author = "Jimmy Durand Wesolowski",
    author_email = "jwesolowski@de.adit-jv.com",
    url = "https://github.com/JimmyDurandWesolowski/python-v4l2capture",
    description = "Capture video with video4linux2",
    long_description = "python-v4l2capture is a slim and easy to use Python "
    "extension for capturing video with video4linux2.",
    license = "LGPLv2.1",
    classifiers = [
        "License :: LGPLv2.1",
        "Programming Language :: C"],
    ext_modules = [
        Extension("v4l2capture", [path.join("src", "v4l2capture.c")],
        libraries=["v4l2"], extra_compile_args=['-DUSE_LIBV4L', ],
        )])
