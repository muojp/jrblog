---
layout: post
title:  "blogに画像を上げられるようにした他"
date:   2019-12-02 01:36:00 +0900
categories: about
---

= blogに画像を上げられるようにした他

== やったこと

 * @<href>{https://twitter.com/kmuto, @kmuto}さんのアドバイスをもとにRe:VIEW→HTML部分を改修
 * 余分なタグ出力を削ってHTMLのvalidityを向上
 * 画像埋め込み対応

1番目は既に@<href>{https://b.muo.jp/2019/12/01/hello-jekyll-review.html, 昨日のエントリ}の追記で書いたとおり。
多謝。

2番目は簡易ステート管理を入れた。
基本的には先頭と末尾の@<code>{<html>}類を削るのみで良いはず。

3番目が少し厄介だったのでメモを残す。

=== Jekyll Converter Pluginでインライン画像埋め込みを制御する

==== 画像の配置場所選択

Re:VIEWでは@<href>{https://github.com/kmuto/review/blob/master/doc/format.ja.md#%E7%94%BB%E5%83%8F%E3%83%95%E3%82%A1%E3%82%A4%E3%83%AB%E3%81%AE%E6%8E%A2%E7%B4%A2, 仕様}に記載のとおり、次の6箇所を画像パス候補と扱う。

//cmd{
1. <imgdir>/<builder>/<chapid>/<id>.<ext>
2. <imgdir>/<builder>/<chapid>-<id>.<ext>
3. <imgdir>/<builder>/<id>.<ext>
4. <imgdir>/<chapid>/<id>.<ext>
5. <imgdir>/<chapid>-<id>.<ext>
6. <imgdir>/<id>.<ext>
//}

今回実装したのは6。

blog用の画像管理として望ましいのは4で、デフォルト状態を想定すると@<code>{images/2019-12-02-updated-blog-system/foo.png}のようなパスに画像を置くことになる。
記事ごとにサブディレクトリを掘って画像を置けるため、短いファイル名でも他記事のものと重複する恐れがないという意味で望ましい。

しかし@<href>{https://b.muo.jp/about/2019/12/01/hello-jekyll-review.html, 前回書いた}ように、JekyllのConverter仕様では「どのファイルを処理しているか」という情報が渡ってこないので、@<code>{<chapid>}を知ることができない。

gemで配布できる範囲のJekyllプラグインを目指す以上、Jekyll本体へ手を入れるのは最後の手段であり（本家チームを説得してupstreamする覚悟をできないならNG）、それを避ける範囲で出来るのは6番目の画像ディレクトリ直下ベタ置きだった。

VSCode+language-review拡張でのプレビューを考えて画像ディレクトリは_postsディレクトリ以下に配置。
このため画像ファイル類がJekyllの変換ソースとして扱われる（Jekyllは.pngでも容赦なくソースとして解釈しようとする）。
_config.ymlの@<code>{exclude}指定に画像ディレクトリを追加して回避@<fn>{fn-exclude}。

//footnote[fn-exclude][該当箇所: @<href>{https://github.com/muojp/jrblog/blob/c3d1890cb98879ea5f365534c0ddfc9d06d2874b/_config.yml#L45-L46}]

==== 公開記事のみ画像を公開エリアへ配備

画像置き場の制約を受け入れたのと引き換えに諦めたくなかったのは「投稿日前の記事から参照される画像がむやみに公開されない」こと。
このため、Re:VIEWからHTMLへの変換結果をフィルタ処理するついでに、本文中から参照されている画像をリスト化して公開ディレクトリ以下へコピーするようにした。

==== コピーした画像がどんどん揮発する

最後の厄介どころは、Jekyllの公開ディレクトリがビルドの都度クリアされる点だった。
そしてクリーンアップはHTML類のビルド後にも走るため、ビルド中に作成した画像もきれいに削除されてしまう。

クリーンアップ処理をサボらせるための仕組みがあるはず、とJekyllのコードを調べていたら、@<href>{https://github.com/jekyll/jekyll/blob/master/lib/jekyll/cleaner.rb, jekyll/cleaner.rb}とそのテストコードである@<href>{https://github.com/jekyll/jekyll/blob/master/test/test_cleaner.rb, test/test_cleaner.rb}が大変役立った。
_config.yml内に@<code>{keep_files}という配列を作って画像配置ディレクトリを除外処理したところ、問題なく保持できた@<fn>{fn-keep}。

//footnote[fn-keep][該当箇所: @<href>{https://github.com/muojp/jrblog/blob/c3d1890cb98879ea5f365534c0ddfc9d06d2874b/_config.yml#L42-L43}]

== 無事

画像を貼り付けて参照（@<img>{2019-12-02-muoya}）も貼れるようになりました。

//image[2019-12-02-muoya][muo-ya]{
//}

これでAdvent Calendarを書くのに最低限必要な機能は揃ったことになりますね！