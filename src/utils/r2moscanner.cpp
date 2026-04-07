#include "r2moscanner.h"
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QFileInfo>

R2moScanner::R2moScanner()
{
}

bool R2moScanner::hasR2mo(const QString& path)
{
    return QDir(path + "/.r2mo").exists();
}

QString R2moScanner::getR2moPath(const QString& vaultPath)
{
    return vaultPath + "/.r2mo";
}

QList<R2moSubProject> R2moScanner::scanVault(const QString& vaultPath)
{
    QList<R2moSubProject> projects;
    
    // Determine the actual vault path and r2mo path
    QString actualVaultPath = vaultPath;
    QString r2moPath;
    
    QDir checkDir(vaultPath);
    if (checkDir.dirName() == ".r2mo") {
        // Path is the .r2mo directory itself
        r2moPath = vaultPath;
        actualVaultPath = QFileInfo(vaultPath).path();  // Parent directory
    } else {
        r2moPath = getR2moPath(vaultPath);
    }
    
    if (!QDir(r2moPath).exists()) {
        return projects;
    }
    
    // Add main project
    R2moSubProject mainProject;
    mainProject.name = QFileInfo(actualVaultPath).fileName();
    mainProject.path = actualVaultPath;
    mainProject.parentPath = QString();
    mainProject.isParent = true;
    mainProject.taskQueueCount = getTaskQueueCount(r2moPath);
    
    QList<TaskInfo> tasks = getHistoricalTasks(r2moPath);
    mainProject.historicalTaskCount = tasks.size();
    for (const TaskInfo& task : tasks) {
        mainProject.historicalTasks.append(task.title);
    }
    
    projects.append(mainProject);
    
    // Scan for sub-projects (directories containing .r2mo)
    QDir vaultDir(actualVaultPath);
    vaultDir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden);
    
    QStringList subDirs = vaultDir.entryList();
    for (const QString& subDir : subDirs) {
        QString subPath = actualVaultPath + "/" + subDir;
        QString subR2moPath = subPath + "/.r2mo";
        
        if (QDir(subR2moPath).exists()) {
            R2moSubProject subProject;
            subProject.name = subDir;
            subProject.path = subPath;
            subProject.parentPath = actualVaultPath;
            subProject.isParent = false;
            subProject.taskQueueCount = getTaskQueueCount(subR2moPath);
            
            QList<TaskInfo> subTasks = getHistoricalTasks(subR2moPath);
            subProject.historicalTaskCount = subTasks.size();
            for (const TaskInfo& task : subTasks) {
                subProject.historicalTasks.append(task.title);
            }
            
            projects.append(subProject);
        }
    }
    
    return projects;
}

int R2moScanner::getTaskQueueCount(const QString& r2moPath)
{
    QString taskPath = r2moPath + "/task";
    QDir taskDir(taskPath);
    
    if (!taskDir.exists()) {
        return 0;
    }
    
    taskDir.setFilter(QDir::Files | QDir::NoDotAndDotDot);
    QStringList filters;
    filters << "task-*.md";
    taskDir.setNameFilters(filters);
    
    return taskDir.entryList().size();
}

QList<TaskInfo> R2moScanner::getHistoricalTasks(const QString& r2moPath)
{
    QList<TaskInfo> tasks;
    QRegularExpression dateDirRegex("^\\d{4}-\\d{2}-\\d{2}$");

    auto scanDateDirectories = [&](const QString& basePath) {
        QDir baseDir(basePath);
        if (!baseDir.exists()) {
            return;
        }

        baseDir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);
        const QStringList dateDirs = baseDir.entryList();

        for (const QString& dirName : dateDirs) {
            if (!dateDirRegex.match(dirName).hasMatch()) {
                continue;
            }

            const QString dateDirPath = basePath + "/" + dirName;
            QDir dateDir(dateDirPath);
            dateDir.setFilter(QDir::Files | QDir::NoDotAndDotDot);
            QStringList mdFilters;
            mdFilters << "*.md";
            dateDir.setNameFilters(mdFilters);

            const QStringList mdFiles = dateDir.entryList();
            for (const QString& fileName : mdFiles) {
                const QString filePath = dateDirPath + "/" + fileName;
                const QString fmTitle = extractFrontmatterTitle(filePath);
                if (fmTitle.isEmpty()) {
                    continue;  // Only count files with YAML frontmatter title
                }

                TaskInfo task;
                task.title = fmTitle;
                task.filePath = filePath;
                task.fileName = fileName;
                task.modifiedTime = getFileModifiedTime(filePath);
                task.runAtTime = extractRunAtFromMd(filePath);
                tasks.append(task);
            }
        }
    };

    // Historical sources:
    // 1) .r2mo/task/YYYY-MM-DD/*.md
    // 2) .r2mo/history/YYYY-MM-DD/*.md
    scanDateDirectories(r2moPath + "/task");
    scanDateDirectories(r2moPath + "/history");
    
    // Sort by runAt time (if available) or modified time (newest first)
    std::sort(tasks.begin(), tasks.end(), [](const TaskInfo& a, const TaskInfo& b) {
        if (a.runAtTime.isValid() && b.runAtTime.isValid()) {
            return a.runAtTime > b.runAtTime;
        }
        if (a.runAtTime.isValid()) return true;
        if (b.runAtTime.isValid()) return false;
        return a.modifiedTime > b.modifiedTime;
    });
    
    return tasks;
}

