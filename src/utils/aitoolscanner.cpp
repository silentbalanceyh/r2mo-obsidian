#include "aitoolscanner.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDirIterator>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QSettings>
#include <QProcess>
#include <QStandardPaths>
#include <QTextStream>
#include <QUrl>

namespace {
QStringList runSqliteQuery(const QString& dbPath, const QString& sql)
{
    QProcess proc;
    proc.start("sqlite3", QStringList() << dbPath << sql);
    if (!proc.waitForFinished(5000)) {
        proc.kill();
        return {};
    }

    if (proc.exitStatus() != QProcess::NormalExit || proc.exitCode() != 0) {
        return {};
    }

    const QString output = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
    if (output.isEmpty()) {
        return {};
    }

    return output.split('\n', Qt::SkipEmptyParts);
}

QString sqlQuote(const QString& value)
{
    QString escaped = value;
    escaped.replace("'", "''");
    return QString("'%1'").arg(escaped);
}
}

AIToolScanner::AIToolScanner()
{
}

QList<AIToolInfo> AIToolScanner::scanProject(const QString& projectPath)
{
    QList<AIToolInfo> tools;
    
    QString homeDir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    
    // Check Claude
    {
        QString projectConfigPath = projectPath + "/.claude";
        bool hasProjectConfig = QDir(projectConfigPath).exists();
        bool hasGlobalProjectInfo = hasSessionData(projectPath, homeDir + "/.claude", "Claude");
        
        if (hasProjectConfig || hasGlobalProjectInfo) {
            AIToolInfo info = scanClaude(projectPath, homeDir + "/.claude");
            tools.append(info);
        }
    }
    
    // Check Codex
    {
        QString projectConfigPath = projectPath + "/.codex";
        bool hasProjectConfig = QDir(projectConfigPath).exists();
        bool hasGlobalProjectInfo = hasSessionData(projectPath, homeDir + "/.codex", "Codex");
        
        if (hasProjectConfig || hasGlobalProjectInfo) {
            AIToolInfo info = scanCodex(projectPath, homeDir + "/.codex");
            tools.append(info);
        }
    }
    
    // Check OpenCode
    {
        QString projectConfigPath = projectPath + "/.opencode";
        bool hasProjectConfig = QDir(projectConfigPath).exists();
        bool hasGlobalProjectInfo = hasSessionData(projectPath, homeDir + "/.opencode", "OpenCode");
        
        if (hasProjectConfig || hasGlobalProjectInfo) {
            AIToolInfo info = scanOpenCode(projectPath, homeDir + "/.opencode");
            tools.append(info);
        }
    }
    
    // Check Gemini
    {
        QString projectConfigPath = projectPath + "/.gemini";
        bool hasProjectConfig = QDir(projectConfigPath).exists();
        bool hasGlobalProjectInfo = hasSessionData(projectPath, homeDir + "/.gemini", "Gemini");
        
        if (hasProjectConfig || hasGlobalProjectInfo) {
            AIToolInfo info = scanGemini(projectPath, homeDir + "/.gemini");
            tools.append(info);
        }
    }
    
    // Check Cursor
    {
        QString projectConfigPath = projectPath + "/.cursor";
        bool hasProjectConfig = QDir(projectConfigPath).exists();
        bool hasGlobalProjectInfo = hasSessionData(projectPath, homeDir + "/.cursor", "Cursor");
        
        if (hasProjectConfig || hasGlobalProjectInfo) {
            AIToolInfo info = scanCursor(projectPath, homeDir + "/.cursor");
            tools.append(info);
        }
    }
    
    // Check Trae
    {
        QString projectConfigPath = projectPath + "/.trae";
        bool hasProjectConfig = QDir(projectConfigPath).exists();
        bool hasGlobalProjectInfo = hasSessionData(projectPath, homeDir + "/.trae", "Trae");
        
        if (hasProjectConfig || hasGlobalProjectInfo) {
            AIToolInfo info = scanTrae(projectPath, homeDir + "/.trae");
            tools.append(info);
        }
    }
    
    return tools;
}

