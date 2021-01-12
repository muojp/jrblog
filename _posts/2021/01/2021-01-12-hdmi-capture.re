---
layout: post
title:  "Amazon's Choiceマーク付きのFullHD 60fps対応HDMIキャプチャデバイスが治安悪い"
date:   2021-01-12 12:10:00 +0900
---
= Amazon's Choiceマーク付きのFullHD 60fps対応HDMIキャプチャデバイスが治安悪い

ZoomやMeetでのリモート会議の場面が増える今日この頃。
コンデジ（RX100M5）からPCにHDMI入力して映像を良い感じにするぞ！ということでUSB 3.0接続のHDMIキャプチャデバイスを購入しました。

@<b>{Chilison HDMI キャプチャーボード ゲームキャプチャー USB3.0 ビデオキャプチャカード 1080P60Hz ゲーム実況生配信、画面共有、録画、ライブ会議に適用 小型軽量 Nintendo Switch、Xbox One、OBS Studio対応 電源不要（アップグレードバージョン）}という長い名前の品で、Amazon's Choiceマークつき2,400円ほどです。

//image[2021-01-12-hdmi-capture-5][Chilison HDMI キャプチャーボード]{
//}

しばらく前に1,000円未満で買えるお手軽HDMIキャプチャデバイスが流行りましたが、USB 3.0対応品もこれほど安くなったんですね。

商品説明には次の記載があります。

//quote{
【1080P 60fpsに更新され】HD解像度ビデオ録画とストリーミングの高品質体験を保証するために、USB 3.0インターフェイスを採用し、1080p/60fpsの高品質で録画できます。最大入力解像度：3840x2160 @ 60Hz、最大出力/録画解像度：1920x1080 @ 60Hz。（ご注意：一部の説明書はまた更新されなかったですが、キャプチャーは全部更新済みました、ご不便をかけて申し訳ございませんでした。）
//}

日本語が若干アレですが、ひとまず気にせずやっていきましょう。

== そもそもUSB 3.0機器ではない

=== USB3. 0という名前のUSB 2.0機器

USBコネクタはUSB 3.0-Aに見えます。
ちゃんと青いし、3.0の追加端子も見えます（@<img>{2021-01-12-hdmi-capture-6}）。

//image[2021-01-12-hdmi-capture-6][USBコネクタ]{
//}

しかし、@<code>{lsusb}（および@<code>{lsusb -t}）の結果は次のとおりです。

//cmd{
Bus 006 Device 028: ID 534d:2109 MacroSilicon USB3. 0 capture
...
/:  Bus 06.Port 1: Dev 1, Class=root_hub, Driver=xhci_hcd/4p, 480M
    |__ Port 2: Dev 28, If 1, Class=Video, Driver=uvcvideo, 480M
    |__ Port 2: Dev 28, If 4, Class=Human Interface Device, Driver=usbhid, 480M
    |__ Port 2: Dev 28, If 2, Class=Audio, Driver=snd-usb-audio, 480M
    |__ Port 2: Dev 28, If 0, Class=Video, Driver=uvcvideo, 480M
    |__ Port 2: Dev 28, If 3, Class=Audio, Driver=snd-usb-audio, 480M
//}

初っ端からおかしいです。

残念なお知らせですが、私が買ったHDMIキャプチャデバイスは@<b>{USB 3.0対応機器}ではなく@<b>{USB3. 0という名前のUSB 2.0機器}です。
親に向かってなんだそのUSB3. 0は、と突っ込みたくなるところですが、冷静に結果を受け止めましょう。

ちなみに、同じデバイスをUSB-Cハブ上のUSB 3.0ポートに挿した場合（カスケード状態）は

//cmd{
Bus 004 Device 026: ID 534d:2109 MacroSilicon USB 2.0 Hub
//}

と出力されます。
すわ返品か！？と噴き上がりそうになりますが、ぐっとこらえます。

=== PCのUSBポートは正常

のっけから結果にインパクトがありすぎて、PC側USBポートの不具合を疑いたくなるところです。
なぜ過去の購入者が気付いていないのか、レビューは全部サクラだったのか、全く分かりませんが、ひとまずPC側が正常であることを最低限確認しておきます。
まともなUSB 3.0対応UVCカメラの例として、手元にあったIntel RealSense SR300での出力例を示します。

