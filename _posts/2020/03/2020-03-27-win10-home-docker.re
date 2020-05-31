---
layout: post
title:  "Windows 10 Home+Docker=💪"
date:   2020-03-27 07:45:00 +0900
---
= Windows 10 Home+Docker=💪

== Windows 10 HomeのInsider Preview版（19041）をインストール

@<href>{https://insider.windows.com/ja-jp/how-to-pc/}に従って@<href>{https://www.microsoft.com/en-us/software-download/windowsinsiderpreviewadvanced}を開き（Insider Previewを有効化したMicrosoftアカウントでのサインインが必要）、ISOファイルを入手。

これを書いているタイミングではfast ringもslow ringもISOファイルのビルド番号は19041。

ISOファイルを入手してインストール。
手元ではNested Virtualizationを有効化したVMインフラ上へインストール。

メモリの空きが乏しかったので6GBとケチケチ。

== Docker

https://forest.watch.impress.co.jp/docs/news/1239275.html
この話題。

== キャッシュ開放

@<href>{https://docs.microsoft.com/en-us/windows/wsl/reference}

新しいwslコマンドではWindows側からデフォルトのディストロのコマンドを呼び出すshort-handが用意されている。

これを使えば、

//cmd{
wsl -- /bin/bash -c "echo 1 > /proc/sys/vm/drop_caches"
//}

という具合でキャッシュを強制廃棄できる。
適当に.batファイルへ書いてデスクトップにでも置いておけば、ファイルへやたらアクセスするイメージの作成/利用後（元記事ではNode.jsのnpmもりもり環境を例にしている）に呼んでやることで環境が平和になるはず。

== 感触

従来のHyper-V版Docker Desktopだとメモリ割り当ての粒度がたいぶあらくて おそらく起動時に上限設定までメモリ押さえてたのが、WSL2ベースだとVM(コンテナ)の必要に応じてPC搭載メモリの75%まで割り当てていく挙動(返却も頑張る)なので 特にメモリ32GB未満の環境で使う時にはWSL2版のほうが使い勝手良い印象
