---
layout: post
title:  "冬を乗り切れるか不安になってきたので意識低いホームオートメーションをする"
date:   2022-12-22 00:00:00 +0900
---
= 冬を乗り切れるか不安になってきたので意識低いホームオートメーションをする

最近寒すぎてこの冬を乗り切れるか怪しい

弊宅にはNature Remoが導入済みであり、超簡易的にエアコンのP制御を組みリモートワーク環境の生産性確保を試みているが、契約している電力会社がどんどん燃料調達に関する値上げをしている中ではこの冬エアコンをまともに使える気がしない

そうなると時代は灯油だ オイルを燃やして暖を取るほかない

ここで問題になるのがIoT的な電源制御であり、廉価帯の石油ファンヒーターはもちろん上位モデルでも基本的にリモートで電源ONにする機能は入っていないように見受けられる（安全上の配慮、法令によるものと自主規制の組み合わせだろう）

致し方ないので点火は手動でおこなうものとする

問題はいつ点火するかであり、リモートワーカーとしては自分のパフォーマンスが落ちてくる前にこれを適切におこなう必要がある

基本的にリモートワークのパフォーマンスは室温による関数であり、つまり室温をほどよく把握しつづけることがパフォーマンスにつながる

Nature Remoのスマホアプリを開けば室温が分かるが、アプリを四六時中開いているのは効率悪い

室温計を追加購入したくはない、となるとやるべきは作業中のPC上に室温をほどよく表示することである（@<img>{2022-12-22-nature-remo-gnome}）

//image[2022-12-22-nature-remo-gnome][Nature Remoで取得した室温をGNOMEで表示する]{
//}

== 意識の低いホームオートメーションとはなにか

 * 全世界で自分だけ使えればいい（自分の特殊ニーズを満たせればよい）
 * 全自動は目指さない。自分を操作インタフェースとして使うことで目的を達成できるなら積極的に使う
 * bashと定番コマンド群でさらっと書く
 * 書いたら書いた通りに動き、簡単に捨てられる

== Nature RemoのAPIから室温を取り出す

先人の知見にありがたく乗っかるものとする

@<href>{https://blog.yuu26.com/nature-remo-api-tutorial/}

複数のNature Remo端末を登録している場合はデバイスIDで更にフィルタするのが無難

//emlist{
  TEMPERATURE=`curl -s -X GET "https://api.nature.global/1/devices" -H "Authorization: Bearer $NATURE_TOKEN" \
  | jq ".[] | select(.id == \"$DEVICE_ID\") | .newest_events.te.val"`
//}

こんな感じだろう

APIから取れる値の更新間隔は1分か2分ごとであるように思うが今回はひとまず気にせず30秒間隔でデータを取得しておく

== ZenityでGUI表示する

手元のデスクトップ環境はUbuntu 22.04であり、標準のGNOME Desktopで動作している

この環境においてざっくり可視化はどうすればいいか

@<code>{curl}で値を取って@<code>{jq}で加工する、だけだとターミナルのタブを1枚専有したりいちいち見に行かなきゃいけなくて面倒なのでみんなのzenityでGUI化する

@<href>{https://help.gnome.org/users/zenity/stable/progress.html.ja, Zenity - プログレスバー}

公式ドキュメントが充実しているので機能カタログを読んでいく

数値変化をそこそこ視覚的に分かりやすく表現し、認知の負荷を下げるのが目的であり、やりたいことを実現するには単発プログレスバーを表示しっぱなしにしておけばいい

 * 標準入力をあけっぱなしにして新しい値を適宜受け取るという扱いやすい仕様
 * 行頭に# を入れてやるとラベル更新、なければ数値更新という割り切った仕様も良い
 * OKとかCancelとかは別に要らないけどわざわざ外すほどでもない
 * 複数の値をプログレスバー表示したい（CO2濃度とか）、となったらまあ適宜ちゃんとやればよい

要求を満たすものはこんな感じ、コマンドからさらっとGUIを制御できるというだけで満点である

//emlist{
(
while :
do
  TEMPERATURE=15
  PROGRESS=0
  echo "$PROGRESS"
  echo "# $TEMPERATURE"
  sleep 30
done
  ) |
zenity --progress \
  --title="Room temperature" \
  --text="fetching..." \
  --percentage=0
//}

== 入力値を適当にクランプする

プログレスバーを扱う際に面倒なのは想定外の範囲の入力値の処理であり、つまりクランプが必要

bash script類で小数込みの値をクランプするのがまあまあ面倒そう、と調べてみたらawkを使う策が提示されておりすばらしい

@<href>{https://stackoverflow.com/a/27688507}

//emlist{
  P=`awk '$0<15{$0=15}$0>30{$0=30}1' <<< "$TEMPERATURE"`
//}

これで15度から30度の範囲外の値を上限値と下限値へクランプできる

== bcで気温をプログレスバーの0-100値へマップする

シンプルな数値計算でもbash組み込みのオペレータで演算すると面倒@<fn>{bash-operators}というのは非常に有名事案につき、bcを使う

bcの素晴らしい点は@<code>{-l}オプションにより小数込みの演算を通せるところである

//emlist{
  PROGRESS=`echo "(100/15)*($P-15)" | bc -l`
//}

//footnote[bash-operators][bash含めてシェルスクリプトを正しく書くのは想像の何倍も困難だと知っているので厳密な理解をしようという気持ちはとうに放り捨てている]

== 出力微調整やデータの扱いやすさを検討して仕上げる

 * 念のため時刻・温度のペアでログをローカルディレクトリへ書き出すようにしておく
 ** あとからGoogle Sheetsあたりへ放り投げるようにすればクラウド化もできて完璧
 * データとしてはTSVにしておくと楽
 * dateをそのまま使うと行末改行が入るので一旦パラメータに受けて回避してる
 ** tabの出力は@<code>{echo}に@<code>{-e}オプションが必要
 * zenityで表示したダイアログ側の後始末をしていないのでダイアログを閉じても最大30秒ほどプロセスが残り、次回の温度取得が走ってから終了するが特に問題ないのでそのままにしておく

== スクリプト全文

//emlist{
#!/bin/bash

LOGFILE='/home/user/templog.txt'
NATURE_TOKEN='NATURE_REMO_TOKEN'
DEVICE_ID='DEVICE_UUID'

(
while :
do
  TEMPERATURE=`curl -s -X GET "https://api.nature.global/1/devices" -H "Authorization: Bearer $NATURE_TOKEN" | jq ".[] | select(.id == \"$DEVICE_ID\") | .newest_events.te.val"`
  P=`awk '$0<15{$0=15}$0>30{$0=30}1' <<< "$TEMPERATURE"`
  PROGRESS=`echo "(100/15)*($P-15)" | bc -l`
  echo "$PROGRESS"
  echo "# $TEMPERATURE"
  echo -n `date +%Y-%m-%dT%H:%M:%S` >> $LOGFILE
  echo -e "\t$TEMPERATURE" >> $LOGFILE
  sleep 30
done
  ) |
zenity --progress \
  --title="Room temperature" \
  --text="fetching..." \
  --percentage=0
//}

== その他

.NET 7も出たことだしMAUIで組めばいいのでは、と思うところはあるがまだLinuxが公式サポート環境に入っていないので今回はナシである（たぶん問題なくMonoで動く範疇ではある）
