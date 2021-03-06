---
layout: post
title:  "Jekyll+Re:VIEW+Netlifyでblogを作ってる"
date:   2019-12-01 00:02:00 +0900
categories: about
---
= Jekyll+Re:VIEW+Netlifyでblogを作ってる

//lead{

本エントリは、@<href>{https://b.muo.jp, muo-ya} Advent Calendarの1日目の記事です。

日々さまざまなメモをRe:VIEW形式で書いているのでblogも同形式でささっと書けるようにしたいというお話です。

//}

== 軽い気持ちで

JekyllもRe:VIEWもRubyで書かれてるから割と簡単にできるでしょ、と思っていた時期がありました。

しかし、書籍制作を前提として開発されている仕組みをインスタントなWeb出力に利用すると、何かと無理があります。
そもそもJekyllのConverter（ファイルからのテキスト変換を担うプラグインの仕組み）では、ファイルコンテンツのみが渡されてファイル名の情報すら一切失われます。

Re:VIEWが特定のディレクトリを書籍のソースディレクトリと捉え、当該ディレクトリ内のファイルリスト（チャプター一覧）を作成するところをワークフローの前提に置いているのとは対照的です。

== できていること

 * 基本的な構文の.re→.html変換
 ** たとえば、本エントリのソースは@<href>{https://github.com/muojp/jrblog/blob/master/_posts/2019-12-01-hello-jekyll-review.re, このファイル}

== これからやること

 * Re:VIEW側headlineのエントリ一覧への反映
 ** UTF-8時代なのでたぶんファイル名に日本語入れてゴリ押しは出来なくもないけど避けたい
 ** シンプルに実装できるなら、Re:VIEWからMarkdownを出してそこから再変換でも良い気はする
 * 更新日時の埋め込み
 ** 独自のメタタグを用意する感じ
 * CSS適用
 ** 少なくともJekyll側のテーマに沿った感じにはしたい
 * インライン画像の埋め込み
 * 他チャプターの参照
 ** ファイル数が増えるとビルドがどんどん遅くなるので、これは実際やるか悩ましい
 * OGP設定
 ** 最低限はやっておきたい

== 苦しみの図

本blogのソース一式は@<href>{https://github.com/muojp/jrblog/}にあります。

そのうち、JekyllでRe:VIEWを扱うための試行錯誤を反映した苦しみの図は@<href>{https://github.com/muojp/jrblog/blob/master/_plugins/review_re.rb}このあたりにあります。

=== 基本方針

@<code>{review-compile}コマンドの挙動を参考にしつつ、最低限のHTMLを出力できるようにちまちま書いている状態ですが、ひとまずもういくらか出来ることを増やしてjekyll-reviewのgemとしてリリースするところまでは行きたいと思います。

=== なぜ/tmp/f.reと/tmp/tmp/f.reを作っているのか

わかりません。

たぶんRuby版Re:VIEW（4.0.0）自体になにかしらの不具合があるものと思いますが、内容をダンプしつつフローを追いかけていったところ「/tmpを書籍ディレクトリとして初期化すると、/tmpを開き、ファイル一覧を作成し、読み込み時には/tmp/tmp/以下から読む」という謎な動きをしていたのでワークアラウンドしたのみです。

なにぶん、前述のパラダイム差を吸収するために/tmpディレクトリへ直接ファイルを生成する乱暴な実装なので、じきにもう少々改善したいところです。

====[column] 追記

と書いていたら、Re:VIEWメンテナの@<href>{https://twitter.com/kmuto, @kmuto}さんからアドバイスを頂きました。

@<raw>{|html|<blockquote class="twitter-tweet"><p lang="ja" dir="ltr">一時ファイル作らずにこういうかんじでどうでしょう? <a href="https://t.co/S1HF5I062h">https://t.co/S1HF5I062h</a></p>&mdash; kmuto (@kmuto) <a href="https://twitter.com/kmuto/status/1200981325529763840?ref_src=twsrc%5Etfw">December 1, 2019</a></blockquote><script async src="https://platform.twitter.com/widgets.js" charset="utf-8"></script>}

アドバイスってレベルじゃなくて答えそのものですね。
ChapterもLocationもin-placeで生成して食わせる、、なるほど。

== Netlifyの使い勝手

Netlifyめちゃくちゃ便利という話はよく聞くものの、遠いインターネットの話かなぁと思ってほとんど使ってなかったんですが、使ってみると人気っぷりがよく理解できる完成度の高さでした。

簡単にメモがてら書き残しておきます。

 * @<href>{https://www.netlify.com/blog/2015/10/28/a-step-by-step-guide-jekyll-3.0-on-netlify/, 公式のGitHub+NetlifyでJekyllやるガイド}が充実していて、まず迷わずにスタートできる
 * gemのキャッシュが十分に効くので、2度目以降のビルドが十数秒レベルで終わる
 ** 特にネイティブモジュールのビルドが必要なgem類はインストールに時間がかかるので、なるべくキャッシュしたいところです。Netlifyのデプロイ環境では少なくとも今のところJekyllとRe:VIEWの実行に必要な依存gemをほどよくキャッシュしてくれているようです
 * カスタムドメインの割り当てがめちゃくちゃ楽
 ** 管理画面ぽちぽちすればドメイン所有権の確認からLet's Encryptベースの証明書デプロイまで3-4分で完了するすばらしさ