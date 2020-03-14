= 高輪ゲートウェイ駅開業にあわせて未来への決断をおこなったJR東日本

山手線に新駅が誕生するというニュースを聞いて以来、気になっていたことがあります。
Suicaほかの交通系ICカードにおける駅順コードをどう割り振るか、です。

== 駅順コード

路線・駅情報のデータベースは有償のもの・無償のものいろいろありますが、個人的には有志によってメンテナンスされている老舗データを参照しています。

http://www014.upp.so-net.ne.jp/SFCardFan/

たまに実在しない駅の情報が入っているなどお茶目な側面を持つデータベースですが、自前でそれなりにクリーニングすれば一般的な用途には十分利用できます。

== 高輪ゲートウェイ駅誕生前の駅順コード

山手線のうち、京浜東北線との併走エリアは東海道本線(線区1)として駅順コードが割り振られています。

該当部分のみを抜き出すと、次のとおりです。

//table[yamanote][山手線の東京-品川間駅順コード(2020年3月13日まで?)]{
線区	駅順コード	会社名	線区名	駅名
1	1	東日本旅客鉄道	東海道本	東京
1	2	東日本旅客鉄道	東海道本	有楽町
1	3	東日本旅客鉄道	東海道本	新橋
1	4	東日本旅客鉄道	東海道本	浜松町
1	6	東日本旅客鉄道	東海道本	田町
1	7	東日本旅客鉄道	東海道本	品川
//}

なんと、駅順コード5番が見事にあいています！
JR東日本はSuica導入当初から高輪ゲートウェイ駅の開業を計画していた・・・？

== 駅順コードの空白

しかし問題があります。

浜松町(1-4) - 田町(1-6) - 品川(1-7)

とあった部分で空きコードを割り当てると

浜松町(1-4) - 田町(1-6) - 高輪ゲートウェイ(1-5) - 品川(1-7)

という格好で、コード順と駅順が揃わないことになります。
この部分をどう解決するのか、概ね

 * 気にせず高輪ゲートウェイ駅に1-5を割り当てる
 * 田町駅の駅順コードをずらして高輪ゲートウェイ駅に1-6を割り当てる

この2択です。
あらかた前者を採るだろうなぁと思いつつ、とても興味を持っていました。

== 高輪ゲートウェイ駅、開業

3/14、予定通りに高輪ゲートウェイ駅が開業したので、早速降車してSuicaの履歴を読み取り、とても驚きました。

//raw|html|<blockquote class="twitter-tweet"><p lang="ja" dir="ltr">Suica Readerでの出場駅は田町ということになってる。JREは駅コードずらしたのか <a href="https://t.co/TVI7mUNMCz">pic.twitter.com/TVI7mUNMCz</a></p>&mdash; Kei Nakazawa (@muo_jp) <a href="https://twitter.com/muo_jp/status/1238665079253372928?ref_src=twsrc%5Etfw">March 14, 2020</a></blockquote> <script async src="https://platform.twitter.com/widgets.js" charset="utf-8"></script>

この結果は、

 * 浜松町(1-4) - 田町(1-6) - 高輪ゲートウェイ(1-5) - 品川(1-7)

の配列をよしとせず

 * 浜松町(1-4) - 田町(1-5) - 高輪ゲートウェイ(1-6) - 品川(1-7)

の駅順コード振り直しをおこなったことを示します。

念のため品川駅そして田町駅で降車してみました。
品川駅は従来通り(これは当然)、そして田町駅が見事にSuica Reader上は未登録の駅で降りた扱いになっています。

== 未来への選択

田町駅のコードをずらして高輪ゲートウェイ駅を配置したことで、駅改札機での履歴出力はもとよりSuica Readerを始めとするICカード履歴読み取りツールや各種経費精算ツールでも日付をもとにした分岐処理が必要となります。

JR東日本はこれを承知のうえで、過去データとそれを読み書きするソフトウェアの一時的な混乱よりも、未来に向けて整った駅順コードを優先するという判断をおこなったことになります。

正直なところ、駅順コードに関しては過去データの扱いを優先して徐々にぐちゃぐちゃになって歴史的経緯として刻まれていくもの、と考えていたので、今回のJR東日本の対応には驚きがありました(羽田空港国際線開業時の京急やほか一部モノレールなどで事例はあるものの)。

今年は東京メトロ日比谷線の虎ノ門駅開業も控えています。
あちらはあちらで駅順コードにまつわる面白い事情があるので、別の機会に紹介しようと思います。

ともかく、高輪ゲートウェイ駅開業おめでとうございます。