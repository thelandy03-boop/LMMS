#include "UpdateChecker.h"
#include "lmmsversion.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include <QRegularExpression>
#include <QDebug>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QProcess>
#include <QCoreApplication>

namespace lmms
{
namespace gui
{

UpdateChecker::UpdateChecker(QObject *parent) :
    QObject(parent)
{
    connect(&m_networkManager, &QNetworkAccessManager::finished,
            this, &UpdateChecker::onReplyFinished);
}

void UpdateChecker::checkForUpdates()
{
    QUrl url("https://api.github.com/repos/thelandy03-boop/LMMS/releases/latest");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "LMMS-DAW-UpdateChecker/1.0");

    m_networkManager.get(request);
}

void UpdateChecker::onReplyFinished(QNetworkReply *reply)
{
    if (!reply) return;
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError)
    {
        qDebug() << "[UpdateChecker] Error de red:" << reply->errorString();
        return;
    }

    if (reply->property("isZipDownload").toBool())
    {
        QString tempZipPath = reply->property("tempZipPath").toString();
        QFile file(tempZipPath);
        if (file.open(QIODevice::WriteOnly))
        {
            file.write(reply->readAll());
            file.close();
            
            applyUpdateAndRestart(tempZipPath);
        }
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) return;

    QJsonObject root = doc.object();
    QString tagName = root.value("tag_name").toString();
    
    QString zipUrl;
    QJsonArray assets = root.value("assets").toArray();
    for (const QJsonValue &val : assets)
    {
        QJsonObject asset = val.toObject();
        QString name = asset.value("name").toString();
        if (name.endsWith(".zip", Qt::CaseInsensitive))
        {
            zipUrl = asset.value("browser_download_url").toString();
            break;
        }
    }

    QString currentVersion = QString(LMMS_VERSION);

    if (isNewerVersion(tagName, currentVersion))
    {
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Actualización de LMMS"));
        msgBox.setText(tr("<h3>¡Nueva versión %1 disponible!</h3>"
                          "<p>Versión actual: <b>%2</b></p>"
                          "<p>¿Deseas actualizar automáticamente ahora?</p>")
                       .arg(tagName, currentVersion));
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::Yes);
        msgBox.setIcon(QMessageBox::Information);

        if (msgBox.exec() == QMessageBox::Yes)
        {
            if (!zipUrl.isEmpty())
            {
                QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
                QString tempZipPath = QDir(tempDir).filePath("lmms_update.zip");

                QNetworkRequest downloadReq((QUrl(zipUrl)));
                downloadReq.setHeader(QNetworkRequest::UserAgentHeader, "LMMS-DAW-UpdateChecker/1.0");

                QNetworkReply *downloadReply = m_networkManager.get(downloadReq);
                downloadReply->setProperty("isZipDownload", true);
                downloadReply->setProperty("tempZipPath", tempZipPath);
            }
            else
            {
                QDesktopServices::openUrl(QUrl("https://github.com/thelandy03-boop/LMMS/releases"));
            }
        }
    }
}

void UpdateChecker::applyUpdateAndRestart(const QString &zipPath)
{
    QString installDir = "C:\\LMMS_Installed";
    QString exePath = installDir + "\\lmms.exe";

    QString psCommand = QString(
        "Start-Sleep -Seconds 2; " +
        "Expand-Archive -Path '%1' -DestinationPath '%2' -Force; " +
        "Remove-Item -Path '%1' -Force; " +
        "Start-Process '%3'"
    ).arg(zipPath, installDir, exePath);

    QStringList args;
    args << "-NoProfile" << "-NonInteractive" << "-Command" << psCommand;

    QProcess::startDetached("powershell.exe", args);
    QCoreApplication::quit();
}

QList<int> UpdateChecker::parseVersionString(const QString &versionStr)
{
    QString clean = versionStr;
    if (clean.startsWith('v', Qt::CaseInsensitive)) clean = clean.mid(1);

    static QRegularExpression re(R"(\d+)");
    QRegularExpressionMatchIterator it = re.globalMatch(clean);
    QList<int> parts;
    while (it.hasNext()) parts.append(it.next().captured(0).toInt());
    return parts;
}

bool UpdateChecker::isNewerVersion(const QString &remoteVersionStr, const QString &currentVersionStr)
{
    QList<int> remote = parseVersionString(remoteVersionStr);
    QList<int> current = parseVersionString(currentVersionStr);

    int maxLen = std::max(remote.size(), current.size());
    for (int i = 0; i < maxLen; ++i)
    {
        int r = (i < remote.size()) ? remote[i] : 0;
        int c = (i < current.size()) ? current[i] : 0;
        if (r > c) return true;
        if (r < c) return false;
    }
    return false;
}

} // namespace gui
} // namespace lmms
