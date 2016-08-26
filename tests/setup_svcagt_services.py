#  setup_svcagt_services.py  
#
#  1. Updates svcagt_test_service.c so that it uses the 
#     current user's home directory
#  2. Updates the three test service files so that they
#     point into the current directory
#  3. Compiles mock_systemctl.c
#  4. Compiles svcagt_test_service.c
#  5. Installs the three test service files in /etc/systemd/system
#
#

import sys, os, shutil, subprocess

#--------------------------------------------------------------
def copy_files (a, b):
  try:
    shutil.copy2 (a, b)
    print "Copied %s to %s" % (a, b)
    return True
  except StandardError, e:
    print "Cannot copy %s to %s\n%s" % (a, b, e)
    return False
#end def

home_dir = os.environ.get ("HOME", "")
if not home_dir:
  print "Cannot get symbol $HOME"
  sys.exit (4)
home_dir = home_dir + '/'

current_dir = os.path.abspath ('.')

test_service_c_fname = "svcagt_test_service.c"

def service_fname(num):
  return "sajwt%d.service" % (num+1)

required_files = [test_service_c_fname,
  service_fname(0), service_fname(1), service_fname(2)]

print "You should be in the tests directory to run this."
print "Files", required_files, "will be modified"
yn = raw_input ("Proceed? (Y/N): ")
if (yn == "y") or (yn == "Y"):
  pass
else:
  sys.exit(0)

for fname in required_files:
  if os.path.exists (fname):
    continue
  print "Required file", fname, "not found"
  sys.exit(4)

for fname in required_files:
  copy_files (fname, fname+".tmp")

def update_test_service_c ():
# Updates the test service program, which will run as root,
# so that it knows the current user's home directory.
  fin = None
  fout = None
  tmp_name = test_service_c_fname + ".tmp"
  try:
    fin = open (tmp_name)
  except StandardError, e:
    print "Unable to open %s" % tmp_name
    print e
    sys.exit(4)

  try:
    fout = open (test_service_c_fname, "w")
  except  StandardError, e:
    print "Unable to open %s" % test_service_c_fname
    print e
    fin.close()
    sys.exit(4)

  count = 0
  found = 0
  for line in fin:
    count += 1
    pos1 = line.find ("#define HOME_DIR")
    if (pos1 >= 0):
      found += 1
      if found == 1:
        fout.write ("#define HOME_DIR \"%s\"\n" % home_dir)
        continue
    fout.write (line)
  #end for

  fout.close()
  fin.close()
  if found != 1:
    print "%d #define HOME_DIR lines found" % found
  print "%d lines written to file %s" % (count, test_service_c_fname)
#end def update_test_service_c

def update_service_file (index):
# updates the service files, which will be executed in root, so that
# they point into the current directory.
  fin = None
  fout = None
  tmp_name = service_fname(index) + ".tmp"
  try:
    fin = open (tmp_name)
  except StandardError, e:
    print "Unable to open %s" % tmp_name
    print e
    sys.exit(4)

  try:
    fout = open (service_fname(index), "w")
  except  StandardError, e:
    print "Unable to open %s" % service_fname(index)
    print e
    fin.close()
    sys.exit(4)

  count = 0
  found = 0
  for line in fin:
    count += 1
    if line.startswith ("ExecStart="):
      found += 1
      if found == 1:
        fout.write ("ExecStart=%s/svcagt_test_service %d\n" \
          % (current_dir, (index+1)) )
        continue
    fout.write (line)
  #end for

  fout.close()
  fin.close()
  if found != 1:
    print "%d ExecStart lines found" % found
  print "%d lines written to file %s" % (count, service_fname(index))
#end def update_service_file

def create_install_file ():
# creates an install shell script, which will be executed as root,
# and must copy the service files from the current directory
# into /etc/systemd/system
  fout = None
  fname = "install_test_services.sh"
  try:
    fout = open (fname, "w")
  except  StandardError, e:
    print "Unable to open %s" % fname
    print e
    sys.exit(4)

  def write_line(index):
    fout.write ("cp -v %s/%s /etc/systemd/system\n" \
      % (current_dir, service_fname(index)) )

  write_line (0)
  write_line (1)
  write_line (2)

  fout.close()  
#end def create_install_file


update_test_service_c ()
update_service_file (0)
update_service_file (1)
update_service_file (2)
create_install_file ()

mock_etc_lib = "mock_systemd_libs/etc"
if os.path.exists (mock_etc_lib):
  copy_files (service_fname(0), mock_etc_lib)
  copy_files (service_fname(1), mock_etc_lib)
  copy_files (service_fname(2), mock_etc_lib)

def gcc (name):
  src_name = name + ".c"
  cmd = ["gcc", "-o", name, "-lm", src_name]
  print "Compiling", src_name
  rtn = subprocess.call (cmd)
  if rtn != 0:
    print "Error on compile of", name
  return rtn

rtn = gcc ("mock_systemctl")
if rtn != 0:
  sys.exit(4)

rtn = gcc ("svcagt_test_service")
if rtn != 0:
  sys.exit(4)

rtn = subprocess.call (["sudo", "bash", "install_test_services.sh"])
if rtn != 0:
  print "sudo install_test_services failed"
  sys.exit(4)

print
print "Done"

   
    
