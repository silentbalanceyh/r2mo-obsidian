#ifndef AITOOLSCANNER_H
#define AITOOLSCANNER_H

#include <QString>
#include <QStringList>
#include <QList>
#include <QDir>
#include <QDateTime>
#include <QSet>

struct AIToolItem {
    QString name;
    QString path;
    QString type;  // "session", "skill", "rule", "mcp"
    QDateTime modifiedTime;
    QString displayText;
    bool isGlobal;
};

struct AIToolCategory {
    QString name;  // "Sessions", "Skills", "Rules", "MCP"
    QString icon;
    int count;
    QList<AIToolItem> items;
};

struct AIToolInfo {
    QString name;
    QString configDir;
    QString iconPath;
    bool exists;
    AIToolCategory sessions;
    AIToolCategory skills;
    AIToolCategory rules;
    AIToolCategory mcp;
};

class AIToolScanner
{
public:
    AIToolScanner();
    
    QList<AIToolInfo> scanProject(const QString& projectPath);

private:
    // Tool-specific scanners
    AIToolInfo scanClaude(const QString& projectPath, const QString& globalConfigDir);
    AIToolInfo scanCodex(const QString& projectPath, const QString& globalConfigDir);
    AIToolInfo scanOpenCode(const QString& projectPath, const QString& globalConfigDir);
    AIToolInfo scanGemini(const QString& projectPath, const QString& globalConfigDir);
    AIToolInfo scanCursor(const QString& projectPath, const QString& globalConfigDir);
    AIToolInfo scanTrae(const QString& projectPath, const QString& globalConfigDir);
    
    // Category scanners
    AIToolCategory scanSessions(const QString& projectPath, const QString& globalConfigDir, const QString& toolName);
    AIToolCategory scanSkills(const QString& configPath, const QString& globalConfigDir);
    AIToolCategory scanRules(const QString& configPath, const QString& projectPath, const QString& globalConfigDir);

    // Session helpers
    QString normalizeProjectPath(const QString& projectPath) const;
    QString toDashedProjectDir(const QString& normalizedPath, bool leadingDash) const;
    QStringList buildSessionProjectDirCandidates(const QString& projectPath, const QString& toolName) const;
    bool hasSessionData(const QString& projectPath, const QString& globalConfigDir, const QString& toolName) const;
    QList<AIToolItem> collectSessionFiles(const QString& projectDirPath, QSet<QString>& seenPaths) const;
    QList<AIToolItem> collectCodexSqliteSessions(const QString& projectPath, const QString& globalConfigDir, QSet<QString>& seenPaths) const;
    QList<AIToolItem> collectCursorWorkspaceSessions(const QString& projectPath, QSet<QString>& seenPaths) const;
    QList<AIToolItem> collectTraeWorkspaceSessions(const QString& projectPath, QSet<QString>& seenPaths) const;
    QList<AIToolItem> collectOpenCodeSessions(const QString& projectPath, QSet<QString>& seenPaths) const;
    QList<AIToolItem> collectWorkspaceStorageSessions(const QString& workspaceRoot,
                                                     const QString& projectPath,
                                                     const QString& toolPrefix,
                                                     QSet<QString>& seenPaths) const;
    AIToolCategory scanOpenCodeMCP(const QString& projectPath);
    
    // MCP scanners
    AIToolCategory scanMCPFromJson(const QString& jsonPath);
    AIToolCategory scanMCPFromToml(const QString& tomlPath);
    
    // Utilities
    QString getIconPath(const QString& toolName);
    QList<AIToolItem> scanDirectory(const QString& dirPath, const QStringList& filters, 
                                     const QString& type, bool isGlobal = false);
};

#endif // AITOOLSCANNER_H
