# mock_systemctl.py
#
# Don't try to run this with sudo in background

import os, sys
import subprocess as SUBP

cmd_pipe_name = "/mock_systemctl_cmds.txt"
response_pipe_name = "/mock_systemctl_responses.txt"
msg_file = "/mock_systemctl_msgs.txt"

def remove_pipe (pipe_name):
  try:
    os.remove (pipe_name)
  except StandardError, e:
    pass

#remove_pipe (cmd_pipe_name)
#remove_pipe (response_pipe_name)

cp = None
rp = None

services = {}

test_services = []

def create_pipe (pipe_name):
	try:
	  os.mkfifo (pipe_name)
	except StandardError, e:
	  print "Error creating fifo", pipe_name
	  print e
	  sys.exit(4)

def open_pipe (pipe_name, mode):
	try:
	  return os.open (pipe_name, mode, 0666)
	except StandardError, e:
	  print "Error opening fifo", pipe_name
	  print e
	  return -1

#create_pipe (cmd_pipe_name)
#create_pipe (response_pipe_name)


def test_is_active(svc_name):
  if svc_name in test_services:
    call_name = svc_name + ".service"
    rtn = SUBP.call (["systemctl", "is-active", call_name])
    if rtn == 0:
      active = "1"
    else:
      active = "0"
    services[svc_name] = active
  else:
    active = services.get (svc_name, None)
  if active is None:
    os.write (rp, "0\n")
  else:
    os.write (rp, "%s\n" % active[0])

def start_service (svc_name):
  if svc_name in test_services:
    call_name = svc_name + ".service"
    SUBP.call (["systemctl", "start", call_name])
  services[svc_name] = "1"
  os.write (rp, "1\n")

def stop_service (svc_name):
  if svc_name in test_services:
    call_name = svc_name + ".service"
    SUBP.call (["systemctl", "stop", call_name])
  services[svc_name] = "0"
  os.write (rp, "0\n")

def invalid_command (line):
  print "Invalid command: ", line
  os.write (rp, "?\n")

home_dir = None

for arg in sys.argv[1:]:
  if not home_dir:
    home_dir = arg
    continue
  if arg.endswith (".service"):
    arg = arg[0:-8]
  test_services.append (arg)

if not home_dir:
  print "Expecting home dir as first cmd line argument"
  sys.exit

cmd_pipe_name = home_dir + cmd_pipe_name
response_pipe_name = home_dir + response_pipe_name
msg_file = home_dir + msg_file

msgf = open (msg_file, "w")
msgf.write ("Opening pipes\n")
msgf.close()

cp = open_pipe (cmd_pipe_name, os.O_RDONLY)
if cp < 0:
  sys.exit (4)
rp = open_pipe (response_pipe_name, os.O_WRONLY)
if rp < 0:
  os.close (cp)
  sys.exit (4)

msgf = open (msg_file, "w")
msgf.write ("Opened pipes\n")
msgf.close()

while (True):
  line = os.read (cp, 128)
  if not line:
    continue;
  cmd = line[0]
  svc_name = line[1:]
  pos = svc_name.find (".service")
  if (pos < 0):
    pos = svc_name.find ("\n")
  if pos < 0:
    invalid_command (line)
    continue
  svc_name = svc_name[0:pos]
  if cmd == "?":
    test_is_active (svc_name)
  elif cmd == "1":
    start_service (svc_name)
  elif cmd == "0":
    stop_service (svc_name)
  else:
    invalid_command (line)
#end while
