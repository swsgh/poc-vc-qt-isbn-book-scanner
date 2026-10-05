#include "bookcsv.h"

#include <QFile>
#include <QStringConverter>
#include <QTextStream>

namespace {
QString escapeField(QString field)
{
    field.replace('"', "\"\"");
    return '"' + field + '"';
}

bool parseRecords(const QString &contents, QList<QStringList> &records)
{
    QStringList record;
    QString field;
    bool insideQuotes = false;
    bool recordHasData = false;

    for (qsizetype index = 0; index < contents.size(); ++index) {
        const QChar character = contents.at(index);
        if (insideQuotes) {
            if (character == '"') {
                if (index + 1 < contents.size() && contents.at(index + 1) == '"') {
                    field.append('"');
                    ++index;
                } else {
                    insideQuotes = false;
                }
            } else {
                field.append(character);
            }
            recordHasData = true;
            continue;
        }

        if (character == '"' && field.isEmpty()) {
            insideQuotes = true;
            recordHasData = true;
        } else if (character == ',') {
            record.append(field);
            field.clear();
            recordHasData = true;
        } else if (character == '\r' || character == '\n') {
            if (character == '\r' && index + 1 < contents.size()
                && contents.at(index + 1) == '\n') {
                ++index;
            }
            if (recordHasData || !record.isEmpty()) {
                record.append(field);
                records.append(record);
            }
            record.clear();
            field.clear();
            recordHasData = false;
        } else {
            field.append(character);
            recordHasData = true;
        }
    }

    if (insideQuotes) return false;
    if (recordHasData || !record.isEmpty()) {
        record.append(field);
        records.append(record);
    }
    return true;
}
}

QStringList BookCsv::headers()
{
    return {"ISBN", "Title", "Author", "Cover URL",
            "First Publication Date", "Publisher", "Page Count"};
}

bool BookCsv::writeFile(const QString &filePath, const QList<BookInfo> &books, QString &error)
{
    error.clear();
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        error = file.errorString();
        return false;
    }

    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);
    stream.setGenerateByteOrderMark(true);
    stream << headers().join(',') << "\r\n";
    for (const BookInfo &book : books) {
        stream << escapeField(book.isbn) << ','
               << escapeField(book.title) << ','
               << escapeField(book.authors) << ','
               << escapeField(book.coverUrl) << ','
               << escapeField(book.publicationDate) << ','
               << escapeField(book.publisher) << ','
               << book.pageCount << "\r\n";
    }
    stream.flush();
    if (stream.status() != QTextStream::Ok) {
        error = "Could not write the CSV file.";
        return false;
    }
    return true;
}

bool BookCsv::readFile(const QString &filePath, QList<BookInfo> &books,
                       int &skippedRows, QString &error)
{
    books.clear();
    skippedRows = 0;
    error.clear();

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        error = file.errorString();
        return false;
    }

    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);
    QList<QStringList> records;
    if (!parseRecords(stream.readAll(), records) || records.isEmpty()) {
        error = "The CSV file is empty or malformed.";
        return false;
    }

    QStringList csvHeaders = records.first();
    if (!csvHeaders.isEmpty() && csvHeaders.first().startsWith(QChar::ByteOrderMark)) {
        csvHeaders[0].remove(0, 1);
    }
    if (csvHeaders != headers()) {
        error = "CSV headers must be: " + headers().join(", ");
        return false;
    }

    for (qsizetype index = 1; index < records.size(); ++index) {
        const QStringList &record = records.at(index);
        const auto valueAt = [&record](int column) {
            return column < record.size() ? record.at(column).trimmed() : QString();
        };

        BookInfo info;
        info.found = true;
        info.isbn = valueAt(0);
        info.title = valueAt(1);
        info.authors = valueAt(2);
        info.coverUrl = valueAt(3);
        info.publicationDate = valueAt(4);
        info.publisher = valueAt(5);

        const QString pageCountText = valueAt(6);
        bool pageCountValid = true;
        info.pageCount = pageCountText.isEmpty() ? 0 : pageCountText.toInt(&pageCountValid);
        if (info.isbn.isEmpty() || info.title.isEmpty()
            || !pageCountValid || info.pageCount < 0) {
            ++skippedRows;
            continue;
        }
        books.append(info);
    }
    return true;
}
