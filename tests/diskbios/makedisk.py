import os
import sys
import subprocess

THISFILE=os.path.realpath(__file__)
THISDIR=os.path.dirname(THISFILE)



def SetENV():
	WATCOM=os.environ["WATCOM"]

	if not os.path.isdir(WATCOM):
		print("Set environment variable WATCOM and try again.")
		quit()

	os.environ["PATH"]=os.path.join(WATCOM,"BINNT")+";"+os.environ["PATH"]
	os.environ["INCLUDE"]=os.path.join(WATCOM,"H")
	os.environ["LIB"]=os.path.join(WATCOM,"LIB386","DOS")+";"+os.path.join(WATCOM,"LIB386")
	os.environ["EDPATH"]=os.path.join(WATCOM,"EDDAT")
	os.environ["WIPFC"]=os.path.join(WATCOM,"WIPFC")


def WatcomBuild(SRCS):
	for src in SRCS:
		FN=src.upper()
		EXT=os.path.splitext(FN)[1]
		if EXT==".C" or EXT==".CPP":
			cmd=["wcc","-ms","-3","-os","-s","-bt=DOS",src]
			if EXT==".CPP":
				cmd[0]="wpp"
			# -ms     Small memory model (supposed to be, but no effect)
			# -3      386.  For eliminating absurd 8-bit limit for conditional jumps.  6809 did better than 8086.
			# -os     Optimize for size.  Open Watcom C creates slightly bigger binary if I use this option though.  WTF!
			# -s      Eliminate stack-overflow check.  This eliminates reference to mysterious __STK label.
			# -bt=DOS DOS binary.
			proc=subprocess.Popen(cmd)
			proc.communicate()
			if 0!=proc.returncode:
				quit(1)

	OBJS=""
	for src in SRCS:
		if 0!=len(OBJS):
			OBJS+=","
		FN=src.upper()
		EXT=os.path.splitext(FN)[1]
		if EXT==".C" or EXT==".CPP":
			OBJS+=os.path.splitext(src)[0]+".OBJ"
		else:
			OBJS+=src

	cmd=["wlink",
		"system",  "com",
		"option",  "map",
		"name",    os.path.splitext(SRCS[0])[0]+".COM",
		"file",    OBJS,
		# "option",  "nodefaultlibs",
	]
	proc=subprocess.Popen(cmd)
	proc.communicate()
	if 0!=proc.returncode:
		quit(1)


def main(argv):
	SetENV()
	WatcomBuild(["FDREAD.CPP","DISKBIOS.C"])
	WatcomBuild(["FDADDR.CPP","DISKBIOS.C"])
	WatcomBuild(["SCSITYPE.CPP"])
	WatcomBuild(["CDDAPLAY.CPP"])
	WatcomBuild(["CDPAUSE.CPP"])
	WatcomBuild(["CDRESUME.CPP"])
	WatcomBuild(["CDSTATE.CPP"])
	WatcomBuild(["CDSUBQ.CPP"])



if __name__=="__main__":
	# os.chdir(THISDIR)
	main(sys.argv)
