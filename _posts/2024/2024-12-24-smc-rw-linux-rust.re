---
layout: post
title:  "Linux向けSMC読み書きツール（I/Oポートアクセス）をRustへ移植した"
date:   2024-12-24 12:02:00 +0900
---
= Linux向けSMC読み書きツール（I/Oポートアクセス）をRustへ移植した

本記事は@<href>{https://qiita.com/advent-calendar/2024/rust, Rust Advent Calendar 2024} Series 3、19日目のエントリです

== 動機

私は日常の外出時用PCとしてMacBook 2016を使っています。
このPCはmacOSのサポートが切れていることもあってUbuntuで使っているのですが、とっくにビンテージ扱いのPCにおいてはバッテリー寿命をのばすために充電上限を設定し、適宜設定更新する必要があります

もちろんこのような端末固有のきめ細かな制御ツールはUbuntu標準機能に入っていないので、以前GNOME拡張として実装しました@<fn>{itawaly}。
しかしUbuntu 22.04からUbuntu 24.04へアップデートするとGNOME拡張のサイドロード方法が変わったようで動作しなくなってしまいました

//footnote[itawaly][@<href>{https://github.com/muojp/itawaly/}]

個人的にはあまりGNOME Desktop固有の事情に深入りしたくないので、他の方法で書き直すことにしました。
PCのI/Oポートを叩く都合上、root権限を付与した単機能のコンパクトなバイナリで実際のI/Oポートオペレーションをおこないつつ、高水準側に便利機能を実装してなんらかのプロセス間通信で連携させるのですが、この低水準側で従来使っていたSmcDumpKey.c（@<href>{https://github.com/floe/smc_util/blob/master/SmcDumpKey.c}）には若干データ出力上の不具合があり、どうせ手を入れるならリライトもアリか、と考えました

あまりCのコードをメンテし続けたくはないので、I/Oポートアクセスのための最小限のコードだけをCの世界に残しつつRustで書き直しました

コードは@<href>{https://github.com/muojp/smc-rw-linux/}にあります

ちなみにRustでのSMCアクセスについて、Swiftで書かれたmacOS用の実装やRustのmacOS用クレート@<fn>{smc-crate}は見かけたのですが、Linux向けに書かれた実装はサッと見つけられませんでした

//footnote[smc-crate][@<href>{https://crates.io/crates/smc}]

== 実装方針

以前はCで書かれたユーティリティーをビルドしてGNOME拡張のJavaScriptコードからプラットフォーム呼び出しで利用していました。
I/Oポートアクセス権限の都合上、便利機能の提供側とI/Oポートアクセス部分は別プロセスにしたいので、構成としては今回もさほど変わりません

Rustでの実装は次のような方針で進めました

 * ccでC側とのインタフェースを用意し、IoPortRw traitでOS差分を吸収
 * それを実装するLinuxIoPortRwに実際のI/Oポートアクセスのunsafe部分を置き
 * これらを利用する低水準側操作としてSmcPrimitive traitを定義
 * 今回作ったMacBook用実装はDefaultSmcRw（リポジトリ内にデバイス製造企業名を残したくなかったので曖昧な命名にしています）として実装
 * 高水準側はSmcOperationのデフォルト実装として作り、テスト範囲を低水準側と切り離しやすくする

== 実装上のポイント

Linuxのsys/io.hはaarch64環境には存在しません。
動作ターゲットのMacBook 2016はIntelアーキテクチャですが私は普段の開発をaarch64のLinux環境でしているので、アーキテクチャ依存部分を分離する必要がありました

//emlist{
#if defined(__x86_64__)

#include <sys/io.h>

void port_outb(unsigned char value, unsigned short port) {
	outb(value, port);
}

unsigned char port_inb(unsigned short port) {
	return inb(port);
}

int port_ioperm(unsigned long from, unsigned long num, int turn_on) {
	return ioperm(from, num, turn_on);
}

#else

void port_outb(unsigned char, unsigned short) {
}

unsigned char port_inb(unsigned short) {
	return 0;
}

int port_ioperm(unsigned long, unsigned long, int) {
	return 0;
}

#endif
//}

若干乱暴ですがこれでクロスターゲットのテスト可能範囲をだいぶ増やせました

== テスト戦略

前述のようにtraitをSmcPrimitiveとSmcOperationと低水準・高水準に分けて実装し、SmcOperationのデフォルト実装に高水準APIを実装することで、低水準側をmockにしたテストをしやすくしました。
この分離がなければ無限にダミーI/Oポート応答シナリオを書くことになってなかなか不毛です

//emlist{
#[cfg(test)]
    mock! {
        SmcTest {}
        impl SmcPrimitive for SmcTest {
            type IoPort = MockIoPortRw;
            fn new(io_port_rw: MockIoPortRw) -> Self;
            fn wait_read(&self) -> libc::c_int;
            fn send_byte(&self, cmd: u8, port: u16) -> Result<libc::c_int, String>;
            fn send_argument(&self, key: [u8; SMC_KEY_NAME_LEN]) -> Result<(), (usize, String)>;
            fn recv_byte(&self) -> Result<u8, ()>;
        }
    }
//}

アプリケーション開発の中でも自動テスト部分ではやはり特殊なことをするケースがそれなりにあって（典型的には多くの人が初めてリフレクションやRTTIを意識するのはテスト文脈ではないでしょうか）、テスト補助ライブラリが複雑な状態に基づくテストケースをテストターゲットの設計へあまり踏み込まずに書ききれる筋力を持つか否かというのは言語自体の書き味と同じくらい重要な場合もあります。
もちろん、testableにするためのプログラム構造変更がよりよい設計につながるケースも多いため、テストの窮屈さは設計改善のシグナルのひとつでもありますが、「ここはこれでよし」と定めた際にその方針通り走り切れることもまたテスト補助ライブラリの出来と言語の記述力次第というか。

今回、I/Oポートへの入力値に応じて出力が想定どおりに変化することを確認するためにArcを介した共有メモリでのデータのやり取りが必要になったり、テストの並列実行時にmock生成部分がスレッドセーフではない挙動をする（flakyに失敗する）ためmockのセットアップ部分へMutexベースのロックを利用したりと細かな手当は必要となりましたが、逆に言えばそれだけで済んだのでmockallは今回の用途では十分に実用域で開発を強力に支えてくれると感じました

automockでは足りずmock!{}ブロックを書く必要が生じるのはmockall設計上のイレギュラーパターンに半分足を突っ込んでいる状態で、あまりサンプルが豊富ではないしコンパイルエラー時のメッセージも分かりづらくなるためあまり複雑化しないほうが良い気配はありました

今回のテストコードを書いている際の学びとして、不正データを投入した際に想定通り@<code>{Err()}が戻ってくることを確認するのに加えて、高水準APIにおいては想定通りpanicで実行を止めてくれることを確認したかったので、次のようなコードを書きました

//emlist{
let hook = panic::take_hook();
panic::set_hook(Box::new(|_| {
    
}));
data.data_len = 16;
let result = std::panic::catch_unwind(|| {
    let mut short_buf = [0u8; 8];
    smc_rw.read_smc(SMC_GET_KEY_TYPE_CMD, [0x01, 0x02, 0x03, 0x04], &mut short_buf[0..data.data_len as usize])
});
panic::set_hook(hook);

assert!(result.is_err());
match result {
    Err(panic_msg) => {
        if let Some(msg) = panic_msg.downcast_ref::<String>() {
            assert_eq!("range end index 16 out of range for slice of length 8", msg);
        }
    }
    Ok(_) => panic!("should receive an error")
}
//}

もともとは@<code>{std::panic::catch_unwind}で単純に情報取得していたのですが、stdoutに出てくるbacktraceが割と邪魔だと気付いたので抑制したいと思ったのがきっかけで対応を練りました。
処理としてはpanic時のフックを一時的にほぼ空の実装へ置き換えてテスト実行、その後にもとのフックへ戻しています

想定通りbacktraceが出力されなくなって自動テストの出力が汚れることはなくなり、さらに副作用として、手元PCでこのテストケースだけ150msほどかかっていたのが数msまで短縮されました。
やはりbacktraceの生成は普通に重い処理なんだな、、という実感がありました（@<code>{cargo test}時は通常Debugビルドで動いているという都合もあるでしょう）

== まとめ

今回の用途は、低水準のI/Oを叩きに行くコンパクトなユーティリティーをそこそこ見通しよく書けてメモリ安全性を確保しやすくテストコードもそれなりに書きやすくコンパイル後のバイナリが許容可能なサイズに収まる@<fn>{binary-size}、というRustとCargo/クレートエコシステムの良さを活かしやすいものでありました

//footnote[binary-size][リリースバイナリが667KiBで決して小さくはないのですが、そのあたりは別エントリに書いたので参照ください。@<href>{https://b.muo.jp/2024/12/22/rust-release-binary-size} ちなみにbuild-stdでビルドすると348KiB、CLI処理用のclapが大半を占めます]
