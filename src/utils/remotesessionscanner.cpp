#include "remotesessionscanner.h"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace {
QString shellQuote(const QString& value)
{
    QString escaped = value;
    escaped.replace('\'', QStringLiteral("'\"'\"'"));
    return QStringLiteral("'%1'").arg(escaped);
}

SessionStatus statusFromString(const QString& value)
{
    if (value.compare(QStringLiteral("working"), Qt::CaseInsensitive) == 0) {
        return SessionStatus::Working;
    }
    if (value.compare(QStringLiteral("ready"), Qt::CaseInsensitive) == 0) {
        return SessionStatus::Ready;
    }
    return SessionStatus::Unknown;
}

bool hasKnownRemoteSessionId(const QString& sessionId)
{
    return !sessionId.isEmpty() && sessionId != QStringLiteral("unknown");
}

bool isBetterRemoteSessionCandidate(const SessionInfo& candidate, const SessionInfo& current)
{
    if (hasKnownRemoteSessionId(candidate.sessionId) != hasKnownRemoteSessionId(current.sessionId)) {
        return hasKnownRemoteSessionId(candidate.sessionId);
    }
    if (candidate.sessionPath.isEmpty() != current.sessionPath.isEmpty()) {
        return !candidate.sessionPath.isEmpty();
    }
    if (candidate.toolName == QStringLiteral("Codex")) {
        const bool candidateIsVendor = candidate.detailText.contains("/vendor/");
        const bool currentIsVendor = current.detailText.contains("/vendor/");
        if (candidateIsVendor != currentIsVendor) {
            return candidateIsVendor;
        }
    }
    if (candidate.status != current.status) {
        return candidate.status == SessionStatus::Working;
    }
    if (candidate.lastActivity.isValid() != current.lastActivity.isValid()) {
        return candidate.lastActivity.isValid();
    }
    if (candidate.lastActivity.isValid() && current.lastActivity.isValid() &&
        candidate.lastActivity != current.lastActivity) {
        return candidate.lastActivity > current.lastActivity;
    }
    return candidate.processPid < current.processPid;
}

