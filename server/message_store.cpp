#include "message_store.hpp"
#include "db_pool.hpp"
#include "logger.hpp"
#include <mysqlx/xdevapi.h>
#include <algorithm>

void MessageStore::push(const ChatMsg& message) {
    try {
        auto session_ptr = db_pool_->acquire_session();
        auto database_schema = session_ptr->getSchema("chatdb");
        auto messages_table = database_schema.getTable("messages");
        messages_table.insert("sender", "recipient", "text", "ts")
             .values(message.from,
                     message.to.empty() ? mysqlx::Value() : message.to,
                     message.text,
                     static_cast<int64_t>(message.ts))
             .execute();
    } catch (const mysqlx::Error& ex) {
        Logger::instance().error("Insert message failed", {{"error", ex.what()}});
    }
}

std::vector<ChatMsg> MessageStore::recent(size_t count) {
    std::vector<ChatMsg> messages;
    try {
        auto session_ptr = db_pool_->acquire_session();
        auto database_schema = session_ptr->getSchema("chatdb");
        auto messages_table = database_schema.getTable("messages");
        auto row_result = messages_table.select("sender", "recipient", "text", "ts").execute();
        std::vector<mysqlx::Row> all_rows = row_result.fetchAll();
        for (size_t i = all_rows.size(); i > 0 && messages.size() < count; --i) {
            const auto& row = all_rows[i-1];
            messages.push_back(ChatMsg{
                row[0].get<std::string>(),
                row[1].isNull() ? "" : row[1].get<std::string>(),
                row[2].get<std::string>(),
                static_cast<uint64_t>(row[3].get<int64_t>())
            });
        }
    } catch (const mysqlx::Error& ex) {}
    return messages;
}

std::vector<ChatMsg> MessageStore::for_user(const std::string& username, size_t count) {
    std::vector<ChatMsg> messages;
    try {
        auto session_ptr = db_pool_->acquire_session();
        auto database_schema = session_ptr->getSchema("chatdb");
        auto messages_table = database_schema.getTable("messages");
        auto row_result = messages_table.select("sender", "recipient", "text", "ts")
                        .where("recipient IS NULL OR recipient = :user OR sender = :user")
                        .bind("user", username)
                        .execute();
        std::vector<mysqlx::Row> all_rows = row_result.fetchAll();
        for (size_t i = all_rows.size(); i > 0 && messages.size() < count; --i) {
            const auto& row = all_rows[i-1];
            messages.push_back(ChatMsg{
                row[0].get<std::string>(),
                row[1].isNull() ? "" : row[1].get<std::string>(),
                row[2].get<std::string>(),
                static_cast<uint64_t>(row[3].get<int64_t>())
            });
        }
    } catch (const mysqlx::Error& ex) {}
    std::reverse(messages.begin(), messages.end());
    return messages;
}