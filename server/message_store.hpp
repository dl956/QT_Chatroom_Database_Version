#pragma once
#include <string>
#include <vector>

struct ChatMsg {
    std::string from;
    std::string to;
    std::string text;
    uint64_t ts;
};

class DBPool;
class MessageStore {
public:
    MessageStore(DBPool* db_pool): db_pool_(db_pool) {}
    void push(const ChatMsg& message);
    std::vector<ChatMsg> recent(size_t count = 50);
    std::vector<ChatMsg> for_user(const std::string& username, size_t count = 50);
private:
    DBPool* db_pool_;
};