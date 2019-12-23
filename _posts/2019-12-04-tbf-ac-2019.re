---
layout: post
title:  "技術書典で出会った推しの1冊、「RustでGBAのプログラムを作ろう!」"
date:   2019-12-04 01:08:00 +0900
categories: ac2019
---
= 技術書典で出会った推しの1冊、「RustでGBAのプログラムを作ろう!」

//lead{

本エントリは、@<href>{https://adventar.org/calendars/4224, 【推し祭り】技術書典で出会った良書 Advent Calendar 2019}兼、@<href>{https://b.muo.jp, muo-ya} Advent Calendarの4日目の記事です。

//}

これまで技術書典で購入した技術同人誌は累計150冊ほどあり、楽しい本がいっぱいなのですが、今回はそんな中でひみつラボの「@<href>{https://booth.pm/ja/items/492956, RustでGBAのプログラムを作ろう！}」をイチオシします。

@<raw>{|html|<blockquote class="twitter-tweet"><p lang="ja" dir="ltr">【ひみつラボ】おまたせしました！ <a href="https://twitter.com/hashtag/%E6%8A%80%E8%A1%93%E6%9B%B8%E5%85%B8?src=hash&amp;ref_src=twsrc%5Etfw">#技術書典</a> にて頒布した新刊の電子書籍版(PDF)、booth.pmにて取扱い開始しました！ - RustでGBAのプログラムを作ろう! | ひみつラボ <a href="https://t.co/30lIS14es6">https://t.co/30lIS14es6</a> <a href="https://twitter.com/hashtag/booth_pm?src=hash&amp;ref_src=twsrc%5Etfw">#booth_pm</a></p>&mdash; kotetu (@kotetu) <a href="https://twitter.com/kotetu/status/853640313700622336?ref_src=twsrc%5Etfw">April 16, 2017</a></blockquote> <script async src="https://platform.twitter.com/widgets.js" charset="utf-8"></script>}

タイトルの通り、Rustでゲームボーイアドバンス（GBA）向けにプログラムを書いていくという話です。
GBAでRustベアメタル、痺れます。

私は技術者が非公式環境で苦しみながら進んでいくのを眺めるのがとても好きなので選びました@<fn>{straightforward}@<fn>{rpi-baremetal}。

//footnote[straightforward][本書に試行錯誤録が描かれているわけではなく、粛々と荒野を道具と知識で進んでいく感じです]
//footnote[rpi-baremetal][近いトピックとしてはRaspberry PiでRustベアメタルをやっていく本があり（これも技術書典で購入しました）あちらはあちらで好みなのですが、今回は1冊に絞って推すぞと決めたので、別の機会に紹介します]

== GBA?

初代ゲームボーイはZ80系の8-bit CPUだったのが、GBAは32-bitのArmということで相応に時代が進んだのを感じつつ、楽しく読みました。

この携帯用ゲーム機はつまりバッテリー駆動でモニタや入力デバイスに音声アンプまで搭載したArm CPUベースのシングルボードコンピューター（SBC）@<fn>{sbc}といえます。

ここでは現代のSBC代表選手であるところのRaspberry Pi（初代）とざっくり比べてみましょう（@<table>{tbl-gba-rpi}）。

//table[tbl-gba-rpi][ゲームボーイアドバンスとRaspberry Piの比較]{
項目	GBA	Raspberry Pi
CPU	ARM7	ARM11
動作クロック	16.78MHz	700MHz
メインメモリ	32KB（CPU内）、ワークメモリ256KB	256MB
//}

GBAはRaspberry Piよりきれいに@<b>{1.5-3桁}非力なのも個人的にはポイント高いです。
たまりません。
ちなみにARM7って案外新しいのでは?と混乱しがちですが、ARM7とARMv7は完全に別物で、世界的に混同されがちらしく公式サイトに@<href>{http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.faqs/ka13706.html, FAQ（ARM7とARMv7はどう違うの?）}まであります。

では、そろそろ本の紹介と推しポイントについて述べていきます。

//footnote[sbc][@<href>{https://www.ifixit.com/Guide/Game+Boy+Advance+Logic+Board+Replacement/2148}を見ると、ロジックボード1枚なのでサブ基板の分かれていた初代とちがって文字通りシングルボードです。よかったですね]

== 本の概要

Rustの環境作りをさらっと、続けて言語の特徴を最低限把握できるようコンパクトにまとめたのち、GBAのアーキテクチャ（ハードウェア構成）についても適宜解説していき、画面にドットを出すところであらかた低レイヤーの話が終わります。

その後、ここまでの流れで無理やり拓いてきた道をいい感じに整備して先へ向かうための小休止、からの「Hello, World!」と画面へ描画するまで走っていきます。

== 推しポイント

一番の推しポイントは、Rustやベアメタルプログラミングにちょっと興味のある人が本書を手にとって読んでいくと「なるほどここまではいけそう」と疑似的な達成感をおそらく得られる書籍構造であるところです。
+60ページで網羅的な解説をできるトピックではないので、センスよく端折るべき部分をばっさり落として「俺たちの戦いはこれからだ」まで持っていく技が光ります。

実際のところ、本書を読んで実践するためのGBAと転送ケーブルが手元にあるケースのレアさを考えると、追体験と「できそうな気持ちになるところ」へ振っているのはよくできていると感じます。

まず、デバイスがどのようにブートしていくのか、最低限のブートストラップから自前コードに飛びつつRust標準ライブラリなしの環境でどう暮らしていくのか、ある程度丁寧にフォローしています。

//quote{
こういった、普段OSが当たり前のようにやっている処理のメカニズムを知ることができるというのも、Bare Metalなプログラムを作る醍醐味だと思います。
//}

本書中でも触れられているように、このあたりの感触を掴んでいるとRaspberry Piを始めとする現代のデバイスへ取り組む際も「あっ、RustでGBA本にあったやつだ！」となるかもしれません。

さて、本書はブートストラップのアセンブリからRust側へ制御を移したあとはビデオ出力モードをレジスタ叩いて設定し、ドットを描画するコードまでとにかく走ります。
そして、これを泥臭く最低限動作するところまで持っていってからMakefileを整備したりCargo（ビルドシステム）の説明を入れ、ここまでに書いたコード/プロジェクトを見直して改修するという流れも学習向きだなぁと好みなところです。

ドットを出せたということは図形やビットマップフォントの描画もいける、ということで図形描画を題材にRustへ慣れるくだりを経て、最後は余裕をもって流すようなリズムでシンプルなフォントを用いた「Hello, World!」へと至って本書は終わります。

とことん、無理なく読むことができる構成ながらしっかり読もうと思えば得られるものが多いし、脚注での参考文献も書き込まれている※ただしGBA実機へ転送してハンズオンできる人はたぶんかなり少ない、面白い本だなという感想でした。

こちらからは以上です。