//cmd{
Bus 007 Device 003: ID 8086:0aa5 Intel Corp. 
...
/:  Bus 07.Port 1: Dev 1, Class=root_hub, Driver=xhci_hcd/2p, 10000M
    |__ Port 2: Dev 2, If 0, Class=Hub, Driver=hub/2p, 5000M
        |__ Port 1: Dev 3, If 0, Class=Video, Driver=uvcvideo, 5000M
        |__ Port 1: Dev 3, If 1, Class=Video, Driver=uvcvideo, 5000M
        |__ Port 1: Dev 3, If 2, Class=Video, Driver=uvcvideo, 5000M
        |__ Port 1: Dev 3, If 3, Class=Video, Driver=uvcvideo, 5000M
        |__ Port 1: Dev 3, If 4, Class=Vendor Specific Class, Driver=, 5000M
//}

ちゃんとSuperSpeed対応であることがわかります。
前述のUSB-Cハブ以下にぶら下げても同様の結果でした。

== キャプチャ対応範囲の仕様を確認する

v4l2-ctlコマンドで調べた結果、キャプチャデバイスとしての仕様は次の通りです。

 * M-JPEG出力では1920x1080 60fpsまで対応
 * YUYV 4:2:2（YUV422）ベタ出力では720x480 30fpsまで対応（1920x1080時は5fpsまで）

詳しくは記事末尾の付録に記載したので、興味のある方は@<hd>{check-format}を参照してください。

== 動作テスト

今回はJavaScript動作のストップウォッチをPC画面に表示し、それをカメラで撮影してHDMIで取り込みます。
PC画面が60fps表示されていることは、あらかじめカメラの録画映像およびスマフォのスローモーション撮影で確認済みです。

=== 1920x1080 60fpsキャプチャできる…?

デバイスの主張を信じると、無圧縮YUV422はともかくM-JPEGなら1080p60を出せるはずです。
実際にキャプチャしてみます。

//cmd{
$ ffmpeg -f v4l2 -input_format mjpeg -i /dev/video4 -c:v copy o.mkv
//}

結果のフォーマットを確認すると、次の通りです。

//cmd{
Input #0, video4linux2,v4l2, from '/dev/video4':
  Duration: N/A, start: 210724.995835, bitrate: N/A
    Stream #0:0: Video: mjpeg (Baseline), yuvj422p(pc, bt470bg/unknown/unknown),
    1920x1080, 60 fps, 60 tbr, 1000k tbn, 1000k tbc
//}

なんだ、ちゃんと1920x1080 60fpsと書いてありますね。

=== 1920x1080 60fpsキャプチャできてる（できてない）

疑り深いので、キャプチャしたフレームをいくつか抜き出して確認します。

//image[2021-01-12-hdmi-capture-rx100m5-1080p][RX100M5から取り込んだ映像の連続キャプチャ]{
//}

…。

確かに秒間60枚キャプチャされていますが、同じ画像が2枚ずつ繰り返されています。
つまり、実質30fpsのものをわざわざ容量割いて60fpsで記録している状態です。
なんてこったい。

