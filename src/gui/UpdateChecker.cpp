#include "UpdateChecker.h"
#include "lmmsversion.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include <QRegularExpression>
#include <QDebug>

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
    // Endpoint oficial de la API de GitHub para la última release del repositorio
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
        qDebug() << "[UpdateChecker] Error de red al verificar actualizaciones:" << reply->errorString();
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);

    if (!doc.isObject())
    {
        qDebug() << "[UpdateChecker] Respuesta JSON inválida desde GitHub.";
        return;
    }

    QJsonObject root = doc.object();
    QString tagName = root.value("tag_name").toString();
    QString htmlUrl = root.value("html_url").toString();

    if (tagName.isEmpty())
    {
        qDebug() << "[UpdateChecker] No se encontró la etiqueta tag_name en la respuesta.";
        return;
    }

    QString currentVersion = QString(LMMS_VERSION);

    if (isNewerVersion(tagName, currentVersion))
    {
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Nueva actualización disponible"));
        msgBox.setText(tr("<h3>¡Hay una nueva versión de LMMS disponible!</h3>"
                          "<p>Versión instalada: <b>%1</b></p>"
                          "<p>Nueva versión disponible: <b>%2</b></p>")
                       .arg(currentVersion, tagName));
        msgBox.setInformativeText(tr("¿Deseas abrir la página de descargas en tu navegador?"));
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::Yes);
        msgBox.setIcon(QMessageBox::Information);

        if (msgBox.exec() == QMessageBox::Yes)
        {
            if (!htmlUrl.isEmpty())
            {
                QDesktopServices::openUrl(QUrl(htmlUrl));
            }
            else
            {
                QDesktopServices::openUrl(QUrl("https://github.com/thelandy03-boop/LMMS/releases"));
            }
        }
    }
}

QList<int> UpdateChecker::parseVersionString(const QString &versionStr)
{
    QString clean = versionStr;
    if (clean.startsWith('v', Qt::CaseInsensitive))
    {
        clean = clean.mid(1);
    }

    static QRegularExpression re(R"(\d+)");
    QRegularExpressionMatchIterator it = re.globalMatch(clean);
    QList<int> parts;
    while (it.hasNext())
    {
        QRegularExpressionMatch match = it.next();
        parts.append(match.captured(0).toInt());
    }
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
