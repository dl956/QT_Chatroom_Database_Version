#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include "session.hpp"
#include "server.hpp"
#include "protocol.hpp"
#include "logger.hpp"
#include <chrono>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <iostream> // For std::cerr

using json = nlohmann::json;
namespace asio = boost::asio;

// 预览文本
static std::string preview_text(const std::string& text, size_t max_length = 200) {
    if (text.size() <= max_length) return text;
    return text.substr(0, max_length) + "...";
}

// 日志脱敏
static json redact_for_logging(const std::string& raw_json_text) {
    try {
        json json_obj = json::parse(raw_json_text);
        if (json_obj.contains("password")) json_obj["password"] = "<REDACTED>";
        if (json_obj.contains("text")) json_obj["text"] = preview_text(json_obj["text"].get<std::string>(), 200);
        return json_obj;
    } catch (...) {
        json out_json;
        out_json["raw_preview"] = preview_text(raw_json_text, 200);
        return out_json;
    }
}

Session::Session(asio::ip::tcp::socket socket, Server& server)
    : socket_(std::move(socket)), server_(server), header_buf_(4) {
    Logger::instance().debug("Session constructed");
}

void Session::start() {
    Logger::instance().info("Session start");
    do_read_header();
}

void Session::do_read_header() {
    auto self = shared_from_this();
    asio::async_read(socket_, asio::buffer(header_buf_), [this, self](std::error_code ec, std::size_t) {
        try {
            if (ec) {
                server_.on_disconnect(self);
                Logger::instance().info("Session read header error/disconnect", { {"ec", ec.message()}, {"user", username_} });
                return;
            }
            uint32_t body_len = parse_length(header_buf_);
            if (body_len == 0) { do_read_header(); return; }
            do_read_body(body_len);
        } catch (const std::exception& ex) {
            Logger::instance().error("Unhandled exception in do_read_header", {{"what", ex.what()}});
            std::cerr << "[fatal] do_read_header std::exception: " << ex.what() << std::endl;
        } catch (...) {
            Logger::instance().error("Unhandled unknown exception in do_read_header");
            std::cerr << "[fatal] do_read_header unknown exception" << std::endl;
        }
    });
}

void Session::do_read_body(uint32_t body_len) {
    body_buf_.assign(body_len, 0);
    auto self = shared_from_this();
    asio::async_read(socket_, asio::buffer(body_buf_), [this, self](std::error_code ec, std::size_t) {
        try {
            if (ec) {
                server_.on_disconnect(self);
                Logger::instance().info("Session read body error/disconnect", { {"ec", ec.message()}, {"user", username_} });
                return;
            }
            std::string payload(body_buf_.begin(), body_buf_.end());
            json redacted_json = redact_for_logging(payload);
            Logger::instance().debug("Received JSON", { {"from", username_}, {"json_len", static_cast<uint64_t>(payload.size())}, {"payload", redacted_json} });
            try {
                json json_obj = json::parse(payload);
                process_message(json_obj);
            } catch (const std::exception& ex) {
                Logger::instance().error("Bad JSON parse", { {"what", ex.what()}, {"payload_preview", preview_text(payload, 200)} });
                std::cerr << "[fatal] JSON parse error: " << ex.what() << std::endl;
            } catch (...) {
                Logger::instance().error("Unknown fatal JSON parse error", { {"payload_preview", preview_text(payload, 200)} });
                std::cerr << "[fatal] Unknown fatal JSON parse error" << std::endl;
            }
            do_read_header();
        } catch (const std::exception& ex) {
            Logger::instance().error("Unhandled exception in do_read_body", {{"what", ex.what()}});
            std::cerr << "[fatal] do_read_body std::exception: " << ex.what() << std::endl;
        } catch (...) {
            Logger::instance().error("Unhandled unknown exception in do_read_body");
            std::cerr << "[fatal] do_read_body unknown exception" << std::endl;
        }
    });
}

