---
layout: post
title:  "IPoEでネット帰宅難民になった / Rakuten Miniテザリング環境からフレッツひかりのIPoE環境へSSH/IPv6でインターネット帰宅する"
date:   2020-12-14 09:10:00 +0900
---
= Rakuten Miniテザリング環境からフレッツひかりのIPoE環境へSSH/IPv6でインターネット帰宅する

遅いネット回線でリモートワークするのは本当にしんどいです。
速度改善の方法は色々ありますが、フレッツひかり回線における手軽な策として接続方式をPPPoEからIPoEへ切り替える手法が有名です。

実際、私の手元でもIPoE化によって夕方時間帯の転送速度改善に一定の効果が見られました。
それと引き換えに、手持ち機材ではネット帰宅@<fn>{fn-back-to-home}できなくなりました。

//footnote[fn-back-to-home][出先からSSHやRDPで自宅内PCへアクセスすること]

== IPv4とv6とDS-Lite

フレッツひかりのIPoE接続はv6ネイティブです。
家庭用ルーターとその下にぶら下がったデバイスはDHCPv6によって割り当てられたIPv6アドレスを使ってインターネットに出ていきます。
逆に、そのままではIPv4のネットワークへ出られません。

このため、IPv4側は各種のv6移行技術を利用することになり、私が契約したISPではDS-Liteを利用します。
DS-Liteは家庭のルーターとISP側の提供するDS-Liteゲートウェイ（AFTR）との間でIPv6上にv4のトンネルを掘り、そこにv4パケットを流す構造です。

//graph[2020-12-04-ipoe][blockdiag][IPoE接続時のインターネットアクセス]{
blockdiag {
  group {
    label = "自宅";
    DeviceIPv4 [label="IPv4端末"];
    DeviceIPv6 [label="IPv6端末"];
  }
  group {
    orientation = "portrait";
    label = "自宅ルーター";
    RouterIPv4 [label="IPv4ゲートウェイ"];
    RouterIPv6 [label="IPv6ゲートウェイ"];
  }
  ISP;
  DeviceIPv4 -> RouterIPv4;
  DeviceIPv6 -> RouterIPv6;
  RouterIPv4 -> RouterIPv6;
  RouterIPv4 -> RouterIPv6 [label="カプセル化", fontsize=8];
  RouterIPv6 -> ISP [label="IPoE"];
}
//}

そしてここが見落とされがちなのですが、あいにく@<b>{グローバル側からv4へのインバウンドポートを固定する方法は提供されません}。
つまり、IPoE環境においてIPv4だけでは外出先から自宅内サーバーへリモートデスクトップでログインしたり、SSH経由でVSCodeのリモート編集をしたりといった@<b>{現代における自然なインターネット呼吸ができません}。

家庭内からインターネット上のホストへとVPN接続を張ることで、外部→内部の通信を受けることは可能ですが、せっかくIPv6アドレスが割り振られているのにそれを使わずトンネル用の外部サーバーを契約するのも癪なので、家庭に転がっている機材を利用してIPv6で家庭内サーバーへとSSHできるようにしてみます@<fn>{fn-ssh}。

//footnote[fn-ssh][自宅内のサーバーへSSHでログインしたら後はなんでも出来るので、ここをゴールとしました]

2020年も終わろうとしているのだし、さらっとend-to-endでIPv6経由のインターネット帰宅セットを組めるだろうと実験を始めたら、想像以上にサーバー・クライアント両面でハードルがあったので、それぞれ整理してみました。

== 元々のネットワーク構成

今回の対応をおこなう前の段階で、自宅のネットワーク機器と用途は次の通りでした。

 * Google Nest Wi-Fi（家庭内通信用）
 * TP-Link Archer A6（IPoEルーター用）

//graph[2020-12-04-network][blockdiag][network]{
blockdiag {
  HomeDevices [label="自宅のスマフォやPC"];
  group {
  orientation = portrait;
    label = "Archer A6";
    IPv4Gateway;
    IPv6Gateway;
  }
  AFTR;
  IPv4Internet [label="IPv4インターネット"];
  GoogleNestWifi [label="Google Nest Wi-Fi"];
  HomeDevices -> GoogleNestWifi [label="Wi-Fi"];
  GoogleNestWifi -> IPv4Gateway [label="有線"];
  IPv4Gateway -> IPv6Gateway;
  IPv6Gateway -> AFTR [folded, label="IPoE"];
  AFTR -> IPv4Internet;
}
//}

