
# 津軽OSによるTOWNSアプリのバッチ実行
# Batch-Execution of TOWNS Apps Using Tsugaru OS

FM TOWNSエミュレータ津軽、互換ROM、津軽OSを組み合わせると、TOWNS用プログラムをバッチ実行させることができます。

互換ROMは使わなくてもいいですが、本物のTOWNSから抜き出したROMでは、起動に至るまでにメモリテストがあったりハードウェアの初期化が合ったりCMOSのチェックがあったりと起動に時間がかかるので、あまりバッチで実行してる気がしません。

しかし、互換ROMではそういうチェックはしないので、ほぼ待ち時間無しで目的のプログラムを実行できます。

また、FM TOWNSエミュレータ津軽でホスト上のディレクトリをVM内で共有するTGDRVを使うことで、ファイルをディスクイメージに書き込んだり書き戻したりする手間無く、ほぼシームレスにTOWNSアプリを実行することができます。



# 用意するもの
# Preparation

まず、TOWNSアプリを実行するディレクトリを準備します。仮に、TOWNSBATディレクトリとしましょう。そのディレクトリに以下のファイルをコピーします。

- CompROM下のFMT_DIC.ROM, FMT_DOS.ROM, FMT_F20.ROM, FMT_FNT.ROM, FMT_SYS.ROM
- 実行したいアプリ (ここでは例題としてLHA.EXEを実行するとしましょう。)
- releasesディレクトリ下のRUNNER.BIN


First you need to prepare a directory where you want to run a TOWNS app.  Let's say we make a directory called TOWNSBAT.  Copy the following files in TOWNSBAT directory.

- CompROM/FMT_DIC.ROM, FMT_DOS.ROM, FMT_F20.ROM, FMT_FNT.ROM, FMT_SYS.ROM
- Program you want to run (Let's say you want to run the archiver called LHA.EXE)
- releases/RUNNER.BIN

次に、同じディレクトリに TASK.BAT というバッチファイルを作ります。LHA.EXEを使ってROMファイルを圧縮してみましょう。以下のような内容にします。

Then, make TASK.BAT.  Let's compress ROM files using LHA.EXE.  (Find LHA.EXE somewhere.  It should be easy.)  The content of TASK.BAT should look like:

```
LHA A ROM.LZH *.ROM
SUCCESS.EXE
```
