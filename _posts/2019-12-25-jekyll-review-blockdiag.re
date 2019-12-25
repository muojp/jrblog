---
layout: post
title:  "Jekyll+Re:VIEWでさらっとグラフを入れるざんまい"
date:   2019-12-25 17:18:00 +0900
---
= Jekyll+Re:VIEWでさらっとグラフを入れるざんまい

//lead{

@<href>{https://b.muo.jp, muo-ya} Advent Calendar 6日目の記事です。
そして今日は12/25です。
メリー・クリスマス！
私の12月はもうしばらく続きます。

あまり手間をかけずにそこそこいい感じの図をblogへ埋め込むために、Jekyll+Re:VIEWの環境を拡張していきます。
最初に単体のRe:VIEWでほどよくグラフ出力できるようにし、その後にJekyll+Re:VIEWへと組み込み、最終的にNetlifyで自動ビルドできるところまで持っていきます。

//}

== Re:VIEWのgraph機能の基礎と出力調整

=== blockdiagのインストールと基本的な使い方

あらかじめpipからblockdiagをインストールするのが前提です。

#@# TODO: インストールのリンクを一応貼る

日本語を含む図の描画には、事前のフォント設定が必要です。

//emlist{
$ cat ~/.blockdiagrc
[blockdiag]
fontpath = /mnt/c/Users/muo2/workspace/blog/test/HGRGE.TTC
//}

手元開発環境かつ手動で画像へのレンダリングを試す（Gitリポジトリに投入しない）範囲ではライセンス上問題なさそうな、HGゴシックを試しに指定しました。
実際にはサーバ上での自由な利用のためにNotoを選ぶことになるでしょう。

Re:VIEW記法にblockdiagで図を埋め込む場合は次のように記述します。
利用したのはblockdiag公式サイトにあったサンプル図を軽く改変したものです。

//emlist{
//graph[foo1][blockdiag]{
blockdiag {
  A [label = "foo1"];
  B [label = "f日本語g"];

  A -> B -> C -> D;
  A -> E;
  E -> F  [label="it's me", textcolor="red"];
  F -> G;
}
//\}
//}

技術的な制限によりブロックの最後が//\}となっていますが、実際には\を入れてはいけません。

この状態で

//cmd{
$ review-compile --target=html ch01.re > ch01.html
//}

というようにドキュメントを簡易ビルドすると、images/html/ディレクトリ以下にfoo1.pngが生成され、HTML内にimgタグも生成されます。

=== 出力フォーマットをPNGからSVGへ変更する

blockdiagから出力したPNG画像は少々文字が潰れ気味です@<img>{2019-12-25-blockdiag-generated-png}@<img>{2019-12-25-blockdiag-generated-svg}。

//image[2019-12-25-blockdiag-generated-png][Re:VIEW+blockdiaで生成した図、PNG版]{
//}

//image[2019-12-25-blockdiag-generated-svg][Re:VIEW+blockdiaで生成した図、SVG版]{
//}

デフォルトのPNGよりは時代的にSVGのほうが良いでしょう。
サイズが小さくきれいに仕上がるケースが多いはずです。

それでは、出力フォーマットを変更すべく、早速Re:VIEWを改造していきましょう。

lib/review/builder.rbを見ると、こういう感じです。

//emlist{
    def graph_blockdiag(id, file_path, _line, tf_path)
      system_graph(id, 'blockdiag', '-a', '-T', image_ext, '-o', file_path, tf_path)
      file_path
    end
//}

ここで参照される@<code>{image_ext}はlib/review/htmlbuilder.rbに

//emlist{
    def image_ext
      'png'
    end
//}

と書かれています。
この部分を試しに無理やり@<code>{'svg'}へ書き換えてみると、想定通りblockdiagでSVGが生成されました。

=== ライブラリ本体へ手を入れるのをreview-ext.rbで回避

gemを書き換えてforkしたり、出力フォーマットを可変にするためのpull-requestを本家へ出すのは少々大掛かりすぎます。
これは6年前に通った道です。

#@# TODO: リンク

ごく単純なコードで出力フォーマットの変更に成功しました。
オープンクラス万歳。

== Jekyll+Re:VIEWへのインテグレーション

まあまあ厄介です。
ひょっとすると、当初の実装のように一時ファイルを作成するようにしたほうが楽かもしれません。

=== gemそのままビルドしてみる

前回も書いたように、Jekyll pluginの仕様上の都合でディレクトリ指定があれこれ厄介なので、このまま通るわけはないのですが、試しに変換してみましょう。

//cmd{
INFO jekyll: blockdiag -a -T svg -o ./images/html/2019-12-25-diagram1.svg /tmp/review_graph20191225-730-1ynlj8a
WARN jekyll: !!! CHAPS is obsoleted. please use catalog.yml.
  Conversion error: Jekyll::ReVIEWConverter encountered an error while converting '_posts/2019/12/2019-12-25-jekyll-review-blockdiag.re':
                    No such file or directory @ rb_sysopen - /mnt/c/..../_site/images/html/2019-12-25-diagram1.png
//}

案の定エラーが出ました。

=== 実際のJekyllからRe:VIEWコンパイル時に図をビルドする

例の作りかけプラグインに手を入れていきます。

HTMLBuilderのimage_extを前述のとおり差し替えるだけで、_post/images/html/ディレクトリへSVGファイルを書き込むところまで通りました。

オープンクラス万歳。

====[column] Builderのgraph_blockdiagを差し替えるパターンの検討

本文中ではHTMLBuilderクラスのimage_extをお手軽書き換えする方向で通しましたが、さきほど確認した@<code>{graph_blockdiag}関数を書き換えるパターンも検討しました。

この関数の変換プロセスのなかで一時ディレクトリを適当に作成し、そこにファイルを書き出し、先日実装した画像コレクタでコピーしてDocumentRoot以下へ回収しつつ、辻褄の合うパスを呼び出し元へ返してやればよい、と考えて試作までしましたが、その後のビルド工程の整合性を保つのが想像より大変でした（book.imgdir自体を当該tmpdirに指定すればなんとかなりそうだが、本末転倒感がある）。

=== ビルドした図を回収する

blockdiagで生成した画像の書き出し先は@<code>{images/html/}以下ですが、img参照時に@<code>{html/}部分をパスへ含める意味はあまりありません。

もちろん、本文中で参照する画像とRe:VIEWから生成する図との間でファイル名の重複を避ける効果はありますが、本文中で参照する画像の大半がJPEGかPNGであることを考えると、拡張子.svg自体が簡易的に名前空間として機能するため、ディレクトリ切り分けによって得られる追加効果は限定的です。

前回書いたように、記事間で参照画像名が被った場合の対応は要検討状態のままなので、今回なにか対応してもすぐに変更する可能性が大です。
このため、今のところはDocumentRoot以下のimages/ディレクトリへどかどかとファイルコピーする仕様としました。

== Jekyll+Re:VIEW+Netlifyでのビルド

さて、ようやく最終段階です。

Re:VIEWとJekyllからみて外部依存にあたるblockdiagをNetlifyのビルド環境へインストールする必要があります。

懸念は日本語対応フォント、pipでのblockdiagインストールとパス解決です。

=== pipでのパッケージ導入

requirements.txtにまとめて投下するとインストールしてくれます。
そして、Python本体のバージョンはruntime.txtに書き込むべしとdocsに書かれていました。

#@# TODO: リンク貼ってもいい

//cmd{
5:19:15 PM: Collecting blockdiag==1.5.4
5:19:16 PM:   Downloading https://files.pythonhosted.org/packages/e6/37/a3a4d09c8cbe16b303ed75fd07381e5460b37a25fe247645f2251477887a/blockdiag-1.5.4-py2.py3-none-any.whl (2.7MB)
5:19:16 PM: Collecting funcparserlib==0.3.6
5:19:16 PM:   Downloading https://files.pythonhosted.org/packages/cb/f7/b4a59c3ccf67c0082546eaeb454da1a6610e924d2e7a2a21f337ecae7b40/funcparserlib-0.3.6.tar.gz
5:19:17 PM: Collecting Pillow==6.2.1
5:19:17 PM:   Downloading https://files.pythonhosted.org/packages/89/3e/31c2e5385d7588016c6f7ac552e81c3fff2bef4bc61b6f82f8177752405c/Pillow-6.2.1-cp37-cp37m-manylinux1_x86_64.whl (2.1MB)
5:19:17 PM: Collecting six==1.13.0
5:19:17 PM:   Downloading https://files.pythonhosted.org/packages/65/26/32b8464df2a97e6dd1b656ed26b2c194606c16fe163c695a992b36c11cdf/six-1.13.0-py2.py3-none-any.whl
5:19:17 PM: Collecting webcolors==1.10
5:19:17 PM:   Downloading https://files.pythonhosted.org/packages/8b/ff/c21df7e08e68a1a84b947992c07dfed9cfe7219d068cb7728358d065c877/webcolors-1.10-py2.py3-none-any.whl
5:19:17 PM: Requirement already satisfied: setuptools in /opt/buildhome/python3.7/lib/python3.7/site-packages (from blockdiag==1.5.4->-r requirements.txt (line 1)) (41.6.0)
5:19:17 PM: Building wheels for collected packages: funcparserlib
5:19:17 PM:   Building wheel for funcparserlib (setup.py): started
5:19:18 PM:   Building wheel for funcparserlib (setup.py): finished with status 'done'
5:19:18 PM:   Created wheel for funcparserlib: filename=funcparserlib-0.3.6-cp37-none-any.whl size=17449 sha256=3abe03ffce86a25872b78e35a53f962a6b5b8ecf0334c6a7199c8465392f529a
5:19:18 PM:   Stored in directory: /opt/buildhome/.cache/pip/wheels/03/eb/48/ade4df39d3eb30e31518e91e4ee0572ca6c1292a94f782f9da
5:19:18 PM: Successfully built funcparserlib
5:19:18 PM: Installing collected packages: six, webcolors, funcparserlib, Pillow, blockdiag
5:19:18 PM: Successfully installed Pillow-6.2.1 blockdiag-1.5.4 funcparserlib-0.3.6 six-1.13.0 webcolors-1.10
5:19:19 PM: Pip dependencies installed
//}

おっ、無事Python 3.7で通っていますね。

== できました

//graph[2019-12-25-diagram1][blockdiag][ここに図が出たということは今回の試みが成功したということですの図]{
blockdiag {
  A [label = "にこごり", color="orange"];
  B [label = "我が家", color="blue"];
  A -> B [label = "くるよ", textcolor="gray"];
}
//}

無事、にこごりが我が家に来てくれました。
