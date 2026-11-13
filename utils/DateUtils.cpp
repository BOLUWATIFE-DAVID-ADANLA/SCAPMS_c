#include "DateUtils.h"

#include <cstdio>
#include <ctime>

namespace scapms::utils::DateUtils {

namespace {
std::tm localTm(std::time_t t) {
    std::tm out{};
#ifdef _WIN32
    localtime_s(&out, &t);
#else
    localtime_r(&t, &out);
#endif
    return out;
}

std::string format(const std::tm& tm, const char* fmt) {
    char buf[32];
    std::strftime(buf, sizeof(buf), fmt, &tm);
    return buf;
}
} // namespace

std::string today() { return format(localTm(std::time(nullptr)), "%Y-%m-%d"); }

std::string currentTime() { return format(localTm(std::time(nullptr)), "%H:%M"); }

long long nowEpoch() { return static_cast<long long>(std::time(nullptr)); }

std::string addDays(const std::string& date, int days) {
    int y = 0, m = 0, d = 0;
    if (std::sscanf(date.c_str(), "%d-%d-%d", &y, &m, &d) != 3) return date;
    std::tm tm{};
    tm.tm_year = y - 1900;
    tm.tm_mon = m - 1;
    tm.tm_mday = d + days;
    tm.tm_hour = 12;  // midday avoids DST edge cases
    std::mktime(&tm);  // normalises day overflow into month/year
    return format(tm, "%Y-%m-%d");
}

std::string epochToTime(long long epoch) {
    return format(localTm(static_cast<std::time_t>(epoch)), "%H:%M");
}

} // namespace scapms::utils::DateUtils
