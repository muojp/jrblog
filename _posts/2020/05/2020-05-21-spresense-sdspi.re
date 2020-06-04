---
layout: post
title:  "Spresenseの汎用SPIを利用してコンパクトなSDカードスロット拡張ボードを作る"
date:   2020-05-21 09:48:00 +0900
---
= Spresenseの汎用SPIを利用してコンパクトなSDカードスロット拡張ボードを作る

みなさん、Spresenseしてますか?

@<href>{https://developer.sony.com/ja/develop/spresense/, Spresense}は2.1cm x 5cmという小型フォームファクタでGNSSやカメラにハイレゾ使い放題という、マシマシ感のある低消費電力ボードです。
いっぽうで標準搭載するストレージはプログラムメモリ兼用の8MBフラッシュメモリのみと、実世界からデータを取り込む用途を考えると少々心もとないです。
やはり、せめて数百MB単位ぐらいのストレージへアクセスしたいものです。

== 追記

SpresenseのI/O電圧が1.8Vだということをすっかり失念していて、3.3Vを叩き込む構造になっているので 本記事に沿ってmicroSDモジュールを作成してSpresenseと組み合わせて利用するとボードの破壊につながる恐れがあります。
作るのはやめてください。

== 結果

//image[2020-05-21-spresense-sdboard][完成した小型マイクロSDボード]{
//}

マイクロSDカードスロットを搭載するAdd-onボードを作成し、Spresense本体から3mmはみ出すだけで収まりました。

== 先行実装例

扱うデータの量によっては外部ストレージが必要というのは自明であり、公式を含めて外部ストレージ拡張が提供されています。

=== 公式拡張ボード（Spresense拡張ボード）

Arduino Uno型のボードにマイクロSDカードスロットほかI/Oポートがくっついているのですが、いかんせん外寸5.3cm x 6.9cmとSpresense本体のコンパクトさをスポイルしてしまうのが惜しいです。

=== KASPI001基板

これは公式拡張ボードの回路からSDカード読み書き部分を抜き出したような作りになっています。
B-to-B（Board to Board）の100ピンコネクタからSDIOのピンを引っ張り出して電源とSDIO制御LSI（TXS02612RTWR）に処理を流す構造です。

SDIOの良さはなんといってもデータ信号線がパラレル4本出ていて高速読み書きできるところです。

値段も3,000円と手頃なので購入しようかと考えましたが、残念ながら2020年5月時点ではスイッチサイエンス・共立ともに品切れ状態で、次回入荷もなさそうです@<href>{https://www.switch-science.com/catalog/5374/}。

公式サイトの@<href>{https://kunutest.jimdofree.com/kaspi001%E5%9F%BA%E6%9D%BF/}サポートページを読むと、GNSS受信感度に悪影響が出たというユーザー報告があったとのことなので、在庫売り切り後の追加製造をしていないふんいきです。

== CXD5602のペリフェラル（SPI関連）

Spresenseの中核はソニーセミコンダクタソリューションズ製のCXD5602チップです。
それなりに仕様が大きいので、興味のある方はマニュアルを読んでみてください。

@<href>{https://www.sony-semicon.co.jp/products/common/pdf/CXD5602GG_technical_manual.pdf}

アプリケーションメモリが1.5MB、I/Oプロセッサまわりでも256KB積んでいて割とリッチな構成です。

ここでは、SDをメインターゲットとしたストレージの読み書きにフォーカスして、搭載するペリフェラルを確認します。

 * SD3.0 Host Controller interface
 * eMMC 4.41 for eMMC Device
 * SPI
 * Quad SPI-FLASH Interface

ざっとこれだけあります。

SD Host Controller interface（SD HCI）は、前述のSDIO用です。

eMMCはざっくりと表面実装向きSDIOのようなもので、データ線が4本（4bit転送）というのも同じです。
媒体の差し替えという面ではSDのほうが圧倒的に有利ですが、eMMCのほうがコンパクトに実装できます。

SPIは、SPI0-5の合計6インタフェースが用意されており、それぞれ用途と性能が異なります。

 * SPI0
 ** 最大8.1Mbps
 * SPI1
 ** QSPI（データ信号線1本または4本を選択可能）動作
 ** ハイパフォーマンス動作時、最大39.000MHz（注：ビットレート記載ではない）
 * SPI2
 ** ハイパフォーマンス動作時、最大4Mbps
 * SPI3
 ** 最大6.5Mbps
 * SPI4
 ** ハイパフォーマンス動作時、Master Half duplex modeで最大39Mbit/s
 * SPI5
 ** ハイパフォーマンス動作時、最大13.64Mbit/s

このうち、SPI1はSpresenseボードにおいては8MBのフラッシュメモリにQSPI接続されています。

== Add-onボードで使えるポート

Spresenseの基板表面に生えているArduino Mini的なコネクタ（13ピンx2列）経由で接続する拡張ボードは、Add-onボードと呼ばれます。

B-to-Bコネクタを使いたくない場合は、この26ピンにマッピングできる範囲でやっていくことになります。

前述のSDIOはここに出てきていないので（コンフィグ頑張れば可能かもしれませんが）、選択肢から除外されます。

=== eMMC

ソフトウェア的にはeMMCのサポートが実装されています。

公式ドキュメントを調べていると、@<href>{https://developer.sony.com/ja/develop/spresense/developer-tools/get-started-using-arduino-ide/developer-guide#emmc_library, eMMC Add-on ボード (今後発売予定)}という表記があるので、そのうち安価に入手できるようになるのかもしれません。

=== SPI

速度面ではSPI4のほうが有利ですが、これはB-to-B経由でしか取り出せません。
Add-on用にはSPI5ポートが出ています。

これを使えるので使っていきます。

== やっていきました

Spresenseのプログラム開発はArduino IDEを利用するお手軽パターンとSpresense SDKを利用するパターンのふたつが主に想定されています。

Espressifのプラットフォームに馴染みのある方には、Arduino Core for ESPxxとESP-IDFの関係と言えば伝わりやすいでしょう。

Spresenseの基盤RTOSであるNuttXのソースとSpresenseのSDKソースコードは共に公開されており（SDKのほうはデバッグビルドに必要なファイルが不足しており完全ではありませんが）、好きに手を入れられます。

公式拡張ボード（Spresense拡張ボード）が利用しているSDIO用の読み書きコードとSPI SD用のコードはほとんどが共通のはずで、適切なポートを選んで初期化を通せばそのままデバイス検出からマウント・読み書きまで実現できるのではないかと考え、その方針で進めました。
コンフィグを変更してビルド、printfデバッグという泥臭いやりかたです。

=== 結果ソース

Spresense SDKのSPI SD機能はSPI4（B-to-Bコネクタ経由でしかアクセスできない）の利用を前提としており、そのままでは利用できないため、Add-onボードからアクセスできるSPI5を利用するために手を入れました。

@<href>{https://github.com/muojp/spresense/tree/support-spi-sd-on-spi5, ソース}

また、カードの挿抜検出用ピンはAdd-onボード上でSPI通信用ピンのすぐ近くに配置しました。

=== ビルド方法

//cmd{
$ cd spresense/sdk
$ ./tools/config.py device/sdcard_spi examples/sdcard_test
$ make
//}

=== ボードを作る

スイッチサイエンスで扱っている@<href>{https://www.switch-science.com/catalog/3902/, Spresense Add-onボード用ユニバーサル基板}のうえにポリウレタン銅線（UEW）で線を引っ張り回します。

今回はSparkFunのマイクロSDブレークアウトボードを利用しました。
このブレークアウトボードは3.3V-5Vの電源/レベル変換を搭載している便利品なのですが、Spresenseの3.3V I/Oで使う分にはレベル変換は不要なので、頑張ればマイクロSDカードスロットをSpresense Add-onボード内部あるいは裏面に実装することも可能でしょう。

//image[2020-05-21-spresense-sdboard2][Spresense本体と小型マイクロSDボード]{
//}

カードスロットを横向きに出すか、それとも縦向きに伸ばすかは悩ましいところですが、今回はせっかくなので縦に伸ばしました。
リセットスイッチ/USBコネクタ側に伸ばしたのは前述のGNSS受信感度への悪影響を避けるためですが、このレイアウトだとカメラモジュールの座りがかなり悪くなるので一長一短あります。

また、マイクロSDカードがブレークアウトボードの裏側へ行ってしまったので抜き差しがちょっと面倒です。
これには少々事情があり、ブレークアウトボードのVCCとGNDをAdd-onコネクタのアサインに揃えた結果です。
よく見ていただくと、GNDに至ってはブレークアウトボード側のピンヘッダを伸ばしてユニバーサル基板を貫通させ、そのままSpresense側のピンソケットまで通す力技です（@<img>{2020-05-21-spresense-sdboard3}）。

//image[2020-05-21-spresense-sdboard3][ブレークアウトボード側のピンヘッダ加工]{
//}

結果的にスイッチやLEDを追加実装するのに十分な領域がAdd-on基板上に残っているので、使ってみてもよさそうです。

=== ベンチマーク

screenでnshへ接続し、@<code>{sdcard_test}を実行します。

//cmd{
nsh> sdcard_test
Writing block size = 4096, count=2621, total of 10735616 bytes.
WRITE: 181867 bytes/s (59030ms)
READ: 1668835 bytes/s (6433ms)
nsh> 
nsh> sdcard_test
Writing block size = 4096, count=2621, total of 10735616 bytes.
WRITE: 181719 bytes/s (59078ms)
READ: 1669095 bytes/s (6432ms)
nsh> 
nsh> sdcard_test
Writing block size = 4096, count=2621, total of 10735616 bytes.
WRITE: 181621 bytes/s (59110ms)
READ: 1668835 bytes/s (6433ms)
//}

3回実行しましたがさすがほとんどバラツキは出ません。
書き込みは@<b>{1.4Mbps}程度、読み込みは@<b>{12.7Mbps}程度でした。

完全に気持ちで書いた@<href>{https://github.com/muojp/spresense/blob/support-spi-sd-on-spi5/examples/sdcard_test/sdcard_test_main.c#L43-L65, テスト用プログラム}がもろもろ考慮していないというのもありますが、書き込みスピードはもう少し出てもよい気がします。
用途のレンジとしては、音声は記録できるけど高品質な映像を記録するにはちょっと厳しい、ぐらいでしょうか。

読み込みはSPI5の理論値である13.64Mbit/sに迫る値が出ていますね。

== まとめ

NuttXとSpresense SDKが提供するSPI SD機能を利用して、少面積のSpresense Add-onボードでマイクロSDカードの読み書きを実現できました。

ストレージ拡張のためだけに公式のSpresense拡張ボードを利用するとフットプリントが+254%と激増しますが、今回試作したAdd-onモジュールでは+6%に抑えられました。

簡易ベンチマークで読み込み12.7Mbps、書き込み1.4Mbpsという用途によっては十分なパフォーマンスを発揮することも確認できました。

== 追記

SpresenseのI/O電圧が1.8Vだということをすっかり失念していて、3.3Vを叩き込む構造になっているので 本記事に沿ってmicroSDモジュールを作成してSpresenseと組み合わせて利用するとボードの破壊につながる恐れがあります。
作るのはやめてください。