PCやスマフォはGoogle Nest Wi-Fiが噴いているアクセスポイントに接続する構造です。

== 自宅ゲートウェイをOpenWrtで作る

=== さらばArcher A6（のルーター機能）

今回、IPoE対応でそれなりのスループットが出るルーターとしてTP-Linkの激安Wi-FiルーターArcher A6を購入したのですが、この機材ではどう設定してもIPv6でぶら下がっているホストへのインバウンド通信を通せませんでした。
一時期騒がれたNTT本家のひかり電話対応ホームゲートウェイのようなフレッツ網内からインバウンド通信を素通しするマズい状態を回避できるのは嬉しいのですが、ポリシーがちょっと厳しすぎます。
Archer A6はDS-Liteを有効にした状態ではND-Proxy（おそらくパケットフィルタの意を含む）が基本的にONでパススルー設定にできず、ND-Proxy以外をうまいことオフにしつつ手元機材がIPv6アドレスを取れる状態に持っていけませんでした（それでいて、WAN側からのpingを拒否しても普通にpingは素通しし続ける、、なぜ、、）。
このため、Archer A6のルーター機能は使わないことにしました。

ルーターのファームウェアを書き換えることで、求める機能を実現できる可能性はありますが、残念ながらArcher A6の日本版ハード（TP-Link製品は販売地域ごとにハードウェア仕様もファームウェアも異なったりします。無線レギュレーション差を考えると当然といえば当然ですが）のソースコードは本稿執筆時点で未公開でした（@<img>{2020-12-04-no-a6}）@<fn>{fn-a6-gpl}。

//image[2020-12-04-no-a6][TP-Link社のGPLコードセンターには日本版Archer A6のコード公開なし]{
//}

//footnote[fn-a6-gpl][@<href>{https://www.tp-link.com/jp/support/gpl-code/}]

念のためTP-Link社へコード開示リクエストを出したうえで、こいつのルーター機能については一旦忘れておきます。
代わりに、別途構築するルーターの下にぶら下げてスイッチングハブ兼有線/無線変換器として利用します。

ルーター機能を使わないということはIPoE接続とLAN内へのアドレス配布を担うサーバーが別途必要になりました。

=== おはようRaspberry Pi 2

Archer A6の標準ファームウェアでは不可能な細かい設定をするために、多くの家庭に数枚は余っていると思われるRaspberry PiシリーズにOpenWrtをインストールします。
手元では、ちょうど余っていたRaspberry Pi 2を利用しました。
Ethernetが1ポートしかないのでUSB Ethernetアダプタを増設しましたが、USB 2.0のバスにもろもろがぶら下がることになるので、ネットワーク速度はそれなりに制限され、実測値で上下ともに80Mbps程度でした。

Archer A6では300-700Mbps出るので、かなり遅いといえますが、Raspberry Pi 2はあくまでもコンフィグ確認用の機材なので一旦気にしないことにします。

OpenWrtをインストールし、DHCPv6でアドレスを取れたらOpenWrtのリポジトリ（ここがv6対応で本当にありがたい）からdsliteパッケージをインストールしてDS-Lite設定をしたり、LuCIをインストールしたりと、割とありがちな環境構築をしていくと、平和に通信できます。

=== /64環境と相性最悪なGoogle Nest Wi-Fi

詳しくは別の機会にしますが、今回は宅内のWi-FiルーターにGoogle Nest Wi-Fiを使いたいという事情がありました。
この機材自体はIPoE非対応ながら、IPv4/v6デュアルスタックなネットワークにぶら下げることでv6機能が有効化されるという仕様なのですが、OpenWrt側でデュアルスタック化して通常のLinuxマシンからv4/v6のアドレスが共に取得出来る状態になっても「ISPがv6非対応」という非情なメッセージを吐いてv6非対応モードから動いてくれません。

どうもISP側で割り当てられるアドレスが/56の場合@<fn>{fn-hikari-denwa}にはv6対応モードで動作するようなのですが、ひかり電話絶対契約したくない呪いを受けた人間としてはこれを自然に達成することは不可能です。

//footnote[fn-hikari-denwa][ひかり電話を契約するとDHCPv6で/56のアドレスが降ってくることになっています]