AIToolInfo AIToolScanner::scanClaude(const QString& projectPath, const QString& globalConfigDir)
{
    AIToolInfo info;
    info.name = "Claude";
    info.configDir = ".claude";
    info.iconPath = getIconPath("Claude");
    info.exists = true;
    
    QString configPath = projectPath + "/.claude";
    
    // Sessions from global projects directory
    info.sessions = scanSessions(projectPath, globalConfigDir, "Claude");
    
    // Skills from project-level + global
    info.skills = scanSkills(configPath, globalConfigDir);
    
    // Rules from project-level + global
    info.rules = scanRules(configPath, projectPath, globalConfigDir);
    
    // MCP from project-level .mcp.json
    info.mcp = scanMCPFromJson(projectPath + "/.mcp.json");
    
    return info;
}

AIToolInfo AIToolScanner::scanCodex(const QString& projectPath, const QString& globalConfigDir)
{
    AIToolInfo info;
    info.name = "Codex";
    info.configDir = ".codex";
    info.iconPath = getIconPath("Codex");
    info.exists = true;
    
    QString configPath = projectPath + "/.codex";
    
    // Sessions from global projects directory
    info.sessions = scanSessions(projectPath, globalConfigDir, "Codex");
    
    // Skills from project-level + global
    info.skills = scanSkills(configPath, globalConfigDir);
    
    // Rules from project-level + global
    info.rules = scanRules(configPath, projectPath, globalConfigDir);
    
    // MCP from global config.toml [mcp_servers.*] section
    info.mcp = scanMCPFromToml(globalConfigDir + "/config.toml");
    
    return info;
}

AIToolInfo AIToolScanner::scanOpenCode(const QString& projectPath, const QString& globalConfigDir)
{
    AIToolInfo info;
    info.name = "OpenCode";
    info.configDir = ".opencode";
    info.iconPath = getIconPath("OpenCode");
    info.exists = true;
    
    QString configPath = projectPath + "/.opencode";
    
    // Sessions from global projects directory
    info.sessions = scanSessions(projectPath, globalConfigDir, "OpenCode");
    
    // Skills from project-level + global
    info.skills = scanSkills(configPath, globalConfigDir);
    
    // Rules from project-level + global
    info.rules = scanRules(configPath, projectPath, globalConfigDir);
    
    // MCP from project/global OpenCode config files
    Q_UNUSED(globalConfigDir);
    info.mcp = scanOpenCodeMCP(projectPath);
    
    return info;
}

AIToolInfo AIToolScanner::scanGemini(const QString& projectPath, const QString& globalConfigDir)
{
    AIToolInfo info;
    info.name = "Gemini";
    info.configDir = ".gemini";
    info.iconPath = getIconPath("Gemini");
    info.exists = true;
    
    QString configPath = projectPath + "/.gemini";
    
    // Sessions from global projects directory
    info.sessions = scanSessions(projectPath, globalConfigDir, "Gemini");
    
    // Skills from project-level + global
    info.skills = scanSkills(configPath, globalConfigDir);
    
    // Rules from project-level + global
    info.rules = scanRules(configPath, projectPath, globalConfigDir);
    
    // MCP
    info.mcp = scanMCPFromJson(configPath + "/settings.json");
    
    return info;
}

AIToolInfo AIToolScanner::scanCursor(const QString& projectPath, const QString& globalConfigDir)
{
    AIToolInfo info;
    info.name = "Cursor";
    info.configDir = ".cursor";
    info.iconPath = getIconPath("Cursor");
    info.exists = true;
    
    QString configPath = projectPath + "/.cursor";
    
    // Sessions from global projects directory
    info.sessions = scanSessions(projectPath, globalConfigDir, "Cursor");
    
    // Skills from project-level + global
    info.skills = scanSkills(configPath, globalConfigDir);
    
    // Rules from project-level + global
    info.rules = scanRules(configPath, projectPath, globalConfigDir);
    
    // MCP from project-level
    info.mcp = scanMCPFromJson(configPath + "/mcp.json");
    
    return info;
}

