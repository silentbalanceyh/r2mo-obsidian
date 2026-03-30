#include "translationmanager.h"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QDebug>
#include <QCoreApplication>

TranslationManager* TranslationManager::instance()
{
    static TranslationManager manager;
    return &manager;
}

TranslationManager::TranslationManager(QObject* parent)
    : QObject(parent)
    , m_translator(nullptr)
    , m_currentLanguage("en_US")
{
    // Initialize available languages
    m_languages["en_US"] = "English";
    m_languages["zh_CN"] = "中文";
}

TranslationManager::~TranslationManager()
{
    if (m_translator) {
        qApp->removeTranslator(m_translator);
        delete m_translator;
    }
}

void TranslationManager::initialize()
{
    // Try to load saved language or system language
    QString savedLang = m_currentLanguage;
    
    // Try system language first
    QString systemLang = QLocale::system().name(); // e.g., "zh_CN"
    
    if (m_languages.contains(systemLang)) {
        switchLanguage(systemLang);
    } else {
        switchLanguage("en_US");
    }
}

void TranslationManager::switchLanguage(const QString& languageCode)
{
    if (!m_languages.contains(languageCode)) {
        qWarning() << "Language not available:" << languageCode;
        return;
    }
    
    // Remove old translator
    if (m_translator) {
        qApp->removeTranslator(m_translator);
        delete m_translator;
        m_translator = nullptr;
    }
    
    // Create new translator
    m_translator = new QTranslator(this);
    
    QString filePath = translationFilePath(languageCode);
    
    qDebug() << "Trying to load translation from:" << filePath;
    
    if (QFile::exists(filePath) && m_translator->load(filePath)) {
        qApp->installTranslator(m_translator);
        m_currentLanguage = languageCode;
        emit languageChanged(languageCode);
        qDebug() << "Language switched to:" << languageCode;
    } else {
        qWarning() << "Failed to load translation:" << filePath;
        delete m_translator;
        m_translator = nullptr;
    }
}

QString TranslationManager::currentLanguage() const
{
    return m_currentLanguage;
}

QStringList TranslationManager::availableLanguages() const
{
    return m_languages.keys();
}

QString TranslationManager::languageName(const QString& languageCode) const
{
    return m_languages.value(languageCode, languageCode);
}

void TranslationManager::loadTranslation(const QString& languageCode)
{
    switchLanguage(languageCode);
}

QString TranslationManager::translationFilePath(const QString& languageCode) const
{
    QString fileName = QString("obsidianmanager_%1.qm").arg(languageCode);
    
    // Application directory (build/ObsidianManager.app/Contents/MacOS/)
    QString appDir = QCoreApplication::applicationDirPath();
    
    // Try multiple locations in order
    QStringList searchPaths;
    
    // 1. Same directory as app (build/ObsidianManager.app/Contents/MacOS/)
    searchPaths << appDir;
    
    // 2. Parent directory (build/ObsidianManager.app/Contents/)
    searchPaths << QDir(appDir).absoluteFilePath("..");
    
    // 3. Two levels up (build/ObsidianManager.app/)
    searchPaths << QDir(appDir).absoluteFilePath("../..");
    
    // 4. Three levels up (build/)
    searchPaths << QDir(appDir).absoluteFilePath("../../..");
    
    // 5. Project root translations directory
    searchPaths << QDir::currentPath() + "/translations";
    
    // 6. Build directory
    searchPaths << QDir::currentPath() + "/build";
    
    for (const QString& path : searchPaths) {
        QString fullPath = QDir::cleanPath(path + "/" + fileName);
        if (QFile::exists(fullPath)) {
            qDebug() << "Found translation file at:" << fullPath;
            return fullPath;
        }
    }
    
    // Return default path
    QString defaultPath = QDir::cleanPath(appDir + "/../../.." + "/" + fileName);
    qDebug() << "Translation file not found, returning default path:" << defaultPath;
    return defaultPath;
}