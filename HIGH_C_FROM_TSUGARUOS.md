# 津軽OSを使ってHigh-Cをホストから使う
# Compiling with High-C from Host using Tsugaru OS

FM TOWNS用アプリを今から開発しよう、などということを考えているとすると、多分プログラミング言語のチョイスはHigh-Cになります。今でもヤフオクにときどき出てくるので、入手可能と思います。

ここでは、CD-ROM版、High-C Multimedia Kit V1.7 L12を使って、FM TOWNSエミュレータ津軽CUIモード、津軽OS、互換ROMでホストOS上のCソースをコマンドでタイプしてコンパイルする方法を説明します。

If you are willing to write an app for FM TOWNS (which is great), probably your first choice of the programming language is High-C compiler.  We see High-C compiler on Yahoo! Auction from time to time.  It is not too difficult to acquire a physical media.

This document explains how you can compile a C program in the host OS by typing a command.




## 必要なファイル
## Necessary Files

High-Cの設定や実行のために、FM TOWNSエミュレータ「津軽」が必要なので、ダウンロードして、Tsugaru_CUI.EXEにパスを通しておいてください。また、ホストOS上で、このレポジトリが、``C:\FreeTOWNSOS``、津軽用互換ROMが ``C:\FreeTOWNSOS\CompROM`` にある前提で以下の説明を進めるので、それぞれ環境に応じて置き替えて読んでください。

FM TOWNS Emulator "Tsugaru" is required.  Please download and put Tsugaru_CUI.EXE in a directory where PATH is pointing.  Also I assume that this repository is in ``C:\FreeTOWNSOS`` and Compatible ROM for Tsugaru is in ``C:\FreeTOWNSOS\CompROM`` in the host OS.  If you keep them in different directories, please replace commands etc. according to your environment.



## 準備
## Preparation

まず、High-C Compiler Multimedia Kit CDから、HC386をコピーします。とりあえず、Cドライブに``TOWNSDEV``ディレクトリを作ってそこにコピーしたとしましょう。このディレクトリは、津軽OSでコンパイル時にDドライブになります。違うところにコピーしたら以下は適宜置き替えて読んでください。

次に、``C:\TOWNSDEV\HC386\BIN\HC386.CNF``をテキストエディタで開きます。そして、``NATIVERUN=Q:\RUN386.EXE -nocrt``
の行を以下のように書き替えます。

```
NATIVERUN=A:\FREE386.COM
```

次に同じディレクトリの``HC386SET.CNF``を開いて、文字列変換で ``q:``を``d:``に変更します。

そして、``C:\TOWNSDEV\TASK.BAT``を新規作成して、以下の内容をコピペしてください。

```
SET SHELL=A:\YAMAND.COM
SET COMSPEC=A:\YAMAND.COM

SET PATH=D:\HC386\BIN;D:\;D:\EXE;D:\BAT;A:\
SET HCDIR=D:\HC386
SET IPATH=D:\HC386\INC;D:\HC386\TOWNSLIB\INCLUDE;D:\HC386\EXLIB1\INCLUDE;D:\HC386\EXLIB2\INCLUDE
SET LPATH=D:\HC386\SMALL;D:\HC386\TOWNSLIB\LIB;D:\HC386\EXLIB1\LIB;D:\HC386\EXLIB2\LIB
SET PATH=%PATH%;D:\HC386\BIN;D:\386ASM;D:\BIN
SET TMP=D:\TMP

E:
A:\YAMAND.COM /C E:\TASK.BAT > OUTPUT.TXT
```
最後の``OUTPUT.TXT``というファイルは、ホストOS上では``C:\TOWNSSRC\OUTPUT.TXT``としてアクセスできて、コンパイラのエラーなどはここに出力されます。

The last ``OUTPUT.TXT`` is ``C:\TOWNSSRC\OUTPUT.TXT`` on the host OS, and you can get compiler error message etc. in this file.

ここでいったん津軽を起動してHigh-Cの設定を完了させます。以下のコマンドで津軽を起動してください。

Then start Tsugaru to finish High-C settings.  STart Tsugaru with the following command.