AIToolInfo AIToolScanner::scanTrae(const QString& projectPath, const QString& globalConfigDir)
{
    AIToolInfo info;
    info.name = "Trae";
    info.configDir = ".trae";
    info.iconPath = getIconPath("Trae");
    info.exists = true;
    
    QString configPath = projectPath + "/.trae";
    
    // Sessions from global projects directory
    info.sessions = scanSessions(projectPath, globalConfigDir, "Trae");
    
    // Skills from project-level + global
    info.skills = scanSkills(configPath, globalConfigDir);
    
    // Rules from project-level + global
    info.rules = scanRules(configPath, projectPath, globalConfigDir);
    
    // MCP
    info.mcp = scanMCPFromJson(configPath + "/mcp.json");
    
    return info;
}

AIToolCategory AIToolScanner::scanSessions(const QString& projectPath, const QString& globalConfigDir, const QString& toolName)
{
    AIToolCategory category;
    category.name = "Sessions";
    category.icon = "💬";
    category.count = 0;
    
    QList<AIToolItem> allSessions;
    QSet<QString> seenSessionPaths;

    const QStringList projectDirCandidates = buildSessionProjectDirCandidates(projectPath, toolName);
    for (const QString& projectDirName : projectDirCandidates) {
        const QString globalProjectPath = globalConfigDir + "/projects/" + projectDirName;
        allSessions.append(collectSessionFiles(globalProjectPath, seenSessionPaths));
    }

    // Tool-specific fallback sources
    if (toolName == "Codex") {
        allSessions.append(collectCodexSqliteSessions(projectPath, globalConfigDir, seenSessionPaths));
    } else if (toolName == "Cursor") {
        allSessions.append(collectCursorWorkspaceSessions(projectPath, seenSessionPaths));
    } else if (toolName == "Trae") {
        allSessions.append(collectTraeWorkspaceSessions(projectPath, seenSessionPaths));
    } else if (toolName == "OpenCode") {
        allSessions.append(collectOpenCodeSessions(projectPath, seenSessionPaths));
    }
    
    category.items = allSessions;
    category.count = allSessions.size();
    
    return category;
}

QString AIToolScanner::normalizeProjectPath(const QString& projectPath) const
{
    QString normalizedPath = projectPath;
    while (normalizedPath.endsWith('/')) {
        normalizedPath.chop(1);
    }
    return normalizedPath;
}

QString AIToolScanner::toDashedProjectDir(const QString& normalizedPath, bool leadingDash) const
{
    QString dirName = normalizedPath;
    dirName.replace("/", "-");
    return leadingDash ? ("-" + dirName) : dirName;
}

QStringList AIToolScanner::buildSessionProjectDirCandidates(const QString& projectPath, const QString& toolName) const
{
    const QString normalizedPath = normalizeProjectPath(projectPath);

    // 兼容不同工具以及历史格式：
    // - Claude/Codex 常见为带前缀 '-'，但也兼容无前缀
    // - 其他工具常见为无前缀，但也兼容带前缀
    QStringList candidates;
    if (toolName == "Claude" || toolName == "Codex") {
        candidates << toDashedProjectDir(normalizedPath, true)
                   << toDashedProjectDir(normalizedPath, false);
    } else {
        candidates << toDashedProjectDir(normalizedPath, false)
                   << toDashedProjectDir(normalizedPath, true);
    }

    candidates.removeDuplicates();
    return candidates;
}

bool AIToolScanner::hasSessionData(const QString& projectPath, const QString& globalConfigDir, const QString& toolName) const
{
    const QStringList candidates = buildSessionProjectDirCandidates(projectPath, toolName);
    for (const QString& projectDirName : candidates) {
        const QString globalProjectPath = globalConfigDir + "/projects/" + projectDirName;
        QSet<QString> seen;
        if (!collectSessionFiles(globalProjectPath, seen).isEmpty()) {
            return true;
        }
    }

    // Tool-specific fallback sources
    {
        QSet<QString> seen;
        if (toolName == "Codex") {
            if (!collectCodexSqliteSessions(projectPath, globalConfigDir, seen).isEmpty()) {
                return true;
            }
        } else if (toolName == "Cursor") {
            if (!collectCursorWorkspaceSessions(projectPath, seen).isEmpty()) {
                return true;
            }
        } else if (toolName == "Trae") {
            if (!collectTraeWorkspaceSessions(projectPath, seen).isEmpty()) {
                return true;
            }
        } else if (toolName == "OpenCode") {
            if (!collectOpenCodeSessions(projectPath, seen).isEmpty()) {
                return true;
            }
        }
    }

    return false;
}

