---
layout: post
title:  "このblogの下回りシステムを少しメンテした"
date:   2024-11-27 09:54:00 +0900
---
= このblogの下回りシステムを少しメンテした

このblogはRe:VIEW記法で書いたエントリをJekyll+Netlifyで静的サイトとして書き出している

ちょっと古めのアーキテクチャだとは感じるけれどRe:VIEW記法で文書を作るのには慣れているので今から変える理由も特にないかなという

さて久々に新エントリを書いたらNetlifyでビルドエラーを吐いた。Python系の依存解決でエラー

//cmd{
7:50:45 AM:   Downloading funcparserlib-0.3.6.tar.gz (30 kB)
7:50:45 AM:   Installing build dependencies: started
7:50:46 AM: Failed during stage 'Install dependencies': dependency_installation script returned non-zero exit code: 1
7:50:46 AM:   Installing build dependencies: finished with status 'done'
7:50:46 AM:   Getting requirements to build wheel: started
7:50:46 AM:   Getting requirements to build wheel: finished with status 'error'
7:50:46 AM:   error: subprocess-exited-with-error
7:50:46 AM:   × Getting requirements to build wheel did not run successfully.
7:50:46 AM:   │ exit code: 1
7:50:46 AM:   ╰─> [1 lines of output]
7:50:46 AM:       error in funcparserlib setup command: use_2to3 is invalid.
7:50:46 AM:       [end of output]
7:50:46 AM:   note: This error originates from a subprocess, and is likely not a problem with pip.
7:50:46 AM: error: subprocess-exited-with-error
7:50:46 AM: × Getting requirements to build wheel did not run successfully.
7:50:46 AM: │ exit code: 1
7:50:46 AM: ╰─> See above for output.
7:50:46 AM: note: This error originates from a subprocess, and is likely not a problem with pip.
//}

@<code>{funcparserlib-0.3.6}、やけに古い

そもそもなんでPython?というのをすっかり忘れていたのだけど、数年前に自分へのクリスマスプレゼントとしてblockdiagで図を埋め込めるようにしたのだった@<fn>{old-post}。すっかり忘れていた

Netlifyでは@<code>{runtime.txt}にPythonのバージョンを書けるのだけど見てみるとバージョン@<code>{3.8}、今年の10月にサポートが切れている

極力最新にするのも良いけれど手元で使っているデスクトップのUbuntu 22.04デフォが@<code>{3.10}系なので一旦それにあわせておく

pipのバージョン指定を手動で細かくやるのも不毛なので、手元環境でvenvを作り直してそこへ入ったバージョンを採用する

//emlist{
python3 -m venv .venv
source .venv/bin/activate
pip install blockdiag
pip freeze > requirements.txt
echo '3.10' > runtime.txt
//}

として再度デプロイする。
どうもまだ図のビルドでエラーを吐いている

//emlist{
blockdiag==3.0.0
funcparserlib==2.0.0a0
pillow==11.0.0
webcolors==24.11.1
//}

ちゃんと今風な依存関係になっているけれど何がまずいのか、デプロイログを見てみる

//cmd{
9:33:56 AM: INFO jekyll: blockdiag -a -T svg -o ./images/html/2019-12-25-diagram1.svg /tmp/review_graph20241127-5628-tmushg
9:33:56 AM: ERROR: 'FreeTypeFont' object has no attribute 'getsize'
9:33:56 AM: ERROR jekyll: failed to run command for id 2019-12-25-diagram1: blockdiag -a -T svg -o ./images/html/2019-12-25-diagram1.svg /tmp/review_graph20241127-5628-tmushg
//}

調べるとPillow 9系から10系以上（現在の推奨は11系）へバージョンアップした際の非互換性FAQらしい


セキュリティ上Pillow 9系を使い続けたくはないが、ここに時間をかけすぎたくもないので一旦撤退がてら9.5.0を使っておくことにした

ひとまず図は再生成されたので一旦よしとする

//footnote[old-post][@<href>{https://b.muo.jp/2019/12/25/jekyll-review-blockdiag}に当時の記事がある]
