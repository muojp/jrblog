---
layout: post
title:  "BashとCloudflareでつくる 無料Dynamic DNS"
date:   2020-03-27 12:46:00 +0900
---
= BashとCloudflareでつくる 無料Dynamic DNS

自宅サーバーといえばDynamic DNS、という事情はなんだかんだで20年近くあまり変わっていません。
IPアドレス更新クライアント付きの自動化可能なサービスとしては@<href>{https://www.noip.com/, No-IP}あたりが有名どころなのですが、毎月1回はサイトに行ってアクティベーションしないとホスト設定が失われるなど面倒がそれなりにあります。

最近No-IPで運用しているホストが数回IPアドレス変更に追従失敗しており移行先を探していたのですが、よく考えると自ドメインのネームサーバーをCloudflareの無料プランに任せており、なんとなくAPIアクセスできそうな気がして調べたら案の定Bashだけでうまくいったので簡単にまとめます。

「Bashだけという割にcurlもjqも使ってるじゃないか！」というBash原理主義の方は@<href>{https://www.youtube.com/channel/UCAL3JXZSzSm8AlZyD3nQdBA, プリミティブ・テクノロジー}の動画でも観て心を落ち着けて下さい。

== 下準備：Dynamic DNSとして利用するAレコードを作成

Cloudflareの管理コンソールから好きなホスト名で作成してください。

== APIトークンの発行

@<href>{https://dash.cloudflare.com/profile/api-tokens}からトークンを発行します。
Cloudflareではトークンごとに与える権限スコープをわりと細かく設定できます。
AND/ORの複雑な組み合わせに対応してはいませんが、そういうことをやりたければ複数トークンを発行してうまく使い分けろということでしょう。

//image[2020-03-27-cloudflare-apikey-setting][APIトークンの設定例]{
//}

サマリー出力でAll zones - Zone:Read, DNS:Editとなっていればいい感じです@<fn>{limit-permission}。

設定完了画面に動作確認用のcurlコマンド例が表示されるので、ひとまず確認しておきましょう。

//cmd{
{"result":{"id":"RESULTID","status":"active"},"success":true,"errors":[],"messages":[{"code":10000,"message":"This API Token is valid and active","type":null}]}
//}

明らかにいけてそうなメッセージが出力されればOKです。

この先は@<href>{https://api.cloudflare.com/}を読み、必要なAPIを呼び出していくことになります。
APIドキュメントが充実しており、それぞれcurlのサンプルも載っているのであまり迷いません。

//footnote[limit-permission][厳密には、後述するDNSレコードIDの特定が終わったらAPIトークンの権限を更新してZone:Readを外しても問題ないと思います]

== Dynamic DNSとして更新するDNSレコードの特定

DNSのAレコードを更新するためにはCloudflareにおけるエンティティ構造を把握する必要があります。
これは@<img>{block-cf}のように図示できます。

//graph[block-cf][blockdiag][Cloudflare APIのエンティティ構造]{
blockdiag {
  Zone1 [label = "Zone 1"];
  Zone2 [label = "Zone 2"];
  Zone3 [label = "Zone 3"];
  Record1 [label = "DNS Record 1"];
  Record2 [label = "DNS Record 2", color = "orange"];
  Record3 [label = "DNS Record 3"];

  Account -> Zone1, Zone2, Zone3;
  Zone1 -> Record1, Record2, Record3;
}
//}

Aレコード更新にたどり着くためには、

 * ゾーンIDの特定
 * DNSレコードIDの特定

が必要です。
スクリプト化を考えるまでもなく、curlでぽちぽちリクエストすれば十分です。

=== ゾーンIDの特定

設定対象のドメイン名を渡すと該当するゾーン情報を取得できるAPIが生えています。

//cmd{
curl -X GET "https://api.cloudflare.com/client/v4/zones?name=DOMAIN-NAME" \
     -H "Authorization: Bearer CLOUDFLARE-API-TOKEN" \
     -H "Content-Type:application/json" \
| jq '.result[].id'
//}

ここでは結果JSONオブジェクトからゾーンIDのみを抜き出していますが、目grepでも全然問題ない分量です。

=== DNSレコードIDの特定

特定ゾーンに含まれるDNSレコードの検索は、次のような呼び出しで実現できます。

//cmd{
curl -X GET "https://api.cloudflare.com/client/v4/zones/ZONEID/dns_records" \
     -H "Authorization: Bearer CLOUDFLARE-API-TOKEN" \
     -H "Content-Type:application/json" \
| jq '.result[] | select(.name == "hostfqdn.example.com") | .id'
//}

ドメインにはメールやらCNAMEやら色々とくっついているのが普通だと思いますので、jqでホスト名絞り込みをおこなっています。

== IPアドレスの変化を検出してAレコードを更新するbashスクリプトを仕込む

必要な情報が揃いました。
あとは自ノードのグローバルIPアドレスを調べて保存済みのものと違ったらCloudflare APIを叩いてAレコードを更新するスクリプトを書くだけです。

こういうファイルを作ってcronに仕込みます。
実行間隔はホストの重要度によって決めて下さい。

//emlist{
#!/bin/bash

LASTIPFILE=`realpath ~/.lastipaddr`
HOST="hostfqdn.example.com"
APITOKEN="CLOUDFLARE-API-TOKEN"
ZONEID="ZONE-ID"
RECORDID="RECORD-ID"

IP=`curl -s https://checkip.amazonaws.com`
if [ -z "$IP" ]; then
  echo "Failed to check global IP address. Network could be down. There's nothing we can do now."
  exit 1
fi

if ! touch $LASTIPFILE; then
  echo "We don't have access on \"$LASTIPFILE\". Please check permission settings."
  exit 2
fi

LASTIP=$(<$LASTIPFILE)
if [ $IP != "$LASTIP" ]; then
  echo "Updating DNS record."
  echo $IP
  curl -X PATCH "https://api.cloudflare.com/client/v4/zones/$ZONEID/dns_records/$RECORDID" \
     -H "Authorization: Bearer $APITOKEN" \
     -H "Content-Type: application/json" \
     --data "{\"type\":\"A\",\"name\":\"$HOST\",\"content\":\"$IP\",\"ttl\":120,\"proxied\":false}"
  echo $IP > $LASTIPFILE
fi
//}

見ての通り、グローバルアドレスのチェックにはAWSのcheckipを利用しています。
AWSのサイトへ定期的なアクセスを発生させるのが嫌なら、適当にLambdaを用意して呼び出せば良いと思います。