念のため、@<href>{https://b.muo.jp/2021/01/12/build-diffimg-ubuntu-2010.html, DiffImg}で画像差分を確認します。

画像間に差分があれば、@<img>{2021-01-12-hdmi-capture-1}のように差分が表示されるはずです。

//image[2021-01-12-hdmi-capture-1][差分ありの場合の参考画像]{
//}

結果は、@<img>{2021-01-12-hdmi-capture-2}のとおり差分なしでした。

//image[2021-01-12-hdmi-capture-2][重複フレーム間で画像差分なし]{
//}

すわ返品か！？と噴き上がりそうになりますが、ぐっとこらえます。

== 1280x720であれば60fpsキャプチャできる

いろいろと試しているうちに、60fpsきっちり撮れるケースがあることに気付きました。
具体的には、Zoomで映像プレビューをかけた後でキャプチャを実行すると、重複フレームのない60fps映像を撮れます。
ただし、720p（1280x720）解像度です。

UVCドライバ/v4l2あたりにモード維持機能がある気配ですが、それはさておきキャプチャ時に映像サイズをhd720へ固定してやれば60fpsキャプチャが可能です。
FFmpegの場合は次のように指定します。

//cmd{
$ ffmpeg -f v4l2 -framerate 60 -video_size hd720 -input_format mjpeg \
  -i /dev/video4 -c:v copy o.mkv
//}

結果は@<img>{2021-01-12-hdmi-capture-rx100m5-720p}のとおりです。
フレーム重複のない、ちゃんとした60fps映像（ただし720p）であることがわかります。

//image[2021-01-12-hdmi-capture-rx100m5-720p][RX100M5から取り込んだ映像の連続キャプチャ（720p）]{
//}

== Chilison HDMI キャプチャーボードについて分かったこと

 * 内部はUSB 2.0動作だが高級感あるUSB 3.0コネクタ（青い！）を使っている
 * M-JPEGフォーマット 1920x1080 60fpsを流せるがデータ重複のため実際は30fps
 ** M-JPEGフォーマット 1280x720 60fpsは問題なく流せる

雑に言えば、@<b>{1,000円未満で売られている品と同等のものを2,500円で買えるというお得な商品}です。

われわれ購買者が成熟しないと、いつまで経っても「デバイス名にUSB3. 0って書いてるしコネクタ青いからヨシ！」というアホなことが続くのですね。

ちなみに、製品のQ&Aには次の記載があります。

@<href>{https://www.amazon.co.jp/ask/questions/Tx1CKNSSTETK4HT/}

//quote{
Q. 購入したら説明書にusb2.0hdmiビデオキャプチャーカードとなっていますけどusb3.0ではないのですか。
A. 最近、当店のキャプチャーはUSB2.0をUSB3.0に変更中で、在庫のUSB2.0は全部売り切れないですから、USB2.0とUSB3.0の仕様が同時に販売しています。
商品の説明は暫くUSB2.0に使用し、USB2.0の在庫売り切れる時、当店はすぐUSB3.0の説明を変更いたします。今後も、こんなの迷惑をお客様に与えられないように、持続的に改善します。
//}

この返答後に商品ページが更新されており、私が購入した1/10時点で冒頭引用のUSB 3.0対応を謳う内容が掲載されていました。
つまり、今回届いた品は購入時点の商品ページで説明されていた「USB 3.0インターフェイスを採用」という文言と一致しない、不良品ということになります。

それ以前の問題として、商品ページにキャプチャ対応仕様について次のように書かれています。

//quote{
最大入力解像度：3840x2160 @ 60Hz、最大出力/録画解像度：1920x1080 @ 60Hz。（ご注意：商品は全部アップグレード済ましたが、説明書は次回在庫補充された後から全部更新済です。）
//}

しかし、そもそも1080p 60fpsでキャプチャできていない（フレームが常に重複しており30fpsしか出ていない）ので、商品説明に書かれている性能を満足しない不良品と呼ぶしかありません。

すわ返品か！？

不良品を売り続けても条件さえ満たせばAmazon's Choiceマークを取得できるので、購入時には信用しちゃだめですよという話でした。

== 付録

=== MacroSilicon 2109?

Chilison HDMI キャプチャーボードは、どのようなコアモジュールを利用しているか公表していません。
しかしVendor IDがMacroSilicon該当、Product IDが0x2109である点から安直に想像すると、1,000円未満で売られている超激安HDMIキャプチャデバイスの常連であるMacroSilicon製MS2109搭載機である可能性があります。

=== 上限データレートを推測する

M-JPEGは圧縮度合いによって相当データ量が変わってくるので、YUV422（16bpp）出力の上限に注目します。
720x480x30fpsということは、158.2MbpsがこのデバイスのUSB転送上限に近い値と考えてよさそうです。

@<code>{lsusb -v}でインタフェースディスクリプタを読むと

//cmd{
        dwMaxBitRate                196608000
//}

と書かれており、187.5Mbpsがインタフェース上限なので、それなりに整合します。

=={check-format} UVCでの対応フォーマット一覧チェック

v4l2コマンド群で対応フォーマットを調べる手順をメモがてら残します。

//cmd{
$ v4l2-ctl --list-devices
Integrated Camera: Integrated C (usb-0000:05:00.0-2):
	/dev/video0
	/dev/video1
	/dev/video2
	/dev/video3

USB3. 0 capture: USB3. 0 captur (usb-0000:06:00.4-2):
	/dev/video4
	/dev/video5
//}

※video[0-3]はノートPC内蔵のカメラです。

今回のキャプチャデバイスが持つのはvideo[4-5]だと分かるので、video4の対応フォーマットを調べてみます。

//cmd{
$ v4l2-ctl -d /dev/video4 --all
Driver Info:
	Driver name      : uvcvideo
	Card type        : USB3. 0 capture: USB3. 0 captur
	Bus info         : usb-0000:06:00.4-2
	Driver version   : 5.8.18
	Capabilities     : 0x84a00001
		Video Capture
		Metadata Capture
		Streaming
		Extended Pix Format
		Device Capabilities
	Device Caps      : 0x04200001
		Video Capture
		Streaming
		Extended Pix Format
Priority: 2
Video input : 0 (Camera 1: ok)
Format Video Capture:
	Width/Height      : 1920/1080
	Pixel Format      : 'MJPG' (Motion-JPEG)
	Field             : None
	Bytes per Line    : 0
	Size Image        : 4147200
	Colorspace        : sRGB
	Transfer Function : Default (maps to sRGB)
	YCbCr/HSV Encoding: Default (maps to ITU-R 601)
	Quantization      : Default (maps to Full Range)
	Flags             : 
Crop Capability Video Capture:
	Bounds      : Left 0, Top 0, Width 1920, Height 1080
	Default     : Left 0, Top 0, Width 1920, Height 1080
	Pixel Aspect: 1/1
Selection Video Capture: crop_default, Left 0, Top 0, Width 1920, Height 1080, Flags: 
Selection Video Capture: crop_bounds, Left 0, Top 0, Width 1920, Height 1080, Flags: 
Streaming Parameters Video Capture:
	Capabilities     : timeperframe
	Frames per second: 30.000 (30/1)
	Read buffers     : 0
                     brightness 0x00980900 (int)    : min=-128 max=127 step=1 default=-11 value=-11
                       contrast 0x00980901 (int)    : min=0 max=255 step=1 default=148 value=148
                     saturation 0x00980902 (int)    : min=0 max=255 step=1 default=180 value=180
                            hue 0x00980903 (int)    : min=-128 max=127 step=1 default=0 value=0
//}

MJPEGで1920x1080の出力に対応していますが、30fpsとありますね。
すわ返品か！？と噴き上がりそうになりますが、ぐっとこらえます。

対応フォーマット詳細を取得する@<code>{--list-formats-ext}というオプションがあります@<fn>{v4l2-usage}。

//footnote[v4l2-usage][@<href>{https://leico.github.io/TechnicalNote/Linux/webcam-usage}がとても役立ちました]

//cmd{
$ v4l2-ctl -d /dev/video4 --list-formats-ext
ioctl: VIDIOC_ENUM_FMT
	Type: Video Capture

	[0]: 'MJPG' (Motion-JPEG, compressed)
		Size: Discrete 1920x1080
			Interval: Discrete 0.017s (60.000 fps)
			Interval: Discrete 0.033s (30.000 fps)
			Interval: Discrete 0.040s (25.000 fps)
			Interval: Discrete 0.050s (20.000 fps)
			Interval: Discrete 0.100s (10.000 fps)
		Size: Discrete 1600x1200
			Interval: Discrete 0.017s (60.000 fps)
			Interval: Discrete 0.033s (30.000 fps)
			Interval: Discrete 0.040s (25.000 fps)
			Interval: Discrete 0.050s (20.000 fps)
			Interval: Discrete 0.100s (10.000 fps)
		Size: Discrete 1360x768
			Interval: Discrete 0.017s (60.000 fps)
			Interval: Discrete 0.033s (30.000 fps)
			Interval: Discrete 0.040s (25.000 fps)
			Interval: Discrete 0.050s (20.000 fps)
			Interval: Discrete 0.100s (10.000 fps)
		Size: Discrete 1280x1024
			Interval: Discrete 0.017s (60.000 fps)
			Interval: Discrete 0.033s (30.000 fps)
			Interval: Discrete 0.040s (25.000 fps)
			Interval: Discrete 0.050s (20.000 fps)
			Interval: Discrete 0.100s (10.000 fps)
		Size: Discrete 1280x960
			Interval: Discrete 0.017s (60.000 fps)
			Interval: Discrete 0.033s (30.000 fps)
			Interval: Discrete 0.040s (25.000 fps)
			Interval: Discrete 0.050s (20.000 fps)
			Interval: Discrete 0.100s (10.000 fps)
		Size: Discrete 1280x720
			Interval: Discrete 0.017s (60.000 fps)
			Interval: Discrete 0.020s (50.000 fps)
			Interval: Discrete 0.033s (30.000 fps)
			Interval: Discrete 0.050s (20.000 fps)
			Interval: Discrete 0.100s (10.000 fps)
		Size: Discrete 1024x768
			Interval: Discrete 0.017s (60.000 fps)
			Interval: Discrete 0.020s (50.000 fps)
			Interval: Discrete 0.033s (30.000 fps)
			Interval: Discrete 0.050s (20.000 fps)
			Interval: Discrete 0.100s (10.000 fps)
		Size: Discrete 800x600
			Interval: Discrete 0.017s (60.000 fps)
			Interval: Discrete 0.020s (50.000 fps)
			Interval: Discrete 0.033s (30.000 fps)
			Interval: Discrete 0.050s (20.000 fps)
			Interval: Discrete 0.100s (10.000 fps)
		Size: Discrete 720x576
			Interval: Discrete 0.017s (60.000 fps)
			Interval: Discrete 0.020s (50.000 fps)
			Interval: Discrete 0.033s (30.000 fps)
			Interval: Discrete 0.050s (20.000 fps)
			Interval: Discrete 0.100s (10.000 fps)
		Size: Discrete 720x480
			Interval: Discrete 0.017s (60.000 fps)
			Interval: Discrete 0.020s (50.000 fps)
			Interval: Discrete 0.033s (30.000 fps)
			Interval: Discrete 0.050s (20.000 fps)
			Interval: Discrete 0.100s (10.000 fps)
		Size: Discrete 640x480
			Interval: Discrete 0.017s (60.000 fps)
			Interval: Discrete 0.020s (50.000 fps)
			Interval: Discrete 0.033s (30.000 fps)
			Interval: Discrete 0.050s (20.000 fps)
			Interval: Discrete 0.100s (10.000 fps)
	[1]: 'YUYV' (YUYV 4:2:2)
		Size: Discrete 1920x1080
			Interval: Discrete 0.200s (5.000 fps)
		Size: Discrete 1600x1200
			Interval: Discrete 0.200s (5.000 fps)
		Size: Discrete 1360x768
			Interval: Discrete 0.125s (8.000 fps)
		Size: Discrete 1280x1024
			Interval: Discrete 0.125s (8.000 fps)
		Size: Discrete 1280x960
			Interval: Discrete 0.125s (8.000 fps)
		Size: Discrete 1280x720
			Interval: Discrete 0.100s (10.000 fps)
		Size: Discrete 1024x768
			Interval: Discrete 0.100s (10.000 fps)
		Size: Discrete 800x600
			Interval: Discrete 0.050s (20.000 fps)
			Interval: Discrete 0.100s (10.000 fps)
			Interval: Discrete 0.200s (5.000 fps)
		Size: Discrete 720x576
			Interval: Discrete 0.040s (25.000 fps)
			Interval: Discrete 0.050s (20.000 fps)
			Interval: Discrete 0.100s (10.000 fps)
			Interval: Discrete 0.200s (5.000 fps)
		Size: Discrete 720x480
			Interval: Discrete 0.033s (30.000 fps)
			Interval: Discrete 0.050s (20.000 fps)
			Interval: Discrete 0.100s (10.000 fps)
			Interval: Discrete 0.200s (5.000 fps)
		Size: Discrete 640x480
			Interval: Discrete 0.033s (30.000 fps)
			Interval: Discrete 0.050s (20.000 fps)
			Interval: Discrete 0.100s (10.000 fps)
			Interval: Discrete 0.200s (5.000 fps)
//}

長いのでまとめると、

 * M-JPEG出力では1920x1080 60fpsまで対応
 * YUYV（YUV2）ベタ出力では720x480 30fpsまで対応（1920x1080時は5fpsまで）

という結果です。

=== おまけ：連続画像のタイル化

本文中に掲載したような連続画像をタイルで敷き詰めたものは、ImageMagickに含まれる@<code>{montage}コマンドで作るのが楽。

デフォでは余白が入りすぎるので、こういう感じ。

//cmd{
$ montage i_09[0-8].png -tile 3x3 -geometry +32+32 intermediate.png
$ convert intermediate.png -geometry 600x out.jpg
//}
