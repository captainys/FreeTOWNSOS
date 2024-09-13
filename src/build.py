import subprocess
import os
import sys
import shutil


THISFILE=os.path.realpath(__file__)
THISDIR=os.path.dirname(THISFILE)

def CopyToResources(filename):
	shutil.copyfile(filename,os.path.join("..","resources",filename))

def Run(argv):
	os.chdir(THISDIR)

	srcs=[
		"FD_IPL",
		"IOSYS",
		"INT8EH",
		"INT90H",
		"INT91H",
		"INT93H",
		"INT96H",
		"INT97H",
		"INT9BH",
		"INTAEH",
		"INTAFH",
		"INTFDH",
		"DOSLOAD",
		"MINVCPI",
	]

	rename=[
		["MINVCPI.bin","MINVCPI.SYS"]
	]

	for src in srcs:
		print(src+".NSM")
		proc=subprocess.Popen(["NASM",src+".NSM","-l",src+".LST","-o",src+".bin"])
		proc.communicate()
		if 0!=proc.returncode:
			print("Error building "+src)
			quit()

	for ren in rename:
		print("Rename "+ren[0]+" to "+ren[1])
		if os.path.isfile(ren[1]):
			os.remove(ren[1])
		os.rename(ren[0],ren[1])

	proc=subprocess.Popen(["cl","assemble.cpp","/EHsc"])
	proc.communicate()
	if 0!=proc.returncode:
		print("Error bulding assemble.exe")
		quit()

	CopyToResources("FD_IPL.bin");

	subprocess.Popen(["./assemble"])



	proc=subprocess.Popen(["cl","../util/dosdisk.cpp","/EHsc"])
	proc.communicate()
	if 0!=proc.returncode:
		print("Error bulding dosdisk.exe")
		quit()

	subprocess.Popen(["./assemble"]).wait()

	subprocess.Popen(["./dosdisk",
		"-o",		"FDIMG.bin",
		"-ipl",		"FD_IPL.bin",
		"-i",		"../resources/IO.SYS",
		"-i",		"../resources/YSDOS.SYS",
		"-i",		"../resources/YAMAND.COM",
		"-i",		"../resources/CONFIG.SYS",
		"-i",		"../resources/AUTOEXEC.BAT",
		"-i",		"../resources/TGDRV.COM",
		"-i",		"../resources/TEST.EXP",
		"-i",		"../src/MINVCPI.SYS",
		"-i",		"../externals/ORICON/ORICON.COM",
		"-i",		"../externals/Free386/free386.com",
	]).wait()

	subprocess.Popen(["./dosdisk",
		"-o",		"FDIMG_USEROM.bin",
		"-ipl",		"FD_IPL.bin",
		"-i",		"../resources/IO.SYS",
		"-i",		"../resources/CONFIG.SYS",
		"-i",		"../resources/AUTOEXEC.BAT",
		"-i",		"../resources/TGDRV.COM",
		"-i",		"../resources/TEST.EXP",
		"-i",		"../src/MINVCPI.SYS",
		"-i",		"../externals/ORICON/ORICON.COM",
		"-i",		"../externals/Free386/free386.com",
	]).wait()


if __name__=="__main__":
	Run(sys.argv[1:])