```
Tsugaru_CUI C:\FreeTOWNSOS\CompROM -FD0 C:\FreeTOWNSOS\release\FDIMG.BIN -SHAREDDIR C:\TOWNSDEV -MEMSIZE 64
```

コマンドプロンプトが出たら、以下のコマンドをタイプします。

Once the command prompt opens, type the following commands.

```
D:
CD \HC386\BIN
AUTOCFIG HCD386.EXE
AUTOCFIG HCD386P.EXP
```



ソースファイルは、``C:\TOWNSSRC``の``MAIN.C``としましょう。なお、VM上の津軽OSでコンパイルする都合で、ファイル名は8文字＋3文字以内、Windows以外のOSがホストの場合はファイル名はすべて大文字にしてください。``C:\TOWNSSRC``はホストVM上でコンパイル時はEドライブになります。

Let's say you want to compile a C program called ``C:\TOWNSSRC\MAIN.C``.  By the way, the file needs to be accessible from the VM.  The file name is limited to 8+3 MS-DOS format.  Long file name is not supported.  If your host is not Windows, the file name must be all capital letters.  The directory ``C:\TOWNSSRC`` will be E drive in the VM.


``C:\TOWNSSRC\TASK.BAT``というファイルを作って、以下の内容をコピペしてください。

Make a file called ``C:\TOWNSSRC\TASK.BAT`` and copy the following two lines.

```
HC386 main.c
SUCCESS
```

とりあえず、``MAIN.C``の内容は、以下のようなものを用意してください。

``MAIN.C`` can be anything, but something like this:

```
#include <stdio.h>
int main(void)
{
    printf("First TOWNS App.\n");
    return 0;
}
```



## コンパイル
## Compile

コンパイルするには、ホスト上で次のコマンドをタイプします。

Type the following command to compile.

```
Tsugaru_CUI C:\FreeTOWNSOS\CompROM -FD0 C:\FreeTOWNSOS\release\RUNNERFD.BIN -SHAREDDIR C:\TOWNSDEV -SHAREDDIR C:\TOWNSSRC -MEMSIZE 64 -UNITTEST
```

最後の-UNITTESTオプションをつけておくと、VM上でSUCCESS.EXEを実行した時点でVMを自動終了します。

The last -UNITTEST option will let Tsugaru close when SUCCESS.EXE is called in the VM.

エラーなど、コンパイラの出力は、``C:\TOWNSSRC\OUTPUT.TXT``に、実行ファイルは、``C:\TOWNSSRC\MAIN.EXP``になります。

Errors etc. are written in ``C:\TOWNSSRC\OUTPUT.TXT``.  The executable file is ``C:\TOWNSSRC\MAIN.EXP``.




## コンパイラとリンカを別に呼ぶ
## Using Compiler and Linker Separately

コンパイラとリンカを別々に呼び出すこともできます。例えば、MAKEでビルドしようと思ったらコンパイラとリンカを別に使う方法も知っておく必要があります。``C:\TOWNSSRC\TASK.BAT``を次のように書き替えます。

You can also use compiler and linker separately.  Edit ``C:\TOWNSSRC\TASK.BAT`` as follows.

```
HCD386 main.c -fsoft
TLINK main.c -lib D:\HC386\SMALL\HCE.LIB
SUCCESS
```

-fsoftオプションは、浮動小数点演算を浮動小数点演算コプロセッサ80387/80487を使わずソフトウェアエミュレーションするオプションです。これをつけないで、FPU有り環境でコンパイルすると、486DX以降専用または浮動小数点コプロセッサ必須のバイナリになってしまいます。

``D:\HC386\SMALL\HCE.LIB``は、C標準ライブラリで、浮動小数点演算コプロセッサ専用バイナリを作るときは、``HCE.LIB``の代わりに``HCC.LIB``としますが、それ以外のTOWNSでも動作するアプリを作るときは``HCE.LIB``を使います。

-fsoft option let High-C output binary that uses software floating-point calculation.  If without -fsoft, and if FPU is enabled in the VM, High-C will generate an executable that requires floating-point co-processor.  Majority of the TOWNS models do not have a floating-point co-processor.

