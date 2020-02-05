---
layout: post
title:  "なぜEdge TPUのWindowsランタイムはWinUSBとUsbDkに依存しているのか"
date:   2020-02-04 20:00:00 +0900
---
= なぜEdge TPUのWindowsランタイムはWinUSBとUsbDkに依存しているのか

2020/01/29（日本時間1/30）付のアップデートでGoogle（Coral）のEdge TPUランタイムがWindowsとmacOSをサポートしました（@<href>{https://coral.ai/news/updates-01-2020/, 公式ニュースエントリ}）。

USBドライバまわりで気になった部分があったので軽く調べました。

== ディレクトリ構成

ざっと構成を確認しておきます。

//cmd{
.
├── install.bat
├── install.sh
├── libedgetpu
│   ├── BUILD
│   ├── LICENSE.txt
│   ├── direct
│   │   ├── aarch64（armv6、armv7a、k8も同じ）
│   │   │   └── libedgetpu.so.1 / libedgetpu.so.1.0
│   │   ├── darwin
│   │   │   └── libedgetpu.1.0.dylib / libedgetpu.1.dylib
│   │   └── x64_windows
│   │       └── edgetpu.dll / edgetpu.dll.if.lib
│   ├── edgetpu-accelerator.rules
│   ├── edgetpu.h
│   ├── edgetpu_c.h
│   └── throttled（directと同じなので省略）
├── third_party
│   ├── coral_usb_accelerator_winusb
│   │   ├── Coral_USB_Accelerator.cat
│   │   ├── Coral_USB_Accelerator.inf
│   │   ├── Coral_USB_Accelerator_(DFU).cat
│   │   ├── Coral_USB_Accelerator_(DFU).inf
│   │   └── amd64
│   │       ├── WdfCoInstaller01009.dll
│   │       ├── license.rtf
│   │       └── winusbcoinstaller2.dll
│   ├── libusb_win
│   │   ├── README
│   │   └── libusb-1.0.dll
│   └── usbdk
│       ├── LICENSE
│       └── UsbDk_1.0.21_x64.msi
├── uninstall.bat
└── uninstall.sh
//}

== WinUSBとUsbDk

さきのファイル一覧を見ると、基本的には従来のLinux用ランタイムと同様、中身はlibusb-1.0系の素直な構成に見えます。

しかしWinUSBとUsbDkの両方に依存しているのが若干謎です。
両方ともlibusb-1.0のバックエンドとして有効なのですが、通常は片方のみの依存で済むはずです。
特に、Windows環境にプリインストールされた汎用USBドライバという立ち位置のWinUSBは、デバイスカタログ登録さえしてしまえばその後のデバイス利用は一般ユーザー権限で完結できる点が大きなメリットなので、敢えてUsbDk依存を組み込む必要はないように見えます。

おそらくこれはCoral USB Acceleratorの起動シーケンスに関する事情です。

== Coral USB Acceleratorの起動シーケンス

同端末をPCへ接続するとまずDFUモード（VID:1A6E PID:089A）で認識し、libedgetpu内から初期化パケットを送ってPCから一旦論理的に切断すると稼働モード（VID:18D1 PID:9302）で再認識するという動作をします。

端末の利用側（USBドライバ）がこの挙動を実現するためには、USBデバイスあるいはUSBハブの比較的低レベルで再接続コマンドを発行する必要があります。

== WinUSBの機能制限とlibusb

さて、libusb1.0系のソースを読むと、WinUSBバックエンドを利用した場合にこの処理が単純にはいかないことがわかります。
@<href>{https://github.com/libusb/libusb/blob/master/libusb/os/windows_winusb.c#L2952-L2961, 該当箇所のコメント}には「IOCTL_INTERNAL_USB_CYCLE_PORTはカーネルモードでしかサポートされず、またIOCTL_INTERNAL_USB_CYCLE_PORTはWindows Vista以降で削除されたので完全には実装できない」旨が書かれています。

実はこのコメントが書かれたのは10年ほど前で、その後にWinUSB事情は若干変わっています。
@<href>{https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/usbioctl/ni-usbioctl-ioctl_usb_hub_cycle_port, IOCTL_USB_HUB_CYCLE_PORT}はWindows 8から管理者権限での動作時に限って再度サポートされています。
しかし、汎用USBドライバはなるべく一般ユーザー権限でも一通りのオペレーションを完結できるのが望ましいため、あまり気軽に叩けるAPIではありません。

さて、libusbのwikiには@<href>{https://github.com/libusb/libusb/wiki/Windows, Windows}という世の様々な厄介（libusb0系からこのかた、ランタイムが乱立していることもあり）への対応も含めた情報をまとめたページがあるぐらいなのですが、その中に関連の話があります。

//quote{
For version 1.0.21 or later, you can also use usbdk backend. usbdk provides another driver option for libusb Windows backend. For 1.0.21, usbdk is a compile-time option, but it becomes a runtime option from version 1.0.22 onwards
//}

ざっくり訳すと「libusb 1.0.21以降でusbdkバックエンドも利用可能。1.0.21でusbdk利用はコンパイル時オプションだったが、1.0.22以降では実行時オプションになった」という内容です。

== Windows版libedgetpuの処理シーケンス

DFUモードに対する一連の処理はデバイスの接続後初回のみ必要なもので、初期化処理以外の推論系や2回目以降の実行はユーザーモード動作で十分です。
このため、Windows版のEdge TPUランタイム（libedgetpu）では次のような実装をしていると推測できます。

 * WinUSBバックエンドでlibusbを初期化
 * 接続済みデバイスを列挙し、DFUモードか稼働モードかを判定
 * DFUモードの場合、wakeup処理を実施
 ** UsbDkバックエンドでlibusbを再初期化し、デバイス起動用のコマンド列を投入
 ** @<code>{libusb_reset_device()}を呼び出してデバイスを再認識させる
 ** WinUSBバックエンドでlibusbを再初期化
 * モデル・パラメータのアップロード処理
 * 推論用データのfeed処理（以下ここだけループ）

どのみち下回りでは特権モードへ降りないと発行出来ない操作が必要なのでユーザーのセキュリティを一定犠牲にするしかないのですが、UsbDkバックエンドをDFUモード限定のバックエンドとして併用することで影響範囲の限定化を図る、というGoogleの判断なのだと思います。

== おまけ：転移学習のチュートリアルが詰まないよう改善された

世界中でTensorFlow 1系から2系への移行が流行っている昨今です。
Coralのチュートリアルもご多分に漏れず影響を受けていました。

具体的には、転移学習のハンズオン用Dockerイメージ（TensorFlow 1.11系前提）の生成フロー内でTensorFlow 2前提なmodels/research/slim/*を拾いに行ってがっつりエラーを吐くという問題がありました。

TFLiteランタイムのベースTFバージョンが2.1系まで引き上げられたとchangelogにあったので、こちらのベースイメージも更新されたかと思って見に行ったところ、@<href>{https://coral.ai/docs/edgetpu/retrain-classification/#prepare-your-dataset, Prepare your dataset}節が拡充されて、古いリビジョンをcheckoutする手順が明示されています。
転移学習ハンズオンのための最低限の手間で効果を得るならこれが手っ取り早い@<fn>{git-checkout}ので、初見で詰まなくなったのは良いことですね。

//footnote[git-checkout][私もある原稿のなかでは近いリビジョンへのcheckout手順を補足していました]
