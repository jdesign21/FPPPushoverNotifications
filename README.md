# FPP Pushover Notifications

A personal, unlisted FPP plugin for FPP 9.3. The repository is public so FPP can retrieve it, but it is not submitted to the official FPP plugin catalog.

## Installation

In FPP, open **Content Setup → Plugin Manager** and use this manifest URL:

`https://raw.githubusercontent.com/jdesign21/FPPPushoverNotifications/main/pluginInfo.json`

The FPP install script registers the native callback handler and builds the C++ shared library against the installed FPP source tree. Updates should be performed in place through FPP Plugin Manager; there should be no need to uninstall/reinstall or run Git/Make manually.

## Current milestone (0.2.2)

The current build monitors native FPP playlist-start and media-change callbacks. When a playlist starts or FPP changes media, the plugin writes a timestamped entry (including available song title and artist) to:

`/home/fpp/media/logs/plugin-FPPPushoverNotifications.log`

Pushover delivery and credential settings are not enabled in this milestone. RemoteFalcon song selections are filtered by playlist origin and duplicate title/artist callbacks received within five seconds are suppressed. The same song can be counted again after that window. No MQTT, Node-RED, broker, or external notification request is used.

## Configuration page

Open **Content Setup → Pushover Notifications** to view plugin status and the playlist event log.

## Compatibility

- FPP 9.3 through 9.x
- Raspberry Pi builds supported by FPP 9.3, subject to successful compilation on the target player

The native library is rebuilt by `scripts/fpp_install.sh` during plugin installation and updates.