QString remoteProbeCommand(const QString& projectPath)
{
    return QStringLiteral(R"PYTHON(python3 - %1 <<'PY'
import glob
import json
import os
import re
import sys
import time

project_path = sys.argv[1]
now = time.time()

def normalize_path(path):
    if not path:
        return ''
    try:
        return os.path.realpath(path)
    except Exception:
        return path

def path_belongs_to_project(candidate, project):
    normalized_candidate = normalize_path(candidate)
    normalized_project = normalize_path(project)
    if not normalized_candidate or not normalized_project:
        return False
    return normalized_candidate == normalized_project or normalized_candidate.startswith(normalized_project + os.sep)

def read(path):
    try:
        with open(path, 'r', errors='replace') as f:
            return f.read()
    except Exception:
        return ''

def readlink(path):
    try:
        return os.path.realpath(os.readlink(path))
    except Exception:
        return ''

def process_start_time(pid):
    try:
        stat = read(f'/proc/{pid}/stat')
        if not stat:
            return 0.0
        parts = stat.split()
        ticks = int(parts[21])
        hz = os.sysconf(os.sysconf_names['SC_CLK_TCK'])
        boot = 0.0
        for line in read('/proc/stat').splitlines():
            if line.startswith('btime '):
                boot = float(line.split()[1])
                break
        return boot + (ticks / hz)
    except Exception:
        return 0.0

def process_parent_pid(pid):
    try:
        stat = read(f'/proc/{pid}/stat')
        if not stat:
            return 0
        parts = stat.split()
        return int(parts[3])
    except Exception:
        return 0

def tool_from_cmd(cmd):
    lower = cmd.lower()
    if 'codex' in lower:
        return 'Codex'
    if re.search(r'(^|/|\s)claude(\s|$)', lower) or '/claude/' in lower:
        return 'Claude'
    if 'opencode' in lower:
        return 'OpenCode'
    return ''

def latest_codex_artifact(project):
    best = None
    for path in glob.glob(os.path.expanduser('~/.codex/sessions/**/*.jsonl'), recursive=True):
        try:
            with open(path, 'r', errors='replace') as f:
                first = f.readline()
            obj = json.loads(first)
            payload = obj.get('payload') or {}
            cwd = normalize_path(payload.get('cwd') or '')
            if not path_belongs_to_project(cwd, project):
                continue
            mtime = os.path.getmtime(path)
            if best is None or mtime > best['mtime']:
                best = {
                    'sessionId': payload.get('id') or os.path.basename(path).replace('rollout-', '').replace('.jsonl', ''),
                    'sessionPath': path,
                    'mtime': mtime,
                }
        except Exception:
            continue
    return best

def latest_opencode_artifact(project):
    best = None
    for path in glob.glob(os.path.join(project, '.omc', 'sessions', '*.json')):
        try:
            with open(path, 'r', errors='replace') as f:
                obj = json.load(f)
            mtime = os.path.getmtime(path)
            session_id = (
                obj.get('session_id')
                or obj.get('sessionId')
                or obj.get('id')
                or os.path.basename(path).replace('.json', '')
            )
            if not session_id:
                continue
            if best is None or mtime > best['mtime']:
                best = {
                    'sessionId': session_id,
                    'sessionPath': path,
                    'mtime': mtime,
                    'endedAt': obj.get('ended_at') or obj.get('endedAt') or '',
                    'reason': obj.get('reason') or '',
                }
        except Exception:
            continue
    return best

def latest_opencode_notification(session_id):
    if not session_id or session_id == 'unknown':
        return None
    path = os.path.expanduser('~/Library/Application Support/ai.opencode.desktop/opencode.global.dat')
    try:
        with open(path, 'r', errors='replace') as f:
            obj = json.load(f)
    except Exception:
        return None
    notification_text = obj.get('notification')
    if not notification_text:
        return None
    try:
        notification_obj = json.loads(notification_text)
    except Exception:
        return None
    entries = notification_obj.get('list') or []
    for entry in reversed(entries):
        if not isinstance(entry, dict):
            continue
        if entry.get('session') != session_id:
            continue
        return entry
    return None

def notification_time_is_fresh(notification):
    open_code_active_fresh_seconds = 8.0
    notification_time = notification.get('time')
    if not notification_time:
        return False
    try:
        return (now * 1000.0) - float(notification_time) <= (open_code_active_fresh_seconds * 1000.0)
    except Exception:
        return False

def event_time_is_fresh(obj, active_fresh_seconds=30.0):
    raw = obj.get('timestamp') or obj.get('time') or obj.get('created_at') or ''
    if not raw:
        return now - os.path.getmtime(obj.get('__path', '')) <= active_fresh_seconds if obj.get('__path') else False
    try:
        if isinstance(raw, (int, float)):
            timestamp = float(raw)
            if timestamp > 100000000000:
                timestamp = timestamp / 1000.0
            return now - timestamp <= active_fresh_seconds
        value = str(raw).replace('Z', '+00:00')
        parsed = __import__('datetime').datetime.fromisoformat(value)
        return now - parsed.timestamp() <= active_fresh_seconds
    except Exception:
        return False

def claude_project_dir_candidates(project):
    normalized = normalize_path(project).strip('/')
    if not normalized:
        return []

    dashed = normalized.replace('/', '-')
    sanitized = re.sub(r'[^A-Za-z0-9_-]+', '-', dashed)
    sanitized = re.sub(r'-+', '-', sanitized).strip('-')

    candidates = []
    for name in (dashed, sanitized):
        if not name:
            continue
        for variant in (name, '-' + name):
            if variant not in candidates:
                candidates.append(variant)
    return candidates

def collect_claude_artifacts(project):
    candidates = [normalize_path(project)]
    try:
        for entry in os.listdir(project):
            child = normalize_path(os.path.join(project, entry))
            if child and os.path.isdir(child):
                candidates.append(child)
    except Exception:
        pass

    artifacts = []
    seen = set()
    for candidate in candidates:
        for project_dir in claude_project_dir_candidates(candidate):
            artifact_dir = os.path.expanduser('~/.claude/projects/' + project_dir)
            for path in glob.glob(artifact_dir + '/*.jsonl'):
                try:
                    mtime = os.path.getmtime(path)
                    session_id = os.path.basename(path).replace('.jsonl', '')
                    if session_id in seen:
                        continue
                    seen.add(session_id)
                    artifacts.append({
                        'sessionId': session_id,
                        'sessionPath': path,
                        'mtime': mtime,
                    })
                except Exception:
                    continue
    artifacts.sort(key=lambda item: item.get('mtime', 0), reverse=True)
    return artifacts

def artifact_status(path, tool):
    if not path:
        return 'ready'
    if tool == 'OpenCode':
        try:
            with open(path, 'r', errors='replace') as f:
                obj = json.load(f)
        except Exception:
            return 'unknown'
        if obj.get('ended_at') or obj.get('endedAt') or obj.get('reason'):
            return 'ready'
        return 'working' if now - os.path.getmtime(path) <= 10 else 'unknown'
    try:
        with open(path, 'rb') as f:
            f.seek(0, os.SEEK_END)
            size = f.tell()
            f.seek(max(0, size - 131072), os.SEEK_SET)
            lines = f.read().decode('utf-8', errors='replace').splitlines()
    except Exception:
        return 'unknown'
    for line in reversed(lines[-96:]):
        try:
            obj = json.loads(line)
        except Exception:
            continue
        typ = obj.get('type', '')
        obj['__path'] = path
        event_is_fresh = event_time_is_fresh(obj)
        if tool == 'Codex':
            if typ == 'task_complete':
                return 'ready'
            if typ == 'task_started':
                return 'working' if event_is_fresh else 'ready'
            payload = obj.get('payload') or {}
            payload_type = payload.get('type', '')
            if payload_type in ('exec_command_begin', 'function_call_begin', 'patch_apply_begin', 'agent_reasoning'):
                return 'working' if event_is_fresh else 'ready'
            if payload_type in ('exec_command_end', 'function_call_output', 'patch_apply_end'):
                return 'working' if event_is_fresh else 'ready'
            if payload_type in ('user_message', 'message'):
                return 'ready'
        if tool == 'Claude':
            if typ == 'assistant':
                stop_reason = (obj.get('message') or {}).get('stop_reason', '')
                if stop_reason == 'tool_use':
                    return 'working' if event_is_fresh else 'ready'
                if stop_reason == 'end_turn':
                    return 'ready'
            attachment = obj.get('attachment') or {}
            hook = attachment.get('hookEvent', '')
            if hook == 'PreToolUse':
                return 'working' if event_is_fresh else 'ready'
            if hook == 'PostToolUse':
                return 'ready'
            if hook == 'Stop' or attachment.get('type') in ('hook_success', 'async_hook_response'):
                return 'ready'
            data = obj.get('data') or {}
            progress_hook = data.get('hookEvent', '')
            if progress_hook == 'PreToolUse':
                return 'working' if event_is_fresh else 'ready'
            if progress_hook in ('PostToolUse', 'Stop'):
                return 'ready'
            if typ == 'system' and obj.get('subtype') in ('turn_duration', 'away_summary'):
                return 'ready'
    return 'ready'

def first_status_scalar(value):
    if isinstance(value, str):
        return value.strip().lower()
    if isinstance(value, dict):
        for key in ('status', 'state', 'phase', 'value'):
            nested = first_status_scalar(value.get(key))
            if nested:
                return nested
    if isinstance(value, list):
        for item in value:
            nested = first_status_scalar(item)
            if nested:
                return nested
    return ''

def is_active_status_word(value):
    return value in ('working', 'running', 'busy', 'streaming', 'thinking', 'generating', 'pending')

def is_ready_status_word(value):
    return value in ('ready', 'idle', 'paused', 'complete', 'completed', 'done', 'error', 'failed')

def opencode_status(session_id, path):
    notification = latest_opencode_notification(session_id)
    if notification:
        notification_type = notification.get('type', '')
        if notification_type == 'turn-start':
            return 'working' if notification_time_is_fresh(notification) else 'ready'
        if notification_type in ('turn-complete', 'error'):
            return 'ready'
        if notification_type == 'session.status':
            status_value = first_status_scalar(notification.get('status'))
            if is_active_status_word(status_value):
                return 'working' if notification_time_is_fresh(notification) else 'ready'
            if is_ready_status_word(status_value) or status_value or notification.get('paused'):
                return 'ready'
            return 'unknown' if notification_time_is_fresh(notification) else 'ready'
    return artifact_status(path, 'OpenCode')

def has_tool_ancestor(pid):
    ancestor = process_parent_pid(pid)
    visited = set()
    while ancestor > 1 and ancestor not in visited:
        visited.add(ancestor)
        parent_cmd = read(f'/proc/{ancestor}/cmdline').replace('\x00', ' ').strip() or read(f'/proc/{ancestor}/comm').strip()
        if tool_from_cmd(parent_cmd):
            return True
        ancestor = process_parent_pid(ancestor)
    return False

rows = []
artifact_cache = {}
for pid in filter(str.isdigit, os.listdir('/proc')):
    cmdline = read(f'/proc/{pid}/cmdline').replace('\x00', ' ').strip()
    comm = read(f'/proc/{pid}/comm').strip()
    cmd = cmdline or comm
    tool = tool_from_cmd(cmd)
    if not tool:
        continue
    cwd = readlink(f'/proc/{pid}/cwd')
    if not path_belongs_to_project(cwd, project_path):
        continue
    if tool == 'Codex' and 'claude-mem' in cmd:
        continue
    if tool == 'Claude' and 'claude-mem' in cmd:
        continue
    if has_tool_ancestor(int(pid)):
        continue
    artifact_key = (tool, cwd)
    artifact = artifact_cache.get(artifact_key)
    if artifact is None:
        artifact = (
            latest_codex_artifact(cwd) if tool == 'Codex' else
            None if tool == 'Claude' else
            latest_opencode_artifact(cwd)
        )
        artifact_cache[artifact_key] = artifact
    started = process_start_time(pid)
    mtime = artifact.get('mtime') if artifact else started
    rows.append({
        'toolName': tool,
        'sessionId': (artifact or {}).get('sessionId') or 'unknown',
        'projectPath': cwd,
        'processPid': int(pid),
        'shellPid': 0,
        'terminalPid': 0,
        'terminalName': 'SSH',
        'sessionPath': (artifact or {}).get('sessionPath') or '',
        'detailText': cmd,
        'status': opencode_status((artifact or {}).get('sessionId') or 'unknown',
                                  (artifact or {}).get('sessionPath') or '') if tool == 'OpenCode'
                  else artifact_status((artifact or {}).get('sessionPath') or '', tool),
        'lastActivityEpoch': mtime,
        'processStartedEpoch': started,
        'runtimeSeconds': int(max(0, now - (started or now))),
    })

rows_by_project_tool = {}
for row in rows:
    rows_by_project_tool.setdefault((normalize_path(row.get('projectPath')), row.get('toolName')), []).append(row)

for (project, tool), grouped_rows in rows_by_project_tool.items():
    if tool == 'Claude':
        artifacts = collect_claude_artifacts(project)
        if tool == 'Claude' and artifacts:
            grouped_rows.sort(key=lambda row: (row.get('processStartedEpoch', 0), row.get('processPid', 0)))
            assign_count = min(len(grouped_rows), len(artifacts))
            for index in range(assign_count):
                grouped_rows[index]['sessionId'] = artifacts[index].get('sessionId') or grouped_rows[index].get('sessionId') or 'unknown'
                grouped_rows[index]['sessionPath'] = artifacts[index].get('sessionPath') or grouped_rows[index].get('sessionPath') or ''
                grouped_rows[index]['lastActivityEpoch'] = artifacts[index].get('mtime') or grouped_rows[index].get('lastActivityEpoch') or 0
                grouped_rows[index]['status'] = artifact_status(grouped_rows[index].get('sessionPath') or '', 'Claude')

print(json.dumps(rows, ensure_ascii=False))
PY)PYTHON").arg(shellQuote(projectPath));
}
}

