
# 津軽OSによるTOWNSアプリのバッチ実行
# Batch-Execution of TOWNS Apps Using Tsugaru OS

FM TOWNSエミュレータ津軽、互換ROM、津軽OSを組み合わせると、TOWNS用プログラムをバッチ実行させることができます。

互換ROMは使わなくてもいいですが、本物のTOWNSから抜き出したROMでは、起動に至るまでにメモリテストがあったりハードウェアの初期化が合ったりCMOSのチェックがあったりと起動に時間がかかるので、あまりバッチで実行してる気がしません。

しかし、互換ROMではそういうチェックはしないので、ほぼ待ち時間無しで目的のプログラムを実行できます。

また、FM TOWNSエミュレータ津軽でホスト上のディレクトリをVM内で共有するTGDRVを使うことで、ファイルをディスクイメージに書き込んだり書き戻したりする手間無く、ほぼシームレスにTOWNSアプリを実行することができます。




# 用意するもの
# Preparation

実行にはFM TOWNSエミュレータ津軽のCUIモード実行ファイル Tsugaru_CUI.EXEが必要です。ダウンロードして、パスを通しておいてください。今からコマンドラインを使おうというのにパスを通すというのがどういう意味かわからない人は、まず学校に行って「パスを通す」とは何か勉強してからきてください。「パスを通すって何？」という質問は僕によって自動的に無視されます。

To batch-execute an FM TOWNS app, you need FM TOWNS Emulator Tsugaru CUI-mode executable, Tsugaru_CUI.EXE.  Download and add PATH to the location where you put Tsugaru_CUI.EXe.  If you don't understand what PATH means, even though you want to use TOWNS app from terminal, please go back to school and learn what it is and then come back.  A question "What's PATH" is automatically ignored by me.

まず、TOWNSアプリを実行するディレクトリを準備します。仮に、TOWNSBATディレクトリとしましょう。そのディレクトリに以下のファイルをコピーします。

- CompROM下のFMT_DIC.ROM, FMT_DOS.ROM, FMT_F20.ROM, FMT_FNT.ROM, FMT_SYS.ROM
- 実行したいアプリ (ここでは例題としてresources/TEST.EXP)
- releasesディレクトリ下のRUNNER.BIN


First you need to prepare a directory where you want to run a TOWNS app.  Let's say we make a directory called TOWNSBAT.  Copy the following files in TOWNSBAT directory.

- CompROM/FMT_DIC.ROM, FMT_DOS.ROM, FMT_F20.ROM, FMT_FNT.ROM, FMT_SYS.ROM
- Program you want to run (Let's use resources/TEST.EXP as an example.)
- releases/RUNNER.BIN

次に、同じディレクトリに TASK.BAT というバッチファイルを作ります。LHA.EXEを使ってROMファイルを圧縮してみましょう。以下のような内容にします。

Then, make TASK.BAT.  Let's compress ROM files using LHA.EXE.  (Find LHA.EXE somewhere.  It should be easy.)  The content of TASK.BAT should look like:

```
FREE386 TEST.EXP > OUTPUT.TXT
SUCCESS.EXE
```

そして、以下のコマンドをタイプします。

Then type the following command:

```
Tsugaru_CUI . -FD0 RUNNERFD.BIN -SHAREDDIR . -UNITTEST
```

このコマンドで、津軽はRUNNERFD.BINの津軽OSを起動して、TGDRV.COMで、カレントディレクトリをDドライブに割り当て、Dドライブに移動して、TASK.BATを実行します。コマンドのコンソール出力は、OUTPUT.TXTというファイルに割り当てられます。

This command will let Tsugaru boot from RUNNERFD.BIN, from which the current directory is mapped to VM's D drive, move to D drive, and run TASK.BAT.  Then you can see the console output in OUTPUT.TXT.
