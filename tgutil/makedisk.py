import subprocess
import os
import sys
import shutil


THISFILE=os.path.realpath(__file__)
THISDIR=os.path.dirname(THISFILE)

def CopyToResources(filename):
	shutil.copyfile(filename,os.path.join("..","resources",filename))

def PrintOutput():
	fp=open(os.path.join(THISDIR,"OUTPUT.TXT"),"r")
	for line in fp:
		print(line)
	fp.close()

def IsTestFile(fn):
	return fn=="EGB_PAI.BIN" or fn=="EGB_PUT.BIN" or fn.endswith(".EXP") or fn.endswith(".SND") or fn.endswith(".FMB") or fn.endswith(".PMB") or fn.endswith(".SND")

def Run(argv):
	os.chdir(THISDIR)

	if not os.path.isdir("EXP"):
		os.makedirs("EXP")
	if not os.path.isdir("LIB"):
		os.makedirs("LIB")
	if not os.path.isdir("BUILD"):
		os.makedirs("BUILD")

	proc=subprocess.Popen([
		"Tsugaru_Headless",
		os.path.join(THISDIR,"..","CompROM"),
		"-FD0",
		os.path.join(THISDIR,"..","resources","RUNNERFD.bin"),
		"-BOOTKEY",
		"F0",
		"-TGDRV",
		os.path.join(THISDIR,"..","..","HC386ENV"),
		"-TGDRV",
		THISDIR,
		"-FREQ","100",
		"-DEBUG",
		#"-UNITTEST",
		"-DONTUSEFPU",	# Let High-C use no-fpu mode.
		"-VMFLAG", "CONSOUT", # Automatic in Tsugaru_Headless.  Needed if Tsugaru_CUI is used.
		"-conscmd","D:\AUTOEXEC.BAT",
		"-conscmd","E:",
		"-conscmd","TASK.BAT"
	])
	proc.communicate()
	if 0!=proc.returncode:
		print("Error bulding TGBIOS.LIB and UTILS")
		PrintOutput()
		quit()



if __name__=="__main__":
	Run(sys.argv[1:])