RemoteSessionScanner::RemoteSessionScanner() = default;

QList<ProjectMonitorData> RemoteSessionScanner::scanRemoteVault(const Vault& vault) const
{
    QList<ProjectMonitorData> result;
    if (!vault.isRemote() || vault.connectionStatus != VaultConnectionStatus::Connected) {
        return result;
    }

    const QString command = remoteProbeCommand(vault.remotePath);
    const SshRemoteCommandResult probe = m_executor.runCommand(
        vault.host,
        vault.username,
        vault.password,
        vault.useKeyAuth,
        command,
        15000);
    if (!probe.success || probe.standardOutput.trimmed().isEmpty()) {
        return result;
    }

    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(probe.standardOutput.toUtf8(), &error);
    if (error.error != QJsonParseError::NoError || !doc.isArray()) {
        return result;
    }

    ProjectMonitorData data;
    data.projectName = vault.name;
    data.projectPath = vault.path;
    data.totalWorking = 0;
    data.totalReady = 0;

    const QJsonArray rows = doc.array();
    QMap<QString, SessionInfo> groupedSessions;
    for (const QJsonValue& value : rows) {
        const QJsonObject object = value.toObject();
        const QString toolName = object.value(QStringLiteral("toolName")).toString();
        const QString sessionId = object.value(QStringLiteral("sessionId")).toString(QStringLiteral("unknown"));
        if (toolName.isEmpty()) {
            continue;
        }

        SessionInfo session;
        session.sessionId = sessionId;
        session.toolName = toolName;
        session.toolIconPath = SessionScanner::toolIconPath(toolName);
        session.projectName = vault.name;
        session.projectPath = vault.path;
        session.terminalName = object.value(QStringLiteral("terminalName")).toString(QStringLiteral("SSH"));
        session.terminalIconPath = SessionScanner::terminalIconPath(session.terminalName);
        session.terminalPid = static_cast<qint64>(object.value(QStringLiteral("terminalPid")).toDouble());
        session.shellPid = static_cast<qint64>(object.value(QStringLiteral("shellPid")).toDouble());
        session.processPid = static_cast<qint64>(object.value(QStringLiteral("processPid")).toDouble());
        session.status = statusFromString(object.value(QStringLiteral("status")).toString());
        session.lastActivity = QDateTime::fromSecsSinceEpoch(
            static_cast<qint64>(object.value(QStringLiteral("lastActivityEpoch")).toDouble()));
        session.processStartedAt = QDateTime::fromSecsSinceEpoch(
            static_cast<qint64>(object.value(QStringLiteral("processStartedEpoch")).toDouble()));
        session.runtimeSeconds = static_cast<qint64>(object.value(QStringLiteral("runtimeSeconds")).toDouble());
        session.sessionPath = object.value(QStringLiteral("sessionPath")).toString();
        session.detailText = object.value(QStringLiteral("detailText")).toString();

        const QString dedupeSessionId = hasKnownRemoteSessionId(session.sessionId)
            ? session.sessionId
            : (!session.sessionPath.isEmpty()
                ? session.sessionPath
                : QStringLiteral("pid-%1").arg(session.processPid));
        const QString key = QStringLiteral("%1|%2|%3|%4")
            .arg(vault.path, session.toolName, QString::number(session.processPid), dedupeSessionId);
        if (!groupedSessions.contains(key)) {
            groupedSessions.insert(key, session);
            continue;
        }

        const SessionInfo current = groupedSessions.value(key);
        if (isBetterRemoteSessionCandidate(session, current)) {
            groupedSessions.insert(key, session);
        }
    }

    for (auto it = groupedSessions.cbegin(); it != groupedSessions.cend(); ++it) {
        const SessionInfo& session = it.value();
        if (session.status == SessionStatus::Working) {
            ++data.totalWorking;
        } else if (session.status == SessionStatus::Ready) {
            ++data.totalReady;
        }
        data.sessions.append(session);
    }

    if (!data.sessions.isEmpty()) {
        result.append(data);
    }
    return result;
}
