#include "specialmonitorfetcher.h"

#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>

namespace {
qint64 jsonLongLong(const QJsonValue& value)
{
    return value.toVariant().toLongLong();
}

int jsonInt(const QJsonValue& value)
{
    return value.toVariant().toInt();
}

bool isCMKeyProviderUrl(const QString& baseUrl)
{
    const QUrl url(SpecialMonitorFetcher::normalizeBaseUrl(baseUrl));
    return url.host() == QStringLiteral("cmkey.cn") &&
        (url.path().isEmpty() || url.path() == QStringLiteral("/api/query"));
}
}

QList<SpecialMonitorSnapshot> SpecialMonitorFetcher::fetchSnapshots(
    const QList<SpecialMonitorSource>& sources) const
{
    QList<SpecialMonitorSnapshot> snapshots;
    for (const SpecialMonitorSource& source : sources) {
        const QString normalizedUrl = normalizeBaseUrl(source.baseUrl);
        if (normalizedUrl == "https://code.ppchat.vip") {
            snapshots.append(fetchPPCodingSnapshot(source));
            continue;
        }
        if (isCMKeyProviderUrl(normalizedUrl)) {
            snapshots.append(fetchCMKeySnapshot(source));
            continue;
        }

        SpecialMonitorSnapshot snapshot;
        snapshot.source = source;
        snapshot.errorMessage = QStringLiteral("Unsupported provider");
        snapshots.append(snapshot);
    }
    return snapshots;
}

QString SpecialMonitorFetcher::normalizeBaseUrl(const QString& baseUrl)
{
    QString normalized = baseUrl.trimmed();
    while (normalized.endsWith('/')) {
        normalized.chop(1);
    }
    return normalized;
}

QString SpecialMonitorFetcher::defaultProviderName(const QString& baseUrl)
{
    if (normalizeBaseUrl(baseUrl) == "https://code.ppchat.vip") {
        return QStringLiteral("💻 PP Coding");
    }
    if (isCMKeyProviderUrl(baseUrl)) {
        return QStringLiteral("🧩 CM Key");
    }
    return QStringLiteral("🔗 Special Monitor");
}

QString SpecialMonitorFetcher::truncateToken(const QString& tokenKey)
{
    if (tokenKey.size() <= 12) {
        return tokenKey;
    }
    return QStringLiteral("%1...%2")
        .arg(tokenKey.left(6), tokenKey.right(4));
}

SpecialMonitorSnapshot SpecialMonitorFetcher::fetchPPCodingSnapshot(
    const SpecialMonitorSource& source) const
{
    SpecialMonitorSnapshot snapshot;
    snapshot.source = source;

    if (source.tokenKey.trimmed().isEmpty()) {
        snapshot.errorMessage = QStringLiteral("Token key is empty");
        return snapshot;
    }

    QUrl url(QStringLiteral("https://his.ppchat.vip/api/token-logs"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("token_key"), source.tokenKey.trimmed());
    query.addQueryItem(QStringLiteral("page"), QStringLiteral("1"));
    query.addQueryItem(QStringLiteral("page_size"), QStringLiteral("10"));
    url.setQuery(query);

    QNetworkAccessManager manager;
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("R2MO-Obsidian/1.0"));

    QNetworkReply* reply = manager.get(request);
    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);
    bool timedOut = false;

    QObject::connect(&timeout, &QTimer::timeout, &loop, [&]() {
        timedOut = true;
        if (!reply->isFinished()) {
            reply->abort();
        }
        loop.quit();
    });
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);

    timeout.start(5000);
    loop.exec();

    if (timedOut) {
        snapshot.errorMessage = QStringLiteral("Request timed out");
        reply->deleteLater();
        return snapshot;
    }

    timeout.stop();
    const QByteArray payload = reply->readAll();
    const QNetworkReply::NetworkError networkError = reply->error();
    const QString errorText = reply->errorString();
    reply->deleteLater();

    if (networkError != QNetworkReply::NoError) {
        snapshot.errorMessage = errorText;
        return snapshot;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(payload, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        snapshot.errorMessage = QStringLiteral("Invalid JSON response");
        return snapshot;
    }

    const QJsonObject root = doc.object();
    if (!root.value(QStringLiteral("success")).toBool()) {
        snapshot.errorMessage = QStringLiteral("API returned failure");
        return snapshot;
    }

    const QJsonObject data = root.value(QStringLiteral("data")).toObject();
    const QJsonObject tokenInfo = data.value(QStringLiteral("token_info")).toObject();
    const QJsonObject pagination = data.value(QStringLiteral("pagination")).toObject();

    const QString tokenName = tokenInfo.value(QStringLiteral("name")).toString();
    const int splitIndex = tokenName.indexOf('-');
    snapshot.accountName = splitIndex > 0 ? tokenName.left(splitIndex) : tokenName;
    snapshot.packageType = splitIndex > 0 ? tokenName.mid(splitIndex + 1) : QString();
    snapshot.todayAddedQuota = jsonLongLong(tokenInfo.value(QStringLiteral("today_added_quota")));
    snapshot.todayUsedQuota = jsonLongLong(tokenInfo.value(QStringLiteral("today_used_quota")));
    snapshot.remainQuota = jsonLongLong(tokenInfo.value(QStringLiteral("remain_quota_display")));
    snapshot.todayUsageCount = jsonInt(tokenInfo.value(QStringLiteral("today_usage_count")));
    snapshot.todayOpusUsage = jsonInt(tokenInfo.value(QStringLiteral("today_opus_usage")));
    snapshot.totalLogCount = jsonInt(pagination.value(QStringLiteral("total")));
    snapshot.updatedAt = data.value(QStringLiteral("logs")).toArray().isEmpty()
        ? QString()
        : data.value(QStringLiteral("logs")).toArray().first().toObject().value(QStringLiteral("created_time")).toString();

    return snapshot;
}

