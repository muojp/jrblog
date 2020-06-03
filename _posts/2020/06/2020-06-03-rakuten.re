---
layout: post
title:  "楽天モバイルおためしユーザーのためのSMS/着信Slack通知システム構築"
date:   2020-06-03 15:10:00 +0900
---
= 楽天モバイルおためしユーザーのためのSMS/着信Slack通知システム構築

== なんとなく楽天モバイル

楽天モバイルがRakuten Miniを1円販売・UN-LIMIT回線も1年間無料というなかなか激しいキャンペーンを実施中です。
MNP予約番号を発行依頼している間にキャンペーンが終わったらイヤとか、長く使えるか分からないので様子見など、ともかくMNPせずに回線追加した方がとても多いキャンペーンだと思います。

私の手元にもRakuten Miniが届きました。
まあしばらく使ってみようと思っていたら、メルカリへのログインでさっそく旧回線のことを思い出す羽目になりました（@<img>{2020-06-03-rusurock-mercari-login}）。

//image[2020-06-03-rusurock-mercari-login][メルカリへのログインでSMS認証]{
//}

== SMSと着信通知をなんとかしたい

私は旧回線を一旦音声専用プランへ落として維持していますが、通話用スマフォを持ち歩くのがいやだったので、きっと一家に2-3台転がっているけれど今後使うことはなさそうなHuawei製のポケットWiFiとRaspberry Pi風デバイスを使い、少なくとも3Gの停波まで5年ぐらい生きていけそうなSMS/着信通知の仕組みを作ってみました。

USBモデムを使ってSMSを送るサンプルはよく見かけますが、受信とPDU処理に着信番号表示・既読管理とストレージ領域確保まで含めるとサンプルは皆無なので、今後似た取り組みを検討する方の参考になれば幸いです。

実は数年前にSMSを他の電話番号へプロキシするAndroid用アプリを公開していたのですが、Google Playの規約アップデートについていくのがしんどくなって公開停止してしまったのと、受け取り側媒体をSlackにして1対1しばりから脱却（Google Voice信者なので）したかったのでPC+USBモデムの構成にしてみました@<fn>{still-android}。

//footnote[still-android][Android端末で受け取ったSMSをSlackへ転送すればよいのではという話ですが、実はもともと通話を自前VoIPへプロキシするフルコースを考えていたのでPCのほうが扱いやすいと考えた次第です]

=== 3GモデムでもATコマンドとなかよく

USB接続のシリアルモデムなのでATコマンドを叩けばよい、には違いないのですが、実際やってみるとなかなか厄介です。

それなりに標準化されていた太古のモデムとは違い、3G/LTEモデムモジュールは基本的に@<b>{データ通信モジュールとして動くこと}を最優先にしており、これらのモジュールを使ってSMSや通話機能を使うことはまずありません。

=== 対象デバイス=HW-01C・D25HW

今回の取り組みにあたって手持ちの複数USBドングルを調査しましたが、ドコモ用によく出回っているLG Electronics製のもの（L-02C、L-03Dなど）は今回必要な機能を実装していない（少なくともユーザーがアクセスできる形では）ので、対象外です。

どうも3G以降のUSBモデムで自由度が高いのはHuaweiという雰囲気があります。

== 留守録? rusurock

