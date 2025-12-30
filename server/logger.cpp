#include "logger.hpp"
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <cstdlib>
#include <cctype>

namespace fs = std::filesystem;

Logger& Logger::instance() {
    static Logger inst;
    return inst;
}

Logger::Logger()
    : file_path_("logs/server.log"),
      log_level_(LogLevel::Info),
      max_file_size_(10ull * 1024 * 1024),
      file_rotate_count_(5),
      is_initialized_(false) {
}

Logger::~Logger() {
    std::lock_guard<std::mutex> lock_guard(log_mutex_);
    if (output_file_stream_.is_open()) output_file_stream_.close();
}

void Logger::init(const std::string& file_path, LogLevel level, std::uint64_t max_size_bytes, int rotate_count) {
    std::lock_guard<std::mutex> lock_guard(log_mutex_);
    file_path_ = file_path;
    log_level_ = level;
    max_file_size_ = max_size_bytes;
    file_rotate_count_ = rotate_count;

    fs::path dir = fs::path(file_path_).parent_path();
    if (!dir.empty() && !fs::exists(dir)) {
        std::error_code ec;
        fs::create_directories(dir, ec);
        (void)ec;
    }

    if (output_file_stream_.is_open()) output_file_stream_.close();
    output_file_stream_.open(file_path_, std::ios::app);
    is_initialized_ = true;
}

std::string Logger::level_to_string(LogLevel level) const {
    switch (level) {
        case LogLevel::Debug: return "debug";
        case LogLevel::Info:  return "info";
        case LogLevel::Warn:  return "warn";
        case LogLevel::Err:   return "error";
        default: return "info";
    }
}

std::string Logger::timestamp_iso() const {
    using namespace std::chrono;
    auto now = system_clock::now();
    auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;

    std::time_t t = system_clock::to_time_t(now);
    std::tm tm;
#ifdef _WIN32
    gmtime_s(&tm, &t);
#else
    gmtime_r(&t, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S");
    oss << '.' << std::setw(3) << std::setfill('0') << ms.count() << "Z";
    return oss.str();
}

void Logger::rotate_if_needed_locked() {
    if (!output_file_stream_.is_open()) {
        output_file_stream_.open(file_path_, std::ios::app);
        if (!output_file_stream_.is_open()) return;
    }

    std::error_code ec;
    auto sz = fs::file_size(file_path_, ec);
    if (ec) return;
    if (sz < max_file_size_) return;

    output_file_stream_.close();

    for (int i = file_rotate_count_ - 1; i >= 0; --i) {
        fs::path src = (i == 0) ? fs::path(file_path_) : fs::path(file_path_ + "." + std::to_string(i));
        fs::path dst = fs::path(file_path_ + "." + std::to_string(i + 1));
        if (fs::exists(src, ec)) {
            if (fs::exists(dst, ec)) fs::remove(dst, ec);
            fs::rename(src, dst, ec);
            (void)ec;
        }
    }

    output_file_stream_.open(file_path_, std::ios::app);
}

void Logger::log(LogLevel level, const std::string& message, const nlohmann::json& extra) {
    if (static_cast<int>(level) < static_cast<int>(log_level_)) return;

    std::lock_guard<std::mutex> lock_guard(log_mutex_);
    if (!is_initialized_) {
        const char* env_file = std::getenv("LOG_FILE");
        std::string file = env_file ? env_file : file_path_;
        const char* env_level = std::getenv("LOG_LEVEL");
        LogLevel env_log_level = log_level_;
        if (env_level) {
            std::string s(env_level);
            for (auto &c: s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            if (s == "debug") env_log_level = LogLevel::Debug;
            else if (s == "info") env_log_level = LogLevel::Info;
            else if (s == "warn") env_log_level = LogLevel::Warn;
            else if (s == "error") env_log_level = LogLevel::Err;
        }
        const char* env_max = std::getenv("LOG_MAX_SIZE");
        std::uint64_t maxsz = max_file_size_;
        if (env_max) {
            try { maxsz = static_cast<std::uint64_t>(std::stoull(env_max)); } catch(...) {}
        }
        const char* env_rot = std::getenv("LOG_ROTATE_COUNT");
        int rc = file_rotate_count_;
        if (env_rot) {
            try { rc = std::stoi(env_rot); } catch(...) {}
        }
        init(file, env_log_level, maxsz, rc);
    }

    rotate_if_needed_locked();

    nlohmann::json json_obj;
    json_obj["timestamp"] = timestamp_iso();
    json_obj["level"] = level_to_string(level);
    const char* service_env = std::getenv("SERVICE_NAME");
    json_obj["service"] = service_env ? service_env : "chat_server";
    json_obj["thread_id"] = std::to_string(std::hash<std::thread::id>{}(std::this_thread::get_id()));
    json_obj["message"] = message;
    if (!extra.is_null()) json_obj["extra"] = extra;

    if (output_file_stream_.is_open()) {
        output_file_stream_ << json_obj.dump() << "\n";
        output_file_stream_.flush();
    } else {
        std::fprintf(stderr, "%s\n", json_obj.dump().c_str());
    }
}

void Logger::debug(const std::string& message, const nlohmann::json& extra) { log(LogLevel::Debug, message, extra); }
void Logger::info(const std::string& message, const nlohmann::json& extra)  { log(LogLevel::Info,  message, extra); }
void Logger::warn(const std::string& message, const nlohmann::json& extra)  { log(LogLevel::Warn,  message, extra); }
void Logger::error(const std::string& message, const nlohmann::json& extra) { log(LogLevel::Err, message, extra); }