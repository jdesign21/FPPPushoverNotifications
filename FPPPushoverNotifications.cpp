#include "fpp-pch.h"
#include "Plugin.h"
#include "mediadetails.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <cerrno>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>

namespace {
std::mutex logMutex;
std::mutex playlistStateMutex;
std::vector<std::string> playlistStack;
std::mutex songDedupMutex;
std::unordered_map<std::string, std::chrono::steady_clock::time_point> recentSongEvents;

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


std::string curlConfigValue(const std::string& value) {
    std::string escaped;
    escaped.reserve(value.size() + 8);
    for (char ch : value) {
        if (ch == '\\' || ch == '"') escaped.push_back('\\');
        if (ch == '\r' || ch == '\n') continue;
        escaped.push_back(ch);
    }
    return escaped;
}

void sendPushoverNotification(const std::string& title,
                              const std::string& message) {
    const std::string path = mediaDirectory() + "/config/plugin.FPPPushoverNotifications";
    std::ifstream file(path);
    std::string line;
    std::string userKey;
    std::string apiToken;
    while (std::getline(file, line)) {
        const auto separator = line.find('=');
        if (separator == std::string::npos) continue;
        const std::string key = trim(line.substr(0, separator));
        std::string value = trim(line.substr(separator + 1));
        if (value.size() >= 2 && value.front() == '"' && value.back() == '"')
            value = value.substr(1, value.size() - 2);
        if (key == "PushoverUserKey") userKey = value;
        else if (key == "PushoverApiToken") apiToken = value;
    }

    if (userKey.empty() || apiToken.empty()) {
        appendLog("PUSHOVER SKIPPED | credentials are not configured");
        return;
    }

    int pipeFds[2];
    if (pipe(pipeFds) != 0) {
        appendLog("PUSHOVER ERROR | could not create request pipe");
        return;
    }

    const std::string config =
        "url = \"https://api.pushover.net/1/messages.json\"\n"
        "request = \"POST\"\n"
        "data-urlencode = \"token=" + curlConfigValue(apiToken) + "\"\n"
        "data-urlencode = \"user=" + curlConfigValue(userKey) + "\"\n"
        "data-urlencode = \"title=" + curlConfigValue(title) + "\"\n"
        "data-urlencode = \"message=" + curlConfigValue(message) + "\"\n"
        "data-urlencode = \"priority=0\"\n";

    const pid_t child = fork();
    if (child < 0) {
        close(pipeFds[0]);
        close(pipeFds[1]);
        appendLog("PUSHOVER ERROR | could not start request process");
        return;
    }

    if (child == 0) {
        close(pipeFds[1]);
        const pid_t worker = fork();
        if (worker < 0) _exit(1);
        if (worker > 0) _exit(0);

        setsid();
        dup2(pipeFds[0], STDIN_FILENO);
        close(pipeFds[0]);
        const int nullFd = open("/dev/null", O_WRONLY);
        if (nullFd >= 0) {
            dup2(nullFd, STDOUT_FILENO);
            dup2(nullFd, STDERR_FILENO);
            close(nullFd);
        }
        execlp("curl", "curl", "--config", "-", "--silent", "--show-error",
               "--max-time", "20", static_cast<char*>(nullptr));
        _exit(127);
    }

    close(pipeFds[0]);
    size_t written = 0;
    while (written < config.size()) {
        const ssize_t count = write(pipeFds[1], config.data() + written, config.size() - written);
        if (count < 0) {
            if (errno == EINTR) continue;
            break;
        }
        written += static_cast<size_t>(count);
    }
    close(pipeFds[1]);
    int status = 0;
    while (waitpid(child, &status, 0) < 0 && errno == EINTR) {}
    if (written == config.size() && WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        appendLog("PUSHOVER QUEUED | " + title);
    } else {
        appendLog("PUSHOVER ERROR | request could not be queued");
    }
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
        // FPP may deliver the same media selection callback more than once.
        // Suppress an identical title/artist pair for five seconds, while
        // allowing the same song to be selected again later.
        const std::string songKey = mediaDetails.title + "|" + mediaDetails.artist;
        const auto now = std::chrono::steady_clock::now();
        {
            std::lock_guard<std::mutex> lock(songDedupMutex);
            for (auto it = recentSongEvents.begin(); it != recentSongEvents.end();) {
                if (now - it->second > std::chrono::seconds(5))
                    it = recentSongEvents.erase(it);
                else
                    ++it;
            }

            const auto previous = recentSongEvents.find(songKey);
            if (previous != recentSongEvents.end() &&
                now - previous->second <= std::chrono::seconds(5)) {
                return;
            }
            recentSongEvents[songKey] = now;
        }

        std::ostringstream line;
        line << "REMOTE FALCON SONG SELECTED"
             << " | title=" << mediaDetails.title
             << " | artist=" << mediaDetails.artist;
        appendLog(line.str());
        sendPushoverNotification("RemoteFalcon Song Selected",
            mediaDetails.title + " by " + mediaDetails.artist + " has been selected.");
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

    if (action == "start") {
        if (name.empty()) return;
        {
            std::lock_guard<std::mutex> lock(playlistStateMutex);
            playlistStack.push_back(name);
        }
        if (!name.empty() && name == settings.mainShowPlaylist && settings.showStartEnabled)
            appendLog("SHOW STARTED | playlist=" + name);
        return;
    }

    if (action == "stop") {
        std::string stoppedPlaylist;
        {
            std::lock_guard<std::mutex> lock(playlistStateMutex);
            if (!playlistStack.empty()) {
                stoppedPlaylist = playlistStack.back();
                playlistStack.pop_back();
            }
        }

        // FPP's stop callback is delivered after its playlist state has gone
        // idle, so its JSON name is empty. Track start/stop as a stack instead;
        // inserted playlists nest and unwind in last-in/first-out order.
        if (!stoppedPlaylist.empty() &&
            stoppedPlaylist == settings.mainShowPlaylist &&
            settings.showEndEnabled) {
            appendLog("SHOW ENDED | playlist=" + stoppedPlaylist);
        }
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
