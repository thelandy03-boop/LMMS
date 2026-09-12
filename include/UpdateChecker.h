#ifndef LMMS_UPDATE_CHECKER_H
#define LMMS_UPDATE_CHECKER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QString>
#include <QList>

namespace lmms
{
namespace gui
{

class UpdateChecker : public QObject
{
    Q_OBJECT
public:
    explicit UpdateChecker(QObject *parent = nullptr);
    ~UpdateChecker() override = default;

    void checkForUpdates();

private slots:
    void onReplyFinished(QNetworkReply *reply);

private:
    void applyUpdateAndRestart(const QString &zipPath);
    static bool isNewerVersion(const QString &remoteVersionStr, const QString &currentVersionStr);
    static QList<int> parseVersionString(const QString &versionStr);

    QNetworkAccessManager m_networkManager;
};

} // namespace gui
} // namespace lmms

#endif // LMMS_UPDATE_CHECKER_H
