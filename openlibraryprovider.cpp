#include "openlibraryprovider.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

OpenLibraryProvider::OpenLibraryProvider(QObject *parent) : AbstractBookProvider(parent)
{
    m_networkManager = std::make_unique<QNetworkAccessManager>(this);
    connect(m_networkManager.get(), &QNetworkAccessManager::finished, this, &OpenLibraryProvider::handleReply);
}

OpenLibraryProvider::~OpenLibraryProvider() = default;

void OpenLibraryProvider::requestMetadata(const QString &isbn)
{
    m_activeIsbn = isbn;
    QString urlString = QString("https://openlibrary.org/api/books?bibkeys=ISBN:%1&jscmd=data&format=json").arg(isbn);
    QNetworkRequest request((QUrl(urlString)));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setHeader(QNetworkRequest::UserAgentHeader, "Qt6ISBNBookScanner/1.0 (OpenLibrary Module)");
    m_networkManager->get(request);
}

void OpenLibraryProvider::handleReply(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        // Log details here directly
        qWarning() << "[OpenLibrary System Trace] API Connection dropped:" << reply->errorString();
        emit lookupFailed("404"); // Forward basic flag so fallback logic triggers smoothly
        reply->deleteLater();
        return;
    }

    QByteArray data = reply->readAll();
    reply->deleteLater();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        emit lookupFailed("Malformed JSON response from Open Library.");
        return;
    }

    QJsonObject root = doc.object();
    QString lookupKey = "ISBN:" + m_activeIsbn;

    if (!root.contains(lookupKey)) {
        emit lookupFailed("404"); // Send a explicit short phrase to signal the fallback manager
        return;
    }

    QJsonObject book = root.value(lookupKey).toObject();
    BookInfo info;
    info.found = true;
    info.isbn = m_activeIsbn;
    info.title = book.value("title").toString("Unknown Title");
    info.engineSource = "Open Library";

    if (book.contains("authors") && book.value("authors").isArray()) {
        QJsonArray authorsArr = book.value("authors").toArray();
        QStringList names;
        for (const QJsonValueRef val : authorsArr) {
            names.append(val.toObject().value("name").toString());
        }
        info.authors = names.join(", ");
    } else {
        info.authors = book.value("by_statement").toString("Unknown Author");
    }
    QString urlSmall = QString("https://covers.openlibrary.org/b/isbn/%1-S.jpg").arg(m_activeIsbn);
    QString urlMedium = QString("https://covers.openlibrary.org/b/isbn/%1-M.jpg").arg(m_activeIsbn);

    emit lookupFinished(info, urlSmall, urlMedium);
}