QList<AIToolItem> AIToolScanner::collectSessionFiles(const QString& projectDirPath, QSet<QString>& seenPaths) const
{
    QList<AIToolItem> items;
    const QDir projectDir(projectDirPath);
    if (!projectDir.exists()) {
        return items;
    }

    QDirIterator it(projectDirPath,
                    QStringList() << "*.json" << "*.jsonl",
                    QDir::Files | QDir::NoDotAndDotDot,
                    QDirIterator::Subdirectories);

    while (it.hasNext()) {
        const QString filePath = it.next();
        if (seenPaths.contains(filePath)) {
            continue;
        }

        seenPaths.insert(filePath);
        const QFileInfo fi(filePath);

        AIToolItem item;
        item.name = fi.fileName();
        item.path = filePath;
        item.type = "session";
        item.isGlobal = false;
        item.modifiedTime = fi.lastModified();
        item.displayText = fi.fileName();
        items.append(item);
    }

    return items;
}

QList<AIToolItem> AIToolScanner::collectCodexSqliteSessions(const QString& projectPath,
                                                            const QString& globalConfigDir,
                                                            QSet<QString>& seenPaths) const
{
    QList<AIToolItem> items;
    const QString dbPath = globalConfigDir + "/state_5.sqlite";
    if (!QFile::exists(dbPath)) {
        return items;
    }

    const QString sql = QString(
        "SELECT id || '\\t' || COALESCE(updated_at, 0) "
        "FROM threads WHERE cwd = %1 ORDER BY updated_at DESC")
                            .arg(sqlQuote(projectPath));

    const QStringList rows = runSqliteQuery(dbPath, sql);
    const QFileInfo dbInfo(dbPath);
    for (const QString& row : rows) {
        const QStringList parts = row.split('\t');
        if (parts.isEmpty()) {
            continue;
        }

        const QString sessionId = parts.value(0).trimmed();
        if (sessionId.isEmpty()) {
            continue;
        }

        const QString stableKey = QString("codex-sqlite:%1").arg(sessionId);
        if (seenPaths.contains(stableKey)) {
            continue;
        }
        seenPaths.insert(stableKey);

        const qint64 updatedAt = parts.value(1).toLongLong();

        AIToolItem item;
        item.name = sessionId;
        item.path = dbPath;
        item.type = "session";
        item.isGlobal = false;
        item.modifiedTime = updatedAt > 0 ? QDateTime::fromSecsSinceEpoch(updatedAt) : dbInfo.lastModified();
        item.displayText = sessionId;
        items.append(item);
    }

    return items;
}

QList<AIToolItem> AIToolScanner::collectCursorWorkspaceSessions(const QString& projectPath,
                                                                QSet<QString>& seenPaths) const
{
    QList<AIToolItem> items;

    const QString homeDir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);

    // 1) Cursor transcript files under ~/.cursor/projects/<project>
    const QString projectsRoot = homeDir + "/.cursor/projects/" + toDashedProjectDir(normalizeProjectPath(projectPath), false);
    if (QDir(projectsRoot).exists()) {
        QDirIterator it(projectsRoot,
                        QStringList() << "*.jsonl",
                        QDir::Files | QDir::NoDotAndDotDot,
                        QDirIterator::Subdirectories);

        while (it.hasNext()) {
            const QString filePath = it.next();
            const QString stableKey = QString("cursor:%1").arg(filePath);
            if (seenPaths.contains(stableKey)) {
                continue;
            }

            seenPaths.insert(stableKey);
            QFileInfo fi(filePath);

            AIToolItem item;
            item.name = fi.fileName();
            item.path = filePath;
            item.type = "session";
            item.isGlobal = false;
            item.modifiedTime = fi.lastModified();
            item.displayText = fi.fileName();
            items.append(item);
        }
    }

    // 2) Cursor workspaceStorage state.vscdb fallback
    items.append(collectWorkspaceStorageSessions(homeDir + "/Library/Application Support/Cursor/User/workspaceStorage",
                                                 projectPath,
                                                 "cursor-ws",
                                                 seenPaths));

    return items;
}

