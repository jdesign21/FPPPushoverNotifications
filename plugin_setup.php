<?php
// FPP Pushover Notifications - configuration and native event monitor page.
if (!isset($pluginName) || $pluginName !== 'FPPPushoverNotifications') {
    echo '<div class="alert alert-danger">Invalid plugin page context.</div>';
    return;
}

$pluginVersion = '0.1.3';
$logFile = rtrim($logDirectory, '/') . '/plugin-FPPPushoverNotifications.log';
$logExists = is_file($logFile);
$logReadable = $logExists && is_readable($logFile);
$logLines = array();
$lastEvent = null;
$lastMediaEvent = null;

if ($logReadable) {
    $allLines = @file($logFile, FILE_IGNORE_NEW_LINES | FILE_SKIP_EMPTY_LINES);
    if (is_array($allLines)) {
        $logLines = array_slice($allLines, -100);
        for ($i = count($allLines) - 1; $i >= 0; $i--) {
            if ($lastMediaEvent === null && strpos($allLines[$i], 'MEDIA CHANGE') !== false) {
                $lastMediaEvent = $allLines[$i];
            }
            if ($lastEvent === null && strpos($allLines[$i], 'PLAYLIST START') !== false) {
                $lastEvent = $allLines[$i];
            }
            if ($lastEvent !== null && $lastMediaEvent !== null) break;
        }
    }
}

$fppVersion = function_exists('getFPPVersion') ? getFPPVersion() : 'Unknown';
function pushover_h($value) {
    return htmlspecialchars((string)$value, ENT_QUOTES, 'UTF-8');
}
?>

<style>
#pushover-page {
    --po-bg: #f4f6f9;
    --po-card: #ffffff;
    --po-text: #202938;
    --po-muted: #697586;
    --po-border: #e2e7ee;
    --po-blue: #2878c7;
    --po-green: #238653;
    color: var(--po-text);
    max-width: 1180px;
    margin: 0 auto;
    padding: 8px 4px 28px;
}
#pushover-page * { box-sizing: border-box; }
#pushover-page .po-hero {
    display: flex; align-items: center; justify-content: space-between; gap: 20px;
    padding: 25px 28px; margin: 8px 0 22px; border-radius: 12px;
    color: #fff; background: linear-gradient(120deg, #183b63, #2878c7);
    box-shadow: 0 5px 18px rgba(23, 52, 85, .16);
}
#pushover-page .po-hero h2 { margin: 0 0 7px; color: #fff; font-size: 25px; font-weight: 650; }
#pushover-page .po-hero p { margin: 0; color: rgba(255,255,255,.84); font-size: 14px; }
#pushover-page .po-version {
    flex: 0 0 auto; padding: 7px 12px; border: 1px solid rgba(255,255,255,.35);
    border-radius: 20px; color: #fff; font-size: 12px; font-weight: 700;
    background: rgba(255,255,255,.12);
}
#pushover-page .po-section { margin: 24px 0 12px; font-size: 17px; font-weight: 700; }
#pushover-page .po-grid { display: grid; grid-template-columns: repeat(3, minmax(0, 1fr)); gap: 14px; }
#pushover-page .po-card {
    min-width: 0; padding: 18px 19px; background: var(--po-card);
    border: 1px solid var(--po-border); border-radius: 10px;
    box-shadow: 0 2px 7px rgba(20, 35, 55, .045);
}
#pushover-page .po-card-label {
    margin-bottom: 9px; color: var(--po-muted); font-size: 11px;
    font-weight: 750; letter-spacing: .07em; text-transform: uppercase;
}
#pushover-page .po-card-value { font-size: 16px; font-weight: 650; overflow-wrap: anywhere; }
#pushover-page .po-card-note { margin-top: 7px; color: var(--po-muted); font-size: 12px; line-height: 1.5; }
#pushover-page .po-status {
    display: inline-flex; align-items: center; gap: 7px; padding: 5px 9px;
    border-radius: 20px; font-size: 12px; font-weight: 700;
}
#pushover-page .po-status:before {
    content: ""; width: 7px; height: 7px; display: inline-block; border-radius: 50%;
    background: currentColor;
}
#pushover-page .po-ok { color: #187447; background: #e8f5ee; }
#pushover-page .po-warn { color: #996313; background: #fff4dc; }
#pushover-page .po-error { color: #a83232; background: #fdecec; }
#pushover-page .po-event { border-left: 4px solid var(--po-blue); }
#pushover-page .po-event.media { border-left-color: #7b61b5; }
#pushover-page .po-event-value {
    margin-top: 8px; font-size: 13px; line-height: 1.55; overflow-wrap: anywhere;
}
#pushover-page .po-empty { color: var(--po-muted); font-weight: 400; }
#pushover-page .po-detail {
    padding: 15px 18px; background: #fff; border: 1px solid var(--po-border);
    border-radius: 10px; color: var(--po-muted); font-size: 13px;
}
#pushover-page .po-detail strong { display: block; margin-bottom: 5px; color: var(--po-text); }
#pushover-page .po-path { overflow-wrap: anywhere; font-family: monospace; color: var(--po-text); }
#pushover-page .po-log {
    max-height: 390px; overflow: auto; margin: 0; padding: 16px 18px;
    border: 1px solid #26384d; border-radius: 10px; background: #111c2a;
    color: #d7e3f1; font: 12px/1.65 Consolas, Monaco, monospace;
    white-space: pre-wrap; overflow-wrap: anywhere;
}
#pushover-page .po-footer { margin-top: 12px; color: var(--po-muted); font-size: 12px; }
@media (max-width: 850px) {
    #pushover-page .po-grid { grid-template-columns: repeat(2, minmax(0, 1fr)); }
}
@media (max-width: 560px) {
    #pushover-page .po-hero { align-items: flex-start; flex-direction: column; padding: 20px; }
    #pushover-page .po-grid { grid-template-columns: 1fr; }
}
</style>

