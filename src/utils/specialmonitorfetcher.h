#ifndef SPECIALMONITORFETCHER_H
#define SPECIALMONITORFETCHER_H

#include <QList>
#include <QString>

struct SpecialMonitorSource {
    QString providerName;
    QString baseUrl;
    QString tokenKey;
};

struct SpecialMonitorSnapshot {
    SpecialMonitorSource source;
    QString accountName;
    QString packageType;
    QString todayAddedQuota;
    QString todayUsedQuota;
    QString remainQuota;
    int todayUsageCount = 0;
    int todayOpusUsage = 0;
    int totalLogCount = 0;
    QString updatedAt;
    QString errorMessage;
};

class SpecialMonitorFetcher
{
public:
    QList<SpecialMonitorSnapshot> fetchSnapshots(const QList<SpecialMonitorSource>& sources) const;

    static QString normalizeBaseUrl(const QString& baseUrl);
    static QString defaultProviderName(const QString& baseUrl);
    static QString truncateToken(const QString& tokenKey);

private:
    SpecialMonitorSnapshot fetchSnapshot(const SpecialMonitorSource& source) const;
    SpecialMonitorSnapshot fetchPPCodingSnapshot(const SpecialMonitorSource& source) const;
    SpecialMonitorSnapshot fetchCMKeySnapshot(const SpecialMonitorSource& source) const;
    SpecialMonitorSnapshot fetchDeepSeekSnapshot(const SpecialMonitorSource& source) const;
    SpecialMonitorSnapshot fetchDeepSeekBalanceSnapshot(
        const SpecialMonitorSource& source,
        const QByteArray& authHeader,
        const QString& accountName = QString(),
        double usedAmount = 0.0,
        int usageCount = 0,
        bool usageCostAvailable = false) const;
};

#endif // SPECIALMONITORFETCHER_H
