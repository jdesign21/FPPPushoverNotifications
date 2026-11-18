# FPP Pushover Notifications — Project Overview and Open Issues

**Repository:** `jdesign21/FPPPushoverNotifications`  
**Current repository version:** 0.4.1  
**Target platform:** Falcon Player (FPP) 9.3 on Raspberry Pi  
**Primary target currently used for development:** Raspberry Pi 4, FPP OS build v2025-11  
**Document status:** Engineering handoff / current known state


## FPP target system snapshot

The following system information was supplied from the target FPP player's System Information page. Boot time and daemon uptime are a snapshot from the time the information was collected; they will change after a reboot or as the service continues running.

| Property | Value |
|---|---|
| FPP Version | 9.3 |
| Platform | Raspberry Pi (Pi 4) |
| FPP OS Build | v2025-11 |
| OS Version | Raspbian GNU/Linux 12 (bookworm) |
| Hardware Serial Number | `10000000a4c29e3b` |
| Kernel Version | `6.12.57-v7l+` |
| System Boot Time | `2026-11-17 20:56:03` |
| fppd Uptime | `02:46` |

This snapshot supplements the target-platform details above and should be used when reproducing build, plugin-loading, callback, settings-persistence, or runtime issues on the same FPP environment.
  
**Last reviewed:** 2026-11-18

---

## 1. Project purpose

FPP Pushover Notifications is a native FPP plugin that sends phone notifications through the Pushover service when selected events occur in a Falcon Player light show.

The intended notifications are:

1. **RemoteFalcon song selection** — notify when a visitor selects a song from the RemoteFalcon playlist.
2. **Main show starts** — notify when the configured main show playlist starts.
3. **Main show ends** — notify when the configured main show playlist finishes.

The plugin also provides a dashboard inside FPP to configure event options and Pushover credentials, send a test notification, review recent events, and inspect automatic-delivery diagnostics.

### Explicit design constraints

- Use FPP's native C++ plugin callbacks for event detection.
- Send HTTPS requests directly to Pushover.
- Do not use MQTT, Node-RED, an MQTT broker, or an ESP32 as part of this notification path.
- Do not notify for ordinary playlist media changes. Song notifications are limited to the exact RemoteFalcon origin marker.
- Store configuration and event logs locally on the FPP player.
- Do not reveal Pushover credentials in the dashboard HTML or visible page content.

## 2. Event behavior

### 2.1 RemoteFalcon song selection

The original Node-RED flow used this exact origin check:

```javascript
var remote = msg.payload["FPP/falcon/player/MASTERSHOW/playlist/name/status"];
var title = msg.payload["FPP/falcon/player/MASTERSHOW/playlist/media/title"];
var artist = msg.payload["FPP/falcon/player/MASTERSHOW/playlist/media/artist"];

if (remote === "RemoteFalcon") {
    msg.payload = `${title} by ${artist} has been selected.`;
    return msg;
}
```

The native plugin implements the equivalent rule by checking that the playlist name supplied to the media callback is exactly `RemoteFalcon`.

When enabled, the expected notification is:

- **Title:** `RemoteFalcon Song Selected`
- **Message:** `<song title> by <artist> has been selected.`

The C++ code suppresses duplicate title/artist callbacks received within five seconds. A later selection of the same song is allowed after that window.

Other playlists are logged as media changes but are not counted as RemoteFalcon song selections and do not trigger song notifications.

### 2.2 Main show start

When a playlist-start callback arrives and its name matches the configured **Main show playlist name**, and the Show starts option is enabled, the plugin records a show-start event and attempts to send:

- **Title:** `Show Started`
- **Message:** `Your <playlist name> playlist has started.`

The current default main playlist name is `Light Show`.

### 2.3 Main show end

FPP's stop callback may arrive after the playlist state has gone idle, with an empty playlist name. The current implementation tracks playlist starts in a stack and removes the most recently started playlist on a stop event. If that stopped playlist matches the configured main playlist and Show ends is enabled, the plugin records and attempts to send:

