HOWTO
=====

注：雑多なメモです。

Raph_Kernelのgdbデバッグ
-----------------------
1. カーネルをqemu上で実行する。

```
(host)$ make KERNEL_DEBUG=gdb run
```

2. 別のターミナルでvagrantに接続し、gdbを起動する。

```
(host)$ vagrant ssh
(vagrant)$ cd /vagrant
(vagrant)$ make debug
```



ネットワークブート
-----------------
1. iPXEのブータブルUSBメモリを作る。

```
(host)$ make burn_ipxe
```

この際、iPXEがネットワークブート元として探すIPは今のネットワークデバイスに振られているIP。
DHCPでアドレスが変更されると動かなくなる事に注意。

2. 別のターミナルでサーバーを動かす。

```
(host)$ make run_pxeserver
```

ずっと動かしっぱなしにしておく事。また、httpのポートが空いている必要があるのでファイアーウォールに注意。

3. サーバーが配布するディスクイメージを作成。

```
(host/vagrant)$ make pxeimg
```

ソースコードを変更する度にしなければいけない操作は3のみ。