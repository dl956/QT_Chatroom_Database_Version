#include <iostream>
#include "user_store.hpp"
#include "db_pool.hpp"
#include "logger.hpp"
#include <mysqlx/xdevapi.h>

bool UserStore::register_user(const std::string& username, const std::string& password) {
    if (username.empty() || password.size() < 3) {
        std::cerr << "[debug] empty username or password too short" << std::endl;
        Logger::instance().warn("Register failed: Invalid username or password", {
            {"username", username},
            {"reason", "empty or password too short"}
        });
        return false;
    }
    try {
        std::cerr << "[debug] before db_pool_->acquire_session()" << std::endl;
        auto session_ptr = db_pool_->acquire_session();
        if (!session_ptr) {
            Logger::instance().error("Failed to get database session", {
                {"username", username}
            });
            std::cerr << "[error] failed to get session" << std::endl;
            return false;
        }
        std::cerr << "[debug] session OK" << std::endl;
        auto database_schema = session_ptr->getSchema("chatdb");
        std::cerr << "[debug] db OK" << std::endl;
        auto users_table = database_schema.getTable("users");
        std::cerr << "[debug] table OK" << std::endl;

        mysqlx::RowResult exist_result = users_table.select("id")
            .where("username = :username")
            .bind("username", username)
            .execute();
        if (exist_result.count() > 0) {
            Logger::instance().warn("Register failed: username already exists", {
                {"username", username}
            });
            std::cerr << "Register failed: username already exists" << std::endl;
            return false;
        }

        users_table.insert("username", "password")
            .values(username, password)
            .execute();
        std::cerr << "[debug] insert OK" << std::endl;
        Logger::instance().info("Register succeeded", {{"username", username}});
        return true;
    } catch (const mysqlx::Error& ex) {
        Logger::instance().warn("Register failed", {
            {"username", username}, {"error", ex.what()}
        });
        std::cerr << "Register failed: " << ex.what() << std::endl;
        return false;
    } catch (const std::exception& ex) {
        std::cerr << "register_user std::exception: " << ex.what() << std::endl;
        Logger::instance().error("register_user uncaught std::exception", {
            {"username", username}, {"error", ex.what()}
        });
        return false;
    } catch (...) {
        std::cerr << "register_user FATAL UNKNOWN EXCEPTION" << std::endl;
        Logger::instance().error("register_user fatal UNKNOWN EXCEPTION", {
            {"username", username}
        });
        return false;
    }
}

bool UserStore::check_login(const std::string& username, const std::string& password) {
    try {
        auto session_ptr = db_pool_->acquire_session();
        if (!session_ptr) {
            Logger::instance().error("Failed to get database session (login)", {
                {"username", username}
            });
            return false;
        }
        auto database_schema = session_ptr->getSchema("chatdb");
        auto users_table = database_schema.getTable("users");
        mysqlx::RowResult row_result = users_table.select("password")
            .where("username = :username")
            .bind("username", username)
            .execute();
        std::vector<mysqlx::Row> rows = row_result.fetchAll();
        if (rows.empty()) {
            Logger::instance().warn("Login failed - no such user (DB)", { {"username", username} });
            return false;
        }
        bool is_success = rows[0][0].get<std::string>() == password;
        Logger::instance().info("Login attempt (DB)", { {"username", username}, {"ok", is_success} });
        return is_success;
    } catch (const mysqlx::Error& ex) {
        Logger::instance().warn("Login failed (DB)", { {"username", username}, {"error", ex.what()} });
        return false;
    } catch (const std::exception& ex) {
        Logger::instance().error("Login fatal exception (DB)", { {"username", username}, {"error", ex.what()} });
        return false;
    } catch (...) {
        Logger::instance().error("Login fatal unknown exception (DB)", { {"username", username} });
        return false;
    }
}