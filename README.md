# FPP Pushover Notifications

A personal, unlisted FPP plugin for FPP 9.3. The repository is public so FPP can retrieve it, but it is not submitted to the official FPP plugin catalog.

## Installation

In FPP, open **Content Setup → Plugin Manager** and use this manifest URL:

`https://raw.githubusercontent.com/jdesign21/FPPPushoverNotifications/main/pluginInfo.json`

The FPP install script registers the native callback handler and builds the C++ shared library against the installed FPP source tree. On FPP 9.3, the callback dispatcher is stored as executable in Git and the installer disables Git file-mode drift tracking. This prevents the install script from making the checkout appear modified and blocking later `git pull` updates.

## Current milestone (0.3.4)

The current build monitors native FPP playlist-start and media-change callbacks. When a playlist starts or FPP changes media, the plugin writes a timestamped entry (including available song title and artist) to:

`/home/fpp/media/logs/plugin-FPPPushoverNotifications.log`

The configuration page stores the Pushover User Key and API Token locally in the FPP plugin settings and provides a test-notification button. When RemoteFalcon song notifications are enabled and credentials are configured, each detected RemoteFalcon song selection sends an HTTPS notification to Pushover. The message contains the song title and artist. Duplicate title/artist callbacks received within five seconds are suppressed; the same song can be selected again after that window. Show start/end events are still tracked locally and are not yet sent to Pushover. The plugin does not use MQTT, Node-RED, or a broker. HTTPS requests are made directly to Pushover using the system curl executable.

## Configuration page

Open **Content Setup → Pushover Notifications** to configure Pushover credentials, send a test notification, view plugin status, and manage the playlist event log. Credentials are not displayed after saving; leave a credential field blank to retain its saved value.

## Compatibility

- FPP 9.3 through 9.x
- Raspberry Pi builds supported by FPP 9.3, subject to successful compilation on the target player

The native library is rebuilt by `scripts/fpp_install.sh` during plugin installation and updates.


### 0.3.4
- Shows a masked configured indicator for each saved Pushover credential without revealing the secret value.
- Removes the automatic 10-second page refresh so copying, pasting, and editing forms are not interrupted.

### 0.3.3
- Reworked automatic Pushover delivery to invoke curl with direct argument passing instead of a piped curl configuration.
- Added a separate Pushover request log at `/home/fpp/media/logs/plugin-FPPPushoverNotifications-pushover.log` for HTTP responses and delivery errors.

### 0.3.2
- Added automatic Pushover notifications for enabled RemoteFalcon song selections.
- Sends song title and artist directly to Pushover over HTTPS without MQTT or Node-RED.
- Keeps duplicate callback suppression and leaves show start/end notifications for a later milestone.

### 0.3.1
- Improved credential-save verification using both FPP's write result (when available) and read-back comparison.
- Reports which credential field could not be saved without exposing either secret.

### 0.3.0
- Added local Pushover User Key and API Token configuration.
- Added a test-notification action using HTTPS with certificate verification and a 20-second request timeout.
- Credentials are never echoed back into the configuration page.

### 0.2.5
- Compact notification history rows and added a confirmation-protected Clear History action that empties the plugin event log.

### 0.2.4
- Fixed settings save handling: FPP 9.3 `WriteSettingToFile()` returns no status value, so the dashboard now verifies saved settings by reading them back instead of treating the void return as a failure.
