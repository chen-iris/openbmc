# Copyright 2014-present Facebook. All Rights Reserved.
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

SUMMARY = "DBus Evaluation"
DESCRIPTION = "Evaluate the impact of DBus on CPU, memory, and latency."
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://DBusServer.c;beginline=4;endline=16;md5=da35978751a9d71b73679307c4d296ec"

LOCAL_URI = " \
    file://CMakeLists.txt \
    file://DBusServer.c \
    file://dbus-cputest.sh \
    file://DBusLatencyTest.c \
    file://dbus-latencytest.sh \
    file://DBusServerMemtest.c \
    file://dbus-memtest.sh \
    "

export SINC = "${STAGING_INCDIR}"
export SLIB = "${STAGING_LIBDIR}"

inherit cmake

DEPENDS += " glib-2.0 \
           "
