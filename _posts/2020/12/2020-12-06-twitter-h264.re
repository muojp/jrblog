---
layout: post
title:  "Twitter「動画ファイルをアップロードできません。詳細についてはこちらをご確認ください。」の対処"
date:   2020-12-06 16:10:00 +0900
---
= Twitter「動画ファイルをアップロードできません。詳細についてはこちらをご確認ください。」の対処

ある朝、PNG画像の山をFFmpegでシンプルにMP4（H.264）変換した。

//cmd{
$ ffmpeg -pattern_type glob -i '*.png' -vf scale=320:-1,framerate=12 out.mp4
//}

これをTwitterにアップロードしたところ、ツイートボタンを押した後で「動画ファイルをアップロードできません。詳細についてはこちらをご確認ください。」とエラーが出た。

//image[2020-12-06-twitter-error][Twitterでの動画アップロードエラー]{
//}

Twitterへアップロードできる動画の仕様は@<href>{https://help.twitter.com/en/using-twitter/twitter-videos, ここ}に書かれている。

 * 最小解像度: 32 x 32
 * 最大解像度: 1920 x 1200 （および 1200 x 1900）
 * アスペクト比: 1:2.39 - 2.39:1 の範囲（これらの値を含む）
 * 最大フレームレート: 40 fps
 * 最大ビットレート: 25 Mbps

が、条件を満たしているのに何故かアップロードできない。
なお、このヘルプ記載おそらく間違っていて、最大解像度の縦長側は1900ではなく1920までいけるのではないか。
関係ないけど、2020年末になっても60fps動画をサポートしないのちょっとイマイチ感もある。

さて、いろいろパラメータを変えてエンコードしたみたところ、色空間指定を@<code>{-pix_fmt yuv420p}としたら無事に投稿できた。

@<raw>{|html|<blockquote class="twitter-tweet"><p lang="ja" dir="ltr">おじさんが休日にハンドドリップするだけの動画 <a href="https://t.co/9WslRnUIwE">pic.twitter.com/9WslRnUIwE</a></p>&mdash; Kei Nakazawa (@muo_jp) <a href="https://twitter.com/muo_jp/status/1335389227035422722?ref_src=twsrc%5Etfw">December 6, 2020</a></blockquote> <script async src="https://platform.twitter.com/widgets.js" charset="utf-8"></script>}

エンコードオプションはこういう感じ。

//cmd{
$ ffmpeg -pattern_type glob -i '*.png' -vcodec libx264 -pix_fmt yuv420p -vf scale=320:-1,framerate=12 out.mp4
//}

1枚/秒で撮影した連続写真を12fpsにしているので、12倍速動画である。
FFmpegというかlibx264は色空間オプション指定を省略すると@<code>{yuv444p}で出力するけれど、Twitterではサポートしていないということだろう。

調べるとFFmpegのtracに「デフォルト値がyuv444pでは一部の環境で再生できず問題がある」旨のissueが起票されてはいた@<fn>{fn-ffmpeg-trac}（2012年）。
考えてみると24bppで色情報を落とさずエンコードしようという意味合いでは正しいデフォルト挙動な気がするし、Twitter側としても再生非対応環境が一定存在するなら非サポート扱いにするのもわかる。
むやみにサーバー側で再エンコードすると資源を消費するし、人々が事情を察せずに「Twitterへ動画アップロードしたら画質下がった」と怒りがちだし。
なやましい。

//footnote[fn-ffmpeg-trac][@<href>{https://trac.ffmpeg.org/ticket/2214}]
