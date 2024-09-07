import subprocess
import os
import sys

THISFILE=os.path.realpath(__file__)
THISDIR=os.path.dirname(THISFILE)

ROMDIR=os.path.join(THISDIR,"..","CompROM")
SRCDIR=os.path.join(THISDIR,"..","src")
FDIMG=os.path.join(SRCDIR,"FDIMG.bin")

proc=subprocess.Popen([
	"Tsugaru_CUI",
	ROMDIR,
	"-USEFPU",
	"-BOOTKEY",
	"F0",
	"-FD0",
	FDIMG,
	"-FD1",
	os.path.join(THISDIR,"..","..","TOWNSEMU_TEST","DISKIMG","TBIOSV1.1L30.d77"),
	"-DEBUG",
	"-INITCMD","ENA FDCMON",
	"-INITCMD","BRKON INT 21 AX=4B03", # Stop at driver installation
	"-SHAREDDIR",os.path.join(THISDIR,"..","tgdrv")
]+sys.argv[1:])

proc.wait()

