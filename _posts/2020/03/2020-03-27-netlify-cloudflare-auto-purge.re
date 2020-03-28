---
layout: post
title:  "Netlifyのデプロイ完了時にCloudflareのキャッシュを自動パージする"
date:   2020-03-27 22:01:00 +0900
---
= Netlifyのデプロイ完了時にCloudflareのキャッシュを自動パージする

以前「@<href>{https://b.muo.jp/2019/12/01/hello-jekyll-review.html, Jekyll+Re:VIEW+Netlifyでblogを作ってる}」で書いたように、このblogはGitHub上のリポジトリにpushしたRe:VIEWファイルをNetlifyで自動ビルドする運用です。
Netlify自体に月間100GBの無料枠があるので転送量は今のところ問題になっていませんが、せっかくなのでCDN（すでに使っているCloudflare）を利用しようと思い立ちました。

CDNといえばキャッシュに始まり、キャッシュに終わるものです（Compute@Edgeみたいなやつもありますが）。
適切なタイミングでキャッシュを破棄（パージ）することで、Netlifyの高速な自動デプロイ機能@<fn>{common-case}を最大限に活かせます。

Netlifyのビルド機能には完了時のwebhook設定が用意されており、CloudflareにはAPI経由のキャッシュパージ機能があるので、これらをAWS LambdaでつないでやればNetlify側の更新完了時にキャッシュをパージできそうです（@<img>{2020-03-27-cloudflare-autopurge}）。

//graph[2020-03-27-cloudflare-autopurge][blockdiag][めざすもの]{
blockdiag {
  node_width = 60;
  node_height = 80;
  span_width = 60;
  span_height = 60;
  Lambda [label = "AWS\nLambda"];
  GitHub -> Netlify [label = "commit\nhook"];
  Netlify -> Lambda [label = "web-\nhook"];
  Lambda -> Cloudflare [label = "API call"];
}
//}

//footnote[common-case][ビルド依存関係が大きいのですが、Netlify側のビルド構成キャッシュがよくできており、GitHubへのpushからおおむね1分以内にはblogが更新されます]

== 下調べ

=== Netlifyのwebhook仕様を簡単に調べる

Netlifyのwebhookメニュー（Settings → Build & deploy）を見ると、わりと柔軟にhookを送信できることがわかります（@<img>{2020-03-27-netlify-hook-cloudflare-purge}）。

//image[2020-03-27-netlify-hook-cloudflare-purge][webhookの種類]{
//}

CDNのキャッシュパージには@<code>{Deploy succeeded}のタイミングが適切でしょう。


//image[2020-03-27-netlify-hook-options][webhookの設定]{
//}

hook先のURIに加えてsecret keyの設定オプションがあります。
これについて@<href>{https://docs.netlify.com/site-deploys/notifications/#outgoing-webhooks, 該当するドキュメント}を読むと、シンプルなJWT（JSON Web Token）ベースのリクエストソース検証が可能のようです。

=== JWTとなかよく

都合良いことに、Webhook送信側のNetlifyも、その受け側のAWS API GatewayもJWTをサポートします@<fn>{api-gateway-http-api}。

が、AWS側の@<href>{https://docs.aws.amazon.com/ja_jp/apigateway/latest/developerguide/http-api-jwt-authorizer.html, ドキュメント}でAPI GatewayがサポートするJWTの仕様を確認すると、issuerへのURI指定が必須、audience指定必須です。
Netlifyが送出に対応しているJWTは単純なHMACなので、標準機能同士ではマッチしません。

//footnote[api-gateway-http-api][実は、API GatewayがJWTを組み込み機能としてサポートしたのは結構最近です。@<href>{https://dev.classmethod.jp/articles/amazon-api-gateway-jwt-authorizers/, 参考記事}]

このため、API Gatewayステージでの認証は諦めてLambda側の前処理に負わせることにします。

=== CloudflareのキャッシュパージAPI

わりと素直なAPIですが、要注意ポイントがあります。
@<href>{https://api.cloudflare.com/#zone-purge-files-by-cache-tags-or-host, ホスト（FQDN）指定のキャッシュパージ}はEnterpriseプラン専用です。

無料版でも利用できるのは@<href>{https://api.cloudflare.com/#zone-purge-files-by-url, URL指定のパージ}もしくは@<href>{https://api.cloudflare.com/#zone-purge-all-files, 全パージ}で、悩ましいところですが発生頻度を考えると全パージで良いでしょう。

Cloudflare APIをうまく叩けているかの確認にもひと工夫必要です。
キャッシュのパージという処理の性質上、Web上で結果確認するのが難しいものですが、Cloudflareのアカウント詳細を見に行くとAudit Log（監査ログ）機能があり、そこでキャッシュパージリクエストの発行ログを見れるので助かりました（@<img>{2020-03-27-netlify-hook-cloudflare-audit-log}）。

//image[2020-03-27-netlify-hook-cloudflare-audit-log][Cloudflareの監査ログ]{
//}

== AWS Lambda+Sinatraで要件を満たす

JWT署名検証のサンプル実装がRubyであることに加え、AWS LambdaのRubyランタイム（つい最近2.7もサポートした）を使ってみたかったので、今回はRubyでやっていきます。
AWS LambdaのRuby（2.7）ランタイムで動作するSinatra+Rackのサイト上でNetlifyから飛んできたwebhookのJWT署名を検証、検証OKならCloudflareへキャッシュパージリクエストを発行する、という内容です。

=== AWS LambdaでSinatra

@<href>{https://github.com/aws-samples/serverless-sinatra-sample}リポジトリのコードをベースに改造して作ります。

=== signed関数

出来上がりはこんな感じです。

//emlist{
def signed(request, body)
  signature = request.env["HTTP_X_WEBHOOK_SIGNATURE"]
  return unless signature

  options = {iss: "netlify", verify_iss: true, algorithm: "HS256"}
  decoded = JWT.decode(signature, PRESHARED_KEY, true, options)

  ## decoded :
  ## [
  ##   { sha256: "..." }, # this is the data in the token
  ##   { alg: "..." } # this is the header in the token
  ## ]
  decoded.first['sha256'] == Digest::SHA256.hexdigest(body)
rescue JWT::DecodeError
  false
end
//}

サンプルコード自体に問題があって署名検証で順調にハマり、中でも↓の問題はRubyよく知らない人間には相応のキツさがありました。

@<raw>{|html|<blockquote class="twitter-tweet"><p lang="ja" dir="ltr">Netlifyのwebhook JWT署名検証サンプルではJWT. decode()の戻りがシンボル指定のハッシュであるように扱われているけど実際は文字列キーで戻ってくるのでdigest検証に一生失敗する<a href="https://t.co/VIp05X3K2g">https://t.co/VIp05X3K2g</a></p>&mdash; Kei Nakazawa (@muo_jp) <a href="https://twitter.com/muo_jp/status/1243497121682083840?ref_src=twsrc%5Etfw">March 27, 2020</a></blockquote> <script async src="https://platform.twitter.com/widgets.js" charset="utf-8"></script>}

=== Sinatraのエンドポイント記述

//emlist{
post "/netlify-hook" do
  request.body.rewind
  body = request.body.read
  halt 403 unless signed(request, body)

  uri = URI.parse("https://api.cloudflare.com/client/v4/zones/#{ZONE_ID}/purge_cache")
  http = Net::HTTP.new(uri.host, uri.port)
  http.use_ssl = uri.scheme === "https"

  params = { purge_everything: true }
  headers = { "Content-Type" => "application/json", "Authorization" => "Bearer #{API_TOKEN}" }
  response = http.post(uri.path, params.to_json, headers)

  ret = { :Output => response.code }.to_json
  puts ret
  ret
end
//}

Sinatra側では次のような問題がありました。

 * ヘッダの渡ってきかたがNetlifyのサンプルとは違うので適宜調整が必要
 * body（POST内容が入っている）がパフォーマンス上の都合かStringIOのインスタンスとして渡ってくる
 ** これは原則的に複数回読み出し用のものではないので、都度rewindする必要がある

=== その他、実装上の厄介ポイント

 * AWS LambdaがRubyランタイムをサポートしたのが割と最近で、さらにSinatraを使っている人となると世界でもかなり少ない
 ** ので、当然に挙動が怪しくても自前でデバッグするほかない。
 * デバッグしづらい。SAM CLIベースのローカル環境なしでいけるやろと取り組んだのが運の尽きで、最初からちゃんと作っておくべきだった
 ** Lambdaのコード実行がうまくいっているかの確認が大変
 ** 署名検証がうまくいっているかの確認も大変

== コードと導入方法

実際に自サイトで導入しているLambda用プロジェクト一式を@<href>{https://github.com/muojp/netlify-webhook-purge-cloudflare-cache, GitHub:muojp/netlify-webhook-purge-cloudflare-cache}に置いてあります@<fn>{debug-output}。

//footnote[debug-output][デバッグ用のリクエストダンプ（@<code>{puts}呼び出し）をいくつかapp/server.rbに残しているので、CloudWatchにリクエストデータが流れると困る場合には除去してください]

READMEに記載のとおり、あらかじめコード格納先のS3 bucketを作っておき、CloudFormationでSinatraAppをデプロイし、3つの環境変数を設定すれば使えます。

//table[tbl-settings][設定する環境変数]{
CLOUDFLARE_API_TOKEN	CloudflareのAPIトークン
ZONE_ID	サイトのゾーンID
NETLIFY_PRESHARED_KEY	Netlifyのwebhookに指定したsecret
//}

CloudflareのAPIトークンは@<img>{2020-03-27-netlify-hook-cloudflare-token}のような設定で作りましょう。

//image[2020-03-27-netlify-hook-cloudflare-token][APIトークン生成時の設定]{
//}

サイトのゾーンIDはCloudflareのダッシュボード（Overview画面）の右下にひっそり書かれているのでコピーします。

Netlify側のwebhook URIには、AWS Lambdaを呼び出すAPI GatewayのURIに/netlify-hookをくっつけたものを指定します。

例：https://g12345676543123456.execute-api.ap-northeast-1.amazonaws.com/Prod/netlify-hook

JWS secret token （optional）という項目に適当な文字列を設定し、同じものをAWS Lambda側環境変数のNETLIFY_PRESHARED_KEYに設定すれば準備完了です。
試しにNetlify側でデプロイを実行し、Cloudflare側のAudit Logにキャッシュパージリクエストが記録されることを確認しましょう。