``D:\HC386\SMALL\HCE.LIB`` is a C runtime library.  If you want an executable that uses hardware floating point calculation (by the co-processor), replace ``HCE.LIB`` with ``HCC.LIB``.




## Towns OS用アプリのコンパイル
## Compiling Apps for Towns OS

せっかくFM TOWNS用アプリを開発するなら、デモを書きましょう。そして、デモパーティーに投稿しましょう。そのためには、Towns OS用のアプリを開発したいですね。ここでは、試しに320x240, 32K色モードに設定して、画面に格子模様を描いて、パッドのAボタンを押すまで待つプログラムを書いてみましょう。

なお、その他の機能は、High-C Multimedia Kit CDの``MANUAL/CL21RAD.DOC``, ``MANUAL/CL21R.DOC``に書いてあります。ドキュメントが無いってろくに調べずに言ってる人がいますが、ここにすべての説明があります。わかりやすいかはまた別の問題ですが。あと、チュートリアル的なものは無いですね。

So you must be ambitious.  Let's write a DEMO and submit to a DEMO Party!  To do so, you may want to write an app for Towns OS.  Here let's make a program that set CRTC to 320x240 32K color mode, draw a lattice, and wait for pad A button.

By the way, all the functions are explained in ``MANUAL/CL21RAD.DOC``, ``MANUAL/CL21R.DOC``.  Despite their extension, these are plain text.  You should be able to get good information from automatic translation.  I see some complaining there's no documentation.  But, the function references are there.  Easy or difficult to understand is a different problem though.  Also there is no tutorial.

First, the program looks like:

```
#include "egb.h"
#include "snd.h"

static char EGB_work[EgbWorkSize];

#define EGB_FOREGROUND_COLOR 0
#define EGB_BACKGROUND_COLOR 1
#define EGB_FILL_COLOR 2
#define EGB_TRANSPARENT_COLOR 3

#define EGB_DISP_START_ORIGIN  0
#define EGB_DISP_START_SCROLL  1
#define EGB_DISP_START_ZOOM  2
#define EGB_DISP_START_SIZE  3

#define EGB_PSET 0
#define EGB_PRESET 1
#define EGB_OR 2
#define EGB_AND 3
#define EGB_XOR 4
#define EGB_NOT 5
#define EGB_MATTE 6
#define EGB_PASTEL 7
#define EGB_OPAGUE 9
#define EGB_MASKSET 13
#define EGB_MASKRESET 14
#define EGB_MASKNOT 15

void WaitForPad(void)
{
	int status=0xFF;
	while(0x30==(status&0x30)) // Wait until one of the buttons are pressed (active-low)
	{
		SND_joy_in_2(0,&status);
	}
	while(0x30!=(status&0x30)) // Wait until both buttons are released.
	{
		SND_joy_in_2(0,&status);
	}
}

void DrawLattice(void)
{
	int i;
	short line[5];

	EGB_color(EGB_work,EGB_FOREGROUND_COLOR,0x1F); // Blue (GGGGGRRRRRBBBBB=000000000011111)
	EGB_writeMode(EGB_work,EGB_PSET);
	for(i=0; i<=320; i+=20)
	{
		line[0]=2;		// 2 points
		line[1]=i;		// x0
		line[2]=0;		// y0
		line[3]=i;		// x1
		line[4]=239;	// y1
		EGB_connect(EGB_work,line);
	}
	for(i=0; i<=240; i+=20)
	{
		line[0]=2;		// 2 points
		line[1]=0;		// x0
		line[2]=i;		// y0
		line[3]=319;	// x1
		line[4]=i;		// y1
		EGB_connect(EGB_work,line);
	}
}


int main(void)
{
	EGB_init(EGB_work,EgbWorkSize);

	EGB_resolution(EGB_work,0,10); // Set page 0 screen mode 10, 320x240 32K Color Mode
	EGB_resolution(EGB_work,1,10); // Set page 1 screen mode 10, 320x240 32K Color Mode

	EGB_writePage(EGB_work,1);
	EGB_clearScreen(EGB_work);

	EGB_writePage(EGB_work,0);
	EGB_clearScreen(EGB_work);

	EGB_displayStart(EGB_work,EGB_DISP_START_ZOOM,2,2);		// Set 2x2 Zoom for the write-page (page 0)
	EGB_displayStart(EGB_work,EGB_DISP_START_SIZE,320,240);	// Set 320x240 visible area for the write-page (page 0)

	DrawLattice();

	WaitForPad();

	return 0;
}
```

