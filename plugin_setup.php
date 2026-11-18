<?php
// FPP Pushover Notifications - initial configuration and event monitor page.
if (!isset($pluginName) || $pluginName !== 'FPPPushoverNotifications') {
    echo '<div class="alert alert-danger">Invalid plugin page context.</div>';
    return;
}

$pluginVersion = '0.1.1';
$logFile = rtrim($logDirectory, '/') . '/plugin-FPPPushoverNotifications.log';
$logExists = is_file($logFile);
$logLines = array();
$lastEvent = null;

if ($logExists && is_readable($logFile)) {
    $allLines = @file($logFile, FILE_IGNORE_NEW_LINES | FILE_SKIP_EMPTY_LINES);
    if (is_array($allLines)) {
        $logLines = array_slice($allLines, -100);
        for ($i = count($allLines) - 1; $i >= 0; $i--) {
            if (strpos($allLines[$i], 'PLAYLIST START') !== false) {
                $lastEvent = $allLines[$i];
                break;
            }
        }
    }
}

$fppVersion = function_exists('getFPPVersion') ? getFPPVersion() : 'Unknown';
function pushover_h($value) {
    return htmlspecialchars((string)$value, ENT_QUOTES, 'UTF-8');
}
?>

<div id="global" class="settings">
    <meta http-equiv="refresh" content="5">

    <h2>Plugin Configuration</h2>
    <p>This page monitors playlist-start events detected by the native FPP plugin.</p>
    <div class="fppTableWrapper fppTableWrapperAsTable">
        <div class="fppTableContents">
            <table class="fppTable" width="100%">
                <tbody>
                    <tr class="fppTableRow">
                        <td><strong>Plugin</strong></td>
                        <td>FPP Pushover Notifications</td>
                    </tr>
                    <tr class="fppTableRow">
                        <td><strong>Plugin Version</strong></td>
                        <td><?php echo pushover_h($pluginVersion); ?></td>
                    </tr>
                    <tr class="fppTableRow">
                        <td><strong>FPP Version</strong></td>
                        <td><?php echo pushover_h($fppVersion); ?></td>
                    </tr>
                    <tr class="fppTableRow">
                        <td><strong>Connector Type</strong></td>
                        <td>Native FPP C++ plugin</td>
                    </tr>
                    <tr class="fppTableRow">
                        <td><strong>Pushover Delivery</strong></td>
                        <td>Not enabled yet — this build only monitors playlist events.</td>
                    </tr>
                </tbody>
            </table>
        </div>
    </div>

    <h2>Playlist Event Monitor</h2>
    <div class="fppTableWrapper fppTableWrapperAsTable">
        <div class="fppTableContents">
            <table class="fppTable" width="100%">
                <tbody>
                    <tr class="fppTableRow">
                        <td><strong>Native Plugin Log</strong></td>
                        <td><?php echo $logExists ? '<span style="color:green;">Found</span>' : '<span style="color:#c00;">Not created yet</span>'; ?></td>
                    </tr>
                    <tr class="fppTableRow">
                        <td><strong>Last Playlist Start</strong></td>
                        <td><?php echo $lastEvent !== null ? pushover_h($lastEvent) : 'No playlist-start event recorded yet.'; ?></td>
                    </tr>
                </tbody>
            </table>
        </div>
    </div>

    <h2>Debug Information</h2>
    <div class="fppTableWrapper fppTableWrapperAsTable">
        <div class="fppTableContents">
            <table class="fppTable" width="100%">
                <tbody>
                    <tr class="fppTableRow">
                        <td><strong>Current Log File</strong></td>
                        <td><?php echo pushover_h($logFile); ?></td>
                    </tr>
                    <tr class="fppTableRow">
                        <td><strong>Log Read Status</strong></td>
                        <td><?php echo ($logExists && is_readable($logFile)) ? 'Readable' : 'Not available'; ?></td>
                    </tr>
                </tbody>
            </table>
        </div>
    </div>

    <h3>Recent Log Entries</h3>
    <pre style="white-space:pre-wrap; max-height:420px; overflow:auto; padding:10px; border:1px solid #888; border-radius:4px;"><?php
        if (!$logExists) {
            echo 'The log file will appear after the native plugin records its first playlist-start event.';
        } elseif (!is_readable($logFile)) {
            echo 'The log file exists but cannot be read by the FPP web process.';
        } elseif (count($logLines) === 0) {
            echo 'The log file is currently empty.';
        } else {
            echo pushover_h(implode("\n", $logLines));
        }
    ?></pre>
    <p><em>This page refreshes every five seconds. Pushover credentials and notification delivery will be added in a later milestone.</em></p>
</div>
