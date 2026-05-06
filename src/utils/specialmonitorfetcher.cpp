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
#include <QRegularExpression>

#include <future>
#include <vector>

namespace {
qint64 jsonLongLong(const QJsonValue& value)
{
    return value.toVariant().toLongLong();
}

QString jsonString(const QJsonValue& value)
{
    return value.toVariant().toString();
}

int jsonInt(const QJsonValue& value)
{
    return value.toVariant().toInt();
}

QString formatMoneyAmount(const QJsonValue& value, bool withCurrency = false)
{
    QString amount = jsonString(value).trimmed();
    if (amount.isEmpty()) {
        return withCurrency ? QStringLiteral("￥0.00") : QStringLiteral("0");
    }
    bool ok = false;
    const double numericValue = amount.toDouble(&ok);
    if (!ok) {
        return amount;
    }
    const QString formatted = QString::number(numericValue, 'f', 2);
    return withCurrency ? QStringLiteral("￥%1").arg(formatted) : formatted;
}

QString formatMoneyAmount(double value, bool withCurrency = false)
{
    const QString formatted = QString::number(value, 'f', 2);
    return withCurrency ? QStringLiteral("￥%1").arg(formatted) : formatted;
}

double moneyAmount(const QJsonValue& value)
{
    bool ok = false;
    const double parsed = jsonString(value).trimmed().toDouble(&ok);
    return ok ? parsed : 0.0;
}

double extractDeepSeekUsageCostAmount(const QJsonValue& value)
{
    if (value.isArray()) {
        double total = 0.0;
        for (const QJsonValue& item : value.toArray()) {
            total += extractDeepSeekUsageCostAmount(item);
        }
        return total;
    }

    if (!value.isObject()) {
        return 0.0;
    }

    const QJsonObject object = value.toObject();
    if (object.contains(QStringLiteral("amount"))) {
        const double amount = moneyAmount(object.value(QStringLiteral("amount")));
        if (amount > 0.0) {
            return amount;
        }
    }

    double total = 0.0;
    for (auto it = object.constBegin(); it != object.constEnd(); ++it) {
        total += extractDeepSeekUsageCostAmount(it.value());
    }
    return total;
}

QByteArray deepSeekAuthHeader(const QString& tokenKey)
{
    const QString trimmed = tokenKey.trimmed();
    if (trimmed.startsWith(QStringLiteral("Bearer "), Qt::CaseInsensitive)) {
        return trimmed.toUtf8();
    }
    return QStringLiteral("Bearer %1").arg(trimmed).toUtf8();
}

int extractDeepSeekModelUsageCount(const QJsonValue& value)
{
    if (value.isArray()) {
        for (const QJsonValue& item : value.toArray()) {
            const int count = extractDeepSeekModelUsageCount(item);
            if (count > 0) {
                return count;
            }
        }
        return 0;
    }

    if (!value.isObject()) {
        return 0;
    }

    const QJsonObject object = value.toObject();
    bool matchesDeepSeekV4Pro = false;
    for (auto it = object.constBegin(); it != object.constEnd(); ++it) {
        if (it.value().isString() &&
            it.value().toString().contains(QStringLiteral("deepseek-v4-pro"), Qt::CaseInsensitive)) {
            matchesDeepSeekV4Pro = true;
            break;
        }
    }

    if (matchesDeepSeekV4Pro) {
        for (const QJsonValue& usageValue : object.value(QStringLiteral("usage")).toArray()) {
            const QJsonObject usageItem = usageValue.toObject();
            const QString typeValue = usageItem.value(QStringLiteral("type")).toString();
            if (typeValue == QStringLiteral("REQUEST")) {
                const int requestCount = jsonInt(usageItem.value(QStringLiteral("amount")));
                if (requestCount > 0) {
                    return requestCount;
                }
            }
        }

        static const QStringList countFields = {
            QStringLiteral("usage_count"),
            QStringLiteral("request_count"),
            QStringLiteral("requests"),
            QStringLiteral("total_calls"),
            QStringLiteral("calls"),
            QStringLiteral("count")
        };
        for (const QString& field : countFields) {
            const int count = jsonInt(object.value(field));
            if (count > 0) {
                return count;
            }
        }
    }

    for (auto it = object.constBegin(); it != object.constEnd(); ++it) {
        const int count = extractDeepSeekModelUsageCount(it.value());
        if (count > 0) {
            return count;
        }
    }

    return 0;
}

QString resolveTokenKey(const QString& tokenKey)
{
    const QString trimmed = tokenKey.trimmed();
    if (trimmed.isEmpty()) {
        return QString();
    }

    QString envName;
    if (trimmed.startsWith(QStringLiteral("env:"))) {
        envName = trimmed.mid(4).trimmed();
    } else if (trimmed.startsWith(QStringLiteral("${")) && trimmed.endsWith('}')) {
        envName = trimmed.mid(2, trimmed.size() - 3).trimmed();
    } else if (trimmed.startsWith('$')) {
        envName = trimmed.mid(1).trimmed();
    } else {
        static const QRegularExpression envNamePattern(QStringLiteral("^[A-Za-z_][A-Za-z0-9_]*$"));
        if (envNamePattern.match(trimmed).hasMatch()) {
            envName = trimmed;
        }
    }

    if (!envName.isEmpty()) {
        const QByteArray envValue = qgetenv(envName.toUtf8().constData());
        if (!envValue.isEmpty()) {
            return QString::fromUtf8(envValue).trimmed();
        }
    }

    return trimmed;
}

bool isCMKeyProviderUrl(const QString& baseUrl)
{
    const QUrl url(SpecialMonitorFetcher::normalizeBaseUrl(baseUrl));
    return url.host() == QStringLiteral("cmkey.cn") &&
        (url.path().isEmpty() || url.path() == QStringLiteral("/api/query"));
}

bool isDeepSeekProviderUrl(const QString& baseUrl)
{
    const QUrl url(SpecialMonitorFetcher::normalizeBaseUrl(baseUrl));
    return url.host() == QStringLiteral("platform.deepseek.com") ||
        url.host() == QStringLiteral("api.deepseek.com");
}

QString extractDeepSeekAccountName(const QJsonObject& currentUserData)
{
    const QJsonObject idProfile = currentUserData.value(QStringLiteral("id_profile")).toObject();
    const QString profileName = idProfile.value(QStringLiteral("name")).toString().trimmed();
    if (!profileName.isEmpty()) {
        return profileName;
    }
    const QString userName = currentUserData.value(QStringLiteral("name")).toString().trimmed();
    if (!userName.isEmpty()) {
        return userName;
    }
    return currentUserData.value(QStringLiteral("username")).toString().trimmed();
}

QJsonObject deepSeekPayloadObject(const QJsonObject& root)
{
    const QJsonValue dataValue = root.value(QStringLiteral("data"));
    if (dataValue.isObject()) {
        const QJsonObject data = dataValue.toObject();
        const QJsonValue bizDataValue = data.value(QStringLiteral("biz_data"));
        return bizDataValue.isObject() ? bizDataValue.toObject() : data;
    }
    return root;
}

bool fetchDeepSeekJson(
    const QUrl& url,
    const QByteArray& authHeader,
    QJsonObject& root,
    QString& errorMessage,
    int timeoutMs = 10000)
{
    QNetworkAccessManager manager;
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("R2MO-Obsidian/1.0"));
    request.setRawHeader("Authorization", authHeader);

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

