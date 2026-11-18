#include "fpp-pch.h"
#include "Plugin.h"
#include "mediadetails.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <string>

namespace {
std::mutex logMutex;

struct PluginSettings {
    bool remoteFalconEnabled = true;
    bool showStartEnabled = true;
    bool showEndEnabled = true;
    std::string mainShowPlaylist = "Light Show";
};

std::string mediaDirectory() {
    const char* mediaDir = std::getenv("MEDIADIR");
    return (mediaDir && *mediaDir) ? mediaDir : "/home/fpp/media";
}

std::string pluginLogPath() {
    const char* logDir = std::getenv("LOGDIR");
    const std::string directory = (logDir && *logDir)
        ? logDir
        : mediaDirectory() + "/logs";
    return directory + "/plugin-FPPPushoverNotifications.log";
}

std::string trim(const std::string& value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

bool settingIsEnabled(const std::string& value, bool fallback) {
    if (value.empty()) return fallback;
    std::string normalized = value;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return normalized == "1" || normalized == "true" ||
           normalized == "yes" || normalized == "on";
}

PluginSettings loadSettings() {
    PluginSettings settings;
    const std::string path = mediaDirectory() + "/config/plugin.FPPPushoverNotifications";
    std::ifstream file(path);
    std::string line;
    while (std::getline(file, line)) {
        const auto separator = line.find('=');
        if (separator == std::string::npos) continue;
        const std::string key = trim(line.substr(0, separator));
        std::string value = trim(line.substr(separator + 1));
        if (value.size() >= 2 && value.front() == '"' && value.back() == '"')
            value = value.substr(1, value.size() - 2);

        if (key == "RemoteFalconEnabled")
            settings.remoteFalconEnabled = settingIsEnabled(value, true);
        else if (key == "ShowStartEnabled")
            settings.showStartEnabled = settingIsEnabled(value, true);
        else if (key == "ShowEndEnabled")
            settings.showEndEnabled = settingIsEnabled(value, true);
        else if (key == "MainShowPlaylist" && !value.empty())
            settings.mainShowPlaylist = value;
    }
    return settings;
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

std::string playlistName(const Json::Value& playlist) {
    if (playlist.isMember("name") && playlist["name"].isString())
        return playlist["name"].asString();
    return "";
}

void logMediaChange(const Json::Value& playlist,
                    const MediaDetails& mediaDetails) {
    const PluginSettings settings = loadSettings();
    const std::string payload = playlist.toStyledString();

    // RemoteFalcon is the exact origin marker used by the original flow.
    // Other media changes remain diagnostic-only and are never counted as requests.
    if (settings.remoteFalconEnabled &&
        playlistName(playlist) == "RemoteFalcon") {
        std::ostringstream line;
        line << "REMOTE FALCON SONG SELECTED"
             << " | title=" << mediaDetails.title
             << " | artist=" << mediaDetails.artist;
        appendLog(line.str());
        return;
    }

    std::ostringstream line;
    line << "MEDIA CHANGE"
         << " | title=" << mediaDetails.title
         << " | artist=" << mediaDetails.artist
         << " | playlist=" << playlistName(playlist)
         << " | callback=";
    std::string compactPayload = payload;
    std::replace(compactPayload.begin(), compactPayload.end(), '\n', ' ');
    std::replace(compactPayload.begin(), compactPayload.end(), '\r', ' ');
    line << compactPayload;
    appendLog(line.str());
}

void handlePlaylistEvent(const Json::Value& playlist,
                         const std::string& action,
                         const std::string& section,
                         int item) {
    const PluginSettings settings = loadSettings();
    const std::string name = playlistName(playlist);
    if (name.empty() || name != settings.mainShowPlaylist) return;

    if (action == "start" && settings.showStartEnabled) {
        appendLog("SHOW STARTED | playlist=" + name);
    } else if ((action == "stop" || action == "end") && settings.showEndEnabled) {
        appendLog("SHOW ENDED | playlist=" + name);
    }
}
}

class FPPPushoverNotifications : public FPPPlugin {
public:
    FPPPushoverNotifications() : FPPPlugin("FPPPushoverNotifications") {}

    void playlistCallback(const Json::Value& playlist,
                          const std::string& action,
                          const std::string& section,
                          int item) override {
        handlePlaylistEvent(playlist, action, section, item);
    }

    void mediaCallback(const Json::Value& playlist,
                       const MediaDetails& mediaDetails) override {
        logMediaChange(playlist, mediaDetails);
    }
};

extern "C" FPPPlugin* createPlugin() {
    return new FPPPushoverNotifications();
}
