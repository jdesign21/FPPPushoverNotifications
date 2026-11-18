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
#include <cstring>
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


std::string pushoverRequestLogPath() {
    const char* logDir = std::getenv("LOGDIR");
    const std::string directory = (logDir && *logDir)
        ? logDir
        : mediaDirectory() + "/logs";
    return directory + "/plugin-FPPPushoverNotifications-pushover.log";
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
        appendLog("PUSHOVER SKIPPED | credentials are not configured | config=" + path);
        return;
    }

    appendLog("PUSHOVER ATTEMPT | title=" + title + " | credentials=loaded");

    const pid_t child = fork();
    if (child < 0) {
        appendLog("PUSHOVER ERROR | could not start request process");
        return;
    }

    if (child == 0) {
        const pid_t worker = fork();
        if (worker < 0) _exit(1);
        if (worker > 0) _exit(0);

        setsid();

        const int logFd = open(pushoverRequestLogPath().c_str(),
                               O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (logFd >= 0) {
            dup2(logFd, STDOUT_FILENO);
            dup2(logFd, STDERR_FILENO);
            if (logFd > STDERR_FILENO) close(logFd);
        } else {
            const int nullFd = open("/dev/null", O_WRONLY);
            if (nullFd >= 0) {
                dup2(nullFd, STDOUT_FILENO);
                dup2(nullFd, STDERR_FILENO);
                close(nullFd);
            }
        }

        const std::vector<std::string> arguments = {
            "curl",
            "--silent",
            "--show-error",
            "--fail-with-body",
            "--connect-timeout", "10",
            "--max-time", "20",
            "--output", "-",
            "--write-out", "\nHTTP_STATUS:%{http_code}\n",
            "--request", "POST",
            "--data-urlencode", "token=" + apiToken,
            "--data-urlencode", "user=" + userKey,
            "--data-urlencode", "title=" + title,
            "--data-urlencode", "message=" + message,
            "--data-urlencode", "priority=0",
            "https://api.pushover.net/1/messages.json"
        };
        std::vector<char*> argv;
        argv.reserve(arguments.size() + 1);
        for (const auto& argument : arguments)
            argv.push_back(const_cast<char*>(argument.c_str()));
        argv.push_back(nullptr);

        // FPP daemon PATH may not include curl; try the standard Debian path too.
        execvp("curl", argv.data());
        execv("/usr/bin/curl", argv.data());
        dprintf(STDERR_FILENO, "CURL EXEC FAILED | errno=%d | %s\\n", errno, strerror(errno));
        _exit(127);
    }

    int status = 0;
    while (waitpid(child, &status, 0) < 0 && errno == EINTR) {}
    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        appendLog("PUSHOVER REQUEST STARTED | " + title);
    } else {
        appendLog("PUSHOVER ERROR | request process did not start");
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
        if (!name.empty() && name == settings.mainShowPlaylist && settings.showStartEnabled) {
            appendLog("SHOW STARTED | playlist=" + name);
            sendPushoverNotification("Show Started",
                "Your " + name + " playlist has started.");
        }
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
            sendPushoverNotification("Show Ended",
                "Your " + stoppedPlaylist + " playlist has ended.");
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