<div id="pushover-page">
    <meta http-equiv="refresh" content="5">

    <div class="po-hero">
        <div>
            <h2>Pushover Notifications</h2>
            <p>Native FPP event monitoring and notification configuration.</p>
        </div>
        <span class="po-version">Version <?php echo pushover_h($pluginVersion); ?></span>
    </div>

    <div class="po-section">Plugin Overview</div>
    <div class="po-grid">
        <div class="po-card">
            <div class="po-card-label">Plugin Status</div>
            <div class="po-card-value"><span class="po-status po-ok">Loaded</span></div>
            <div class="po-card-note">Native FPP C++ connector</div>
        </div>
        <div class="po-card">
            <div class="po-card-label">FPP Version</div>
            <div class="po-card-value"><?php echo pushover_h($fppVersion); ?></div>
            <div class="po-card-note">Detected player software version</div>
        </div>
        <div class="po-card">
            <div class="po-card-label">Notification Delivery</div>
            <div class="po-card-value"><span class="po-status po-warn">Not configured</span></div>
            <div class="po-card-note">Pushover delivery will be enabled in a later build.</div>
        </div>
    </div>

    <div class="po-section">Event Monitor</div>
    <div class="po-grid">
        <div class="po-card">
            <div class="po-card-label">Native Plugin Log</div>
            <div class="po-card-value">
                <?php if ($logReadable): ?>
                    <span class="po-status po-ok">Available</span>
                <?php elseif ($logExists): ?>
                    <span class="po-status po-error">Not readable</span>
                <?php else: ?>
                    <span class="po-status po-warn">Waiting for events</span>
                <?php endif; ?>
            </div>
            <div class="po-card-note">The log is created when the plugin records an event.</div>
        </div>
        <div class="po-card po-event">
            <div class="po-card-label">Last Playlist Start</div>
            <div class="po-event-value"><?php echo $lastEvent !== null ? pushover_h($lastEvent) : '<span class="po-empty">No playlist-start event recorded yet.</span>'; ?></div>
        </div>
        <div class="po-card po-event media">
            <div class="po-card-label">Last Song / Media Change</div>
            <div class="po-event-value"><?php echo $lastMediaEvent !== null ? pushover_h($lastMediaEvent) : '<span class="po-empty">No media-change event recorded yet.</span>'; ?></div>
        </div>
    </div>

    <div class="po-section">Recent Activity</div>
    <pre class="po-log"><?php
        if (!$logExists) {
            echo 'The log will appear after the native plugin records its first event.';
        } elseif (!$logReadable) {
            echo 'The log exists but cannot be read by the FPP web process.';
        } elseif (count($logLines) === 0) {
            echo 'No log entries yet.';
        } else {
            echo pushover_h(implode("\n", $logLines));
        }
    ?></pre>

    <div class="po-section">Diagnostics</div>
    <div class="po-detail">
        <strong>Log file location</strong>
        <span class="po-path"><?php echo pushover_h($logFile); ?></span>
    </div>
    <p class="po-footer">This page refreshes automatically every five seconds. Media callback details are currently being captured to identify RemoteFalcon selections. Pushover messages are not being sent yet.</p>
</div>
