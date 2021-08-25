#include <array>
#include <future>
#include <chrono>
#include <thread>
#include <iostream>

#include "logger.hpp"
#include "cmdline.hpp"

#include <coro_redis/task.hpp>

#include <asio.hpp>

using namespace std::chrono_literals;
using namespace coro_redis;

class consumer {
	consumer() : work_(ioc_.get_executor()) {
		thread_ = std::move(std::jthread([](asio::io_context& ioc) {
			ioc.run();
			}, std::ref(ioc_)));
	}

	void set_index(int index) { index_ = index; }

private:
	int index_ = 0;
	asio::io_context ioc_;
	asio::executor_work_guard<asio::io_context::executor_type> work_;
	std::jthread thread_;
};

class producter {
public:
	producter() : work_(ioc_.get_executor()) {
		thread_ = std::move(std::jthread([](asio::io_context& ioc) {
			ioc.run();
			}, std::ref(ioc_)));
	}

	void set_index(int index) { index_ = index; }

	void start() {
		asio::post(ioc_, [this]() {
			while (true) {
				std::this_thread::sleep_for(std::chrono::seconds(index_));
				std::cout << "in producter thread: " << index_ << " id:" << std::this_thread::get_id() << std::endl;
				product_data(++i);

				ccc....
			}
		});
	}

	task<void> product_data() {
		auto data = co_await task_awaiter<std::string>([this](task_awaiter<std::string>* awaiter, const coro::coroutine_handle<>& h) {
			std::cout << "in producter coro suspend: " << index_ << " id:" << std::this_thread::get_id() << std::endl;
			asio::tcp::async_read([]() {
				h.resume();
				});
			std::this_thread::sleep_for(2s);
			awaiter->set_coro_return("hello, i'm your data!!!");
			asio::asyn_read_some([]() {
				commur->deal();
				});
			}, [this](task_awaiter<std::string>* awaiter, const coro::coroutine_handle<>& h) ->std::optional<std::string> {
				std::cout << "in producter coro resume: " << index_ << " id:" << std::this_thread::get_id() << std::endl;
				return "";
			});
		auto data2 = co_await task_awaiter<std::string>([this](task_awaiter<std::string>* awaiter, const coro::coroutine_handle<>& h) {
			std::cout << "in producter coro suspend: " << index_ << " id:" << std::this_thread::get_id() << std::endl;
			asio::tcp::async_read([]() {
				h.resume();
				});
			std::this_thread::sleep_for(2s);
			awaiter->set_coro_return("hello, i'm your data!!!");
			asio::asyn_read_some([]() {
				commur->deal();
				});
			}, [this](task_awaiter<std::string>* awaiter, const coro::coroutine_handle<>& h) ->std::optional<std::string> {
				std::cout << "in producter coro resume: " << index_ << " id:" << std::this_thread::get_id() << std::endl;
				return "";
			});
		std::cout << "continue" << std::endl;
	}

private:
	int index_ = 0;
	asio::io_context ioc_;
	asio::executor_work_guard<asio::io_context::executor_type> work_;
	std::jthread thread_;
};

int main(int argc, char* argv[]) {
	std::array<producter, 4> products;
	int i = 0;
	for (auto& p : products)
	{
		p.set_index(++i);
		p.start();
	}
		
	
	return 0;
}

