// server.cpp
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>

#include <boost/asio.hpp>
#include "server.hpp"
#include "session.hpp"
#include "logger.hpp"
#include <nlohmann/json.hpp>

namespace asio = boost::asio;
using tcp = asio::ip::tcp;
using json = nlohmann::json;

Server::Server(asio::io_context& io_context, unsigned short port, UserStore* user_store, MessageStore* message_store)
    : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)), io_context_(io_context), user_store_(user_store), message_store_(message_store) {
    Logger::instance().info("Server constructed", { {"port", port} });
}

void Server::run_accept() {
    acceptor_.async_accept([this](std::error_code ec, tcp::socket socket) {
        if (!ec) {
            auto session_ptr = std::make_shared<Session>(std::move(socket), *this);
            Logger::instance().info("New connection accepted");
            session_ptr->start();
        } else {
            Logger::instance().error("Accept error", { {"what", ec.message()}, {"value", ec.value()} });
        }
        run_accept();
    });
}

void Server::on_login(std::shared_ptr<Session> session_ptr, const std::string& username) {
    {
        std::lock_guard<std::mutex> lock_guard(mutex_);
        online_users_[username] = session_ptr;
        Logger::instance().info("User logged in", { {"username", username}, {"online_count", static_cast<uint64_t>(online_users_.size())} });
    }
    broadcast_user_list();
}

void Server::on_disconnect(std::shared_ptr<Session> session_ptr) {
    {
        std::lock_guard<std::mutex> lock_guard(mutex_);
        for (auto it = online_users_.begin(); it != online_users_.end();) {
            if (it->second == session_ptr) {
                Logger::instance().info("User disconnected", { {"username", it->first} });
                it = online_users_.erase(it);
            } else ++it;
        }
    }
    broadcast_user_list();
}

void Server::broadcast(const std::string& json_text, std::shared_ptr<Session> except_session) {
    std::lock_guard<std::mutex> lock_guard(mutex_);
    Logger::instance().debug("Broadcasting message", { {"len", static_cast<uint64_t>(json_text.size())}, {"except", except_session ? except_session->username() : ""} });
    for (auto& kv : online_users_) {
        if (kv.second != except_session) kv.second->deliver(json_text);
    }
}

void Server::send_to_user(const std::string& username, const std::string& json_text) {
    std::lock_guard<std::mutex> lock_guard(mutex_);
    auto it = online_users_.find(username);
    if (it != online_users_.end()) {
        it->second->deliver(json_text);
        Logger::instance().debug("Sent message to user", { {"to", username}, {"len", static_cast<uint64_t>(json_text.size())} });
    } else {
        Logger::instance().warn("User not online for send", { {"to", username} });
    }
}

void Server::broadcast_user_list() {
    json json_obj;
    {
        std::lock_guard<std::mutex> lock_guard(mutex_);
        json_obj["type"] = "user_list";
        json_obj["users"] = json::array();
        for (auto& kv : online_users_) {
            json_obj["users"].push_back(kv.first);
        }
    }
    broadcast(json_obj.dump());
}

std::vector<std::string> Server::online_usernames() {
    std::lock_guard<std::mutex> lock_guard(mutex_);
    std::vector<std::string> username_list;
    username_list.reserve(online_users_.size());
    for (auto& kv : online_users_) username_list.push_back(kv.first);
    return username_list;
}