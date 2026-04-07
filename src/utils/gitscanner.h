#ifndef GITSCANNER_H
#define GITSCANNER_H

#include <QString>
#include <QStringList>

struct GitStatusInfo {
    bool isGitRepo;
    QString branch;
    int aheadCount;       // commits not pushed
    int behindCount;     // commits not fetched
    int stagedCount;     // staged changes (ready to commit)
    int modifiedCount;   // modified but not staged
    int untrackedCount;  // untracked files
    bool hasConflicts;
    
    // Status color classification
    // Green: completely synced (ahead=0, behind=0, modified=0, untracked=0)
    // Blue: has unpushed commits (ahead>0) but clean otherwise
    // Red: has local changes (modified>0 or untracked>0) OR no commits at all
    QString statusColor() const;
    QString statusText() const;
};

class GitScanner
{
public:
    GitScanner();
    
    GitStatusInfo scanRepository(const QString& repoPath);
    
private:
    bool runGitCommand(const QString& repoPath, const QStringList& args, QString& output);
    QString runGitCommandSimple(const QString& repoPath, const QStringList& args);
};

#endif // GITSCANNER_H