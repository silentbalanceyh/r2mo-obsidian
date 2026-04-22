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

def process_cpu_ticks(pid):
    try:
        stat = read(f'/proc/{pid}/stat')
        if not stat:
            return 0
        parts = stat.split()
        return int(parts[13]) + int(parts[14])
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

def latest_claude_artifact(project):
    best = None
    for path in glob.glob(os.path.expanduser('~/.claude/projects/**/*.jsonl'), recursive=True):
        try:
            with open(path, 'rb') as f:
                head = f.read(32768).decode('utf-8', errors='replace').splitlines()
            cwd = ''
            session_id = ''
            for line in head[:12]:
                try:
                    obj = json.loads(line)
                except Exception:
                    continue
                session_id = obj.get('sessionId') or obj.get('session_id') or session_id
                cwd = normalize_path(
                    obj.get('cwd')
                    or (obj.get('attachment') or {}).get('cwd')
                    or (obj.get('message') or {}).get('cwd')
                    or ''
                )
                if cwd:
                    break
            if not cwd:
                continue
            if not (path_belongs_to_project(cwd, project) or path_belongs_to_project(project, cwd)):
                continue
            mtime = os.path.getmtime(path)
            session_path = path
            if '/subagents/' in path and session_id:
                primary_path = os.path.join(os.path.dirname(os.path.dirname(path)), session_id + '.jsonl')
                if os.path.exists(primary_path):
                    session_path = primary_path
                    mtime = max(mtime, os.path.getmtime(primary_path))
            if best is None or mtime > best['mtime']:
                best = {
                    'sessionId': session_id or os.path.basename(session_path).replace('.jsonl', ''),
                    'sessionPath': session_path,
                    'mtime': mtime,
                }
        except Exception:
            continue
    return best

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
    mtime_is_fresh = (now - os.path.getmtime(path)) <= 8
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
        event_time = None
        for key in ('timestamp', 'time', 'created_at'):
            raw = obj.get(key)
            if isinstance(raw, str) and raw:
                try:
                    from datetime import datetime, timezone
                    normalized = raw.replace('Z', '+00:00')
                    event_time = datetime.fromisoformat(normalized)
                    if event_time.tzinfo is None:
                        event_time = event_time.replace(tzinfo=timezone.utc)
                    break
                except Exception:
                    event_time = None
        event_is_fresh = False
        if event_time is not None:
            event_is_fresh = (time.time() - event_time.timestamp()) <= 8
        if tool == 'Codex':
            if typ == 'task_complete':
                return 'working' if mtime_is_fresh else 'ready'
            if typ == 'task_started':
                return 'working' if event_is_fresh else 'ready'
            payload = obj.get('payload') or {}
            payload_type = payload.get('type', '')
            if payload_type in ('exec_command_begin', 'function_call_begin', 'patch_apply_begin', 'agent_reasoning'):
                return 'working' if event_is_fresh else 'ready'
            if payload_type in ('exec_command_end', 'function_call_output', 'patch_apply_end', 'user_message', 'message'):
                return 'ready'
            if typ == 'response_item':
                if payload_type in ('function_call', 'custom_tool_call', 'reasoning'):
                    return 'working' if event_is_fresh else 'ready'
                if payload_type == 'function_call_output':
                    return 'ready'
                if payload_type == 'message':
                    phase = payload.get('phase', '')
                    if not phase or phase == 'final_answer':
                        return 'ready'
                    if phase == 'commentary':
                        return 'working' if event_is_fresh else 'ready'
        if tool == 'Claude':
            if typ == 'assistant':
                stop_reason = (obj.get('message') or {}).get('stop_reason', '')
                if stop_reason == 'tool_use':
                    return 'working'
                if stop_reason == 'end_turn':
                    return 'ready'
            attachment = obj.get('attachment') or {}
            hook = attachment.get('hookEvent', '')
            if hook in ('PreToolUse', 'PostToolUse'):
                return 'working' if event_is_fresh else 'ready'
            if hook == 'Stop' or attachment.get('type') in ('hook_success', 'async_hook_response'):
                return 'ready'
            if typ == 'system' and obj.get('subtype') in ('turn_duration', 'away_summary'):
                return 'ready'
    return 'ready'

rows = []
artifact_cache = {}
cpu_samples_before = {}
candidate_pids = []
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
    candidate_pids.append((pid, tool, cwd, cmd))
    cpu_samples_before[int(pid)] = process_cpu_ticks(pid)

time.sleep(2.0)
cpu_samples_after = {int(pid): process_cpu_ticks(pid) for pid, _, _, _ in candidate_pids}

for pid, tool, cwd, cmd in candidate_pids:
    if tool == 'Codex' and 'claude-mem' in cmd:
        continue
    if tool == 'Claude' and 'claude-mem' in cmd:
        continue
    artifact_key = (tool, cwd)
    artifact = artifact_cache.get(artifact_key)
    if artifact is None:
        artifact = (
            latest_codex_artifact(cwd) if tool == 'Codex' else
            latest_claude_artifact(cwd) if tool == 'Claude' else
            latest_opencode_artifact(cwd)
        )
        artifact_cache[artifact_key] = artifact
    started = process_start_time(pid)
    mtime = artifact.get('mtime') if artifact else started
    cpu_delta = max(0, cpu_samples_after.get(int(pid), 0) - cpu_samples_before.get(int(pid), 0))
    status = artifact_status((artifact or {}).get('sessionPath') or '', tool)
    if cpu_delta > 0:
        status = 'working'
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
        'status': status,
        'lastActivityEpoch': mtime,
        'processStartedEpoch': started,
        'runtimeSeconds': int(max(0, now - (started or now))),
    })

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
        const QString key = QStringLiteral("%1|%2|%3")
            .arg(vault.path, session.toolName, dedupeSessionId);
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
