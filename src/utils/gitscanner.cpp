#include "gitscanner.h"
#include <QProcess>
#include <QDir>
#include <QFileInfo>

GitScanner::GitScanner()
{
}

GitStatusInfo GitScanner::scanRepository(const QString& repoPath)
{
    GitStatusInfo info;
    info.isGitRepo = false;
    info.branch = "";
    info.aheadCount = 0;
    info.behindCount = 0;
    info.stagedCount = 0;
    info.modifiedCount = 0;
    info.untrackedCount = 0;
    info.hasConflicts = false;
    
    QString actualRepoPath = repoPath;
    QDir dir(repoPath);
    if (!dir.exists()) {
        return info;
    }
    
    QString gitDir = repoPath + "/.git";
    if (!QDir(gitDir).exists()) {
        QDir checkDir(repoPath);
        while (checkDir.cdUp()) {
            if (QDir(checkDir.path() + "/.git").exists()) {
                actualRepoPath = checkDir.path();
                break;
            }
            if (checkDir.isRoot()) {
                break;
            }
        }
        if (actualRepoPath == repoPath) {
            return info;
        }
    }
    
    info.isGitRepo = true;
    
    QString branchOutput = runGitCommandSimple(actualRepoPath, {"symbolic-ref", "--short", "HEAD"});
    if (branchOutput.isEmpty()) {
        branchOutput = runGitCommandSimple(actualRepoPath, {"rev-parse", "--short", "HEAD"});
        if (!branchOutput.isEmpty()) {
            info.branch = "HEAD:" + branchOutput.left(7);
        } else {
            info.branch = "NO_BRANCH";
        }
    } else {
        info.branch = branchOutput;
    }
    
    QString aheadBehind = runGitCommandSimple(actualRepoPath, {"rev-list", "--left-right", "--count", "@{upstream}...HEAD"});
    if (!aheadBehind.isEmpty()) {
        QStringList parts = aheadBehind.split('\t');
        if (parts.size() == 2) {
            info.behindCount = parts[0].toInt();
            info.aheadCount = parts[1].toInt();
        }
    }
    
    QString statusOutput;
    if (runGitCommand(actualRepoPath, {"status", "--porcelain"}, statusOutput)) {
        QStringList lines = statusOutput.split('\n', Qt::SkipEmptyParts);
        for (const QString& line : lines) {
            if (line.length() < 2) continue;
            QString indexStatus = line.left(1);
            QString workTreeStatus = line.mid(1, 1);
            
            if (indexStatus == "U" || workTreeStatus == "U" || 
                indexStatus == "A" && workTreeStatus == "A" ||
                indexStatus == "D" && workTreeStatus == "D") {
                info.hasConflicts = true;
            }
            
            if (indexStatus == "M" || indexStatus == "A" || indexStatus == "D" || indexStatus == "R" || indexStatus == "C") {
                info.stagedCount++;
            }
            
            if (workTreeStatus == "M" || workTreeStatus == "D") {
                info.modifiedCount++;
            }
            
            if (indexStatus == "?" && workTreeStatus == "?") {
                info.untrackedCount++;
            }
        }
    }
    
    return info;
}

bool GitScanner::runGitCommand(const QString& repoPath, const QStringList& args, QString& output)
{
    QProcess process;
    process.setWorkingDirectory(repoPath);
    process.start("git", args);
    
    if (!process.waitForFinished(10000)) {
        process.kill();
        return false;
    }
    
    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        return false;
    }
    
    output = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
    return true;
}

QString GitScanner::runGitCommandSimple(const QString& repoPath, const QStringList& args)
{
    QString output;
    runGitCommand(repoPath, args, output);
    return output;
}

QString GitStatusInfo::statusColor() const
{
    if (!isGitRepo) {
        return "#86868b"; // Gray - not a git repo
    }
    
    // Green if all changes are committed (staged=0, modified=0, untracked=0) and synced (behind=0)
    // ahead > 0 is OK (committed but not pushed yet)
    bool hasUncommitted = (stagedCount > 0 || modifiedCount > 0 || untrackedCount > 0);
    bool needsPull = (behindCount > 0);
    
    if (!hasUncommitted && !needsPull) {
        return "#34c759"; // Green - all committed and synced
    }
    
    return "#ff3b30"; // Red - has uncommitted changes or needs pull
}

QString GitStatusInfo::statusText() const
{
    if (!isGitRepo) {
        return "";
    }
    
    QString text = branch;
    
    if (aheadCount > 0) {
        text += QString(" ↑%1").arg(aheadCount);
    }
    if (behindCount > 0) {
        text += QString(" ↓%1").arg(behindCount);
    }
    if (stagedCount > 0) {
        text += QString(" !%1").arg(stagedCount);
    }
    if (modifiedCount > 0) {
        text += QString(" ✗%1").arg(modifiedCount);
    }
    if (untrackedCount > 0) {
        text += QString(" ?%1").arg(untrackedCount);
    }
    
    return text;
}