開発日誌(by Liva)
=================

About
-----
大きな変更などがコミットされた時はこの日誌上で簡単に解説します。
コミットログを追わなくても、ここさえ読んでおけば大体追いかけられる感じにできるといいなぁ、と。

2018/01/29
----------
Thread周りのバグが結構残っていたので、その修正（あまりにも修正コミットが多すぎるのでコミットへのリンクは省略）

[fa99c36](https://github.com/PFLab-OS/Raph_Kernel/commit/fa99c368f2f2b8d4795f85f90b171ad5876dec55)
hikaliumさんの提案により、clang-formatの導入を決定。（上のコミットもhikaliumさんから）
formatされていないコードはCIで落ちるようになった。

[20c4a3c](https://github.com/PFLab-OS/Raph_Kernel/commit/20c4a3c8ec06efb5508f148c7be246be04ce8326)
UdpCtrl::DummyServerを自走起動するプロトコルスタックにした。
これに伴い、udp_setupは廃止

2018/01/13
----------
[9aae0c7](https://github.com/PFLab-OS/Raph_Kernel/pull/171/commits/9aae0c7664bf4bb18d789632c9c05f71113efb18)
RingBuffer2の改良
RingBuffer2を書き直し、制約（マルチスレッドでのPush、割り込みハンドラ内からのPop）を撤廃
これにより、RingBufferはいかなる状況下でも使えるデータ構造となる。

[d2eaa4c](https://github.com/PFLab-OS/Raph_Kernel/commit/d2eaa4c0fd96d8485779c38d8655498c3b595e6e)
RingBufferをRingBufferで置き換え

2018/01/05
----------
[0dd8ca7](https://github.com/PFLab-OS/Raph_Kernel/commit/0dd8ca7af894adc02c779cd7695649a692d30019)
マルチスレッドを利用したユニットテストがCI上でスタックする事が多いので、CIで動かす場合はこれらの類のテストを行わないようにした。
新しいCI環境の導入が急務。それまではレビュー時の手動テストで確認するしかない。

2017/12/24
----------
[a8e9f4f](https://github.com/PFLab-OS/Raph_Kernel/commit/a8e9f4f648c176e4e69f91ae1a5ee5542d35228a)
Queueを全てNewQueueでリプレースし、NewQueueをQueueへ改名

2017/12/23
----------
[7a69e92](https://github.com/PFLab-OS/Raph_Kernel/pull/158/commits/7a69e92da0cedc6ab4be79b6b525138c2a98d494)
新しいスケーラブルなキュー(NewQueue)の実装
スピンロックベースで遅い現状のキューのリプレースを視野に入れて、IntQueueが導入されたが、このキューは複数のスレッドからのPopが出来ない。
これを補完する物として、複数スレッドがPopできる（が、特定のスレッドからしかPushできない）RingBuffer2が実装されており、プログラミングモデルとしてはこれで十分なはずだが、状況によってIntQueueを使ったり、RingBuffer2を使ったりと使い分けるのはあまりプログラマフレンドリーではないため、複数スレッドからのPush,Popに対応した新しいキューを実装した。

NewQueueはIntQueueと比較するとPushの性能は低下するが、RingBuffer2と比較するとPop性能は大幅に向上し、かつバッファサイズ制限も無くなる。
今後RingBufferやRingBuffer2はNewQueueに統合していく予定。

2017/12/18
----------
[ca839fc](https://github.com/PFLab-OS/Raph_Kernel/commit/ca839fc57c533e562e6d02d0b57abfe822129a63)
Doxygenによって生成されたhtmlファイルの削除
htmlファイルは更新されていない & Doxygen向けのコメントも書かれていない & ルートディレクトリでのgrepを阻害する 等の理由により除去
ソースコードの解説は、[ドキュメント](#!doc/index.md)に今後移行

[85f4962](https://github.com/PFLab-OS/Raph_Kernel/commit/85f4962d34a25f79a17b88a94dcd6932d10cc0ff)
makeの高速化。
makeの際、ディスクイメージのマウント、アンマウントに時間が掛かるので、不要な場合はさせないようにした。

現状、再コンパイルが省略されてもディスクイメージの再ビルドが走ってしまうので、/tmp/image以下にディスクイメージ内と同一のデータを配置し、このデータとの更新日時の比較により、ディスクイメージ内へのデータコピーの必要可否を判断するようにしている。

2017/12/14
----------

[209f466](https://github.com/PFLab-OS/Raph_Kernel/commit/209f4662a11167024b2b97407b9066635e0958bb)

DHCPクライアントを実装
また、setipコマンドを変更

例：
```
setip ixgbe1 dhcp
```

```
setip ixgbe1 static 192.168.12.1
```

これまでは`setip ixgbe1 192.168.12.1`だったので、互換性がない事に注意

2017/12/13
----------

[42d4d31](https://github.com/PFLab-OS/Raph_Kernel/commit/42d4d3128f69beae84172cf7700ba4083ec91ba9)

TaskCtrlをThreadCtrlに置き換え

これまでのTaskCtrlの問題点

* TaskCtrl::Run()が複雑で処理がわかりづらい
* CalloutとTaskで異なるインターフェースを呼び出す必要があり、煩雑
* スマートポインタを用いて管理しようとすると、循環参照が発生してメモリリークせざるを得ない

これらを解決するため、TaskCtrlを廃止し、新規設計したThreadCtrlにリプレースした。
また、Join等のメソッドを新たに追加し、利便性を向上させた。

ThreadCtrlの使い方は[ドキュメント](#!doc/kernel/thread.md)を参照。TaskCtrlとは大きく使い方が異なるので注意。

