<?php
// FPP Pushover Notifications - settings and event dashboard.
if (!isset($pluginName) || $pluginName !== 'FPPPushoverNotifications') {
    echo '<div class="alert alert-danger">Invalid plugin page context.</div>';
    return;
}

$pluginVersion = '0.2.0';
$settingKeys = array('RemoteFalconEnabled', 'ShowStartEnabled', 'ShowEndEnabled', 'MainShowPlaylist');
$defaults = array(
    'RemoteFalconEnabled' => '1',
    'ShowStartEnabled' => '1',
    'ShowEndEnabled' => '1',
    'MainShowPlaylist' => 'Light Show'
);
$saveMessage = '';
$saveError = '';

if ($_SERVER['REQUEST_METHOD'] === 'POST' && isset($_POST['saveNotificationSettings'])) {
    $values = array(
        'RemoteFalconEnabled' => isset($_POST['RemoteFalconEnabled']) ? '1' : '0',
        'ShowStartEnabled' => isset($_POST['ShowStartEnabled']) ? '1' : '0',
        'ShowEndEnabled' => isset($_POST['ShowEndEnabled']) ? '1' : '0',
        'MainShowPlaylist' => isset($_POST['MainShowPlaylist']) ? trim($_POST['MainShowPlaylist']) : 'Light Show'
    );
    if ($values['MainShowPlaylist'] === '' || strlen($values['MainShowPlaylist']) > 128 ||
        preg_match('/[\r\n=]/', $values['MainShowPlaylist'])) {
        $saveError = 'Enter a valid main show playlist name (1–128 characters).';
    } else {
        $saved = true;
        foreach ($values as $key => $value) {
            if (!WriteSettingToFile($key, $value, $pluginName)) $saved = false;
        }
        if ($saved) $saveMessage = 'Notification settings saved.';
        else $saveError = 'FPP could not save the settings. Check the FPP configuration file permissions.';
    }
}

$settings = array();
foreach ($settingKeys as $key) {
    $value = ReadSettingFromFile($key, $pluginName);
    $settings[$key] = ($value === false) ? $defaults[$key] : $value;
}
$logFile = rtrim($logDirectory, '/') . '/plugin-FPPPushoverNotifications.log';
$logExists = is_file($logFile);
$logReadable = $logExists && is_readable($logFile);
$allLines = ($logReadable) ? @file($logFile, FILE_IGNORE_NEW_LINES | FILE_SKIP_EMPTY_LINES) : array();
if (!is_array($allLines)) $allLines = array();

$history = array();
$songCount = 0;
$showEventCount = 0;
$lastSong = null;
$lastShowStart = null;
$lastShowEnd = null;
foreach ($allLines as $line) {
    $type = '';
    if (strpos($line, 'REMOTE FALCON SONG SELECTED') !== false) {
        $type = 'Song selected';
        $songCount++;
        $lastSong = $line;
    } elseif (strpos($line, 'SHOW STARTED') !== false) {
        $type = 'Show started';
        $showEventCount++;
        $lastShowStart = $line;
    } elseif (strpos($line, 'SHOW ENDED') !== false) {
        $type = 'Show ended';
        $showEventCount++;
        $lastShowEnd = $line;
    }
    if ($type !== '') $history[] = array('type' => $type, 'line' => $line);
}
$history = array_slice(array_reverse($history), 0, 20);
$fppVersion = function_exists('getFPPVersion') ? getFPPVersion() : 'Unknown';

function pushover_h($value) {
    return htmlspecialchars((string)$value, ENT_QUOTES, 'UTF-8');
}
function pushover_checked($value) {
    return ($value === '1' || strtolower((string)$value) === 'true') ? ' checked' : '';
}
function pushover_event_detail($line) {
    $parts = explode(' ', $line, 2);
    return isset($parts[1]) ? $parts[1] : $line;
}
?>

