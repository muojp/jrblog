---
layout: post
title:  "最近のUbuntuでDiffImgをビルドする"
date:   2021-01-12 12:10:00 +0900
---
= 最近のUbuntu環境でDiffImgをビルドする

diffは大事。
テキストでも画像でも。

== OSS画像diffツールの昨今

OSSの画像diffツールとしては@<href>{http://sourceforge.net/projects/diffimg/, DiffImg}が有名。
本家プロジェクトは昔懐かしSourceForge（以下SF）にホストされており、近年更新されていない。
Ubuntu用のビルドは@<href>{https://launchpad.net/~dhor/+archive/ubuntu/myway?field.series_filter=}にあるが、16.04を最後にビルドされていない。
このため、20.10（Groovy Gorilla）のような先鋭的Ubuntu環境においては自前でビルドする必要がある@<fn>{fn-1804}。

//footnote[fn-1804][先鋭的どころか、18.04（Bionic Beaver）や20.04（Focal Fossa）といった近年のLTS勢でも状況は同じはず]

さて、ソースにバグフィックス類の手を加えてコツコツとメンテされているGitHub上のリポジトリが存在する（@<href>{https://github.com/sandsmark/diffimg}）ので、今回はこれを使う。

== ビルド環境を整える

ビルドにあたり、依存関係を揃える。
DiffImgはQtにがっつり依存しているため、Qtもろもろをインストールする。

//cmd{
$ sudo apt install libopencv-dev libfreeimage-dev qt5-default \
  libqwt-qt5-dev qt5-qmake qttools5-dev-tools
//}

DiffImgのドキュメントには@<code>{libqwt-dev}を入れろとあるが、Ubuntu 18.04あたりでパッケージの命名ルールが変わったようで、最新版には存在しない。
このため、@<code>{libqwt-qt5-dev}を指定するのがよい。
これに気付かず、ソースからlibqwtをインストールして遠回りをした。

== ソースコード取得・ビルド

//cmd{
$ git clone https://github.com/sandsmark/diffimg.git
//}

forkのコード差分を信用すべきではないと考える（まったくもって正しい疑い方だと思う）場合はちゃんと差分を確認するか、SFのコードを拾ってきてほしい。

普段どおりのCMake作法でビルドする。
今回はfork版（CMakeでビルドが通るように手を加えられている）を使っているのですんなり行ったけれど、fork版をメンテしているSF版ではすんなりいかないかもしれない。

//cmd{
$ mkdir build
$ cd build/
$ cmake ..
-- The CXX compiler identification is GNU 10.2.0
-- Check for working CXX compiler: /usr/bin/c++
-- Check for working CXX compiler: /usr/bin/c++ -- works
-- Detecting CXX compiler ABI info
-- Detecting CXX compiler ABI info - done
-- Detecting CXX compile features
-- Detecting CXX compile features - done
-- Found Qwt: /usr/lib/libqwt-qt5.so (found version "6.1.4") 
-- Configuring done
-- Generating done
-- Build files have been written to: /home/muo/workspace/diffimg/build
$ make -j
...
[100%] Built target diffimg
//}

== インストール

@<code>{diffimg}バイナリが生成されるので、そのまま使うなり@<code>{sudo make install}するなり好きにやる。
うまくビルドできていれば、無事に画像diffをとれる（@<img>{2021-01-12-diffimg}）。

//cmd{
$ diffimg i_183.png i_188.png
//}

//image[2021-01-12-diffimg][DiffImgを実行したところ]{
//}

これで、老眼入りつつある👀でもやっていける。