QList<AIToolItem> AIToolScanner::collectTraeWorkspaceSessions(const QString& projectPath,
                                                              QSet<QString>& seenPaths) const
{
    QList<AIToolItem> items;
    const QString homeDir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);

    // 1) Trae transcript files under ~/.trae/projects/<project> (if exists)
    const QString projectsRoot = homeDir + "/.trae/projects/" + toDashedProjectDir(normalizeProjectPath(projectPath), false);
    if (QDir(projectsRoot).exists()) {
        QDirIterator it(projectsRoot,
                        QStringList() << "*.jsonl",
                        QDir::Files | QDir::NoDotAndDotDot,
                        QDirIterator::Subdirectories);

        while (it.hasNext()) {
            const QString filePath = it.next();
            const QString stableKey = QString("trae:%1").arg(filePath);
            if (seenPaths.contains(stableKey)) {
                continue;
            }

            seenPaths.insert(stableKey);
            QFileInfo fi(filePath);

            AIToolItem item;
            item.name = fi.fileName();
            item.path = filePath;
            item.type = "session";
            item.isGlobal = false;
            item.modifiedTime = fi.lastModified();
            item.displayText = fi.fileName();
            items.append(item);
        }
    }

    // 2) Trae workspaceStorage state.vscdb fallback
    items.append(collectWorkspaceStorageSessions(homeDir + "/Library/Application Support/Trae/User/workspaceStorage",
                                                 projectPath,
                                                 "trae-ws",
                                                 seenPaths));

    return items;
}

QList<AIToolItem> AIToolScanner::collectOpenCodeSessions(const QString& projectPath,
                                                         QSet<QString>& seenPaths) const
{
    QList<AIToolItem> items;

    const QString appDataRoot = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    Q_UNUSED(appDataRoot);

    const QString homeDir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    const QString globalDatPath = homeDir + "/Library/Application Support/ai.opencode.desktop/opencode.global.dat";
    if (!QFile::exists(globalDatPath)) {
        return items;
    }

    QFile file(globalDatPath);
    if (!file.open(QIODevice::ReadOnly)) {
        return items;
    }

    QJsonParseError parseError;
    const QJsonDocument rootDoc = QJsonDocument::fromJson(file.readAll(), &parseError);
    file.close();
    if (parseError.error != QJsonParseError::NoError || !rootDoc.isObject()) {
        return items;
    }

    const QJsonObject rootObj = rootDoc.object();
    const QString globalProjectRaw = rootObj.value("globalSync.project").toString();
    if (globalProjectRaw.isEmpty()) {
        return items;
    }

    QJsonParseError projectParseError;
    const QJsonDocument projectDoc = QJsonDocument::fromJson(globalProjectRaw.toUtf8(), &projectParseError);
    if (projectParseError.error != QJsonParseError::NoError || !projectDoc.isObject()) {
        return items;
    }

    const QJsonArray projectArray = projectDoc.object().value("value").toArray();
    QString matchedProjectId;
    QString matchedWorktree;
    for (const QJsonValue& value : projectArray) {
        const QJsonObject projectObj = value.toObject();
        if (projectObj.value("worktree").toString() == projectPath) {
            matchedProjectId = projectObj.value("id").toString();
            matchedWorktree = projectObj.value("worktree").toString();
            break;
        }
    }

    if (matchedProjectId.isEmpty()) {
        // fallback: allow nearest ancestor worktree mapping
        int bestLen = -1;
        for (const QJsonValue& value : projectArray) {
            const QJsonObject projectObj = value.toObject();
            const QString worktree = projectObj.value("worktree").toString();
            if (worktree.isEmpty()) {
                continue;
            }
            if (projectPath == worktree || projectPath.startsWith(worktree + "/")) {
                if (worktree.length() > bestLen) {
                    bestLen = worktree.length();
                    matchedProjectId = projectObj.value("id").toString();
                    matchedWorktree = worktree;
                }
            }
        }
    }

    if (matchedProjectId.isEmpty()) {
        return items;
    }

    // OpenCode session records in current env are stored by workspace .dat files without direct project id mapping.
    // Use mapped worktree/project entry as stable session container fallback.
    const QString stableKey = QString("opencode:%1").arg(matchedProjectId);
    if (!seenPaths.contains(stableKey)) {
        seenPaths.insert(stableKey);
        AIToolItem item;
        item.name = matchedWorktree.isEmpty() ? matchedProjectId : matchedWorktree;
        item.path = globalDatPath;
        item.type = "session";
        item.isGlobal = false;
        item.modifiedTime = QFileInfo(globalDatPath).lastModified();
        item.displayText = matchedWorktree.isEmpty()
                               ? QString("mapped:%1").arg(matchedProjectId.left(12))
                               : QString("mapped worktree: %1").arg(matchedWorktree);
        items.append(item);
    }

    return items;
}