QString R2moScanner::extractTitleFromMd(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }
    
    QTextStream in(&file);
    QString content = in.readAll();
    file.close();
    
    // Try to extract title from frontmatter
    QRegularExpression frontmatterRegex("^---\\s*\n(.*?)\\n---", 
                                         QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatch match = frontmatterRegex.match(content);
    
    if (match.hasMatch()) {
        QString frontmatter = match.captured(1);
        
        // Look for title: or title:
        QRegularExpression titleRegex("title:\\s*['\"]?(.+?)['\"]?\\s*$", 
                                       QRegularExpression::MultilineOption);
        QRegularExpressionMatch titleMatch = titleRegex.match(frontmatter);
        
        if (titleMatch.hasMatch()) {
            return titleMatch.captured(1).trimmed();
        }
    }
    
    // Fallback: try to find # heading
    QRegularExpression headingRegex("^#\\s+(.+)$", QRegularExpression::MultilineOption);
    QRegularExpressionMatch headingMatch = headingRegex.match(content);
    
    if (headingMatch.hasMatch()) {
        return headingMatch.captured(1).trimmed();
    }
    
    // Last resort: use filename without extension
    QFileInfo fi(filePath);
    return fi.baseName();
}

QString R2moScanner::extractFrontmatterTitle(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }
    
    QTextStream in(&file);
    QString content = in.readAll();
    file.close();
    
    QRegularExpression frontmatterRegex("^---\\s*\n(.*?)\\n---", 
                                         QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatch match = frontmatterRegex.match(content);
    
    if (match.hasMatch()) {
        QString frontmatter = match.captured(1);
        QRegularExpression titleRegex("title:\\s*['\"]?(.+?)['\"]?\\s*$", 
                                       QRegularExpression::MultilineOption);
        QRegularExpressionMatch titleMatch = titleRegex.match(frontmatter);
        if (titleMatch.hasMatch()) {
            return titleMatch.captured(1).trimmed();
        }
    }
    return QString();
}

QDateTime R2moScanner::getFileModifiedTime(const QString& filePath)
{
    QFileInfo fi(filePath);
    return fi.lastModified();
}

QList<TaskInfo> R2moScanner::getTaskQueueFiles(const QString& r2moPath)
{
    QList<TaskInfo> tasks;
    QString taskPath = r2moPath + "/task";
    QDir taskDir(taskPath);
    
    if (!taskDir.exists()) {
        return tasks;
    }
    
    taskDir.setFilter(QDir::Files | QDir::NoDotAndDotDot);
    QStringList filters;
    filters << "*.md";
    taskDir.setNameFilters(filters);
    
    QStringList taskFiles = taskDir.entryList();
    for (const QString& fileName : taskFiles) {
        QString filePath = taskPath + "/" + fileName;
        QString fmTitle = extractFrontmatterTitle(filePath);
        if (fmTitle.isEmpty()) {
            continue;  // Only count files with yaml frontmatter title
        }
        QDateTime runAt = extractRunAtFromMd(filePath);
        
        TaskInfo task;
        task.title = fmTitle;
        task.filePath = filePath;
        task.fileName = fileName;
        task.modifiedTime = getFileModifiedTime(filePath);
        task.runAtTime = runAt;
        tasks.append(task);
    }
    
    // Sort by file number (task-001, task-002, etc.)
    std::sort(tasks.begin(), tasks.end(), [](const TaskInfo& a, const TaskInfo& b) {
        // Extract number from filename (task-NNN.md)
        QRegularExpression numRegex("task-(\\d+)\\.md");
        QRegularExpressionMatch matchA = numRegex.match(a.fileName);
        QRegularExpressionMatch matchB = numRegex.match(b.fileName);
        
        if (matchA.hasMatch() && matchB.hasMatch()) {
            int numA = matchA.captured(1).toInt();
            int numB = matchB.captured(1).toInt();
            return numA < numB;  // Ascending order by number
        }
        return a.fileName < b.fileName;  // Fallback to alphabetical
    });
    
    return tasks;
}

QDateTime R2moScanner::extractRunAtFromMd(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QDateTime();
    }
    
    QTextStream in(&file);
    QString content = in.readAll();
    file.close();
    
    // Try to extract runAt from frontmatter
    QRegularExpression frontmatterRegex("^---\\s*\n(.*?)\\n---", 
                                         QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatch match = frontmatterRegex.match(content);
    
    if (match.hasMatch()) {
        QString frontmatter = match.captured(1);
        
        // Look for runAt: field (format: YYYY-MM-DD.HH-MM-SS)
        QRegularExpression runAtRegex("runAt:\\s*['\"]?(\\d{4}-\\d{2}-\\d{2}\\.\\d{2}-\\d{2}-\\d{2})['\"]?\\s*$", 
                                       QRegularExpression::MultilineOption);
        QRegularExpressionMatch runAtMatch = runAtRegex.match(frontmatter);
        
        if (runAtMatch.hasMatch()) {
            QString runAtStr = runAtMatch.captured(1);
            // Parse format: YYYY-MM-DD.HH-MM-SS
            QRegularExpression dateRegex("(\\d{4})-(\\d{2})-(\\d{2})\\.(\\d{2})-(\\d{2})-(\\d{2})");
            QRegularExpressionMatch dateMatch = dateRegex.match(runAtStr);
            
            if (dateMatch.hasMatch()) {
                int year = dateMatch.captured(1).toInt();
                int month = dateMatch.captured(2).toInt();
                int day = dateMatch.captured(3).toInt();
                int hour = dateMatch.captured(4).toInt();
                int min = dateMatch.captured(5).toInt();
                int sec = dateMatch.captured(6).toInt();
                
                return QDateTime(QDate(year, month, day), QTime(hour, min, sec));
            }
        }
    }
    
    return QDateTime();
}
