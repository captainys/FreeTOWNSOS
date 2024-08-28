import subprocess
import os

THISFILE=os.path.realpath(__file__)
THISDIR=os.path.dirname(THISFILE)

ROMDIR=os.path.join(THISDIR,"..","CompROM")
SRCDIR=os.path.join(THISDIR,"..","src")
HDIMG=os.path.join(SRCDIR,"HDIMG.h3")

proc=subprocess.Popen([
	"Tsugaru_CUI",
	ROMDIR,
	"-BOOTKEY",
	"H3",
	"-HD3",
	HDIMG,
	"-DEBUG"
])

proc.wait()