QList<AIToolItem> AIToolScanner::collectWorkspaceStorageSessions(const QString& workspaceRoot,
                                                                 const QString& projectPath,
                                                                 const QString& toolPrefix,
                                                                 QSet<QString>& seenPaths) const
{
    QList<AIToolItem> items;
    const QDir root(workspaceRoot);
    if (!root.exists()) {
        return items;
    }

    QDirIterator it(workspaceRoot,
                    QStringList() << "workspace.json",
                    QDir::Files | QDir::NoDotAndDotDot,
                    QDirIterator::Subdirectories);

    while (it.hasNext()) {
        const QString workspaceJsonPath = it.next();
        QFile wsFile(workspaceJsonPath);
        if (!wsFile.open(QIODevice::ReadOnly)) {
            continue;
        }

        QJsonParseError wsErr;
        const QJsonDocument wsDoc = QJsonDocument::fromJson(wsFile.readAll(), &wsErr);
        wsFile.close();
        if (wsErr.error != QJsonParseError::NoError || !wsDoc.isObject()) {
            continue;
        }

        QString folderPath = wsDoc.object().value("folder").toString();
        if (folderPath.startsWith("file://")) {
            folderPath = QUrl(folderPath).toLocalFile();
        }
        if (folderPath.isEmpty()) {
            continue;
        }

        const bool pathRelated = (projectPath == folderPath)
                                 || projectPath.startsWith(folderPath + "/")
                                 || folderPath.startsWith(projectPath + "/");
        if (!pathRelated) {
            continue;
        }

        const QString dbPath = QFileInfo(workspaceJsonPath).dir().filePath("state.vscdb");
        if (!QFile::exists(dbPath)) {
            continue;
        }

        QSet<QString> sessionIds;
        const QStringList rows = runSqliteQuery(
            dbPath,
            "SELECT key || '\\t' || replace(replace(CAST(value AS TEXT), char(10), ' '), char(13), ' ') FROM ItemTable");

        for (const QString& row : rows) {
            const int sep = row.indexOf('\t');
            if (sep <= 0) {
                continue;
            }

            const QString key = row.left(sep);
            const QString value = row.mid(sep + 1);

            if (key == "interactive.sessions") {
                QJsonParseError arrErr;
                const QJsonDocument arrDoc = QJsonDocument::fromJson(value.toUtf8(), &arrErr);
                if (arrErr.error == QJsonParseError::NoError && arrDoc.isArray()) {
                    const QJsonArray arr = arrDoc.array();
                    for (const QJsonValue& v : arr) {
                        const QString id = v.toString();
                        if (!id.isEmpty()) sessionIds.insert(id);
                    }
                }
            }

            const QString panePrefix = "workbench.panel.composerChatViewPane.";
            if (key.startsWith(panePrefix)) {
                const QString paneId = key.mid(panePrefix.length());
                if (!paneId.isEmpty()) sessionIds.insert(paneId);
            }

            if (key.contains("ai-chat:sessionRelation:modelMap") || key.contains("ai-chat:sessionRelation:modeMap")) {
                QJsonParseError mapErr;
                const QJsonDocument mapDoc = QJsonDocument::fromJson(value.toUtf8(), &mapErr);
                if (mapErr.error == QJsonParseError::NoError && mapDoc.isObject()) {
                    const QJsonObject mapObj = mapDoc.object();
                    for (auto it2 = mapObj.begin(); it2 != mapObj.end(); ++it2) {
                        if (!it2.key().isEmpty()) {
                            sessionIds.insert(it2.key());
                        }
                    }
                }
            }
        }

        if (sessionIds.isEmpty()) {
            // fallback: this workspace has AI panel/session state, count as one container session
            const QString fallbackId = QString("%1").arg(QFileInfo(dbPath).dir().dirName());
            sessionIds.insert(fallbackId);
        }

        const QFileInfo dbInfo(dbPath);
        for (const QString& sid : std::as_const(sessionIds)) {
            const QString stableKey = QString("%1:%2:%3").arg(toolPrefix, dbPath, sid);
            if (seenPaths.contains(stableKey)) {
                continue;
            }

            seenPaths.insert(stableKey);

            AIToolItem item;
            item.name = sid;
            item.path = dbPath;
            item.type = "session";
            item.isGlobal = false;
            item.modifiedTime = dbInfo.lastModified();
            item.displayText = QString("%1 (workspace)").arg(sid.left(24));
            items.append(item);
        }

    }

    return items;
}