SpecialMonitorSnapshot SpecialMonitorFetcher::fetchCMKeySnapshot(
    const SpecialMonitorSource& source) const
{
    SpecialMonitorSnapshot snapshot;
    snapshot.source = source;

    if (source.tokenKey.trimmed().isEmpty()) {
        snapshot.errorMessage = QStringLiteral("Token key is empty");
        return snapshot;
    }

    QUrl url(QStringLiteral("https://cmkey.cn/api/query"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("key"), source.tokenKey.trimmed());
    url.setQuery(query);

    QNetworkAccessManager manager;
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("R2MO-Obsidian/1.0"));

    QNetworkReply* reply = manager.get(request);
    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);
    bool timedOut = false;

    QObject::connect(&timeout, &QTimer::timeout, &loop, [&]() {
        timedOut = true;
        if (!reply->isFinished()) {
            reply->abort();
        }
        loop.quit();
    });
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);

    timeout.start(5000);
    loop.exec();

    if (timedOut) {
        snapshot.errorMessage = QStringLiteral("Request timed out");
        reply->deleteLater();
        return snapshot;
    }

    timeout.stop();
    const QByteArray payload = reply->readAll();
    const QNetworkReply::NetworkError networkError = reply->error();
    const QString errorText = reply->errorString();
    reply->deleteLater();

    if (networkError != QNetworkReply::NoError) {
        snapshot.errorMessage = errorText;
        return snapshot;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(payload, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        snapshot.errorMessage = QStringLiteral("Invalid JSON response");
        return snapshot;
    }

    const QJsonObject root = doc.object();
    if (!root.value(QStringLiteral("success")).toBool()) {
        snapshot.errorMessage = QStringLiteral("API returned failure");
        return snapshot;
    }

    const QJsonObject data = root.value(QStringLiteral("data")).toObject();
    snapshot.accountName = data.value(QStringLiteral("token_name")).toString();
    const int periodHours = jsonInt(data.value(QStringLiteral("period_limit_hours")));
    snapshot.packageType = periodHours > 0
        ? QStringLiteral("每%1%2").arg(periodHours).arg(QStringLiteral("小时"))
        : data.value(QStringLiteral("primary_label")).toString();
    snapshot.todayAddedQuota = jsonLongLong(data.value(QStringLiteral("period_limit")));
    snapshot.todayUsedQuota = jsonLongLong(data.value(QStringLiteral("period_used")));
    snapshot.remainQuota = jsonLongLong(data.value(QStringLiteral("period_remain")));
    snapshot.todayUsageCount = jsonInt(data.value(QStringLiteral("total_calls")));
    snapshot.todayOpusUsage = 0;
    snapshot.totalLogCount = snapshot.todayUsageCount;
    snapshot.updatedAt = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));

    return snapshot;
}
