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
		"HD_MBR",
		"HD_IPL",
		"CD_IPL",
		"IOSYS",
		"INT8EH",
		"INT90H",
		"INT91H",
		"INT93H",
		"INT96H",
		"INT97H",
		"INT9BH",
		"INT9EH",
		"INTAEH",
		"INTAFH",
		"INTFDH",
		"DOSLOAD",
		"MINVCPI",
		"FAKENSDD",
		"SYSXXXX0",
		"RAMDRIVE"
	]

	rename=[
		["MINVCPI.bin","MINVCPI.SYS"],
		["FAKENSDD.bin","FAKENSDD.SYS"],
		["SYSXXXX0.bin","SYSXXXX0.COM"],
		["RAMDRIVE.bin","RAMDRIVE.SYS"],
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

	proc=subprocess.Popen(["cl","assemble.cpp","/EHsc"])
	proc.communicate()
	if 0!=proc.returncode:
		print("Error bulding assemble.exe")
		quit(1)

	CopyToResources("FD_IPL.bin");
	CopyToResources("FAKENSDD.SYS");
	CopyToResources("MINVCPI.SYS");
	CopyToResources("SYSXXXX0.COM");

	subprocess.Popen(["./assemble"])



	proc=subprocess.Popen(["cl","../util/makefd.cpp","../util/dosdisk.cpp","/EHsc"])
	proc.communicate()
	if 0!=proc.returncode:
		print("Error bulding makefd.exe")
		quit()

	subprocess.Popen(["./assemble"]).wait()

	subprocess.Popen(["./makefd",
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

	subprocess.Popen(["./makefd",
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
	quit(0)
