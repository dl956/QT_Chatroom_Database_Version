#include <iostream>
#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>
#include <thread>
#include <mysqlx/xdevapi.h>
#include "server.hpp"
#include "logger.hpp"
#include "db_pool.hpp"

// 全局未捕获异常钩子
void custom_terminate_handler() {
    std::cerr << "Uncaught Exception: std::terminate called!" << std::endl;
    abort();
}

int main(int argc, char** argv) {
    std::set_terminate(custom_terminate_handler);

    try {
        unsigned short server_port = 9000;
        if (argc > 1) server_port = static_cast<unsigned short>(std::stoi(argv[1]));
        std::cout << "Starting server..." << std::endl;

        try {
            Logger::instance().init("logs/server.log", LogLevel::Debug, 10ull * 1024 * 1024, 5);
            std::cout << "Logger initialized" << std::endl;
        } catch (...) {
            std::cout << "Logger initialization failed!" << std::endl;
        }

        std::unique_ptr<DBPool> db_pool_ptr;
        try {
            db_pool_ptr = std::make_unique<DBPool>("127.0.0.1", 33060, "root", "mypassword", 10);
            std::cout << "DBPool created OK!" << std::endl;
        } catch (const std::exception& ex) {
            std::cerr << "Fatal error: DBPool construction failed: " << ex.what() << std::endl;
            return 1;
        }
        UserStore user_store(db_pool_ptr.get());
        MessageStore message_store(db_pool_ptr.get());

        boost::asio::io_context io_context;
        boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work_guard(io_context.get_executor());

        auto keep_alive_timer = std::make_shared<boost::asio::steady_timer>(io_context, std::chrono::seconds(1));
        std::function<void()> tick_function;
        tick_function = [keep_alive_timer, &tick_function]() {
            keep_alive_timer->expires_after(std::chrono::seconds(1));
            keep_alive_timer->async_wait([keep_alive_timer, &tick_function](const boost::system::error_code& ec) {
                std::cout << "[dummy timer] keepalive tick, error='" << ec.message() << "'" << std::endl;
                tick_function();
            });
        };
        tick_function();

        Server server(io_context, server_port, &user_store, &message_store);
        server.run_accept();

        size_t thread_count = std::thread::hardware_concurrency();
        if (thread_count == 0) thread_count = 2;
        std::vector<std::thread> worker_threads;
        for (size_t i = 0; i < thread_count; ++i) {
            worker_threads.emplace_back([&io_context, i]() {
                std::cout << "[worker " << i << "] thread running!" << std::endl;
                io_context.run();
                std::cout << "[worker " << i << "] thread exiting!" << std::endl;
            });
        }
        std::cout << "Waiting for worker threads to exit..." << std::endl;
        for (auto& thread : worker_threads) thread.join();

        while (true) {
            std::cout << "[main] thread still alive!" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(10));
        }
    }
    catch (const std::exception& ex) {
        std::cerr << "Fatal error (main): " << ex.what() << std::endl;
    }
    return 0;
}