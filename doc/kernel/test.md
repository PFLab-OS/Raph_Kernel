ユーザー空間ユニットテスト
=========================

概要
---
カーネル内で使用するモジュールのうち、ハードウェアと密結合していない部分をテストする事ができる。
（主に、データ構造等のアルゴリズム実装系クラスに対して使用する事を想定）

このユニットテストはCI実行時にも走る。逆に言えば、developにmergeされた物はユニットテストには成功している事になる。

テスト方法
---------
ルートディレクトリで

```
(host/vagrant)$ make test
```

とする。

テストの書き方
-------------
source/unit_test ディレクトリ内にテストコードを記述するための.ccファイル（以後、テストファイルと呼ぶ）を書く。
ヘッダを用意する必要はないし、Makefileにファイルを追加しなくても自動で取り込まれる。

まず、テストファイルの先頭に`#include "test.h"`と書く。
次に、「テストの数だけ」テストクラスを作る。テスト毎に「意味のある」テストクラス名をつける事。

以下に、テストクラス(TestA)の最小構成を示す。
```c++
class TestA : public Tester {
public:
  virtual bool Test() override {
    return true;
  }
private:
} static OBJ(__LINE__);
```

Testerクラスを継承し、Tester::Test()を実装すれば良い。
Tester::Test()の戻り値はテスト結果に対応するが、基本的にはtrueを返し、テストの失敗はkassertでハンドリングする事。これによって、戻り値だけでテストの失敗成功を示すよりも、より多くの情報を得る事ができる。

作成したクラスについてグローバルなインスタンスを１つ作成すれば、自動的にテストが呼び出される。
最後のstatic OBJ(__LINE__)においてそれを実現している。

libc
----
ユニットテストはLinuxアプリケーションとして動作するため、テストクラス内ではlibcを自由に用いる事ができる。

kassert
-------
assertionによるチェックを行いたい場合はassertではなく、kassertを使うのが良い。
あるテストがkassertで落ちた時、内部では例外として処理され、テスト呼び出し元が適切に例外ハンドリングをした後、他のテストが継続して呼び出される。
（assertを用いてしまうと、assertが失敗した時点でテストが止まってしまう）

kassertで落ちる事をテストしたい時
--------------------------------
テスト対象の関数について、「敢えてinvalidな引数を与え、それがkassertによって落ちる事を確かめたい」時は以下のようなコードを書くと良い。

```c++
bool err = false;
try {
  // test code
} catch (ExceptionAssertionFailure t) {
  err = true;
};
kassert(err);
```

これによって、関数の内部で発生したkassertがハンドリングされた場合は最後のkassertを通過し、そうでない場合は弾く事ができる。

スレッドを使う場合
-----------------
ユニットテストにおいては、スレッドを用いて並行処理を行う事ができるが、kassertのハンドリングが面倒になる場合があるので注意する事。

スレッド内で発生したassertion failureを親スレッドで検知したい場合は以下のようなコードを書けば良い。
```c++
#include <pthread.h>
#include <thread>
#include <exception>
#include <stdexcept>

class TestA : public ThreadTester {
public:
  virtual bool Test() override {
    std::exception_ptr ep;
    std::thread th(&QueueTester_ReusePoppedQElement::Func, this, &ep, i);
    th.join();
    bool rval = true;
    try {
      if (ep) {
        std::rethrow_exception(ep);
      }
    } catch (ExceptionAssertionFailure t) {
      t.Show();
      rval = false;
    }
    // kill other threads
    return rval;
  }
private:
  void Func(std::exception_ptr *ep) {
    try {
      // test code
    } catch (...) {
      *ep = std::current_exception();
    }
  }
```
複数のスレッドを実行している場合、ある子スレッドがassertionで落ちたら、他の子スレッドも終了させる必要があり、上のようにして親スレッドに通知しなければならない。
assertion失敗時のメッセージはExceptionAssertionFailure::Show()によって行われる。また、ExceptionAssertionFailureをキャッチしてしまうと、assertionに失敗した事がテストクラス呼び出し元に伝わらないので、TestA::Test()の戻り値としてfalseを返している。

また、継承クラスがTesterではなく、ThreadTesterである事に注意する事。ThreadTesterはCI環境下でのテストの実行をスキップする。（Circle CI環境のマルチスレッド性能が貧弱であると推測されるため）
