#ifndef TRANSLATIONMANAGER_H
#define TRANSLATIONMANAGER_H

#include <QObject>
#include <QTranslator>
#include <QString>
#include <QMap>

class TranslationManager : public QObject
{
    Q_OBJECT

public:
    static TranslationManager* instance();
    
    void initialize();
    void switchLanguage(const QString& languageCode);
    QString currentLanguage() const;
    QStringList availableLanguages() const;
    QString languageName(const QString& languageCode) const;

signals:
    void languageChanged(const QString& languageCode);

private:
    explicit TranslationManager(QObject* parent = nullptr);
    ~TranslationManager();
    
    Q_DISABLE_COPY(TranslationManager)
    
    void loadTranslation(const QString& languageCode);
    QString translationFilePath(const QString& languageCode) const;
    
    QTranslator* m_translator;
    QString m_currentLanguage;
    QMap<QString, QString> m_languages;
};

#endif // TRANSLATIONMANAGER_H