---
layout: post
title:  "KVMで動作するWindows 10 VMにBitLocker回復キーを簡単入力する"
date:   2020-04-06 12:45:00 +0900
---
= KVMで動作するWindows 10 VMにBitLocker回復キーを簡単入力する

最近全然VirtualBoxの出番がないな、という程度にはLinux環境でKVMをヘビーユーズしています。
VirtIOなしでもそれなりのパフォーマンスが出るので、「Windows 10を1時間ほどほったらかして適当にWindows Updateを当てさせる」といった、よりよいLinux環境への引きこもり策としても有用だと思います。

さて、BitLockerディスクフル暗号化の利用者は、とくにマルチブートやVM環境において数字48桁の回復キー入力を求められるケースがそれなりにあります。

//image[2020-04-06-kvm-recovery][KVMでWindows 10を実行した際のBitLocker回復キー入力]{
//}

これを手打ちするのはなかなか不毛なので、セキュアさを損なわない範囲で簡略化する方法を考えてみました。
応用としてはBitLocker回復キーだけでなくMicrosoftアカウントでのWindowsサインイン用パスワード入力も自動化できるので、やたら長いパスワードを設定して普段指紋認証している人も安心（危険）かもしれません。

== 想定するBitLocker回復キーの入手手段

ローカルに平文保存されていないことを前提とします。

 * 1PasswordやLastPassといった、なんらかのsecure-vaultに格納されている
 * Microsoftアカウントのログインを前提とした遠隔ストレージに格納されている

== 使うもの

 * vncdotool（@<href>{https://github.com/sibson/vncdotool}）
 ** VNC経由のキー入力・マウスクリック・画面キャプチャといった操作全般を自動化するツール
 * Display VNC
 ** KVM組み込みのVNCサーバー

== 設定手順

=== vncdotoolをインストール

@<href>{https://github.com/sibson/vncdotool}に記載のとおりですが、@<code>{pip}をインストールした状態で

//cmd{
$ pip3 install vncdo
//}

の一発インストールです。

=== KVMのマシン設定を変更し、モニターをSpice→VNCへ

KVM+virt-managerは内蔵モニターの標準としてSpiceを採用しています。
これを@<code>{VNC server}へと変更すると、ローカルまたはリモートからアクセスできるように設定可能です（@<img>{2020-04-06-vnc-server}）@<fn>{vbox-vnc}。

//image[2020-04-06-vnc-server][KVMで利用するモニターをVNC serverへと変更する]{
//}

Address指定は@<code>{Localhost only}にしておくのが無難です。
VNCの1桁番台ディスプレイは何かと埋まっていることが多いので、5910番ポート(=10番ディスプレイ)を指定しました。

//footnote[vbox-vnc][ちなみにVirtualBoxもVNC経由でアクセスするルートを用意してくれているので、つまりそういうことです]

=== BitLocker回復キーの送信用スクリプト作成

//emlist{
#!/bin/bash

VNCDISP='10'

read -sp "BitLocker recovery key: " BLKEY
vncdo -s localhost:$VNCDISP type "$BLKEY"
echo ""
//}

シンプルなスクリプトですが、VNCのディスプレイ番号はさきに指定したものと揃えておいて下さい。

bash組み込みのreadコマンドの機能を使っているので、/bin/sh経由ではダメです。
@<code>{~/bin/bitlocker-unlock}というファイル名で作成しました。
実行権限は与えておきます。

== 使い方

 * Windows 10 VMを起動
 ** virt-managerのUIからでも、CLIでも、好きな方法で
 * virt-managerのコンソールを開いている場合は一旦閉じる
 ** vncdoはVNCクライアントとして振る舞います。VNCでは複数クライアントからの入力は原則受け付けないので、virt-manager側を一旦閉じます
 * BitLocker回復キーをクリップボードへコピー
 * @<code>{bitlocker-unlock}コマンドを実行
 * BitLocker回復キーをクリップボードからペースト
 * virt-managerのコンソールを（再び）開き、回復キーが入力されたことを確認して続行

VM起動、一定時間のウェイト@<fn>{intelligent-wait}、キー入力と改行コード送信までは簡単にアレンジできるでしょう。
ほどよく楽をしていきましょう。

//footnote[intelligent-wait][@<code>{vncdo}には@<code>{rexpect}という矩形領域のビットマップが指定ファイルの内容と一致するまでwaitをかける機能もあり、このあたりを使えば適切なタイミングをとる自動化も可能です]

== 簡単なリスク評価

BitLockerの回復キーはそれなりの機密情報です。
各ユーザーのドキュメントディレクトリへのアクセスを許す可能性が高いので、雑に管理するとBitLockerでの暗号化の意味合いを大きく損なう恐れがあります。

今回の想定ワークフローにおいて情報を守れるパターンと守れないパターンを簡単に整理しておきます。

=== 守れるもの

readコマンドの@<code>{-p}オプションでプロンプトを表示、@<code>{-s}オプションでエコーバックを抑止することで

 * ログインシェルのhistoryにキーが残らないように
 ** Linux環境のホームディレクトリが暗号化されていない場合やアンロック状態で物理アクセスされる場合に保護
 * ターミナルの画面上ヒストリにキーが残らないように
 ** 覗き見やうっかり離席時にログから拾われることを防止

=== 守れないもの

 * BitLocker回復キーを流し込む瞬間にリモートまたはローカルから当該ユーザーの権限でプロセス一覧を取得できる権限がすでに奪われている場合、@<code>{vncdo}コマンドのパラメータを盗み取ることでBitLocker回復キーを奪われる可能性がある
 * クリップボードの監視を伴うキーロガーを仕込まれている場合、BitLocker回復キーを奪われる可能性がある

== 別の道筋

今回は@<code>{vncdo}の@<code>{type}サブコマンドを利用しましたが、@<code>{typefile}というサブコマンドも用意されており、こちらは指定ファイルの内容を入力する挙動をします。

今回想定した環境は「LinuxとWindowsが同一SSD上に同居しており、物理Linuxから仮想Windowsを起動して利用する」というものです。
この場合、Linuxのユーザーディレクトリが十分に守られていなければWindows側のセキュリティレベルがLinux側に引っ張られて下がってしまいます。
このため、Linux側の環境になるべくBitLocker回復キーの痕跡を残さずに済むことを重視しました。

Linuxローカル環境の防御に極振りしている環境であれば、@<code>{typefile}を利用しても十分なセキュリティ水準を保つことが可能でしょう。
