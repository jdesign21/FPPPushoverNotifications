# FPP Pushover Notifications

Personal FPP 9.3 plugin project.

## Milestone 0.1 — playlist-start callback test

When FPP calls the native playlist callback with action `start`, this plugin writes a timestamped line containing the playlist name to the FPP plugin log.

Log file: `plugin-FPPPushoverNotifications.log` in FPP's configured log directory.

No Pushover credentials or outbound network requests are used yet. No MQTT, Node-RED, or broker is involved.

## Install on FPP 9.3

In Content Setup → Plugin Manager, enter:

`https://raw.githubusercontent.com/jdesign21/FPPPushoverNotifications/main/pluginInfo.json`

Retrieve the plugin information and install. The install script builds the native shared library against the FPP source tree.

## Test

1. Install the plugin and let FPP complete its normal plugin reload.
2. Start a playlist.
3. Open the FPP log viewer and inspect `plugin-FPPPushoverNotifications.log`.
4. Confirm a `PLAYLIST START` entry appears with the playlist name.

This is an initial integration build. It must be compiled and exercised on the target FPP 9.3 Raspberry Pi before it is considered validated.