AIToolCategory AIToolScanner::scanOpenCodeMCP(const QString& projectPath)
{
    AIToolCategory category;
    category.name = "MCP";
    category.icon = "🔌";
    category.count = 0;

    QList<AIToolItem> merged;
    QSet<QString> seenNames;

    const QString homeDir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    const QStringList candidates = {
        projectPath + "/.mcp.json",
        projectPath + "/.opencode/opencode.json",
        projectPath + "/.opencode/settings.json",
        homeDir + "/.opencode/opencode.json"
    };

    for (const QString& path : candidates) {
        const AIToolCategory part = scanMCPFromJson(path);
        for (const AIToolItem& item : part.items) {
            if (item.name.isEmpty() || seenNames.contains(item.name)) {
                continue;
            }
            seenNames.insert(item.name);
            merged.append(item);
        }
    }

    category.items = merged;
    category.count = merged.size();
    return category;
}

AIToolCategory AIToolScanner::scanSkills(const QString& configPath, const QString& globalConfigDir)
{
    AIToolCategory category;
    category.name = "Skills";
    category.icon = "⚡";
    category.count = 0;
    
    QList<AIToolItem> allSkills;
    QStringList filters;
    filters << "*.md" << "*.SKILL.md" << "*.json" << "*.yaml" << "*.yml";
    
    // Project-level skills
    QDir skillsDir(configPath + "/skills");
    if (skillsDir.exists()) {
        allSkills.append(scanDirectory(configPath + "/skills", filters, "skill", false));
    }
    
    // Project-level commands
    QDir commandsDir(configPath + "/commands");
    if (commandsDir.exists()) {
        allSkills.append(scanDirectory(configPath + "/commands", filters, "skill", false));
    }
    
    // Global skills
    QDir globalSkillsDir(globalConfigDir + "/skills");
    if (globalSkillsDir.exists()) {
        allSkills.append(scanDirectory(globalConfigDir + "/skills", filters, "skill", true));
    }
    
    // Global commands
    QDir globalCommandsDir(globalConfigDir + "/commands");
    if (globalCommandsDir.exists()) {
        allSkills.append(scanDirectory(globalConfigDir + "/commands", filters, "skill", true));
    }
    
    category.items = allSkills;
    category.count = allSkills.size();
    
    return category;
}

AIToolCategory AIToolScanner::scanRules(const QString& configPath, const QString& projectPath, const QString& globalConfigDir)
{
    AIToolCategory category;
    category.name = "Rules";
    category.icon = "📋";
    category.count = 0;
    
    QList<AIToolItem> allRules;
    QStringList filters;
    filters << "*.md" << "*.mdc" << "*.json" << "*.yaml" << "*.yml";
    
    // Project-level rules directory
    QDir rulesDir(configPath + "/rules");
    if (rulesDir.exists()) {
        allRules.append(scanDirectory(configPath + "/rules", filters, "rule", false));
    }
    
    // Root-level rule files (project)
    QStringList ruleFiles;
    ruleFiles << "CLAUDE.md" << "CODEX.md" << "AGENTS.md" << "CURSOR.md" << "TRAE.md" << "GEMINI.md";
    
    for (const QString& ruleFile : ruleFiles) {
        QString filePath = projectPath + "/" + ruleFile;
        if (QFile::exists(filePath)) {
            AIToolItem item;
            item.name = ruleFile;
            item.path = filePath;
            item.type = "rule";
            item.isGlobal = false;
            QFileInfo fi(filePath);
            item.modifiedTime = fi.lastModified();
            item.displayText = ruleFile;
            allRules.append(item);
        }
    }
    
    // Global rules directory
    QDir globalRulesDir(globalConfigDir + "/rules");
    if (globalRulesDir.exists()) {
        allRules.append(scanDirectory(globalConfigDir + "/rules", filters, "rule", true));
    }
    
    category.items = allRules;
    category.count = allRules.size();
    
    return category;
}