結果は@<href>{https://github.com/muojp/rusurock}で公開しました。

実装済みの機能は次のとおりです。

 * USBモデムを初期化する
 * SMSの受信を非ポーリングでおこなう
 * 受信したSMSをSlackのwebhookへ投げてSIM上から削除する
 * 着信をうけたら発信者番号をSlackへ通知し通話を自動切断する

以下ではそれぞれの中身を紹介していきます。

== 優雅にSMSを受信し、適宜ストレージをクリアする

まずは主にSMSをSlackへ流せるようにするまでを、簡単に実装紹介しつつ解説します。

=== USBモデムとの接続

モデムに接続します。
権限の都合上、実行ユーザーはdialoutグループに参加させておくのが無難です。

コード内で@<code>{/dev/ttyUSB0}と決め打ちしていますが、これはHW-01Cの場合にモデムのシリアルデバイスを1つしか認識しないためです。
割とよくあるUSBモデムは3デバイス（ドライバ用のUMSを含めると4デバイス）構成で認識するようです。
実際、L-02C/L-03Dは@<code>{/dev/ttyUSB0-2}の3デバイス認識しました。
残念ながら日本国内で合法利用できてドコモ回線につながるHuawei製モデムをひとつしか持っていなかったので、他の事情を調べられていません。
他のHuawei製モデムのサポートを加えたいぞという方は@muo_jpに端末を貸して下さい...。

=== SIMから電話番号を取得

モデムが最低限ATコマンドへ応答することを確認したら、@<code>{AT+CNUM}コマンドでSIMの電話番号取得をします。
この作業自体はSMS受信と着信検出において必要ありませんが、ここで電話番号を取得できないということはSIMが正しく刺さっていないか何かのトラブルがあると想定できるので、エラー検出用に入れた感じです。
もちろん、@<code>{AT+CPIN?}を発行してPINロック状態を確認するなどもありでしょう。

=== Slackへ自電話番号通知

無事に電話番号を取得できたら、SMS/着信監視の開始をSlackへ通知します。
ホストのRaspberry Piなり何なりの端末側ネットワークの疎通確認も兼ねています。
後述しますが、systemdで起動自動化した場合は端末再起動時点でSlack通知が流れることを期待できるので、watchdogタイマーが発動してリブートかかったなどのイベントを捕まえられて便利かもしれません。

続けて、@<code>{AT+CLIP=1}コマンドで着信にまつわる設定をします。
これは後述します。

=== 新規SMS受信時にプッシュ通知

標準状態では、新たなSMSが届いてもシリアルモデム経由の通知をしてくれないのでポーリングが必要です。
ポーリングは優雅な方法とはいえません。
プッシュ用の仕組みが用意されており、せっかくなのでこれを使います。
具体的には@<code>{AT+CNMI=2,1}コマンドを発行しています。
ここで受け取るパラメータの数は割とデバイス世代依存が激しいので、試行錯誤して探すのが無難です。
このコマンドの発行後にSMSを新規受信すると、

//cmd{
+CMTI: "SM",18
//}

というイベントが流れてきます。
第1パラメータがSMのものは、SIM内のメッセージストレージに新データが格納されたから読みに来いという意味です。
ほかにもMEやSRが来る可能性がありますが、実際に届いたことがないためよくわかりません。
第2パラメータはストレージ内のインデックスで、これを指定して@<code>{AT+CMGR=N}コマンドを実行すると該当メッセージを読み出せます。

=== 未読SMSを取得

しかし実装を見ていただくとわかるように、実際発行しているのは@<code>{AT+CMGL}です。

しばらくぶりにSIMを網へ接続すると、センター側で保持されていたSMSがまとめて降ってくることになります。
センターから引き取ったら適宜@<code>{+CMTI}通知が流れてきますが、たまたまUSBシリアル部分でのデータ受信に失敗したりプログラムのバグで複数メッセージをうまく処理できずに@<code>{AT+CMGR}での読み出しを網羅できないかもしれません。
このような場合でも、とりあえず@<code>{AT+CMGL}を発行しておくと、SIM内に未読メッセージがあれば取得する（当然いましがた届いたメッセージも読み出す）挙動をします。
もしも何かの理由で未読のまま放置されてしまったメッセージを救い出すことができるので、これは望ましい挙動です。
読んだSMSはどんどんSlackへ投げつけます。

なお、あちこちの送受信例を見ると、@<code>{AT+CMGL}コマンドで取得するメッセージ種別の指定では@<code>{AT+CMGL="ALL"}のような文字列を食ってくれそうな感じがしますが、@<code>{AT+CMGF=1}でテキストモードへ入れてからこれを発行してもエラーが戻ってきたり空リストが戻ってくるなど、挙動はまちまちでした。
PDUモードでの指定はサンプルがあまり見当たりませんが、参考文献（末尾参照）を読みつつ調べていきます。
0が未読取得、1が既読取得です@<fn>{no-restore-unread}。

//footnote[no-restore-unread][テスト用にSIM内の既読SMSを未読へ戻せると便利ですが、そういう機能はなさそうです]

=== 読み出したSMSを削除

SIM内のストレージも無限ではないので、既読のメッセージはどんどん消していきます。
安心運用するなら、Slackへの通知送信が少なくともネットワーク的には成功したことを確認したほうがよいかもしれません。
念のため手元SQLiteや適当なファイルへWAL的な書き出しをして送信管理すればデータが失われる危険性を下げられますが、データを失ったら再送してもらう使い方（≒SMS認証）をする分にはそこまでしっかり作らなくても良いでしょう。
きっちりしたい場合はgammu/wammuのようなソリューションに流れたほうがよいでしょう。

さて、@<code>{AT+CMGD}コマンドはSIM内に保存されたメッセージの削除を担います。
デフォルト状態ではメッセージ番号を指定して個別削除ですが、第2パラメータへ1-4の数値を指定すると一括削除が可能です。
一括削除モードでは、第1パラメータで指定されたインデックスは無視されます。

一括削除のモード4を指定すると未読分も含めた削除をしてしまうので、なかなかシビれます。
今回の用途では@<code>{AT+CMGL}で一括読み出しをした際の既読管理を信じ、1を指定して既読分の全削除が無難でしょう。

== 着信通知

これもなかなか厄介でした。

=== 相手の電話番号が分からない

着信情報のなかで最も重要なのは相手の電話番号です。
しかしこれはデフォルト状態で通知されてきません。

ここで、さきほどSMS側の説明でスキップした@<code>{AT+CLIP}コマンドです。
これを適切に発行すると、着信イベント発生時に相手の番号が@<code>{+CLIP}として流れてくるようになります。
パラメータの詳細は適宜マニュアルを確認してください。

関連として@<code>{AT+CLIR}コマンドを発行すると、番号通知のない（いわゆる184）電話を拒否するような設定もできそうですが、あまり調べていません。

=== 音声を送受信できない

Google Voice信者（2回目）としては、着信に対してクールなメッセージを流してボイスメッセージ記録・音声認識にかけてSlackへ流したいところです。
もちろん@<href>{https://github.com/muojp/rusurock, rusurock}という名前も留守録から来ています。

@<href>{https://habr.com/ru/post/193586/, このロシア語のblog}にHuawei製e1550モデムへ音声を流し込むサンプルPerlスクリプトがあり、これならいけるかもとあれこれ試してみました。

しかし残念ながら手元のHW-01Cでは音声の送受信に成功していません。
そもそもUSBシリアルポートを1つしか認識していない時点で、この端末はデータ通信に振り切った仕様と考えたほうがよさそうです。
一部のHuawei製モデムでは音声モードへ入れる拡張コマンドの発行が必要という情報を見かけましたが、該当コマンドは未実装（非公開?）のようでした。

Huaweiのモデムの多くはひっそりと音声通話機能を積んでいて、SIMロックを解除&機能ロックを頑張って解除すれば使えるという話も見かけましたが、ひとまず手を出していません。

=== 通話切断できずハマり通話料お布施の回

今回試したHW-01Cの場合、電話をかけること自体は@<code>{ATD}コマンドで可能です@<fn>{l-02c-no-atd}。

//footnote[l-02c-no-atd][L-02Cではこれ自体が通らずにだめな感じでした]

かかった電話は切断しなければなりません。
通話のハングアップについてATコマンドを調べると、@<code>{ATH}コマンドで切れそうな例が見つかりますが、実際にはこれでは全然切断できません。
@<code>{ATD}コマンドで架電し、切断したつもりになって5分間つながりっぱなしで青ざめるみたいなことが複数回ありました。

@<code>{ATH}ではなく@<code>{AT+CHUP}コマンドを発行すると、自分側から架電した場合でも受電した場合（相手側では呼び出し音が鳴っている状態）でもほぼサックリ通話を切れます。
ほぼというのは、受電中にATコマンドでの制御が極端に通りづらくなる場面があり、タイミングを確実に読めるとはいえないためです。
感覚的には2-3コール中にコマンドが通って相手側では「通話終了」表示されます。

この間、モデムからは@<code>{RING}というメッセージと@<code>{+CLIP}応答が数秒おきに発生します。
当然これらは継続イベントなので、適切にステート管理して多重通知を抑止するのが本来のあり方です。
しかし受電→何もしないうちに相手側で切断した場合はハングアップイベントが飛んでくるわけではないので、何も考えずに作ると終了条件のないループに陥いるパターンが生じます。

今回は、受電すると速やかに切断要求を出す構造にして、2-3回程度のRING/着信番号通知イベントがSlack側へ流れていくのは許容しました。

== 実戦投入

rusurockは、@<href>{https://github.com/muojp/rusurock#usage, README}のとおりSlackのWebhook設定を環境変数へ記載してNodeのプロセスを起こすだけで簡単起動できます。
しかしさすがに手動で頻繁にコマンドを叩くのはいやなので、systemdの設定ファイルを最低限書きます。

=== systemd向けの設定例

//emlist{
[Unit]
Description=rusurock daemon

[Service]
Type=simple
EnvironmentFile=/home/muo/.rusurock
ExecStart=/usr/bin/node /home/muo/rusurock/

[Install]
WantedBy=multi-user.target
//}

systemd設定のインストール方法はインターネットにいっぱい書かれているので適宜参考にしてください。

より実用的に利用するなら、開始だけではなく停止設定も書きたいところです。
また、OS起動時点ではttyUSB0デバイスが見えていない可能性が高いので、ちゃんとデバイス列挙と初期化試行をウェイト込みでさせたほうがよいかもしれません。

=== 死活監視

死活監視も軽く検討しておきます。
アイドル状態1分おきにRSSIをMackerelへ投げるような方法で十分でしょう@<fn>{csq}。

//footnote[csq][@<code>{AT+CSQ}コマンドを発行して戻りの第1パラメータを解釈してdB値でMackerelへ流すのがよいでしょう]

== USBモデム探窟家へのヒント

=== モデムとして動き始めるまでに時間がかかる

ポケットWi-Fiは、起動するとモバイルネットワークへ接続してWi-Fiホットスポットになろうとします。
本来の機能を考えれば当然の挙動なのですが、接続試行中はLinux側からのモデム制御へ応答しなくなるので、ATコマンドを通したい側からすると邪魔な仕様です。

この起動即接続挙動はWindows用のHuawei設定ツールで変更可能のようなので、気になる方は設定してください。
私の手元の環境では本システムが平均再起動間隔100万秒ぐらいの想定で、そのうち最初の30秒程度は必要な犠牲と考えて気にしないことにしました。

=== 複数の仕様書/マニュアルにあたる

データ通信用デバイスのマニュアルには音声系のコマンド情報が含まれておらず、網羅的な情報を1つの情報源から得るのは困難です。

今回は4つのドキュメントを読みつつ仕様を調べていきました。

 * @<href>{http://download-c.huawei.com/download/downloadCenter?downloadId=93984&version=375409&siteCode=, Integration Userguide for Huawei Dongles}
 * @<href>{http://download-c.huawei.com/download/downloadCenter?downloadId=51047&version=120450&siteCode, HUAWEI ME906s LTE M.2 Module V100R002 AT Command Interface Specification}
 * @<href>{https://www.nttdocomo.co.jp/binary/pdf/support/manual/HW-01C_J_All.pdf, HW-01C取扱説明書}
 * @<href>{https://welinkjapan.co.jp/pdf/GOSUNCN%20AT%20Command%20Reference%20Guide%20of%20Module%20Product%20ME3630_V2.0.pdf, GOSUNCN ME3630 AT Command Reference Guide}

=== ATコマンド探索

たとえば@<code>{AT+CNMI}コマンドであれば、@<code>{AT+CNMI=?}と発行すればモデム側でサポートする値の範囲を返してくれます。
世の中にあるサンプルと手元のモデムのサポートするものが一致しない場合（よく一致しません）、とにかく@<code>{?}つきでコマンドを発行して探索する感じでした。

=== OEMを知り、利用可能な端末の当たりをつける

SMS/着信通知の自動化に都合のよい端末が手元にあるかという問題です。

スマフォでのテザリングが開放されていなかった時期にポケットWiFi類がアホみたいに売れた結果、現在はメルカリで安価に購入可能です。
だいたい送料込み1,000円が相場なので、余っているラズパイと組み合わせるのにほどよい価格ではないでしょうか。

実は、「Pocket WiFi」はソフトバンク（旧イー・アクセスから）の登録商標です@<fn>{pocket-wifi}。

このため、今回使っているドコモのHW-01Cは公式には「Pocket WiFi」ではありません。
EMOBILE時代にばら撒かれていた端末にはSIMロックフリーの端末も多いはずなので、案外現行のソフトバンク/ドコモSIMを救えるパターンも多いかもしれません。
しかしau回線で頑張ろうと思ったらLTEから外れた瞬間にCMDA2000の上昇負荷で命を落としそうなのでやめましょう。

今回利用したHW-01CはD25HWをベースとしてドコモ向けカスタマイズ&USBインタフェースをmicroへ変更したもので、ハードウェア世代はだいぶ古いです。
もう少し新しい世代の製品を使えば音声もうまく扱えるかもしれません。
このあたりを多少手間かけて調べていくと、別名出品/販売ゆえ見逃されてお安く入手できる端末があったりなかったりします。

//footnote[pocket-wifi][@<href>{https://ja.wikipedia.org/wiki/Pocket_WiFi}]