    timeout.start(timeoutMs);
    loop.exec();

    if (timedOut) {
        errorMessage = QStringLiteral("Request timed out");
        reply->deleteLater();
        return false;
    }

    timeout.stop();
    const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QByteArray payload = reply->readAll();
    const QNetworkReply::NetworkError networkError = reply->error();
    const QString networkErrorText = reply->errorString();
    reply->deleteLater();

    if (networkError != QNetworkReply::NoError || httpStatus >= 400) {
        errorMessage = networkErrorText;
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(payload, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        errorMessage = QStringLiteral("Invalid JSON response");
        return false;
    }

    root = doc.object();
    if (root.contains(QStringLiteral("code")) &&
        root.value(QStringLiteral("code")).toInt(-1) != 0) {
        errorMessage = root.value(QStringLiteral("msg")).toString(QStringLiteral("API returned failure"));
        return false;
    }

    return true;
}
}

QList<SpecialMonitorSnapshot> SpecialMonitorFetcher::fetchSnapshots(
    const QList<SpecialMonitorSource>& sources) const
{
    std::vector<std::future<SpecialMonitorSnapshot>> snapshotFutures;
    snapshotFutures.reserve(sources.size());

    for (const SpecialMonitorSource& source : sources) {
        snapshotFutures.push_back(std::async(std::launch::async, [source]() {
            SpecialMonitorFetcher fetcher;
            return fetcher.fetchSnapshot(source);
        }));
    }

    QList<SpecialMonitorSnapshot> snapshots;
    snapshots.reserve(snapshotFutures.size());
    for (std::future<SpecialMonitorSnapshot>& future : snapshotFutures) {
        snapshots.append(future.get());
    }

    return snapshots;
}