void Session::process_message(const json& json_obj) {
    std::string msg_type = json_obj.value("type", "");
    Logger::instance().debug("Processing message", { {"type", msg_type}, {"user", username_} });

    if (msg_type == "register") {
        std::string username_input = json_obj.value("username", "");
        std::string password_input = json_obj.value("password", "");
        bool is_registered = false;
        try {
            Logger::instance().debug("About to call register_user", { {"username", username_input} });
            std::cerr << "(debug) About to call register_user" << std::endl;
            is_registered = server_.user_store().register_user(username_input, password_input);
            Logger::instance().debug("register_user returned", {{"ok", is_registered}});
            std::cerr << "(debug) register_user returned: " << is_registered << std::endl;
        } catch(const std::exception& ex) {
            Logger::instance().error("Exception in register user", {{"what", ex.what()}});
            std::cerr << "Exception in register user: " << ex.what() << std::endl;
            is_registered = false;
        } catch(...) {
            Logger::instance().error("FATAL UNKNOWN in register user", {{"username", username_input}});
            std::cerr << "FATAL UNKNOWN in register user" << std::endl;
            is_registered = false;
        }
        json resp_json = { {"type","register_result"}, {"ok", is_registered} };
        if (!is_registered) {
            resp_json["reason"] = "username_exists";
            Logger::instance().warn("Register failed", { {"username", username_input}, {"reason", "username_exists"} });
        } else {
            Logger::instance().info("User registered (via session)", { {"username", username_input} });
        }
        Logger::instance().debug("Delivering register_result");
        std::cerr << "(debug) Delivering register_result" << std::endl;
        deliver(resp_json.dump());

    } else if (msg_type == "login") {
        std::string username_input = json_obj.value("username", "");
        std::string password_input = json_obj.value("password", "");
        bool is_login_success = false;
        try {
            is_login_success = server_.user_store().check_login(username_input, password_input);
        } catch(const std::exception& ex) {
            Logger::instance().error("Exception in login", {{"what", ex.what()}});
        }
        json resp_json = { {"type","login_result"}, {"ok", is_login_success} };
        if (!is_login_success) {
            resp_json["reason"] = "invalid";
            Logger::instance().warn("Login failed", { {"username", username_input}, {"reason", "invalid"} });
        } else {
            username_ = username_input;
            server_.on_login(shared_from_this(), username_input);
            Logger::instance().info("Login success", { {"username", username_input} });
            resp_json["username"] = username_input;
        }
        Logger::instance().info("login_result JSON", {{"json", resp_json.dump()}});
        deliver(resp_json.dump());
        if (is_login_success) {
            auto history_msgs = server_.message_store().for_user(username_input, 100);
            for (auto& chat_msg : history_msgs) {
                json msg_json = {
                    {"type", chat_msg.to.empty() ? "message" : "private"},
                    {"from", chat_msg.from},
                    {"to", chat_msg.to},
                    {"text", chat_msg.text},
                    {"ts", chat_msg.ts}
                };
                deliver(msg_json.dump());
            }
        }

    } else if (msg_type == "message") {
        if (username_.empty()) {
            json err_json = { {"type", "error"}, {"error", "not_logged_in"} };
            deliver(err_json.dump());
            Logger::instance().warn("Message rejected - not logged in");
            return;
        }
        std::string text_val = json_obj.value("text", "");
        uint64_t ts_val = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
        ChatMsg chat_msg{ username_, "", text_val, ts_val };
        try {
            server_.message_store().push(chat_msg);
        } catch(const std::exception& ex) {
            Logger::instance().error("Exception in push message", {{"what", ex.what()}});
        }
        json msg_json = { {"type","message"}, {"from", chat_msg.from}, {"text", chat_msg.text}, {"ts", chat_msg.ts} };
        server_.broadcast(msg_json.dump());
        Logger::instance().info("Broadcast message", { {"from", chat_msg.from}, {"len", static_cast<uint64_t>(text_val.size())}, {"text_preview", preview_text(text_val, 200)} });
        Logger::instance().debug("Broadcast full message", { {"from", chat_msg.from}, {"text", text_val} });

    } else if (msg_type == "private") {
        if (username_.empty()) {
            json err_json = { {"type", "error"}, {"error", "not_logged_in"} };
            deliver(err_json.dump());
            Logger::instance().warn("Private message rejected - not logged in");
            return;
        }
        std::string to_val = json_obj.value("to", "");
        std::string text_val = json_obj.value("text", "");
        uint64_t ts_val = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
        ChatMsg chat_msg{ username_, to_val, text_val, ts_val };
        try {
            server_.message_store().push(chat_msg);
        } catch(const std::exception& ex) {
            Logger::instance().error("Exception in push private message", {{"what", ex.what()}});
        }
        json msg_json = { {"type","private"}, {"from", chat_msg.from}, {"to", chat_msg.to}, {"text", chat_msg.text}, {"ts", chat_msg.ts} };
        server_.send_to_user(to_val, msg_json.dump());
        deliver(msg_json.dump());
        Logger::instance().info("Private message", { {"from", chat_msg.from}, {"to", chat_msg.to}, {"len", static_cast<uint64_t>(text_val.size())}, {"text_preview", preview_text(text_val, 200)} });
        Logger::instance().debug("Private message full", { {"from", chat_msg.from}, {"to", chat_msg.to}, {"text", text_val} });

    } else if (msg_type == "heartbeat") {
        json pong_json = { {"type","pong"} };
        deliver(pong_json.dump());

    } else if (msg_type == "history") {
        size_t count = json_obj.value("n", 50);
        try {
            auto history_msgs = server_.message_store().for_user(username_, count);
            for (auto& chat_msg : history_msgs) {
                json msg_json = {
                    {"type", chat_msg.to.empty() ? "message" : "private"},
                    {"from", chat_msg.from},
                    {"to", chat_msg.to},
                    {"text", chat_msg.text},
                    {"ts", chat_msg.ts}
                };
                deliver(msg_json.dump());
            }
        } catch(const std::exception& ex) {
            Logger::instance().error("Exception in history fetch", {{"what", ex.what()}});
        }

    } else if (msg_type == "list_users") {
        auto users = server_.online_usernames();
        json resp_json;
        resp_json["type"] = "user_list";
        resp_json["users"] = users;
        deliver(resp_json.dump());

    } else if (msg_type == "logout") {
        Logger::instance().info("User requested logout", { {"username", username_} });
        socket_.close();
        return;
    } else {
        Logger::instance().warn("Unknown message type", { {"type", msg_type} });
    }
}

void Session::deliver(const std::string& json_text) {
    auto frame = make_frame(json_text);
    bool writing = !write_queue_.empty();
    write_queue_.push_back(std::move(frame));
    if (!writing) do_write();
}

void Session::do_write() {
    auto self = shared_from_this();
    boost::asio::async_write(socket_, boost::asio::buffer(write_queue_.front()), [this, self](std::error_code ec, std::size_t) {
        try {
            if (ec) {
                server_.on_disconnect(self);
                Logger::instance().info("Session write error/disconnect", { {"ec", ec.message()}, {"user", username_} });
                return;
            }
            write_queue_.pop_front();
            if (!write_queue_.empty()) do_write();
        } catch (const std::exception& ex) {
            Logger::instance().error("Unhandled exception in do_write", {{"what", ex.what()}});
            std::cerr << "[fatal] do_write std::exception: " << ex.what() << std::endl;
        } catch (...) {
            Logger::instance().error("Unhandled unknown exception in do_write");
            std::cerr << "[fatal] do_write unknown exception" << std::endl;
        }
    });
}

std::string Session::username() const { return username_; }