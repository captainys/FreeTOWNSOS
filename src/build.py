import subprocess
import os


THISFILE=os.path.realpath(__file__)
THISDIR=os.path.dirname(THISFILE)


proc=subprocess.Popen(["NASM","IPL.NSM","-l","IPL.LST"])
proc.communicate()
if 0!=proc.returncode:
	print("Error building IPL")
	quit()



proc=subprocess.Popen(["NASM","IOSYS.NSM","-l","IOSYS.LST"])
proc.communicate()
if 0!=proc.returncode:
	print("Error building IO.SYS")
	quit()



proc=subprocess.Popen(["NASM","CONDEV.NSM","-l","CONDEV.LST"])
proc.communicate()
if 0!=proc.returncode:
	print("Error building CONDEV")
	quit()

proc=subprocess.Popen(["NASM","CLOCKDEV.NSM","-l","CLOCKDEV.LST"])
proc.communicate()
if 0!=proc.returncode:
	print("Error building CLOCKDEV")
	quit()



proc=subprocess.Popen(["cl","assemble.cpp","/EHsc"])
proc.communicate()
if 0!=proc.returncode:
	print("Error bulding assemble.exe")
	quit()



subprocess.Popen(["./assemble"])
