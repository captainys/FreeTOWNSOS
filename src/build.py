import subprocess
import os
import sys


THISFILE=os.path.realpath(__file__)
THISDIR=os.path.dirname(THISFILE)


def Run(argv):
	os.chdir(THISDIR)

	srcs=[
		"IPL",
		"IOSYS",
		"CONDEV",
		"CLOCKDEV",
		"DUMMYDEV",
		"INT8EH",
		"INT90H",
		"INT91H",
		"INT93H",
		"INT96H",
		"INT9BH",
		"INTAEH",
		"INTAFH",
	]

	for src in srcs:
		print(src+".NSM")
		proc=subprocess.Popen(["NASM",src+".NSM","-l",src+".LST","-o",src+".bin"])
		proc.communicate()
		if 0!=proc.returncode:
			print("Error building "+src)
			quit()

	proc=subprocess.Popen(["cl","assemble.cpp","/EHsc"])
	proc.communicate()
	if 0!=proc.returncode:
		print("Error bulding assemble.exe")
		quit()

	subprocess.Popen(["./assemble"])



if __name__=="__main__":
	Run(sys.argv[1:])