これを、``C:\TOWNSSRC\TBIOS.C``としてセーブしましょう。ファイル名はすべて大文字で、8文字＋3文字範囲内に収まるようにしてください。

Let's save as ``C:\TOWNSSRC\TBIOS.C``.  Make sure the file name is all in capital.

そして、次に、``C:\TOWNSSRC\TBIOS.LN``の中にリンクするライブラリを記述します。これは初期のMS-DOSは、ゴミ仕様でコマンドラインが127文字（多分）までという制限があります。80文字だったっけかな? それだとそのうちオブジェクトファイルやライブラリの数が増えてくるとコマンドに書ききれないので、代わりにTLINKがファイルからオプションを読む機能があります。

Then, let's make a file called ``C:\TOWNSSRC\TBIOS.LN``.  We use this file because earlier MS-DOS could take only up to 127 bytes command line.  But, you eventually may need to link a lot more libraries.  Instead of supplying everything in the command line, you can give parameters to the linker by a file.

```
-lib
D:\hc386\small\hce.lib
D:\hc386\townslib\lib\tbios.lib
D:\hc386\townslib\lib\snd.lib
```

そして、``C:\TOWNSSRC\TASK.BAT``を次のように変更します。

Then modify ``C:\TOWNSSRC\TASK.BAT`` as follows:

```
HCD386 tbios.c -fsoft
TLINK tbios.obj @TBIOS.LN
SUCCESS
```
TLINKの``@TBIOS.LN``は、残りのコマンドオプションを指定したファイルから読むという意味です。

TLINK option ``@TBIOS.LN`` tells TLINK to read command-line options from the specified file.

このTASK.BATでは、まずHCD386が``TBIOS.C``をコンパイルして、``TBIOS.OBJ``を作ります。その後TLINKは``TBIOS.OBJ``と``TBIOS.LN``の中に記述したライブラリをリンクして``TBIOS.EXP``を作成します。

In this batch script, first HCD386 compiles ``TBIOS.C`` (remember the VM does not distinguish capital and small), and generates ``TBIOS.OBJ``.  Then TLINK links ``TBIOS.OBJ`` and the libraries specfied in ``TBIOS.LN`` to generate ``TBIOS.EXP``.

ホスト側でコンパイルするコマンドは同じで以下の通りです。

To compile, run the same command:

```
Tsugaru_CUI C:\FreeTOWNSOS\CompROM -FD0 C:\FreeTOWNSOS\release\RUNNERFD.BIN -SHAREDDIR C:\TOWNSDEV -SHAREDDIR C:\TOWNSSRC -MEMSIZE 64 -UNITTEST
```

コンパイルしてそのまま実行しようと思ったら、``TASK.BAT``を以下のように書き替えればコンパイルしてすぐ実行できます。

If you compile and run in the VM, you can add one more line to make it like this:

```
HCD386 tbios.c -fsoft
TLINK tbios.obj @TBIOS.LN
FREE386 tbios.exp
SUCCESS
```



## もっと現実的に開発するために
## More Realistic Setting

``C:\TOWNSSRC\TASK.BAT``を書き替えれば、VMにさせる作業をカスタマイズできるわけですが、これはいくつもプロジェクトがある場合はそれぞれのプロジェクト用に書き替える必要が出ます。が、この部分を自動かすればもっと開発が楽になるでしょう。

実のところ自分で使うときはPythonスクリプトを書いて、この部分を自動化しています。個人的にPythonは嫌いですが。

You can describe what VM should do in ``C:\TOWNSSRC\TASK.BAT``, which should be different for different project.  By automating the sequence of TASK.BAT generation to launching Tsugaru_CUI, it gets much easier.

In fact, I write a Python script to generate TASK.BAT and start Tsugaru_CUI to automate the process (although I don't like Python).
