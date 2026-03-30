#ifndef R2MOSCANNER_H
#define R2MOSCANNER_H

#include <QString>
#include <QStringList>
#include <QList>
#include <QDateTime>

struct R2moSubProject {
    QString name;
    QString path;
    QString parentPath;
    bool isParent;
    int taskQueueCount;
    int historicalTaskCount;
    QList<QString> historicalTasks;  // Task titles
};

struct TaskInfo {
    QString title;
    QString filePath;
    QDateTime modifiedTime;
    QDateTime runAtTime;  // From frontmatter runAt field
    QString fileName;     // Original filename (e.g., task-001.md)
};

class R2moScanner
{
public:
    R2moScanner();
    
    // Scan a vault for .r2mo directories
    QList<R2moSubProject> scanVault(const QString& vaultPath);
    
    // Get task queue count from .r2mo/task/
    int getTaskQueueCount(const QString& r2moPath);
    
    // Get historical tasks (md files with title attribute)
    QList<TaskInfo> getHistoricalTasks(const QString& r2moPath);
    
    // Get task queue files (task-NNN.md pattern)
    QList<TaskInfo> getTaskQueueFiles(const QString& r2moPath);
    
    // Check if path has .r2mo
    bool hasR2mo(const QString& path);
    
    // Get .r2mo path for a vault
    QString getR2moPath(const QString& vaultPath);

private:
    QString extractTitleFromMd(const QString& filePath);
    QDateTime extractRunAtFromMd(const QString& filePath);
    QDateTime getFileModifiedTime(const QString& filePath);
};

#endif // R2MOSCANNER_H