SpecialMonitorSnapshot SpecialMonitorFetcher::fetchSnapshot(const SpecialMonitorSource& source) const
{
    const QString normalizedUrl = normalizeBaseUrl(source.baseUrl);
    if (normalizedUrl == "https://code.ppchat.vip") {
        return fetchPPCodingSnapshot(source);
    }
    if (isCMKeyProviderUrl(normalizedUrl)) {
        return fetchCMKeySnapshot(source);
    }
    if (isDeepSeekProviderUrl(normalizedUrl)) {
        return fetchDeepSeekSnapshot(source);
    }

    SpecialMonitorSnapshot snapshot;
    snapshot.source = source;
    snapshot.errorMessage = QStringLiteral("Unsupported provider");
    return snapshot;
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
    if (isDeepSeekProviderUrl(baseUrl)) {
        return QStringLiteral("🐋 DeepSeek");
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

    const QString tokenKey = resolveTokenKey(source.tokenKey);
    if (tokenKey.isEmpty()) {
        snapshot.errorMessage = QStringLiteral("Token key is empty");
        return snapshot;
    }

    QUrl url(QStringLiteral("https://his.ppchat.vip/api/token-logs"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("token_key"), tokenKey);
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
    snapshot.todayAddedQuota = QString::number(jsonLongLong(tokenInfo.value(QStringLiteral("today_added_quota"))));
    snapshot.todayUsedQuota = QString::number(jsonLongLong(tokenInfo.value(QStringLiteral("today_used_quota"))));
    snapshot.remainQuota = QString::number(jsonLongLong(tokenInfo.value(QStringLiteral("remain_quota_display"))));
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

    const QString tokenKey = resolveTokenKey(source.tokenKey);
    if (tokenKey.isEmpty()) {
        snapshot.errorMessage = QStringLiteral("Token key is empty");
        return snapshot;
    }

    QUrl url(QStringLiteral("https://cmkey.cn/api/query"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("key"), tokenKey);
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
    snapshot.todayAddedQuota = QString::number(jsonLongLong(data.value(QStringLiteral("period_limit"))));
    snapshot.todayUsedQuota = QString::number(jsonLongLong(data.value(QStringLiteral("period_used"))));
    snapshot.remainQuota = QString::number(jsonLongLong(data.value(QStringLiteral("period_remain"))));
    snapshot.todayUsageCount = jsonInt(data.value(QStringLiteral("total_calls")));
    snapshot.todayOpusUsage = 0;
    snapshot.totalLogCount = snapshot.todayUsageCount;
    snapshot.updatedAt = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));

    return snapshot;
}

SpecialMonitorSnapshot SpecialMonitorFetcher::fetchDeepSeekSnapshot(
    const SpecialMonitorSource& source) const
{
    SpecialMonitorSnapshot snapshot;
    snapshot.source = source;

    const QString tokenKey = resolveTokenKey(source.tokenKey);
    if (tokenKey.isEmpty()) {
        snapshot.errorMessage = QStringLiteral("Token key is empty");
        return snapshot;
    }

    const QByteArray authHeader = deepSeekAuthHeader(tokenKey);

    QJsonObject currentUserRoot;
    QString currentUserError;
    if (fetchDeepSeekJson(QUrl(QStringLiteral("https://platform.deepseek.com/auth-api/v0/users/current")),
                          authHeader,
                          currentUserRoot,
                          currentUserError)) {
        const QString accountName = extractDeepSeekAccountName(
            currentUserRoot.value(QStringLiteral("data")).toObject());
        if (!accountName.isEmpty()) {
            snapshot.accountName = accountName;
        }
    }

    const QDate currentDate = QDateTime::currentDateTime().date();
    QUrl usageAmountUrl(QStringLiteral("https://platform.deepseek.com/api/v0/usage/amount"));
    QUrlQuery usageAmountQuery;
    usageAmountQuery.addQueryItem(QStringLiteral("month"), QString::number(currentDate.month()));
    usageAmountQuery.addQueryItem(QStringLiteral("year"), QString::number(currentDate.year()));
    usageAmountUrl.setQuery(usageAmountQuery);

    QJsonObject usageAmountRoot;
    QString usageAmountError;
    QJsonObject usageAmountData;
    if (fetchDeepSeekJson(usageAmountUrl, authHeader, usageAmountRoot, usageAmountError)) {
        usageAmountData = deepSeekPayloadObject(usageAmountRoot);
    }

    QUrl usageCostUrl(QStringLiteral("https://platform.deepseek.com/api/v0/usage/cost"));
    QUrlQuery usageCostQuery;
    usageCostQuery.addQueryItem(QStringLiteral("month"), QString::number(currentDate.month()));
    usageCostQuery.addQueryItem(QStringLiteral("year"), QString::number(currentDate.year()));
    usageCostUrl.setQuery(usageCostQuery);

    QJsonObject usageCostRoot;
    QString usageCostError;
    QJsonObject usageCostData;
    const bool usageCostAvailable = fetchDeepSeekJson(usageCostUrl, authHeader, usageCostRoot, usageCostError);
    if (usageCostAvailable) {
        usageCostData = deepSeekPayloadObject(usageCostRoot);
    }
    const double usageCostAmount = extractDeepSeekUsageCostAmount(usageCostData);

    QJsonObject summaryRoot;
    QString summaryError;
    if (!fetchDeepSeekJson(QUrl(QStringLiteral("https://platform.deepseek.com/api/v0/users/get_user_summary")),
                           authHeader,
                           summaryRoot,
                           summaryError)) {
        SpecialMonitorSnapshot balanceSnapshot = fetchDeepSeekBalanceSnapshot(
            source,
            authHeader,
            snapshot.accountName,
            usageCostAmount,
            extractDeepSeekModelUsageCount(usageAmountData),
            usageCostAvailable);
        if (balanceSnapshot.errorMessage.isEmpty() &&
            (!currentUserError.isEmpty() || !usageAmountError.isEmpty() || !usageCostError.isEmpty() || !summaryError.isEmpty())) {
            balanceSnapshot.errorMessage = QStringLiteral(
                "DeepSeek account and usage fields require platform Bearer token; API key only supports balance fallback");
        }
        return balanceSnapshot;
    }

    const QJsonObject bizData = deepSeekPayloadObject(summaryRoot);

    const QJsonArray normalWallets = bizData.value(QStringLiteral("normal_wallets")).toArray();
    const QJsonArray bonusWallets = bizData.value(QStringLiteral("bonus_wallets")).toArray();
    double remainAmount = 0.0;
    QString normalBalance;
    for (const QJsonValue& w : normalWallets) {
        const QJsonObject wallet = w.toObject();
        remainAmount += moneyAmount(wallet.value(QStringLiteral("balance")));
        normalBalance = formatMoneyAmount(wallet.value(QStringLiteral("balance")));
    }
    for (const QJsonValue& w : bonusWallets) {
        const QJsonObject wallet = w.toObject();
        remainAmount += moneyAmount(wallet.value(QStringLiteral("balance")));
    }

    const QJsonArray monthlyCosts = bizData.value(QStringLiteral("monthly_costs")).toArray();
    QString monthlyCost;
    double usedAmount = 0.0;
    for (const QJsonValue& c : monthlyCosts) {
        const QJsonObject monthlyCostObject = c.toObject();
        monthlyCost = QStringLiteral("¥%1").arg(formatMoneyAmount(monthlyCostObject.value(QStringLiteral("amount"))));
        usedAmount += moneyAmount(monthlyCostObject.value(QStringLiteral("amount")));
    }
    const bool hasMonthlyCostAmount = usedAmount > 0.0;
    if (!hasMonthlyCostAmount && usageCostAmount > 0.0) {
        usedAmount = usageCostAmount;
        monthlyCost = formatMoneyAmount(usedAmount, true);
    }

    const double addedAmount = remainAmount + usedAmount;
    snapshot.accountName = snapshot.accountName.isEmpty()
        ? QStringLiteral("DeepSeek")
        : snapshot.accountName;
    snapshot.packageType = QStringLiteral("DeepSeek / 累积");
    snapshot.todayAddedQuota = formatMoneyAmount(addedAmount, true);
    snapshot.todayUsedQuota = formatMoneyAmount(usedAmount, true);
    snapshot.remainQuota = formatMoneyAmount(remainAmount, true);
    snapshot.todayUsageCount = extractDeepSeekModelUsageCount(usageAmountData);
    snapshot.todayOpusUsage = 0;
    snapshot.totalLogCount = 0;
    snapshot.updatedAt = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));

    return snapshot;
}