次善の策としては、OpenWrt側でNAT6設定をしてデカいアドレス空間を見せてやることになります。
しかしOpenWrtの@<href>{https://openwrt.org/docs/guide-user/network/ipv6/ipv6.nat6, NAT6チュートリアル}を読みつつ設定を流し込んでみた限りではうまくアドレスを配布出来なかったので、結構厄介そうだなーと一旦投げました。
さらに、NAT6ではどのみち家庭内サーバーへのアクセスに際してポートフォワードが必要です。

結局@<b>{Google Nest Wi-FiはIPv4で動作させ、家庭内ネットワークは原則v4とする}方針でほとんど問題ないのでは、と考えました。

=== OpenWrtのグローバル側ポート開放

前述のとおり、IPoE接続のフレッツひかりにおいてインバウンドを受けられるのはIPv6のみです。
いっぽう、Google Nest Wi-Fiの制限のため家庭内の端末はIPv4となってしまいました。

このため、どうにかしてOpenWrtでIPv6を受けて、家庭内へと転送することにします。

//graph[2020-12-04-global-port][blockdiag][OpenWrtのグローバル側ポート開放]{
blockdiag {
  ISP [color = "#77FF77"];
  IPv6Gateway [color = "#77FF77"];
  IPv4Gateway;
  GoogleNestWifi [label="Google Nest Wi-Fi"];
  HomeServer;

  ISP -> IPv6Gateway -> IPv4Gateway;
  IPv4Gateway -> GoogleNestWifi [folded];
  GoogleNestWifi -> HomeServer;
}
//}

まずはデフォルトで著しく開放度の低いファイアウォール設定を変更し、グローバル側でSSH用に使うポート（今回は2022としました）を通せるように@<code>{/etc/config/firewall}へ次の内容を追記します。

//emlist{
config rule
	option name 'inssh'
	option target 'ACCEPT'
	option proto 'tcp'
	option dest_port '2022'
	option src 'wan'
//}

もちろん、LuCIで同等の変更をしてもOKです。

=== OpenWrtでのポート転送・tcpproxy経由Google Nest Wi-Fiへの通信路確保

続いて、OpenWrtが動作しているルーター機（Raspberry Pi 2）にて、IPv6で受けた通信をIPv4へと翻訳してGoogle Nest Wi-Fiへとポート転送する必要があります。

//graph[2020-12-04-v4-transfer][blockdiag][OpenWrt上でv6→v4転送]{
blockdiag {
  ISP;
  IPv6Gateway [color = "#77FF77"];
  IPv4Gateway [color = "#77FF77"];
  GoogleNestWifi [label="Google Nest Wi-Fi", color = "#77FF77"];
  HomeServer;

  ISP -> IPv6Gateway -> IPv4Gateway;
  IPv4Gateway -> GoogleNestWifi [folded];
  GoogleNestWifi -> HomeServer;
}
//}

プロキシサーバー設定により、Google Nest Wi-Fiの特定ポートまでパケットが届くことになります。

おそらくマイナーなパッケージだと思いますが、OpenWrtのリポジトリに@<href>{https://openwrt.org/packages/pkgdata_owrt18_6/tcpproxy, tcpproxy}というIPv6対応のTCPプロキシがあるので、これを利用します。
/etc/config/tcpproxyを開き、サンプル記述を参考に設定を書きます。

//emlist{
config tcpproxy
  option username 'nobody'
  option groupname 'nogroup'

config listen
  option local_port '2022'
  option remote_addr '192.168.1.100'
  option remote_port '3022'
//}

@<code>{local_port}はOpenWrtがグローバル側から着信させたいポート、@<code>{remote_addr}はGoogle Nest Wi-FiのWAN側アドレス（これはOpenWrtの設定でDHCPの固定割り当てをしておきましょう）を指定します。
@<code>{remote_port}は、Google Nest Wi-Fiから自宅内サーバーのSSHポートへとフォワードされる予定のポート（Google Wifiアプリで設定します）を指定します。
ややこしいですね。

そして、tcpproxyデーモンの起動タイミングがデフォルトでは早すぎて正常に起動しないので、だいぶ後のほうへと変更します。

//cmd{
# cd /etc/rc.d
# mv S50tcpproxy S97tcpproxy
//}

再起動すれば、tcpproxyデーモンが起動してくることを確認できるはずです。

Google WifiアプリでGoogle Nest Wi-Fi→自宅サーバーのポートフォワード設定をするのも忘れずに。
ここまで一通り設定できたら、当該ポートを経由して自宅サーバーへとSSHアクセスできることも確認しておきましょう。

====[column] 設定のバックアップと書き戻し

デーモン起動タイミングの変更に関連し、LuCIのインタフェースから「設定のバックアップ」を実行して得られる.tar.gzファイル内について注意が必要です。
このファイル内にパッケージ情報は残りますがrc.d以下の情報は失われてしまうので、他のルーター機へ設定を持っていく際には書き戻しついでに再度起動順の調整が必要そうです。

=== カメ専用Wi-Fiアクセスポイント

宅内を全部IPv4へ寄せた結果、もう2020年が終わるというのに静止画カメしか見れないなんて・・・、と辛い気持ちになったので、前述の激安ルーターArcher A6をブリッジモードへ落としてRaspberry Pi 2のLAN側インタフェース直下に配置しました。

//graph[2020-12-04-kame][blockdiag][カメ専用Wi-Fiアクセスポイント]{
blockdiag {
  PC [color = "#77FF77"];
  PC -> ArcherA6 [label="Wi-Fi", style="dotted"];
  GoogleNestWifi -> ArcherA6 [label="有線"];
  ArcherA6 -> RaspberryPi2;
  RaspberryPi2 -> ISP [folded];
  ISP -> kame;
}
//}

このアクセスポイントへ接続すればさらっとDHCP/DHCPv6でアドレスを取得でき、カメが動いてくれます@<fn>{fn-kame}。
かわいいね。

//footnote[fn-kame][@<href>{http://www.kame.net/}]

前述のGoogle Nest Wi-Fiは、このArcher A6の有線ポートへぶら下がる構造です@<fn>{fn-why-no-direct-connection}。
もちろん、カメが動くのを見る以外に一般のIPv6ネットワークへ出ていくことも出来るので、「あ〜〜〜、なんか今日はIPv4で仕事したくないな！」とIPv4では気分が乗らない日には切り替えて使えます。
この環境からはGoogle Nest Wi-Fiでポート転送しているSSHサーバー経由でないと自宅内の他の端末と通信できないので、気分は宅外作業です。

//footnote[fn-why-no-direct-connection][Google Nest Wi-FiをRaspberry Pi 2の直下にぶら下げたほうが速度面で無駄がなくて良いのでは、と感じるところですが、ただでさえUSB-Ethernetを無理やり増設してWAN/LANポートを確保した状態なので、これ以上Raspberry Pi 2側にポートを生やしたくありませんでした。スイッチを足すならブリッジモードのルータを置いても同じことなので、ここにArcher A6を挟めば一石二鳥という寸法です]

== Rakuten MiniテザリングでIPv6環境へSSH

楽天モバイルの4G回線はIPv4/v6のデュアルスタックです。
このため、ConnectBotのようなアプリを利用すれば前述の環境へとすんなりSSH接続できます。

同様に、Rakuten Miniをテザリング端末としてLinux PCから自宅マシンへのSSHも簡単にできそうな気がします。
しかし残念ながら、Rakuten Miniのテザリング環境はIPv4-onlyです（@<img>{2020-12-04-rakuten-mini}）。

//image[2020-12-04-rakuten-mini][Rakuten MiniのWi-Fiテザリング下のPCに割り当てられたアドレス]{
//}

Androidにおいて、テザリングがIPv6対応か否かはAndroid OS側で規定されておらず端末ベンダー裁量なので、このようなことが起こります@<fn>{fn-iij-ouchi}。

//footnote[fn-iij-ouchi][4年前にIIJの大内さんが調査されたデータが公開されています。SIMロックフリー端末なら対応、ただしDNSサーバーアドレスが取れない場合あり、など。 @<href>{https://www.slideshare.net/IIJ_techlog/iijmio-meeting-10-57396555}]

外出先のPCはIPv4を使いつつ、GCPの実質無料インスタンスを中継してv6アクセス、という方法も考えましたが、残念ながらGCPにおいてv6終端は提供されているものの、Compute Engineからv6インターネットへ出ていくことは出来ませんでした。
このため、帰宅ルート確保のためにAndroid端末上でプロキシサーバーを動作させ、Linux PCからはそのプロキシ経由でIPv6の世界へアクセスすることにしました。

=== Servers Ultimate

Android界には@<href>{https://play.google.com/store/apps/details?id=com.icecoldapps.serversultimate&hl=ja&gl=US, Servers Ultimate}という、HTTPサーバーどころかLAMP丸ごとAndroid端末上に立てることすら可能な万能サーバーアプリという狂ったものが存在します。
いろいろ試したところ、このアプリでRakuten Mini上にプロキシサーバーを用意する方法が簡単で安定性も高かったので紹介します@<fn>{fn-netshare}。

Ultimateを謳うだけあって、Servers Ultimateには当然の如くプロキシサーバー機能も含まれています。
なんと2つも。

このうちHTTP Proxy Server（@<img>{2020-12-04-http-proxy}）は、端末ローカルポートを特定のHTTPサーバー/ポートの組へと転送する、HTTP限定のLocalForwardのようなものです。

//image[2020-12-04-http-proxy][HTTP Proxy Server]{
//}

IPv6テザリングに必要なのはこれではなく、Proxy Server（@<img>{2020-12-04-proxy}）です。

//image[2020-12-04-proxy][Proxy Server]{
//}


//footnote[fn-netshare][当初、NetShareというAndroid端末をWi-Fiホットスポット化するアプリに組み込まれているプロキシサーバーの利用を試みましたが、WebブラウザでのIPv6サイト表示は出来るもののSSHへのプロキシアクセスがうまく動作しませんでした。connect-proxyのデバッグ出力を確認する限りでは、送出しているHTTP/1.0のリクエストにNetShare組み込みプロキシサーバーが非対応なのかもしれません]

ポートを適当に指定してサーバーを起動します。

あとはこのプロキシを利用してconnect-proxyでSSH接続すれば良いのですが、ひとつ考慮漏れがあります。
テザリング環境という事情から、このプロキシはLinux PCから見たゲートウェイ上で動作することになりますが、

 * Wi-Fiテザリング時、テザリングホスト（Rakuten Mini）側のIPアドレスが固定されない
 * Wi-FiテザリングとBluetoothテザリングでゲートウェイアドレス帯が異なる

という挙動をします。
手元で確認した限り、Bluetoothテザリング時のRakuten Mini側IPアドレスは192.168.44.1に固定されますが、Wi-Fi時には192.168.43.0/24のどこかを適当に確保するようでした。

こうなると、connectコマンド側のパラメータを固定せず、SSHログイン実行時のネットワーク状態にあわせてゲートウェイアドレスを指定するのが無難でしょう。
具体的には、@<code>{ip route}コマンドの出力をフィルタしてデフォルトゲートウェイアドレスを取得します。

//cmd{
$ ip route | grep default | cut --delim=' ' -f3
192.168.43.68
//}

こんな感じですね。
この内容を盛り込んだSSHホスト定義を@<code>{~/.ssh/config}に追記すると、

//emlist{
Host nanopi-remote-gw-proxy
  HostName [2xxx::xxxx]
  Port 2022
  ProxyCommand connect -H `ip route | grep default | cut --delim=' ' -f3`:[Servers Ultimateのプロキシポート] %h %p
//}

無事

//graph[2020-12-04-diagram1][blockdiag][Rakuten Miniテザリングで自宅IPoEへIPv6 SSH]{
blockdiag {
  group {
    orientation = portrait;
    label = "Linux PC";
    OpenSSHクライアント;
    connect-proxy;
  }
  group {
    orientation = portrait;
    label = "Rakuten Mini";
    ServersUltimateProxy [label="Servers Ultimate (Proxy)"];
  }
  group {
    label = "自宅";
    color = "#77FF77";
    group {
      orientation = portrait;
      label = "RasPi自宅ルーター";
      shape = "line";
      style = "dashed";
      Firewall;
      tcpproxy;
      Firewall -> tcpproxy;
    }
    GoogleNestWiFi [label="Google Nest Wi-Fi"];
    HomeServer [label="自宅サーバ（SSHホスト）"];
  }
  Cellular [label="楽天モバイル4G網/フレッツ網", shape="cloud", width=160, height=80];

  OpenSSHクライアント -> connect-proxy;
  connect-proxy -> ServersUltimateProxy;
  ServersUltimateProxy -> Cellular;
  Cellular -> Firewall [folded];
  tcpproxy -> GoogleNestWiFi;
  GoogleNestWiFi -> HomeServer;
}
//}

インターネット帰宅できます。

====[column] Bluetoothテザリングのススメ

Rakuten MiniやJelly Proのような小型テザリング端末を使う場合、私はWi-Fiよりも圧倒的にバッテリー持ちのよいBluetoothテザリングを好んで使っています。
バッテリー容量の少ない端末でWi-FiテザリングをONにすると3-4時間で電源が切れるというシビれる状態に陥りますが、Bluetoothテザリングなら7-8時間は問題なく使えます。
テザリングで端末がぶら下がっていない状態なら普通に3-4日間バッテリーが保ったりします。
通信速度は数百kbps出ればよいほうですが、ターミナル作業やGoogle Mapsの経路検索ぐらいなら十分快適に利用できます。
