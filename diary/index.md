開発日誌(by Liva)
=================

About
-----
大きな変更などがコミットされた時はこの日誌上で簡単に解説します。
コミットログを追わなくても、ここさえ読んでおけば大体追いかけられる感じにできるといいなぁ、と。

2017/12/18
----------

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
* TaskCtrl::Runが複雑で処理がわかりづらい
* CalloutとTaskで異なるインターフェースを呼び出す必要があり、煩雑
* スマートポインタを用いて管理しようとすると、循環参照が発生してメモリリークせざるを得ない

これらを解決するため、TaskCtrlを廃止し、新規設計したThreadCtrlにリプレースした。
また、Join等のメソッドを新たに追加し、利便性を向上させた。

ThreadCtrlの使い方は[ドキュメント](doc/kernel/thread.md)を参照。TaskCtrlとは大きく使い方が異なるので注意。