SpecialMonitorSnapshot SpecialMonitorFetcher::fetchDeepSeekBalanceSnapshot(
    const SpecialMonitorSource& source,
    const QByteArray& authHeader,
    const QString& accountName,
    double usedAmount,
    int usageCount,
    bool usageCostAvailable) const
{
    SpecialMonitorSnapshot snapshot;
    snapshot.source = source;

    QUrl url(QStringLiteral("https://api.deepseek.com/user/balance"));

    QNetworkAccessManager manager;
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("R2MO-Obsidian/1.0"));
    request.setRawHeader("Authorization", authHeader);

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

    timeout.start(10000);
    loop.exec();

    if (timedOut) {
        snapshot.errorMessage = QStringLiteral("Request timed out");
        reply->deleteLater();
        return snapshot;
    }

    timeout.stop();
    const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QByteArray payload = reply->readAll();
    const QNetworkReply::NetworkError networkError = reply->error();
    const QString errorText = reply->errorString();
    reply->deleteLater();

    if (networkError != QNetworkReply::NoError || httpStatus >= 400) {
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
    if (!root.value(QStringLiteral("is_available")).toBool(false)) {
        snapshot.errorMessage = QStringLiteral("DeepSeek balance is unavailable");
        return snapshot;
    }

    const QJsonArray balanceInfos = root.value(QStringLiteral("balance_infos")).toArray();

    QString totalBalance;
    for (const QJsonValue& b : balanceInfos) {
        const QJsonObject bi = b.toObject();
        if (bi.value(QStringLiteral("currency")).toString() == QStringLiteral("CNY")) {
            totalBalance = formatMoneyAmount(bi.value(QStringLiteral("total_balance")), true);
        }
    }

    const double remainAmount = totalBalance.isEmpty() ? 0.0 : totalBalance.mid(1).toDouble();
    const double totalAmount = remainAmount + qMax(0.0, usedAmount);
    snapshot.accountName = accountName.trimmed().isEmpty() ? QStringLiteral("DeepSeek") : accountName.trimmed();
    snapshot.packageType = QStringLiteral("DeepSeek / 累积");
    snapshot.todayAddedQuota = formatMoneyAmount(totalAmount, true);
    Q_UNUSED(usageCostAvailable);
    snapshot.todayUsedQuota = formatMoneyAmount(qMax(0.0, usedAmount), true);
    snapshot.remainQuota = totalBalance.isEmpty() ? QStringLiteral("￥0.00") : totalBalance;
    snapshot.todayUsageCount = usageCount;
    snapshot.todayOpusUsage = 0;
    snapshot.totalLogCount = 0;
    snapshot.updatedAt = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));

    return snapshot;
}
