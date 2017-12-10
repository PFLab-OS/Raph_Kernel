トラブルシューティング
=====================

Vagrantにsshできない。
--------------------
VM内で

`sudo service networking restart`

をすると復活する事がある。
根本原因は不明。

VM自体が完全に死んでる事も良くあり、その場合は`vagrant reload`するしかない。