- **Title:** `Show Ended`
- **Message:** `Your <playlist name> playlist has ended.`

The stack behavior is intended to account for nested/inserted playlists. It still needs validation against the actual callback sequence on the target FPP installation.

## 3. Architecture and important files

| File | Responsibility |
|---|---|
| `pluginInfo.json` | FPP Plugin Manager manifest, repository metadata, compatibility and privacy information |
| `callbacks.sh` | Declares the plugin's native C++ callback type to FPP |
| `FPPPushoverNotifications.cpp` | Native plugin implementation: FPP callbacks, settings reads, event detection, logging, duplicate suppression and Pushover sender |
| `Makefile` | Builds the shared library against the installed FPP source tree |
| `scripts/fpp_install.sh` | Validates the callback registration, rebuilds the native library and asks FPP to reload it |
| `scripts/fpp_uninstall.sh` | Requests an FPP restart/reload during uninstall |
| `plugin_setup.php` | FPP dashboard, settings UI, credential save/test actions, event history and delivery diagnostics |
| `README.md` | Installation and release notes |
| `docs/PROJECT_OVERVIEW.md` | This engineering overview and issue-tracking document |

The native shared library is expected to be:

`libFPPPushoverNotifications.so`

FPP discovers the callback type from `callbacks.sh --list`, then loads the native library and calls its exported `createPlugin()` factory.

## 4. Runtime data and configuration

### 4.1 Plugin configuration

The expected FPP plugin configuration file is:

`/home/fpp/media/config/plugin.FPPPushoverNotifications`

The C++ implementation currently reads this path using:

- `MEDIADIR` environment variable plus `/config/plugin.FPPPushoverNotifications`, if `MEDIADIR` is set.
- Otherwise, `/home/fpp/media/config/plugin.FPPPushoverNotifications`.

The PHP dashboard uses FPP's `ReadSettingFromFile()` and `WriteSettingToFile()` helpers with the plugin name `FPPPushoverNotifications`. In FPP 9.3, the helper resolves the plugin file through FPP's configured `configDirectory`.

**Important:** The PHP-resolved configuration path and the C++-resolved configuration path must be confirmed to be identical on the target player. If they differ, the dashboard/test action and native sender can read different settings even though both appear to be using the same plugin name.

Expected settings:

| Setting | Default | Used by |
|---|---:|---|
| `RemoteFalconEnabled` | `1` | C++ media callback |
| `ShowStartEnabled` | `1` | C++ playlist callback |
| `ShowEndEnabled` | `1` | C++ playlist callback |
| `MainShowPlaylist` | `Light Show` | C++ playlist callback |
| `AutoRefreshEnabled` | `0` | PHP dashboard only |
| `PushoverUserKey` | empty | PHP test action and C++ sender |
| `PushoverApiToken` | empty | PHP test action and C++ sender |

The C++ event logic reads the four event-related settings when processing callbacks. Dashboard auto-refresh is a PHP/UI behavior and is not used by the C++ event monitor.

### 4.2 Logs

Expected log files:

- Event and sender-decision log:  
  `/home/fpp/media/logs/plugin-FPPPushoverNotifications.log`
- Curl request/response log:  
  `/home/fpp/media/logs/plugin-FPPPushoverNotifications-pushover.log`

The C++ code uses `LOGDIR` if set, otherwise it uses `<MEDIADIR>/logs` or `/home/fpp/media/logs`.

The PHP dashboard reads its log directory from FPP's `$logDirectory`. As with the configuration file, verify that PHP and C++ resolve to the same physical log directory. A path mismatch could make the dashboard show an empty diagnostics panel even when the native process wrote entries elsewhere.

### 4.3 Expected diagnostic messages

The current C++ sender is designed to write these messages:

| Diagnostic | Meaning |
|---|---|
| `NATIVE PLUGIN LOADED | version=0.4.1` | The C++ plugin constructor ran and the native plugin was loaded |
| `PUSHOVER SENDER ENTERED` | An enabled event reached the sender function |
| `PUSHOVER SKIPPED | credentials missing` | Sender ran but could not find one or both credentials in its config file |
| `PUSHOVER ATTEMPT` | Both credentials were read and the sender is preparing a request |
| `PUSHOVER REQUEST STARTED` | The detached curl process was launched |
| `PUSHOVER ERROR` | The sender encountered a process-launch problem |
| `HTTP_STATUS:200` | Curl received HTTP 200 from the Pushover API; the response body should also be inspected |

A `PUSHOVER REQUEST STARTED` entry only proves that the curl process was launched. It does not by itself prove that Pushover accepted the message. The curl response log must be checked for the HTTP status and response body.

## 5. Pushover integration

The operator must have:

- A Pushover account and a valid User Key.
- A Pushover application/API token created for this plugin.
- The FPP player able to make outbound HTTPS requests to `api.pushover.net`.
- A working system curl executable for the native C++ sender.
- PHP cURL enabled for the dashboard's test-notification action.

The PHP test action sends a direct HTTPS POST to:

`https://api.pushover.net/1/messages.json`

The native C++ sender invokes the system curl executable and posts the same essential fields: token, user, title, message and priority.

The test action and automatic event sender are separate code paths:

- **Test action:** PHP cURL in `plugin_setup.php`.
- **Automatic notifications:** native C++ callback and detached system curl process in `FPPPushoverNotifications.cpp`.

Therefore, a successful test notification proves that the credentials used by the PHP test action and the PHP HTTPS path work. It does **not** prove that the C++ process can read the same credential file, reach its sender function, execute curl, or write to the dashboard-visible log directory.

Credentials must never be printed in logs, diagnostics, error messages or HTML.

## 6. Settings save behavior in version 0.4.1

The notification-preferences form currently uses JavaScript to save five settings through FPP's plugin settings API:

`PUT api/plugin/FPPPushoverNotifications/settings/<SettingName>`

After each PUT, the page issues a GET for the same setting and compares the returned value with the submitted value. The page reports success only after all five values have been read back successfully.

This follows the FPP 9.3 plugin settings API route pattern. FPP 9.3's underlying `WriteSettingToFile()` helper does not return a success boolean, so the correct approach is to read the setting back rather than trust the helper's return value.

The PHP page still contains a legacy POST handler for notification settings. The current UI JavaScript intercepts the form submission and uses the API path instead. The legacy handler should eventually be removed or retained only as a deliberate non-JavaScript fallback; there should not be two competing save implementations.

The Pushover credential form is separate from the notification-preferences form. It still uses the PHP POST handler, FPP's `WriteSettingToFile()`, and read-back verification.

## 7. Current outstanding issues

These issues were reported after installing version 0.4.0 and remain the primary items to verify/fix. Version 0.4.1 contains changes intended to improve settings persistence and diagnostics, but it has not yet been confirmed working on the Raspberry Pi.

### Issue 1 — Pushover credentials show "Not configured"

**Observed behavior**

The dashboard displays:

- Pushover User Key: `Not configured`
- Pushover API Token: `Not configured`

The C++ sender reads these values from the plugin configuration file. The dashboard credential indicators and PHP test action read the same named settings using FPP's PHP settings helper.

**What must be established**

1. Does `/home/fpp/media/config/plugin.FPPPushoverNotifications` exist on the player?
2. Does it contain non-empty `PushoverUserKey` and `PushoverApiToken` entries? Do not copy the secret values into tickets, logs or chat.
3. What exact path does FPP's PHP `configDirectory` resolve to?
4. What exact path does the C++ process resolve from `MEDIADIR`?
5. Does the web-server account have permission to write the plugin configuration file?
6. Does the credential-save POST report success only after both values have been read back?

**Likely diagnostic direction**

Compare the PHP-resolved plugin config path with the C++ path. A mismatch is a plausible explanation for the dashboard/native sender disagreement. Also verify the credential-save POST and read-back behavior independently of the notification-preferences API.

