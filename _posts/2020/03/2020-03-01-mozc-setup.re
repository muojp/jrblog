---
layout: post
title:  "Ubuntu 19.10+Xfce4でMozcの設定ウィンドウを開けなかった"
date:   2020-02-04 20:00:00 +0900
---
= Ubuntu 19.10+Xfce4でMozcの設定ウィンドウを開けなかった

最近使っているUbuntuのデスクトップ環境でIBus設定でなぜか開かない。

ibus-mozcパッケージは導入してるけれど、調べてみると/usr/lib/mozc内にmozc_toolがない。

@<img>{2020-03-01-ibus-mozc}のPreferencesボタンを押しても反応がない。

//image[2020-03-01-ibus-mozc][IBusの設定画面]{
//}

なんか設定失敗してるかなぁと思って調べたら@<href>{https://wiki.debian.org/JapaneseEnvironment/Mozc}のエントリが見つかった。

//quote{
/usr/lib/mozc/mozc_tool を提供している mozc-utils-gui パッケージは Recommends に指定しているので、 お使いの環境によってはインストールされていない場合があります。
//}

まさにこれだった。
環境構築用のスクリプトでrecommendsは入れないようにしている分かな。

//cmd{
$ sudo apt install mozc-utils-gui
//}

で事なきを得ました。