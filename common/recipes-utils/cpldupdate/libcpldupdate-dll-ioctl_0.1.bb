# Copyright 2020-present Facebook. All Rights Reserved.
#
# This program file is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program in a file named COPYING; if not, write to the
# Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301 USA

SUMMARY = "CPLD update IOCTL DLL"
DESCRIPTION = "CPLD update IOCTL DLL"
SECTION = "dev"

LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://cpldupdate_dll_ioctl.c;beginline=4;endline=16;md5=da35978751a9d71b73679307c4d296ec"

SRC_URI += "file://lib/ioctl \
           "

S = "${WORKDIR}/lib/ioctl"

inherit cmake

DEPENDS += "libcpldupdate-dll-helper"

# The shared library is used used as package instead of -dev
FILES_SOLIBSDEV = ""
FILES:${PN} += "usr/lib"