**Acceptance criteria**

- Save both credentials once.
- Reload the dashboard.
- Both fields show the masked saved/configured indicator.
- The test notification succeeds.
- The C++ sender reads the same saved values without exposing them.

### Issue 2 — Automatic Pushover diagnostics panel is empty

**Observed behavior**

The dashboard has shown event history entries such as `SHOW STARTED | playlist=Light Show`, while the Pushover Delivery Diagnostics panel says:

`No automatic Pushover request has been recorded yet.`

The event history proves that some event-tracking code has written to a log. It does not prove that the native sender was reached.

**What the empty panel can mean**

- The native C++ plugin was not loaded, or its updated library was not rebuilt/reloaded.
- The plugin constructor or sender is writing to a different log directory from the one PHP reads.
- The PHP page cannot read the C++ log file.
- The event was logged by one code path but the sender branch was not entered because the corresponding event option was disabled or the playlist/origin did not match.
- The dashboard's diagnostic filter or log parsing is not showing the actual entries.

**Required diagnostic sequence**

1. Confirm the native load marker appears in the same log file/directory that the dashboard reads.
2. Trigger one enabled main-show start event.
3. Confirm `PUSHOVER SENDER ENTERED` appears.
4. If the next line is `PUSHOVER SKIPPED`, investigate config path and credential persistence.
5. If `PUSHOVER ATTEMPT` appears, inspect the separate curl log for process execution and HTTP response.
6. If no sender-entry line appears, inspect callback registration, native library loading, event settings and playlist matching.
7. Confirm the dashboard reads the exact files written by the C++ process.

**Acceptance criteria**

A real event must produce a visible sender-entry diagnostic, then a clear skipped/attempt/error result. If a request is launched, the dashboard or log must show the HTTP response. An empty diagnostics panel after a confirmed event is not acceptable.

### Issue 3 — Notification Settings do not persist

**Observed behavior**

The page previously displayed `Notification settings saved.`, but after a refresh the controls returned to their defaults.

**Version 0.4.1 change**

The save handler now uses FPP's per-setting plugin API and reads every setting back before showing:

`Notification settings saved and verified.`

If an API call fails or the read-back value differs, it should show which setting could not be saved instead of displaying success.

**What still needs testing**

1. Change all five settings to values that differ from their defaults.
2. Click Save Settings.
3. Confirm the page reports that all settings were saved and verified.
4. Confirm the page reloads with the same values.
5. Reopen the page manually and verify the values still remain.
6. Inspect the plugin config file locally on the player to confirm the five keys are persisted.
7. Test each checkbox independently, including turning it off, to ensure `0` is saved rather than the key being left at `1`.

**Potential remaining causes**

- The API request URL or method is not reaching the FPP 9.3 plugin settings route.
- The FPP API returns an error that the UI does not expose clearly.
- The settings API writes to a different config path than the dashboard reads.
- The page's JavaScript handler is not running, for example if jQuery is unavailable or the browser reports a JavaScript error.
- A later page-load/defaulting step is overriding the saved value.
- The FPP web-server account cannot write the plugin config file.

**Acceptance criteria**

All five settings survive a page reload and a subsequent FPP dashboard navigation. The UI must never display a success message unless the saved values have been read back and verified.

## 8. Installation and update requirements

The intended installation/update path is FPP **Content Setup → Plugin Manager**, using the manifest:

`https://raw.githubusercontent.com/jdesign21/FPPPushoverNotifications/main/pluginInfo.json`

The manifest currently targets FPP 9.3 through 9.99 and the `main` branch.

The install script expects:

- FPP installed at `/opt/fpp`, unless `FPPDIR` is supplied.
- FPP's `scripts/common` file.
- FPP source/build files under `/opt/fpp/src`, including `Plugin.h` and FPP makefiles.
- A working C/C++ toolchain and Make.
- The FPP shared library and JSON library available to the linker.

