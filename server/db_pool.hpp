#pragma once
#include <mysqlx/xdevapi.h>
#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <stdexcept>

// 高可靠/无野指针的 MySQL Session 连接池
class DBPool {
public:
    DBPool(const std::string& host, unsigned port, const std::string& username, const std::string& password, size_t connection_pool_size)
        : host_(host), port_(port), username_(username), password_(password), connection_pool_size_(connection_pool_size) {
        for (size_t i = 0; i < connection_pool_size_; ++i) {
            available_sessions_.push(create_session());
        }
    }

    // 线程安全获取 session：只返回管理好生命周期的 shared_ptr，析构即归还
    std::shared_ptr<mysqlx::Session> acquire_session() {
        std::unique_lock<std::mutex> lock_guard(mutex_);
        while (available_sessions_.empty()) condition_variable_.wait(lock_guard);
        auto session_ptr = available_sessions_.front();
        available_sessions_.pop();
        // 归还池时一定用原始 shared_ptr归队，避免裸指针包装问题
        return std::shared_ptr<mysqlx::Session>(
            session_ptr.get(),
            [this, session_ptr](mysqlx::Session* /*ptr*/) mutable {
                std::unique_lock<std::mutex> lock_guard(mutex_);
                available_sessions_.push(session_ptr);
                condition_variable_.notify_one();
            }
        );
    }

private:
    std::shared_ptr<mysqlx::Session> create_session() {
        try {
            auto* raw_session = new mysqlx::Session(host_, port_, username_, password_);
            return std::shared_ptr<mysqlx::Session>(raw_session, [](mysqlx::Session* s){ if(s) delete s; });
        } catch (const mysqlx::Error& e) {
            throw std::runtime_error(std::string("Failed to connect to MySQL: ") + e.what());
        }
    }

    std::string host_, username_, password_;
    unsigned port_;
    size_t connection_pool_size_;
    std::queue<std::shared_ptr<mysqlx::Session>> available_sessions_;
    std::mutex mutex_;
    std::condition_variable condition_variable_;
};