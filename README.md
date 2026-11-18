# FPP Pushover Notifications

A personal, unlisted FPP plugin for FPP 9.3. The repository is public so FPP can retrieve it, but it is not submitted to the official FPP plugin catalog.

## Installation

In FPP, open **Content Setup → Plugin Manager** and use this manifest URL:

`https://raw.githubusercontent.com/jdesign21/FPPPushoverNotifications/main/pluginInfo.json`

The FPP install script registers the native callback handler and builds the C++ shared library against the installed FPP source tree. On FPP 9.3, the callback dispatcher is stored as executable in Git and the installer disables Git file-mode drift tracking. This prevents the install script from making the checkout appear modified and blocking later `git pull` updates.

## Current milestone (0.2.4)

The current build monitors native FPP playlist-start and media-change callbacks. When a playlist starts or FPP changes media, the plugin writes a timestamped entry (including available song title and artist) to:

`/home/fpp/media/logs/plugin-FPPPushoverNotifications.log`

Pushover delivery and credential settings are not enabled in this milestone. RemoteFalcon song selections are filtered by playlist origin and duplicate title/artist callbacks received within five seconds are suppressed. The same song can be counted again after that window. No MQTT, Node-RED, broker, or external notification request is used.

## Configuration page

Open **Content Setup → Pushover Notifications** to view plugin status and the playlist event log.

## Compatibility

- FPP 9.3 through 9.x
- Raspberry Pi builds supported by FPP 9.3, subject to successful compilation on the target player

The native library is rebuilt by `scripts/fpp_install.sh` during plugin installation and updates.


### 0.2.4
- Fixed settings save handling: FPP 9.3 `WriteSettingToFile()` returns no status value, so the dashboard now verifies saved settings by reading them back instead of treating the void return as a failure.
