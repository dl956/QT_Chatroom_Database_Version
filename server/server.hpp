#pragma once
#include <boost/asio.hpp>
#include <memory>
#include <unordered_map>
#include <mutex>
#include <string>
#include <vector>
#include "user_store.hpp"
#include "message_store.hpp"

class Session;

class Server {
public:
    Server(boost::asio::io_context& io_context, unsigned short port, UserStore* user_store, MessageStore* message_store);
    void run_accept();
    void on_login(std::shared_ptr<Session> session_ptr, const std::string& username);
    void on_disconnect(std::shared_ptr<Session> session_ptr);
    void broadcast(const std::string& json_text, std::shared_ptr<Session> except_session = nullptr);
    void send_to_user(const std::string& username, const std::string& json_text);

    void broadcast_user_list();
    std::vector<std::string> online_usernames();

    UserStore& user_store() { return *user_store_; }
    MessageStore& message_store() { return *message_store_; }

private:
    boost::asio::ip::tcp::acceptor acceptor_;
    boost::asio::io_context& io_context_;
    std::mutex mutex_;
    std::unordered_map<std::string, std::shared_ptr<Session>> online_users_;
    UserStore* user_store_;
    MessageStore* message_store_;
};