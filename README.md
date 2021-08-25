---
date: 2021-08-18
title: "coro_redis 开发手记"
linkTitle: "coro_redis 开发手记"
author: asura9527 ([@gqw](https://gqw.github.io))

menu:
  sidebar:
    name: "coro_redis 开发手记"
    identifier: c++-coro_redis
    parent: C++
    weight: 10
---


自从上次写完[《从HelloWold开始，深入浅出C++ 20 Coroutine TS》](https://gqw.gitee.io/posts/c++/coroutine/)已经有一阵子没有写C++代码了，在那篇文章中给自己立了FLAG说要继续写一篇关于asio中使用协程开发的文章也一直没有兑现。现在趁着空闲用协程写了一个redis client库。

代码还是像[mrpc](https://github.com/gqw/mrpc.git)一样追求少而精，希望大家能够学会怎样使用c++的协程，而不是直接把库拿过去用。

协程是从`C++20`开始被加进标准库的，但是仅支持基本的协程功能，用起来还不是很方便，估计需要等到`C++23`才能完善（请参考：[C++23的目标](https://zhuanlan.zhihu.com/p/107360459)）。VisualStudio 2022 PREVIEW（ToolSet v143）已经正式支持协程，相关头文件也已经从`experimental`目录移到正式目录，并且不需再添加 `/await` 编译选项（v143之前使用协程请参考[`/await`](https://docs.microsoft.com/en-us/cpp/build/reference/await-enable-coroutine-support?view=msvc-160)选项说明）。GCC从10.0版本开始支持协程，11.0版本不再是`experimental`，但还是需要 `-fcoroutines` 编译选项（请参考：[C++ Standards Support in GCC](https://gcc.gnu.org/projects/cxx-status.html)）。

以下是开发使用的环境：


|  windows   					| linux  					|
|  ----  						| ----  					|
| Windows 10 版本 2004  		| Ubuntu 20.04.2 LTS(wsl2) 	|
| Visual Studio 2022(v143)  	| GCC 11.1.0 				|
| cmake version 3.21.1		  	| cmake version 3.21.1 		|
|Vcpkg  version 2021-07-26-9425cf5f512f242c0bcbabac31f08832825aee81| Same as Windows |


## 编译说明

1. 安装[vcpkg](https://github.com/microsoft/vcpkg)
2. 设置并导出环境变量 `VCPKG_ROOT` 为 `vcpkg` 根目录
3. 如果再Winodws下开发，由于Visual Studio还是预览版，vcpkg默认不会使用，所有需要在`%VCPKG_ROOT%/triplets`目录下添加`x86-windows-v143.cmake`文件，内容如下：
```
	set(VCPKG_TARGET_ARCHITECTURE x86)
	set(VCPKG_CRT_LINKAGE dynamic)
	set(VCPKG_LIBRARY_LINKAGE dynamic)
	set(VCPKG_PLATFORM_TOOLSET "v143")
	set(VCPKG_DEP_INFO_OVERRIDE_VARS "v143")
```
4. 安装cmake
5. 执行cmake_build_x64_linux.sh(Linux) 或 cmake_build_x86-v143.bat(Windows)脚本，安装依赖包和生成工程文件


## 使用方法

协程调用通用命令：
```cpp
task<void> watchdog::coro_call_test_cmds(std::string_view msgs) {
    std::stringstream ss(std::string(msgs.data(), msgs.size()));
    std::string msg;
    while (std::getline(ss, msg, ';')) {
        auto reply = co_await redis_client::get().coro_command<void>(msg);
        if (!reply.has_value()) {
			continue;
		}
        auto r = reply.value();
        if (r) {
            LOG_INFO("echo return, type: {},  integer return: {}, content: {}", r->type, r->integer, std::string(r->str, r->len));
        }
    }
}
```
或者调用封装的方法：
```cpp
task<void> watchdog::coro_call_test() {
    auto r = co_await redis_client::get().coro_echo("test");
    auto var1 = co_await redis_client::get().coro_incr("var1");
    auto var2 = co_await redis_client::get().coro_incr("var2");
    auto var3 = std::to_string(var1.value() + var2.value());
    co_await redis_client::get().coro_set("var3", var3);
    auto var3_2 = co_await redis_client::get().coro_get("var3");
    LOG_DEBUG("r: {}, var1: {}, var2: {}, var3: {}, var3_2: {}",
        r.value(), var1.value(), var2.value(), var3, var3_2.value());
}
```
具体使用请参考测试代码：examples/301

## 代码说明

协程的本质，是通过代码的封装使得异步调用看起来更像同步调用，使得在一个协程函数内部的代码执行顺序能够像普通函数那样按照代码书写的顺序执行。就像前面例子中`coro_call_test` 协程函数内部虽然每行`co_await`后面调用都是异步调用，但是协程通过“暂停”、“恢复”的方式能够保证在一次协程函数调用过程中，代码是被从上到下依次执行的，不会出现前后颠倒，也不会并发执行。比如在得到var1前肯定不会去计算var2, 在得到var2前也不会执行求var3的代码。总之代码的执行顺序和你看到的顺序是一致的。那它与普通的函数有什么区别呢？区别在于每次执行比较耗时的操作，如远程方法调用普通的函数一直等待，并且一直占用线程资源，CPU做不了其他事情，干等。但是协程函数不一样，遇到比较耗时的操作它会挂起自己，暂停执行后面的逻辑，让出线程资源让宝贵的CPU执行其他代码指令，等到合适的时机（例如，已经得到远程方法的返回结果）再恢复执行刚才暂停后的代码。 这样协程函数的调用就会变成 “运行->暂停->恢复运行->暂停->恢复运行->...” 这样下去，并且暂停期间并不占用CPU资源。

利用协程的特性我们便可以对现有的异步代码进行改造了， 使得异步调用的代码看起来像同步代码一样简单，清晰，符合直觉，但是还能保持异步调用的效率。

既然是对异步调用的封装，那么让我们看看`reids`的异步调用的代码是怎么写的，这里我们选用`Redis`官方给出的的`hiredis`库，在此基础上进行协程封装。

```cpp
int redis_asynccall_test(std::string_view host, uint16_t port) {
	auto actx = redisAsyncConnect(host.data(), port);
	ASSERT_RETURN(actx != nullptr, 1, "connect redis failed, {}:{}", host, port);
	ASSERT_RETURN(actx->err == 0, 1, "connect redis failed, {}:{}", actx->err, actx->errstr);
	struct event_base* base = event_base_new();
	redisLibeventAttach(actx, base);
	...
	redisAsyncCommand(actx, [](struct redisAsyncContext*, void* reply, void*){
		redisReply *r = (redisReply *)reply;
		LOG_INFO("echo return, type: {},  integer return: {}, content: {}",
			r->type, r->integer, std::string(r->str, r->len));
	}, nullptr, "echo %s", "hello async");

	event_base_dispatch(base);
	event_base_free(base);
	return 0;
}
```

这里最需关心的是`redisAsyncCommand`函数，如果我们想要实现`coro_call_test`函数效果，我们可能需要写出如下代码：
```cpp
	struct result {
		std::string r;
		int var1;
		int var2;
		std::string var3;
		std::string var3_2;
	};
	auto pret = new result;
	redisAsyncCommand(actx, [](struct redisAsyncContext* actx, void* reply, void* p){
		redisReply *r = (redisReply *)reply;
		result* pret = (result*)p;
		pret->r = std::string(r->str, r->len);

		redisAsyncCommand(actx, [](struct redisAsyncContext* actx, void* reply, void* p) {
			redisReply* r = (redisReply*)reply;
			result* pret = (result*)p;
			pret->var1 = r->integer;

			redisAsyncCommand(actx, [](struct redisAsyncContext* actx, void* reply, void* p) {
				redisReply* r = (redisReply*)reply;
				result* pret = (result*)p;
				pret->var2 = r->integer;

				pret->var3 = std::to_string(pret->var1 + pret->var2);
				redisAsyncCommand(actx, [](struct redisAsyncContext* actx, void* reply, void* p) {
					redisReply* r = (redisReply*)reply;
					result* pret = (result*)p;
					pret->var2 = r->integer;

					redisAsyncCommand(actx, [](struct redisAsyncContext* actx, void* reply, void* p) {
						redisReply* r = (redisReply*)reply;
						result* pret = (result*)p;
						pret->var3_2 = std::string(r->str, r->len);

						LOG_DEBUG("r: {}, var1: {}, var2: {}, var3: {}, var3_2: {}",
							pret->r, pret->var1, pret->var2, pret->var3, pret->var3_2);
					}, pret, "get", "var3");
				}, pret, "set", "var3", pret->var3.c_str());
			}, pret, "incr", "var2");
		}, pret, "incr", "var1");
	}, pret, "echo %s", "test");
```

通过简单的对比便可以看出协程代码的优势与优雅。上面的代码实际情况可能会更加复杂，因为我们并没有处理异常情况和资源释放问题，工程实践中的处理逻辑也应该会更复杂。希望能够通过这个简单的例子能够帮你建立起对协程的好感。好了，下面就让我们来看看协程是怎么做到这么优雅的事情的。

对于异步调用，我们可以将其拆分成两个基本操作——发起异步调用和处理回调结果。 等等，这话是不是有点熟悉，刚才我们在说协程时好像也将协程分成两个步骤——“暂停”和“恢复”。仔细回味下，“发起异步调用”是不是有“暂停”的味道，是不是将任务交给其它人执行，然后自己暂停相关的逻辑，将CPU资源让给别人， 同样“处理回调结果”也意味着从前面停止的地方恢复逻辑处理。区别在于异步调用，“暂停”和“恢复”的代码是分开的（调用函数和回调函数），而协程更像是“胶水”一样将这两者连接在了一起。

## 协程实现细节

那么协程是怎样做到的呢？ 协程的使用是简单优雅的，但是协程的实现却是繁杂且充满陷阱的，就像C++其它技术一样简单留给使用者，灵活强大留给库实现者。对协程实现原理感兴趣的可以看下我的另一篇文章[《从HelloWold开始，深入浅出C++ 20 Coroutine TS》](https://gqw.gitee.io/posts/c++/coroutine/)，在此借用下里面协程展开的代码（后面简称“展开式”），方便大家理解后面的代码：

```cpp
template<typename ...Args>
return_ignore test_coroutine(Args... args) {
  using T = coroutine_traits<return_type, Args...>;
  using promise_type = T::promise_type;
  using frame_type = tuple<frame_prefix, promise_type, Args...>;
  auto *frame = (frame_type *)promise_type::operator new(sizeof(frame_type));
  auto *p = addressof(get<1>(*frame));
  return_ignore*__return_object;
  *__return_object = { p->get_return_object() };

  {
    // co_await p->initial_suspend();
    auto&& tmp = p->initial_suspend();
    if (!tmp.await_ready()) {
      __builtin_coro_save() // frame->suspend_index = n;
      if (tmp.await_suspend(<coroutine_handle>)) {
        __builtin_coro_suspend() // jmp
      }
    }

    resume_label_n:
      tmp.await_resume();
  }


  try {
	auto retm, retn, ...;
    // co_await std::experimental::suspend_always{};
	{
		auto&& tmp = std::experimental::suspend_always{};
		if (!tmp.await_ready()) {
			__builtin_coro_save() // frame->suspend_index = m;
			if (tmp.await_suspend(<coroutine_handle>)) {
				__builtin_coro_suspend() // jmp
			}
		}

	resume_label_m:
		retm = tmp.await_resume();
	}
	// co_await std::experimental::suspend_always{};
	{
		auto&& tmp = std::experimental::suspend_always{};
		if (!tmp.await_ready()) {
			__builtin_coro_save() // frame->suspend_index = n;
			if (tmp.await_suspend(<coroutine_handle>)) {
				__builtin_coro_suspend() // jmp
			}
		}

	resume_label_n:
		retn = tmp.await_resume();
	}
	// co_return ...
	auto ret = ...;
	p->return_value(ret);
    goto __final_suspend_point;
  } catch (...) {
    p->unhandled_exception();
  }

  __final_suspend_point:
    co_await p->final_suspend();

  __destroy_point:
    promise_type::operator delete(frame, sizeof(frame_type));
}
```

上面的代码便是C++编译器在我们写协程函数时给我们“偷偷”添加的代码，理解了上面的代码也就基本理解了协程。


### 协程表达式

我们看下`examples/300/`代码，这是一个比较简单的协程DEMO，在协程函数`task_test`中我们首先关注下关键字`co_await`后面被称为协程表达式的代码。

```cpp
task<int> task_test() {
	int i = 0;
	auto ret = co_await task_awaiter<int>([i](coro::coroutine_handle<task_promise<int>>){
		std::cout << "wait set: " << i << std::endl;
	}, [i]() -> std::optional<int>{
		std::cout << "resume set: " << i+1 << std::endl;
		return i + 1;
	});
	i = *ret;
	ret = co_await task_awaiter<int>([i](coro::coroutine_handle<task_promise<int>>){
		std::cout << "wait set: " << i << std::endl;
	}, [i]() -> std::optional<int>{
		std::cout << "resume set: " << i+1 << std::endl;
		return i + 1;
	});
	i = *ret;
	ret = co_await task_awaiter<int>([i](coro::coroutine_handle<task_promise<int>>){
		std::cout << "wait set: " << i << std::endl;
	}, [i]() -> std::optional<int>{
		std::cout << "resume set: " << i+1 << std::endl;
		return i + 1;
	});
	i = *ret;
	co_return i;
}
```

C++ 标准中规定协程表达式必须具有`await_ready`, `await_suspend`, `await_resume` 三个方法，也就是说一个对象只有拥有了这三个方法才可以被co_await/co_yield调用。参考上面的协程函数展开式可以知道这三个函数的作用。`await_ready`可以先不用看，这只是一个优化手段，用于决定要不要真正调用`await_suspend`。剩下的`await_suspend`, `await_resume`两个方法便对应我们前面说的暂停和恢复，你也可以把他俩简单的看成“触发异步调用”和“回调函数”，虽然并不一样但是为了方便理解可以暂且这么认为。下面让我们看下`task_awaiter`的定义：

```cpp
template<typename T>
struct task_awaiter {
    task_awaiter(std::function<void(coro::coroutine_handle<task_promise<T>>)> suspend_callback,
        std::function<std::optional<T>()> resume_callback) :
        suspend_callback_(suspend_callback),
        resume_callback_(resume_callback) {
    }

    bool await_ready() const noexcept {
        return false;
    }

    void await_suspend(coro::coroutine_handle<task_promise<T>> coroutine) {
        if (suspend_callback_) suspend_callback_(coroutine);
    }

    decltype(auto) await_resume() noexcept {
        return resume_callback_();
    }

private:
    std::function<void(coro::coroutine_handle<task_promise<T>>)> suspend_callback_;
    std::function<std::optional<T>()> resume_callback_;
};
```

在这里我们按照规定，我们定义了`await_ready`, `await_suspend`, `await_resume`这三个方法。 这三个方法的内容也非常简单， `await_ready` 非常直接的返回false, 表示我们每次调用都会暂停协程，不需要做优化。 `await_suspend`, `await_resume` 也只是简单的调用从构造函数获得的回调函数， 之所以这样做只是为了让`task_awaiter`保持稳定，我们可不希望每次co_await/co_yeild调用都要定义一个新的协程表达式。让变化的部分放到外面调用的地方，在`suspend_callback_`函数中我们可以定义各种触发异步调用的代码，例如 socket.async_readsome, redisAsyncCommand等等， `resume_callback_` 里面便可以是我们以前我们定义的回调函数内容了。

回看协程展开式，co_await调用对应这段代码：

```cpp
	// co_await std::experimental::suspend_always{};
	{
		// auto&& tmp = std::experimental::suspend_always{};
		auto&& tmp = task_awaiter<T>{...};
		if (!tmp.await_ready()) {
			__builtin_coro_save() // frame->suspend_index = m;
			if (tmp.await_suspend(<coroutine_handle>)) {
				__builtin_coro_suspend() // jmp
			}
		}

	resume_label_m:
		retm = tmp.await_resume();
	}
```

首先程序会生成协程表达式对象tmp，再检查tmp的await_ready的结果，如果await_ready返回true, 表示此次调用不需要暂停，可以直接通过await_resume拿到结果。如果为false，则先保存“suspend_index”（这个字段在vs2017中的resumable头文件_Resumable_frame_prefix结构体中有定义，vs2022中未见到）它代表的是在这个协程函数中的第几次暂停，恢复的时候会根据这个值计算出resume_label_m的指令地址（每个恢复点的地址编译器是已知的，编译器还会为每个协程函数建立一个恢复点的索引数组，这样根据suspend_index便能计算出恢复点位置）， 然后在调用`await_suspend`，我们可以将发起请求的代码放在这个函数里面执行，调用完后，会将本次协程调用挂起，执行指令会恢复到上次调用协程函数的地方或者恢复到调用resume的地方。当在适当的时候，当程序需要恢复执行协程函数后面的代码时，协程会更具前面记录的suspend_index找到 resume_label_m 然后便可以执行`await_resume`函数了。

这里有个地方需要注意下， await_suspend 返回值有void或bool两种形式， 如果是bool, 则功能与`await_ready`函数类似，所以个人感觉`await_ready`的规定有点多余，还增加了本来就复杂的协程复杂性。

### 协程句柄

看到这里你可能还会有些困惑，协程到底是什么时候恢复执行的呢？前面一直说在“适当”的时候，但到底什么时候才是“适当”的时候呢？还有是谁触发了恢复动作？还有是怎么触发的？

#### 谁触发的？

代码实现者。系统和编译器都是无权也无法控制协程的暂停和恢复，因为他们不知道什么时候该暂停与恢复。一切主动权和责任都在于代码实现者。

#### 什么时间？

获得调用结果的时候。在获得异步调用的结果后便可以恢复, 比如在 coro_reids中我们就是在redisAsyncFormattedCommand回调函数中调用的resume方法的。

#### 怎么触发？

通过调用协程函数关联的协程句柄`coro::coroutine_handle<>`的resume()方法恢复。协程句柄的获得途径有以下几种：

1. await_resume 的参数
2. 协程返回对象的构造函数参数
3. 通过promise的地址。coro::coroutine_handle<>有个静态方法from_promise，可以通过promise地址直接得到
4. 通过协程帧地址。同样是coro::coroutine_handle<>有个静态方法from_address


### 协程函数返回类型

协程函数返回类型是一个比较奇怪和复杂的类型, 通过它我们才有了一个从外部操控协程的机会。前面说的协程句柄其实有个promise_type的模版参数（即`coro::coroutine_handle<promise_type>`），那么这个模版参数是怎么来的呢？通过以下的特征函数获得：

```CPP
template <class _Ret>
struct _Coroutine_traits<_Ret, void_t<typename _Ret::promise_type>> {
    using promise_type = typename _Ret::promise_type;
};
```

这里的_Ret便是协程函数返回类型，这样一来，如果不特化这个结构体，_Ret就必须要有`promise_type`子类型。说白了，就是你在定义返回类型时必须定义个`promise_type`子类型。 而这`promise_type`又要具有以下几个规定的方法：

 1. get_return_object
 2. initial_suspend
 3. final_suspend
 4. return_void/return_value
 5. unhandled_exception

这些定义有许多反直觉的地方和陷阱。

我们再次回看协程展开式，看下开头的几行，协程帧创建的过程：

```CPP
  using promise_type = T::promise_type;
  using frame_type = tuple<frame_prefix, promise_type, Args...>;
  auto *frame = (frame_type *)promise_type::operator new(sizeof(frame_type));
```

协程帧指的是每个协程函数所使用的内存空间，看上面的代码协程帧正是通过promise_type的new方法在堆上分配出来的（如果不重载operator new方法），但是值得注意的是new的大小并不是promise_type自己的大小，而是带上frame_prefix + promise_type + Args的大小， frame_prefix在vs2017中的定义如下，vs2020已经改为编译器内置函数，头文件里已经看不到了：

```cpp
	struct _Resumable_frame_prefix
	{
		typedef void(__cdecl *_Resume_fn)(void *);
		_Resume_fn _Fn;
		uint16_t _Index;
		uint16_t _Flags;
	};
```
_Fn 是个固定的编译器内嵌的函数地址， _Index 便是前面提到的suspend_index根据这个值可以计算出协程恢复地址，_Flags 不太清楚其原理。

除此之外协程帧还包含协程函数的参数和临时变量的空间，由于这些变量都是在堆上分配的，所以在整个协程函数执行过程中这些变量都是有效的，在visual studio Debug版本中这些变量是还是先在栈上分配，再拷贝到协程帧上，但是Release版本中由于栈上的变量一直没有用到所以直接优化掉了，所以协程帧上的变量使用和栈上的变量效果是一样的。

有了协程帧，通过计算我们就可以得到promise_type对象了，先在我们看下它的几个方法：

1. get_return_object 这个方法也挺奇怪的，首先这个方法的返回值类型必须是promise_type的外部类，即例子中的`task<T>`, 需要注意的是这个返回值不能是指针类型。他必须能够直接赋值给协程函数的返回值。另外是谁什么时间调用了这个方法呢？ 首先get_return_object并不是给协程外部使用的，而是协程内部使用的，其次它是在协程第一次暂停后返回给接受者的，而不是在整个协程调用结束后给的，这点有点反直觉。

2. initial_suspend/final_suspend 这两个函数给了协程函数在进入和退出的时候暂停的机会。

3. return_void/return_value 这两个函数是二选一的关系，如果返回值是void类型协程就会调用return_void，如果非void类型就会调用return_value方法。是的, void在模版编程里面总是个麻烦鬼，所以你最好使用例子中的那种方法先写个基类再对有值和无值分别做特化处理。协程函数通过这个方法将最终的结果传递出去，但是有个陷阱是return_values虽然是promise_type的成员方法，但是我们通常却不能把结果存放在promise_type对象中，我们再次回到协程展开式的最后：

```cpp
	auto ret = ...;
	p->return_value(ret);                   // 设置返回值
    goto __final_suspend_point;


  __final_suspend_point:
    co_await p->final_suspend();

  __destroy_point:
    promise_type::operator delete(frame, sizeof(frame_type)); // 刚设置完，就销毁了，来不及使用
```

设置完返回值后promise就被立即销毁了， 这时如果我们通过返回对象拿到promise，再通过promise获得返回值已经晚了。比如下面的代码：

```cpp
template<typename T>
class task<T> {
	...

    T return_value() {
        return h_coro_.promise().get_return_value();
    }
}
```

有什么方法解决这个问题呢？ 有两个方法：

1. 想办法在promise_type::return_value(T value)方法中给task的成员方法赋值。如：

```CPP
template<typename T>
class task_promise final : public task_promise_base {
  public:
    using task_return_type = task<T>::return_type;
    task<T> get_return_object() noexcept {
        task<T> task(coro::coroutine_handle<task_promise>::from_promise(*this));
        ret_ = task.ret_value_ = std::make_shared<task_return_type>(); // 保存task的ret_value_指针
        return task;
    }

    void return_value(T value) {
        LOG_DEBUG("return_value: {}", value);
        *ret_ = value;
    }
    std::shared_ptr<typename task<T>::return_type> ret_;
};
```

这里需要注意的是`get_return_object()`方法的返回值是固定的，不能是指针或引用类型，所以我们不能通过保存task指针或引用的方式将结果保存到task对象中，因为在`get_return_object()`函数创建的是临时对象，同样在task中的ret_value_也只能是指针类型，不然你设置的只能是临时对象的值。

2. 推迟`promose_type`对象的析构时间。还记得 `promose_type` 中 `final_suspend` 方法吗？ 会看前面的展开式：

```cpp
	auto ret = ...;
	p->return_value(ret);
    goto __final_suspend_point;


  __final_suspend_point:
    co_await p->final_suspend();  // 可以通过这步暂停后面销毁操作的执行

  __destroy_point:
    promise_type::operator delete(frame, sizeof(frame_type));
```

可以看到在协程函数调用 `promose_type` 析构函数（实际上析构的是协程帧）前 co_await 了 final_suspend 的返回结果。所以我们可以利用这点，来实现 `promose_type` 析构的推迟。如下面的代码：

```cpp
template<typename T>
class task {
	~task() {
		// 外部对象析构时才真正销毁协程帧
		h_coro_.destory();
	}

	struct promise_type {

		auto final_suspend() noexcept {
			return std::suspend_always{};
		}
	}

	...

	std::coroutine_handle<promise_type> h_coro_;
}

```

注意，理论上 `co_await final_suspend()` 后应该能够通过 coroutine_handle 的 resume 方法恢复然后继续执行析构操作，但是无论是 visual studio 还是 gcc 这样操作都会导致异常，目前还不清楚原因。所以只能在外部对象task的析构函数中主动调用destory方法去完成析构任务。

在实际的使用中我还是选择了第一种方案，因为第一种更符合“职责单一”的原则，task负责外部数据的生命周期，promise负责协程内部资源的生命周期，另外由于第一种方案更“及时”的释放协程帧资源，所以内存使用率更高效。

## 使用协程对hiredis进行封装

C++ 协程的概念虽多，但是经过前面的分析，我们可以对其进行整理将不易变动的部分提取出来（见coro_task.h）。使用coro_task.h中代码，我们可以套用同一套代码来实现不同的协程功能。对于task及其promise_type在我们定义后基本不会在变动它们，而需要变动的是task_awaiter的await_suspend两个函数已经被提取为构造函数的两个回调参数。

对于hiredis的异步调用接口封装，只需在task_awaiter的suspend_callback中添加命令调用的代码，然后在resume_callback处理回调结果便可以了，可以参考reids_client中coro_command的代码。其实coro_task不单单对于hiredis是有效的，对于大多数需要回调函数的调用都不要修改代码。

在coro_command代码中一个有意思的地方是，suspend_callback中redisAsyncFormattedCommand异步调用的结果怎样传给resume_callback。一开始我的想法是暂存在promise_type对象中，因为可以通过await_suspend的协程句柄参数可以方便的得到promise_type对象，另一方面promise_type的生命周期是到整个协程调用结束，暂存在这里比较安全。但是有个问题，我们先看下await_suspend的协程句柄参数的声明：

```CPP
	coro::coroutine_handle<task_promise<TASK_RET>>
```

在这里我们必须要知道task的模版参数TASK_RET，这样其实没什么大问题，但是有个不方便的地方是我们在构造task_awaiter的时候就需要两个模版参数（co_await返回值类型CORO_RET和协程函数返回值类型TASK_RET，代码可参考examples/301）。如果每次调用都带上这两个参数还是挺麻烦的。CORO_RET还好，我们在封装redis的每个命令时已经知道了其返回类型，可以对其特化，如下面的代码：

```CPP
template<typename TASK_RET>
task_awaiter<TASK_RET, std::string> coro_echo(std::string_view msg) {
	return coro_command<TASK_RET, std::string>(fmt::format("echo {}", msg));
}
```

但是TASK_RET便比较讨厌了，只有在调用的时候才能知道，如果按现在的方案就需要写下如下的代码：

```CPP
task<int> watchdog::coro_call_test() {
    auto r = co_await redis_client::get().coro_echo<int>("test");
    auto var1 =co_await redis_client::get().coro_incr<int>("var1");
    auto var2 = co_await redis_client::get().coro_incr<int>("var2");
    auto var3 = std::to_string(var1.value() + var2.value<int>());
    co_await redis_client::get().coro_set<int>("var3", var3);
    auto var3_2 = co_await redis_client::get().coro_get<int>("var3");
    LOG_DEBUG("r: {}, var1: {}, var2: {}, var3: {}, var3_2: {}",
        r.value(), var1.value(), var2.value(), var3, var3_2.value());
    co_return 1;
}
```

每个`coro_*`命令都要带上与`task<T>`一样的模版参数。有什么办法解决这个烦恼呢？让我们先看下`coro::coroutine_handle<T>`的定义：

```cpp
template <class = void>
struct coroutine_handle;
```

他有两种特化：

1. 不可访问promise

```cpp
template <>
struct coroutine_handle<void>{...}
```

2. 可访问promise

```cpp
template <class _Promise>
struct coroutine_handle {...}
```

所以如果我们想使用不带模版参数的版本就无法访问promise，一旦我们无法访问promise，那么redis的异步调用结果可以放哪里呢？让我们回到suspend_callback和resume_callback定义的地方，能暂存在task_awaiter对象中吗？让我们再次分析下协程展开式：

```cpp
  try {
	auto retm, retn, ...;
    // co_await std::experimental::suspend_always{};
	{
		auto&& tmp = std::experimental::suspend_always{};
		if (!tmp.await_ready()) {
			__builtin_coro_save() // frame->suspend_index = m;
			if (tmp.await_suspend(<coroutine_handle>)) {
				__builtin_coro_suspend() // jmp
			}
		}

	resume_label_m:
		retm = tmp.await_resume();
	}
	...
```

虽然协程表达式是临时变量但是它并不会因为协程暂停而销毁，所以将异步结果存放在协程表达式中更符合我们的预期，而且一旦await_resume调用结束临时对象变被销毁，这样内存利用率更高。

既然可行，我们就对task_awaiter进行改造吧，先添加set_coro_return方法，然后在suspend_callback和resume_callback方法中添加task_awaiter指针参数，最后去掉task_awaiter的模版参数TASK_RET。经过改造task和task_awaiter都只需要关心与自己相关的模版参数，两者再无瓜葛。终于可以像下面的代码那样愉快的调用了, 代码见`tests/303`：

```CPP
task<bool> coro_redis_test(event_base* base, std::string_view host, uint16_t port) {
	auto conn = co_await client::get().connect(base, host, port, 50);
	ASSERT_CO_RETURN(conn.has_value(), false, "connect to redis failed.");

	auto scan_ret = co_await conn->scan(0);
	ASSERT_CO_RETURN(scan_ret.has_value(), false, "call scan failed.");
	auto [cursor, keys] = *scan_ret;
	LOG_TRACE("scan {}", cursor);

	auto echo_ret = co_await conn->echo("hello");
	ASSERT_CO_RETURN(echo_ret.has_value(), false, "call echo failed.");
	LOG_TRACE("echo {}", *echo_ret);

	auto incr_ret = co_await conn->incr("val1");
	ASSERT_CO_RETURN(incr_ret.has_value(), false, "call incr failed.");
	LOG_TRACE("incr {}", *incr_ret);

	auto set_ret = co_await conn->set("val3", 100);
	ASSERT_CO_RETURN(set_ret.has_value(), false, "call set failed.");
	LOG_TRACE("set {}", *set_ret);

	auto set_ret2 = co_await conn->set("val4", "100");
	ASSERT_CO_RETURN(set_ret2.has_value(), false, "call set failed.");
	LOG_TRACE("set {}", *set_ret2);

	auto get_ret = co_await conn->get("val3");
	ASSERT_CO_RETURN(get_ret.has_value(), false, "call get failed.");
	LOG_TRACE("get {}", *get_ret);

	auto del_cont = co_await conn->del("test", "val1", "val2");
	ASSERT_CO_RETURN(del_cont.has_value(), false, "del keys failed");
	LOG_TRACE("del key count {}", *del_cont);

	co_return true;
}
```

自此hiredis协程的基本封装工作已经结束了，剩下的就是针对redis的各种命令进行优化处理。

## 待完成内容

	1. redis指令还未完全封装完
	2. 网络库封装，现在默认使用libevent
	3. 连接池







