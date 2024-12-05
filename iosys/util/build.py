import subprocess
import os
import sys
import shutil


THISFILE=os.path.realpath(__file__)
THISDIR=os.path.dirname(THISFILE)

def CopyToResources(filename):
	shutil.copyfile(filename,os.path.join("..","..","resources",filename))

def Run(argv):
	os.chdir(THISDIR)

	srcs=[
		"FORCE31K",
	]

	rename=[
		["FORCE31K.bin","FORCE31K.COM"],
	]

	for src in srcs:
		print(src+".NSM")
		proc=subprocess.Popen(["NASM",src+".NSM","-l",src+".LST","-o",src+".bin"])
		proc.communicate()
		if 0!=proc.returncode:
			print("Error building "+src)
			quit(1)

	for ren in rename:
		print("Rename "+ren[0]+" to "+ren[1])
		if os.path.isfile(ren[1]):
			os.remove(ren[1])
		os.rename(ren[0],ren[1])


	CopyToResources("FORCE31K.COM");





if __name__=="__main__":
	Run(sys.argv[1:])
	quit(0)