During installation/update, `scripts/fpp_install.sh`:

1. Loads FPP's common shell helpers.
2. Validates that `callbacks.sh --list` returns `c++`.
3. Runs `make clean` and `make all` against the installed FPP source.
4. Verifies that the native shared library was produced.
5. Sets its permissions.
6. Requests an FPP plugin reload/restart as appropriate for the installer context.

A dashboard version string or GitHub commit alone does not prove that the native library compiled successfully or was loaded. The native load marker is intended to provide runtime evidence.

## 9. Security and reliability requirements

- Never log or display the Pushover User Key or API Token.
- Keep credentials in the local FPP plugin configuration file.
- Use HTTPS with certificate verification for the PHP test request.
- Do not send credentials through query strings.
- Avoid blocking FPP event callbacks on network timeouts.
- The current C++ sender uses a detached curl process so callback processing does not wait for the HTTP request to finish.
- Keep diagnostic messages useful but free of secrets.
- Avoid claiming a notification was delivered based only on a successful fork/exec. Confirm the Pushover HTTP response.
- Validate playlist names and setting values before persisting them.
- Preserve the exact RemoteFalcon-only origin check.

## 10. Recommended work order

1. **Verify the settings API on FPP 9.3.** Confirm the browser PUT and GET requests, their HTTP responses, and the actual config file contents after save.
2. **Verify both config paths.** Compare PHP's configured plugin config directory with the C++ process's MEDIADIR-derived path.
3. **Verify credential save/read-back.** Resolve why the dashboard currently says both credentials are not configured.
4. **Verify native plugin loading.** Confirm the 0.4.1 library was rebuilt and that the constructor marker is written.
5. **Verify log-directory agreement.** Make sure the dashboard reads the exact main and curl logs written by C++.
6. **Trace one show-start event.** Follow it from callback to sender-entry, credentials decision, curl launch and HTTP response.
7. **Trace one RemoteFalcon song selection.** Confirm the exact RemoteFalcon marker and message contents.
8. **Validate show-end stack behavior.** Test a normal show end and a show containing inserted/nested playlists.
9. **Remove or formalize the legacy settings POST handler.** Keep one clear settings persistence implementation, with a deliberate fallback if required.
10. **Update README and release notes** after target-device verification, not merely after source edits.

## 11. Test/verification checklist

- [ ] Plugin Manager update completes without build errors.
- [ ] Native plugin load marker appears after FPP loads the library.
- [ ] PHP and C++ resolve the same plugin configuration file.
- [ ] PHP and C++ resolve the same log directory.
- [ ] Notification preference changes persist after reload.
- [ ] Turning each notification option off persists the disabled state.
- [ ] Main show playlist name persists exactly.
- [ ] Pushover User Key saved indicator appears after saving.
- [ ] Pushover API Token saved indicator appears after saving.
- [ ] Test notification succeeds.
- [ ] Main show start creates a sender-entry diagnostic.
- [ ] Main show end creates a sender-entry diagnostic.
- [ ] RemoteFalcon song selection creates a sender-entry diagnostic.
- [ ] Non-RemoteFalcon media changes do not send song notifications.
- [ ] Duplicate callback suppression works as intended.
- [ ] Curl log contains a meaningful HTTP status and response.
- [ ] No credential values appear in either log or dashboard.
- [ ] Dashboard diagnostics display the same events found in the underlying log files.

## 12. Current status summary

Version 0.4.1 is committed to the repository. It includes a revised notification-preferences save flow using FPP's per-setting API and a native-plugin load marker intended to make runtime loading visible in the dashboard diagnostics.

**The three reported problems are not yet confirmed resolved on the Raspberry Pi:**

1. Both Pushover credentials currently display `Not configured`.
2. The automatic-delivery diagnostics panel has been empty.
3. Notification preferences have reverted to defaults after a save.

No successful target-device build, native runtime load, automatic Pushover delivery, or persistence test has been verified as part of this source review. These must remain open until the checklist above is completed on the actual FPP player.
