#  setup_svcagt_services.py

import sys, os, shutil

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

update_test_service_c ()
update_service_file (0)
update_service_file (1)
update_service_file (2)

etc_lib = "mock_systemd_libs/etc"
if os.path.exists (etc_lib):
  copy_files (service_fname(0), etc_lib)
  copy_files (service_fname(1), etc_lib)
  copy_files (service_fname(2), etc_lib)

print
print "To complete the setup, you should:"
print " 1) gcc -o svcagt_test_service -lm svcagt_test_service.c"
print " 2) sudo cp sajwt1.service /etc/systemd/system"
print " 3) sudo cp sajwt2.service /etc/systemd/system"
print " 4) sudo cp sajwt3.service /etc/systemd/system"
print
print "Done"

   
    