<style>
#pushover-page { --po-text:#202938; --po-muted:#697586; --po-border:#e2e7ee; color:var(--po-text); max-width:1180px; margin:0 auto; padding:8px 4px 28px; }
#pushover-page * { box-sizing:border-box; }
#pushover-page .po-hero { display:flex; align-items:center; justify-content:space-between; gap:20px; padding:25px 28px; margin:8px 0 22px; border-radius:12px; color:#fff; background:linear-gradient(120deg,#183b63,#2878c7); box-shadow:0 5px 18px rgba(23,52,85,.16); }
#pushover-page .po-hero h2 { margin:0 0 7px; color:#fff; font-size:25px; font-weight:650; }
#pushover-page .po-hero p { margin:0; color:rgba(255,255,255,.84); font-size:14px; }
#pushover-page .po-version { flex:0 0 auto; padding:7px 12px; border:1px solid rgba(255,255,255,.35); border-radius:20px; color:#fff; font-size:12px; font-weight:700; background:rgba(255,255,255,.12); }
#pushover-page .po-section { margin:25px 0 12px; font-size:17px; font-weight:700; }
#pushover-page .po-grid { display:grid; grid-template-columns:repeat(3,minmax(0,1fr)); gap:14px; }
#pushover-page .po-card { min-width:0; padding:18px 19px; background:#fff; border:1px solid var(--po-border); border-radius:10px; box-shadow:0 2px 7px rgba(20,35,55,.045); }
#pushover-page .po-label { margin-bottom:9px; color:var(--po-muted); font-size:11px; font-weight:750; letter-spacing:.07em; text-transform:uppercase; }
#pushover-page .po-number { font-size:30px; line-height:1.1; font-weight:750; }
#pushover-page .po-note { margin-top:7px; color:var(--po-muted); font-size:12px; line-height:1.5; }
#pushover-page .po-status { display:inline-flex; align-items:center; gap:7px; padding:5px 9px; border-radius:20px; font-size:12px; font-weight:700; }
#pushover-page .po-status:before { content:""; width:7px; height:7px; display:inline-block; border-radius:50%; background:currentColor; }
#pushover-page .po-ok { color:#187447; background:#e8f5ee; }
#pushover-page .po-warn { color:#996313; background:#fff4dc; }
#pushover-page .po-settings { display:grid; gap:0; }
#pushover-page .po-setting { display:flex; justify-content:space-between; align-items:center; gap:20px; padding:17px 0; border-bottom:1px solid #edf0f4; }
#pushover-page .po-setting:last-child { border-bottom:0; }
#pushover-page .po-setting-title { font-size:14px; font-weight:700; }
#pushover-page .po-setting-desc { margin-top:4px; color:var(--po-muted); font-size:12px; line-height:1.45; }
#pushover-page .po-toggle { position:relative; display:inline-block; flex:0 0 auto; width:48px; height:27px; }
#pushover-page .po-toggle input { position:absolute; opacity:0; width:1px; height:1px; }
#pushover-page .po-toggle span { position:absolute; inset:0; cursor:pointer; border-radius:20px; background:#c6ced8; transition:.18s; }
#pushover-page .po-toggle span:before { content:""; position:absolute; width:21px; height:21px; left:3px; top:3px; border-radius:50%; background:white; box-shadow:0 1px 3px #0003; transition:.18s; }
#pushover-page .po-toggle input:checked + span { background:#238653; }
#pushover-page .po-toggle input:checked + span:before { transform:translateX(21px); }
#pushover-page .po-toggle input:focus-visible + span { outline:3px solid #75b7f1; outline-offset:2px; }
#pushover-page .po-input { width:100%; max-width:430px; padding:10px 12px; border:1px solid #cbd4df; border-radius:7px; font-size:14px; }
#pushover-page .po-save { margin-top:18px; padding:10px 18px; border:0; border-radius:7px; background:#2878c7; color:#fff; font-weight:700; cursor:pointer; }
#pushover-page .po-save:hover { background:#1d639f; }
#pushover-page .po-alert { margin:12px 0; padding:11px 14px; border-radius:7px; font-size:13px; }
#pushover-page .po-success { background:#e8f5ee; color:#187447; }
#pushover-page .po-error { background:#fdecec; color:#a83232; }
#pushover-page .po-history { overflow:hidden; background:#fff; border:1px solid var(--po-border); border-radius:10px; }
#pushover-page .po-history-row { display:flex; justify-content:space-between; align-items:flex-start; gap:16px; padding:14px 17px; border-bottom:1px solid #edf0f4; }
#pushover-page .po-history-row:last-child { border-bottom:0; }
#pushover-page .po-history-type { font-size:13px; font-weight:700; }
#pushover-page .po-history-detail { margin-top:4px; color:var(--po-muted); font-size:12px; overflow-wrap:anywhere; }
#pushover-page .po-time { flex:0 0 auto; color:var(--po-muted); font-size:12px; white-space:nowrap; }
#pushover-page .po-empty { padding:24px; color:var(--po-muted); font-size:13px; text-align:center; }
#pushover-page .po-footer { margin-top:12px; color:var(--po-muted); font-size:12px; }
@media(max-width:850px) { #pushover-page .po-grid { grid-template-columns:repeat(2,minmax(0,1fr)); } }
@media(max-width:560px) { #pushover-page .po-hero { align-items:flex-start; flex-direction:column; padding:20px; } #pushover-page .po-grid { grid-template-columns:1fr; } #pushover-page .po-setting { gap:12px; } }
</style>

<div id="pushover-page">
    <?php if ($_SERVER['REQUEST_METHOD'] !== 'POST'): ?><meta http-equiv="refresh" content="10"><?php endif; ?>
    <div class="po-hero">
        <div><h2>Pushover Notifications</h2><p>FPP Notification Center</p></div>
        <span class="po-version">Version <?php echo pushover_h($pluginVersion); ?></span>
    </div>

    <?php if ($saveMessage !== ''): ?><div class="po-alert po-success"><?php echo pushover_h($saveMessage); ?></div><?php endif; ?>
    <?php if ($saveError !== ''): ?><div class="po-alert po-error"><?php echo pushover_h($saveError); ?></div><?php endif; ?>

    <div class="po-grid">
        <div class="po-card"><div class="po-label">Plugin status</div><span class="po-status po-ok">Plugin Active</span><div class="po-note">Native FPP C++ event monitor</div></div>
        <div class="po-card"><div class="po-label">Songs selected</div><div class="po-number"><?php echo number_format($songCount); ?></div><div class="po-note">RemoteFalcon selections only</div></div>
        <div class="po-card"><div class="po-label">Show events</div><div class="po-number"><?php echo number_format($showEventCount); ?></div><div class="po-note">Main show starts and ends</div></div>
    </div>

    <div class="po-section">Notification Settings</div>
    <div class="po-card">
        <form method="post" action="">
            <div class="po-settings">
                <div class="po-setting">
                    <div><div class="po-setting-title">RemoteFalcon song selection</div><div class="po-setting-desc">Notify when someone selects a song from your RemoteFalcon playlist. Other playlists will be ignored.</div></div>
                    <label class="po-toggle"><input type="checkbox" name="RemoteFalconEnabled" value="1"<?php echo pushover_checked($settings['RemoteFalconEnabled']); ?>><span></span></label>
                </div>
                <div class="po-setting">
                    <div><div class="po-setting-title">Show starts</div><div class="po-setting-desc">Notify when your main Christmas light show begins.</div></div>
                    <label class="po-toggle"><input type="checkbox" name="ShowStartEnabled" value="1"<?php echo pushover_checked($settings['ShowStartEnabled']); ?>><span></span></label>
                </div>
                <div class="po-setting">
                    <div><div class="po-setting-title">Show ends</div><div class="po-setting-desc">Notify when your main Christmas light show finishes.</div></div>
                    <label class="po-toggle"><input type="checkbox" name="ShowEndEnabled" value="1"<?php echo pushover_checked($settings['ShowEndEnabled']); ?>><span></span></label>
                </div>
            </div>
            <div style="margin-top:18px">
                <label for="mainShowPlaylist" class="po-setting-title">Main show playlist name</label>
                <div class="po-setting-desc">Only this playlist will count as your show starting or ending.</div>
                <input class="po-input" id="mainShowPlaylist" name="MainShowPlaylist" maxlength="128" value="<?php echo pushover_h($settings['MainShowPlaylist']); ?>">
            </div>
            <button class="po-save" type="submit" name="saveNotificationSettings" value="1">Save Settings</button>
        </form>
    </div>

    <div class="po-section">Notification History</div>
    <div class="po-history">
        <?php if (empty($history)): ?>
            <div class="po-empty">No tracked events yet. Events will appear here as FPP detects them.</div>
        <?php else: foreach ($history as $event):
            $line = $event['line'];
            $timestamp = substr($line, 0, 19);
            $detail = preg_replace('/^\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2} /', '', $line);
            $detail = preg_replace('/^(REMOTE FALCON SONG SELECTED|SHOW STARTED|SHOW ENDED)\s*\|?\s*/', '', $detail);
            $time = strtotime($timestamp);
        ?>
            <div class="po-history-row">
                <div><div class="po-history-type"><?php echo pushover_h($event['type']); ?></div><div class="po-history-detail"><?php echo pushover_h($detail); ?></div></div>
                <div class="po-time"><?php echo $time ? date('g:i A', $time) : pushover_h($timestamp); ?></div>
            </div>
        <?php endforeach; endif; ?>
    </div>

    <div class="po-footer">Dashboard refreshes every 10 seconds. Event tracking is local to this FPP player. Pushover delivery and credential setup are the next milestone; this version records events but does not send phone notifications yet.</div>
</div>