AIToolCategory AIToolScanner::scanMCPFromJson(const QString& jsonPath)
{
    AIToolCategory category;
    category.name = "MCP";
    category.icon = "🔌";
    category.count = 0;
    
    QList<AIToolItem> allMcp;
    
    if (!QFile::exists(jsonPath)) {
        category.items = allMcp;
        return category;
    }
    
    QFile file(jsonPath);
    if (!file.open(QIODevice::ReadOnly)) {
        category.items = allMcp;
        return category;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError || !doc.isObject()) {
        category.items = allMcp;
        return category;
    }
    
    QJsonObject root = doc.object();
    QJsonObject mcpServers = root["mcpServers"].toObject();
    
    for (auto it = mcpServers.begin(); it != mcpServers.end(); ++it) {
        AIToolItem item;
        item.name = it.key();
        item.path = jsonPath;
        item.type = "mcp";
        item.isGlobal = false;
        item.displayText = it.key();
        allMcp.append(item);
    }
    
    category.items = allMcp;
    category.count = allMcp.size();
    
    return category;
}

AIToolCategory AIToolScanner::scanMCPFromToml(const QString& tomlPath)
{
    AIToolCategory category;
    category.name = "MCP";
    category.icon = "🔌";
    category.count = 0;
    
    QList<AIToolItem> allMcp;
    
    if (!QFile::exists(tomlPath)) {
        category.items = allMcp;
        return category;
    }
    
    // Parse TOML file for [mcp_servers.*] sections
    QFile file(tomlPath);
    if (!file.open(QIODevice::ReadOnly)) {
        category.items = allMcp;
        return category;
    }
    
    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        
        // Look for [mcp_servers.servername] pattern
        if (line.startsWith("[mcp_servers.") && line.endsWith("]")) {
            // Extract server name
            QString serverName = line.mid(13, line.length() - 14); // Remove [mcp_servers. and ]
            
            AIToolItem item;
            item.name = serverName;
            item.path = tomlPath;
            item.type = "mcp";
            item.isGlobal = false;
            item.displayText = serverName;
            allMcp.append(item);
        }
    }
    file.close();
    
    category.items = allMcp;
    category.count = allMcp.size();
    
    return category;
}

QString AIToolScanner::getIconPath(const QString& toolName)
{
    QString iconBase = "/Users/lang/zero-cloud/app-zero/r2mo-apps/app-mom/assets/app-icons/ai-tag/png/";
    
    QMap<QString, QString> iconMap;
    iconMap["Claude"] = "Claude.png";
    iconMap["Codex"] = "Codex.png";
    iconMap["OpenCode"] = "OpenCode.png";
    iconMap["Gemini"] = "Antigravity.png";
    iconMap["Cursor"] = "Cursor.png";
    iconMap["Trae"] = "Trae.png";
    
    if (iconMap.contains(toolName)) {
        return iconBase + iconMap[toolName];
    }
    
    return QString();
}

QList<AIToolItem> AIToolScanner::scanDirectory(const QString& dirPath, const QStringList& filters, 
                                                const QString& type, bool isGlobal)
{
    QList<AIToolItem> items;
    
    QDir dir(dirPath);
    if (!dir.exists()) {
        return items;
    }
    
    dir.setFilter(QDir::Files | QDir::NoDotAndDotDot);
    dir.setNameFilters(filters);
    
    QStringList files = dir.entryList();
    for (const QString& fileName : files) {
        QString filePath = dirPath + "/" + fileName;
        QFileInfo fi(filePath);
        
        AIToolItem item;
        item.name = fileName;
        item.path = filePath;
        item.type = type;
        item.isGlobal = isGlobal;
        item.modifiedTime = fi.lastModified();
        item.displayText = fileName;
        if (isGlobal) {
            item.displayText += " (global)";
        }
        
        items.append(item);
    }
    
    return items;
}
