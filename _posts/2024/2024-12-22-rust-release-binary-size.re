---
layout: post
title:  "Rust製ツールのリリースバイナリを無理なくコンパクトにする"
date:   2024-12-22 20:17:00 +0900
---
= Rust製ツールのリリースバイナリを無理なくコンパクトにする

シンプルな機能のRustプログラムを書いていたら、リリースビルドでも420KB超と予想以上にサイズが大きくなった。バイナリサイズの削減がどの程度現実的なのかをざっと検証してみる@<fn>{min-sized-rust}

== 初期状態の確認

releaseビルドの初期状態で429104 bytes

@<code>{cargo bloat}で中身を確認すると、大半が標準ライブラリ由来、特にpanic時のハンドラから呼ばれていると思しきbacktrace生成用のライブラリが上位を占める

//cmd{
$ cargo bloat --release -n 10
...

 File  .text     Size Crate Name
 0.9%   6.3%  14.7KiB   std std::sys::backtrace::_print_fmt::{{closure}}
 0.7%   5.2%  12.1KiB   std std::backtrace_rs::symbolize::gimli::Context::new
 0.6%   4.0%   9.4KiB   std gimli::read::dwarf::Unit<R>::new
 0.6%   4.0%   9.3KiB   std miniz_oxide::inflate::core::decompress
 0.4%   2.9%   6.9KiB   std addr2line::ResUnit<R>::find_function_or_location::{{closure}}
...
//}

== バイナリサイズの削減

=== Cargo.tomlへの定番設定

Cargo.tomlに以下を追加する

//emlist{
[profile.release]
# opt-level = "z"
lto = true
codegen-units = 1
strip = true
panic = "abort"
//}

@<code>{panic = "abort"}がキー！という書き方をしているblog記事類も見つかるが、ここへの期待度はさほど高くない。
よほどきれいにstripされない限りは周辺ライブラリの容量削減は見込めないため

これらの設定を追加後にリビルドして326040 bytes、最初の429104 bytesと比較すると24%の削減

@<code>{opt-level = "z"}指定を有効化するとさらに9KBほど減るが、速度を犠牲にする判断をしたくないケースも多い気はするので今回は最終的にコメントアウトした。
典型的には短めの固定長配列に対するループのアンロール処理を無効化することでサイズを削るのだろう

=== stdのコンパクト版ビルド

2018年頃には確立していた手法として、stdの実行時用コンパクト版をビルドする方法がある@<fn>{xargo}。
MSVCでいうところのVC++ Runtime debug版とrelease版のようなもので、Rustでは通常debug時でもrelease時でも同一のプリビルトstdライブラリを静的リンクしているが、ビルド時にソースからstdを構築してバンドルする仕組み（概念としてはstd Aware Cargoと表現されている@<fn>{std-aware-cargo}によって不要なものを削ってバイナリサイズを削れる。
当初はXargoで実現されていた機能だが、現在はnightly版の@<code>{build-std}を使用する

//cmd{
$ cargo +nightly build -Z build-std=std,panic_abort -Z build-std-features=panic_immediate_abort --release
//}

ここで指定している@<code>{-Z}オプション群はnightly版のCargoでしかまだ使えない（2024年12月時点）ので、rustupから予めnightly版のtoolchainとstdビルド用のrust-srcを導入しておく必要がある

//cmd{
rustup toolchain install nightly
rustup component add rust-src --toolchain nightly
//}

@<code>{build-std-features=panic_immediate_abort}指定でCargo.toml側の@<code>{panic = "abort"}指定を補完しているのは良い

これで26KBまで劇的に削減できる

なお、stdをビルドする都合上リリースビルドの所要時間は伸びる。
手元の古いPCでは30秒程度のオーバーヘッドが発生するが、リリースビルドをそう頻繁にやるわけではないので開発体験を悪化させる性質のものではない。
配布バイナリのサイズ最適化という文脈では許容できる範囲と考えられる

ただ、build-stdは前述のように2024年時点でもnightlyの機能である。
std-aware Cargoの仕様策定には複雑な要件が絡み、安定化のためのディスカッションポイントはどれもパワーが必要そうで、すでに数年間議論が止まっていたり当初の議論リード者の関与が薄くなっている点も複数ある。
バイナリサイズ削減がビジネス価値につながる企業が急にスポンサーとして付く、のような大幅加速イベントが発生しない限り、2030年ぐらいまでnightlyのままであっても驚きはない状況に見える

@<code>{build-std}はまだpreliminaryでexperimentalな存在という扱いであることには注意が必要。
開発段階で各種自動テストを通していても言語ランタイム/標準ライブラリが差し替えられた状態でそれらの結果が信用できるか、というのは現状での各種リリースバイナリをbuild-std版のstdライブラリと静的リンクして出力することを躊躇するに十分な理由で、そう考えるとよほど特殊な事情がない限りはnightly範囲外のCargo.toml設定群で満足しておくのが無難ではある

== 実行環境に応じた検討事項

=== 配布サイズと実行時のトレードオフ

配布時のサイズをさらに小さくする選択肢としてはUPXによる実行可能ファイルの圧縮@<fn>{upx}やlzipなどの一般的な圧縮形式の利用が考えられる。
UPXは実行時にメモリ上でバイナリを展開する必要があり、実際の配布サイズとしては一般的な圧縮形式と大きな差は出ないため、見かけ上の容量削減（自己満足とも言える）に留まる可能性がそれなりに高い。
たしかに今回のターゲットでは26KB→15KBぐらいに縮みはしたが作っているツールの性質的には特にUPXは不要だと感じた

=== WebAssembly向けの考慮

wasm環境での実行を想定する場合、stdをカスタムビルドするよりも@<code>{no_std}を選択するほうが現実的なケースも多そう。
しかしstdの要素は案外あちこちで必要になるのでその都度代替実装を探したり書いたりとしていくハードルは結構高い@<fn>{nostd-playbook}

//footnote[min-sized-rust][@<href>{https://github.com/johnthagen/min-sized-rust} というチュートリアルが丁寧に各手法を紹介しているので参考になる。結果としてはこの内容を辿っていった]

//footnote[std-aware-cargo][@<href>{https://github.com/rust-lang/wg-cargo-std-aware}]

//footnote[xargo][Xargoは2018年以降アクティブな開発は行われていない。 @<href>{https://github.com/japaric/xargo/issues/193}]

//footnote[upx][Ultimate Packer for eXecutables @<href>{https://github.com/upx/upx}]

//footnote[nostd-playbook][wasm向けのno-stdについては@<href>{https://hackmd.io/@alxiong/rust-no-std}が出発点としてよさそう]