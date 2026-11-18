#include "fpp-pch.h"
#include "Plugin.h"
#include "mediadetails.h"

#include <algorithm>
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

std::string timestamp() {
    const std::time_t now = std::time(nullptr);
    std::tm localTime{};
    localtime_r(&now, &localTime);
    std::ostringstream value;
    value << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S");
    return value.str();
}

void appendLog(const std::string& message) {
    std::lock_guard<std::mutex> lock(logMutex);
    std::ofstream log(pluginLogPath(), std::ios::out | std::ios::app);
    if (log) log << timestamp() << " " << message << '\n';
}

void logPlaylistStart(const Json::Value& playlist,
                      const std::string& section,
                      int item) {
    std::ostringstream line;
    line << "PLAYLIST START";
    if (playlist.isMember("name") && playlist["name"].isString())
        line << " | " << playlist["name"].asString();
    if (!section.empty())
        line << " | section=" << section;
    line << " | item=" << item;
    appendLog(line.str());
}

void logMediaChange(const Json::Value& playlist,
                    const MediaDetails& mediaDetails) {
    std::ostringstream line;
    line << "MEDIA CHANGE"
         << " | title=" << mediaDetails.title
         << " | artist=" << mediaDetails.artist
         << " | playlist=";

    if (playlist.isMember("name") && playlist["name"].isString())
        line << playlist["name"].asString();
    else
        line << "(name unavailable)";

    // Capture the native callback payload temporarily to identify the
    // RemoteFalcon-origin field for the exact filter used by the old flow.
    std::string payload = playlist.toStyledString();
    std::replace(payload.begin(), payload.end(), '\n', ' ');
    std::replace(payload.begin(), payload.end(), '\r', ' ');
    line << " | callback=" << payload;
    appendLog(line.str());
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

    void mediaCallback(const Json::Value& playlist,
                       const MediaDetails& mediaDetails) override {
        logMediaChange(playlist, mediaDetails);
    }
};

extern "C" FPPPlugin* createPlugin() {
    return new FPPPushoverNotifications();
}
