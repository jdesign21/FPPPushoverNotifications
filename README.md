# FPP Pushover Notifications

A personal, unlisted FPP plugin for FPP 9.3. The repository is public so FPP can retrieve it, but it is not submitted to the official FPP plugin catalog.

## Installation

In FPP, open **Content Setup → Plugin Manager** and use this manifest URL:

`https://raw.githubusercontent.com/jdesign21/FPPPushoverNotifications/main/pluginInfo.json`

The FPP install script registers the native callback handler, builds the C++ shared library against the installed FPP source tree, and requests an FPPD reload. Updates are handled through FPP Plugin Manager; there should be no need to run Git or Make manually.

## Current milestone

The current build verifies native FPP playlist-start callback detection. When FPP starts a playlist, the plugin writes a timestamped entry to:

`/home/fpp/media/logs/plugin-FPPPushoverNotifications.log`

Pushover delivery and credential settings are not enabled in this milestone. No MQTT, Node-RED, broker, or external notification request is used.

## Configuration page

Open **Content Setup → Pushover Notifications** to view plugin status and the playlist event log.

## Compatibility

- FPP 9.3 through 9.x
- Raspberry Pi builds supported by FPP 9.3, subject to successful compilation on the target player

The native library is rebuilt by `scripts/fpp_install.sh` during plugin installation and updates.
