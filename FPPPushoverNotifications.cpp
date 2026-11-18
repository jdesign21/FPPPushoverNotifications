#include "fpp-pch.h"
#include "Plugin.h"

#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <string>

namespace {
std::mutex logMutex;

std::string pluginLogPath() {
    const char* logDir = std::getenv("LOGDIR");
    const std::string directory = (logDir && *logDir)
        ? logDir
        : "/home/fpp/media/logs";
    return directory + "/plugin-FPPPushoverNotifications.log";
}

void logPlaylistStart(const Json::Value& playlist,
                      const std::string& section,
                      int item) {
    std::lock_guard<std::mutex> lock(logMutex);
    const std::time_t now = std::time(nullptr);
    std::tm localTime{};
    localtime_r(&now, &localTime);

    std::ostringstream line;
    line << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S")
         << " PLAYLIST START";
    if (playlist.isMember("name") && playlist["name"].isString())
        line << " | " << playlist["name"].asString();
    if (!section.empty())
        line << " | section=" << section;
    line << " | item=" << item << '\n';

    // Open for each write so FPP's log rotation can replace the file safely.
    std::ofstream log(pluginLogPath(), std::ios::out | std::ios::app);
    if (log) log << line.str();
}
}

class FPPPushoverNotifications : public FPPPlugin {
public:
    FPPPushoverNotifications() : FPPPlugin("FPPPushoverNotifications") {}

    void playlistCallback(const Json::Value& playlist,
                          const std::string& action,
                          const std::string& section,
                          int item) override {
        if (action == "start")
            logPlaylistStart(playlist, section, item);
    }
};

extern "C" FPPPlugin* createPlugin() {
    return new FPPPushoverNotifications();
}
