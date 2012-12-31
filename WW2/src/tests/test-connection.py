#! /usr/bin/env python
######################################################################
# Test how many sockets we can open on this system.

import os
from socket import *

sockets = []
accepts = []

s = None
address = ("127.0.0.1", 27010)

######################################################################
# Cleanup.

def cleanup():
	if s: s.close()

######################################################################
# Helper function that reports an error and exits.

def dieWith(msg, e):
	print("ERROR: %s: %s" %( msg, e))
	exit(-1)

######################################################################
# Main code.

# Create a listener socket.
try: s = socket(AF_INET, SOCK_STREAM)
except Exception as e: dieWith("Unable to create socket", e)

# Attempt to connect.
try: s.connect(address)
except Exception as e: dieWith("failed to connect", e)

x = chr(0x99)
s.send(x)

import time
time.sleep(5)
