ThreadCtrl
==========

概要
---
カーネル内スレッド
指定マイクロ秒後に実行(callout)したり、スタックスイッチが可能

関連ファイル
-----------
[source/kernel/thread.h](https://github.com/PFLab-OS/Raph_Kernel/blob/develop/source/kernel/thread.h)
[source/kernel/thread.cc](https://github.com/PFLab-OS/Raph_Kernel/blob/develop/source/kernel/thread.cc)

ThreadCtrl
----------
Threadを管理するためのクラス。runqueueやwaitqueueの管理、Threadリソースの管理を行う。
各コアごとに存在し、Threadは、自分の属するThreadCtrl上のqueueで実行を待つ。

ThreadCtrlのqueue
----------------
runqueueとwaitqueueが存在する。
runqueue上のスレッドはラウンドロビンで順次実行される。
waitqueueは実行予定時刻までThreadが待機するためのキューで、現在時刻が実行予定時刻を過ぎるとそのThreadはrunqueueに移動させられる。

Thread
------
カーネルスレッド本体
ThreadはThreadCtrlが紐付くコア上でのみ実行可能で、他のコアでは実行できない。

ThreadCtrl::AllocNewThreadを呼び出す事で新しいインスタンスを確保できる。
それ以外でThreadのインスタンスを確保する手段はない。
AllocNewThreadはuptr<Thread>を返すため、Threadを直接扱う事はできず、uptr<Thread>の形でしか扱えない。

初期化時にStackStateを設定するが、これによってそのThreadがスタックスイッチ可能かどうかを設定する。

Thread::Operator
----------------
Threadの制御はThread::Operator経由で行う。Thread::CreateOperator()によってThread::Operatorを取得可能。

Threadがuptr<Thread>としてしか扱えないが、Thread::Operatorであればコピー可能であるため、Threadを扱う上で問題はないはず。Thread::Operatorはuptr<Thread>のweak pointerであると考えるのが良い。

ただし、Thread::Operatorの参照カウントが0でない時にuptr<Thread>が開放されると、kernel panicが発生するので注意。

Thread::StackState
------------------
kIndependentの場合は、そのスレッドはスタック切り替えを行える。
kSharedの場合はスタック切り替えを行えない。

Threadの停止/スタック切り替え
----------------------------
Threadの実行を一時中断し、ThreadCtrlに処理を戻す事。ThreadCtrl::WaitCurrentThread()によって現在のThreadを停止する事ができる。
中断時のスタックが保存されているため、Thread::Operator::Schedule()によってそのThreadが再度ThreadCtrlのキューに登録されると、途中から実行を再開する事ができる。

Thread::StackStateがkIndependentの時のみThreadを停止できる。kSharedの場合はassertionで落ちる。

Threadの開放
-----------
uptr<Thread>が開放された時点でそのThreadは開放される。実行中のThreadは終了するまで開放が遅延されるが、queue内のThreadはqueueから削除され、実行されない。

Thread::Join()
-------------
「Thread::Join()を実行したThreadを」停止する。このThreadがスタック切り替え可能でない場合、assertionで落ちる。Thread::Join()が終了した時、Thread::Join()を実行したThreadに対してThread::Operator::Schedule()が呼び出される。
Thread::Join()によって停止されたスレッドは、他のThreadによるThread::Operator::Schedule()によっても実行が再開される可能性がある事に注意。

Threadの終了とは、Threadがqueueに投入されておらず、このThreadのOperatorインスタンスが１つも存在しない状態である。

Thread::CreateOperator()
------------------------
Threadに紐付けられたThread::Operatorを取得する。

Thread::Operator::Schedule()
----------------------------
ThreadCtrlのqueueにThreadを投入する。
引数が無い場合はrunqueueに投入され、引数がある場合は、指定時間後に実行されるよう、waitqueueに投入される。

Thread::Operator::Stop()
-----------------------
queue上のThreadを除去する。既に実行中の場合は、実行が終了した時点で（再投入されても）除去される。

ThreadCtrl::GetCtrl()
---------------------
指定されたコアのThreadCtrlを取得する。

ThreadCtrl::GetCurrentCtrl()
----------------------------
現在実行中のコアのThreadCtrlを取得する。

ThreadCtrl::AllocNewThread
--------------------------
ThreadCtrlが管理するThreadを新しく確保する。

ThreadCtrl::WaitCurrentThread
-----------------------------
現在実行中のThreadを停止し、スタックを切り替えてThreadCtrlに処理を戻す。
現在実行中のThreadのStackStateはkIndenpendentでなければならない。

ThreadCtrl::GetCurrentThreadOperator
------------------------------------
現在実行中のThreadのOperatorを取得する。

サンプルコード
-------------

### Example 1
```c++
uptr<Thread> thread = ThreadCtrl::GetCtrl(cpuid).AllocNewThread(Thread::StackState::kIndependent);
do {
  auto t_op = thread->CreateOperator();
  t_op.SetFunc(make_uptr(new Function<void *>([](void *){
          static int cnt = 0;
          cnt++;
          if (cnt < 5) {
            ThreadCtrl::GetCurrentThreadOperator().Schedule(1000);
          }
       }, nullptr)));
  t_op.Schedule();
} while(0);
thread->Join();
```

このコードはThread::SetFunc()で登録された関数を5回実行する。

Threadを扱う上でのポイントは以下の通り。

1. do {} while(0);のスコープによって、Thread::CreateOperator()で作成されたOperatorを開放した後にThread::Join()を実行する。これを行わない場合、Thread::Operatorの参照カウンタが0にならないため、Thread::Join()が戻って来ない。

2. Functionの引数にt_opを渡す事も可能だが、この場合Thread::Operatorの参照カウンタが0にならないため、Thread::Join()が戻ってこない。このため、スレッド関数内部ではThread::OperatorThreadCtrl::GetCurrentThreadOperator()からThread::Operatorを取得するのが良い。

3. Thread::SetFunc()で登録されたFunctionは内部で自分自身に対するThread::Operator::Schedule()を呼ぶため、Thread::Join()が呼ばれない。cnt == 5となり、Thread::Operator::Schedule()が呼ばれないと、Threadはqueueから完全に除去され、かつThread::Operatorの参照カウンタも0になるため、Thread::Join()に処理が返る。

設計意図
--------

* なぜThreadはuptrでしか手にはいらないのか

まず、生ポインタで管理するのは論外。
使い終わったThreadはThreadCtrlに返却され、再利用される。生ポインタの場合、ポインタの不適切な運用が発生する可能性があり、特定が難しいバグに繋がる可能性があるため、スマートポインタでは管理される必要がある。

次に、sptrはuptrよりもコストが高いため、できれば使いたくない。また、uptrはインスタンスが１つしか存在しないため、「Threadを管理する親」の存在を明確化する事ができる。

よってThreadはuptrで管理し、実際の制御はThread::Operatorを用いる。

* なぜThread::Join()はThread::Operatorの関数ではないのか

Thread::Join()はThread作成元の関数によって呼び出される。（それ以外のスレッドから呼び出したくなったら、設計を再考すべき）
作成元の関数はuptr<Thread>を保持しているはずなので、Thread::Join()を呼び出せる。逆に他のスレッドからThread::Join()を呼ぶ状況は考えにくいため、Thread::Operatorには敢えて実装されていない。