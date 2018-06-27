#!/usr/bin/env python
#
# Copyright 2015-present Facebook. All Rights Reserved.
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
#


import os
from subprocess import *
from node import node
from pal import *

class serverNode(node):
    def __init__(self, num = None, info = None, actions = None):
        self.num = num

        if info == None:
            self.info = {}
        else:
            self.info = info
        if actions == None:
            self.actions = []
        else:
            self.actions = actions

    def getInformation(self, param={}):
        ret = pal_get_server_power(self.num)
        if ret == 0:
            status = 'power-off'
        elif ret == 1:
            status = 'power-on'
        elif ret == 5:
            status = '12V-off'
        else:
            status = 'error'

        info = { "status": status }

        return info

    def doAction(self, data, is_read_only=True):
        if is_read_only:
            result = { "result": 'failure' }
        else:
            ret = pal_server_action(self.num, data["action"])
            if ret == -2:
                res = 'Should not execute power on/off/graceful_shutdown/cycle/reset on device card'
                result = { "Warning": res }
                return result
            elif ret == -1:
                res = 'failure'
            else:
                res = 'success'

            result = { "result": res }

        return result

def get_node_server(num, is_read_only=True):
    if is_read_only:
        actions =  []
    else:
        actions =  ["power-on",
                    "power-off",
                    "power-reset",
                    "power-cycle",
                    "graceful-shutdown",
                    "12V-on",
                    "12V-off",
                    "12V-cycle",
                    "identify-on",
                    "identify-off",
                    ]
    return serverNode(num = num, actions = actions)

def get_node_device(num, is_read_only=True):
    if is_read_only:
        actions =  []
    else:
        actions =  ["12V-on",
                    "12V-off",
                    "12V-cycle",
                    "identify-on",
                    "identify-off",
                    ]
    return serverNode(num = num, actions = actions)
