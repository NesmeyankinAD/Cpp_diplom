#include "HttpLogger.h"

#include <mutex>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>

std::mutex g_logMutex;

void HttpLogger::log(const std::string& tag, const std::string& message)
{
    using namespace std::chrono;
    auto now = system_clock::now();
    std::time_t t = system_clock::to_time_t(now);
    auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;

    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif

    std::lock_guard<std::mutex> lock(g_logMutex);
    std::cerr << "[" << std::put_time(&tm, "%F %T")
        << "." << std::setw(3) << std::setfill('0') << ms.count()
        << "] [" << tag << "] " << message << "\n";
}