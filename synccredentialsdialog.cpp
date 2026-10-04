#include "synccredentialsdialog.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QSettings>
#include <QUrl>
#include <QVBoxLayout>
#include <algorithm>

SyncCredentialsDialog::SyncCredentialsDialog(bool registering, QWidget *parent)
    : QDialog(parent), m_registering(registering)
{
    QSettings settings;
    const QString environmentUrl = qEnvironmentVariable("BOOKSHELF_SYNC_URL");
    const QString defaultUrl = !environmentUrl.isEmpty()
        ? environmentUrl
        : settings.value("sync/server_url", "http://127.0.0.1:8000").toString();
    const QString rememberedUsername = settings.value("sync/username").toString();

    setWindowTitle(registering ? "Register Sync Account" : "Log In to Sync");
    const int parentWidth = parent ? parent->width() : 560;
    const int maximumDialogWidth = std::max(1, parentWidth * 4 / 5);
    setMaximumWidth(maximumDialogWidth);
    setMinimumWidth(std::min(375, maximumDialogWidth));
    setStyleSheet("QDialog QLabel { background-color: transparent; border: none; }");

    m_serverUrlInput = new QLineEdit(defaultUrl, this);
    m_usernameInput = new QLineEdit(this);
    if (!registering) m_usernameInput->setText(rememberedUsername);
    m_passwordInput = new QLineEdit(this);
    m_passwordInput->setEchoMode(QLineEdit::Password);
    if (registering) {
        m_confirmationInput = new QLineEdit(this);
        m_confirmationInput->setEchoMode(QLineEdit::Password);
    } else {
        m_rememberUsername = new QCheckBox("Remember username", this);
        m_rememberUsername->setChecked(!rememberedUsername.isEmpty());
    }

    auto *form = new QGridLayout;
    form->setColumnMinimumWidth(0, 130);
    form->setColumnStretch(1, 1);
    const auto addField = [form, this](int row, const QString &labelText,
                                       QLineEdit *field) {
        auto *label = new QLabel(labelText, this);
        label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        label->setFrameShape(QFrame::NoFrame);
        label->setAutoFillBackground(false);
        label->setStyleSheet(
            "QLabel { background-color: transparent; border: none; padding: 0px; }");
        form->addWidget(label, row, 0);
        form->addWidget(field, row, 1);
    };
    addField(0, "Server URL:", m_serverUrlInput);
    addField(1, "Username:", m_usernameInput);
    addField(2, "Password:", m_passwordInput);
    int nextRow = 3;
    if (m_confirmationInput) {
        addField(nextRow++, "Confirm password:", m_confirmationInput);
    }
    if (m_rememberUsername) form->addWidget(m_rememberUsername, nextRow, 1);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, this, [this]() {
        QString enteredUrl = m_serverUrlInput->text().trimmed();
        while (enteredUrl.endsWith('/')) enteredUrl.chop(1);
        const QUrl url(enteredUrl);
        const QString scheme = url.scheme().toLower();
        if (!url.isValid() || url.host().isEmpty()
            || (scheme != "http" && scheme != "https")) {
            QMessageBox::warning(
                this, "Invalid Server URL", "Enter an absolute http:// or https:// server URL.");
            return;
        }
        if (m_usernameInput->text().trimmed().isEmpty() || m_passwordInput->text().isEmpty()) {
            QMessageBox::warning(this, windowTitle(), "Enter a username and password.");
            return;
        }
        if (m_confirmationInput && m_passwordInput->text() != m_confirmationInput->text()) {
            QMessageBox::warning(this, windowTitle(), "The passwords do not match.");
            return;
        }

        QSettings settings;
        settings.setValue("sync/server_url", enteredUrl);
        if (m_rememberUsername) {
            if (m_rememberUsername->isChecked()) {
                settings.setValue("sync/username", m_usernameInput->text().trimmed());
            } else {
                settings.remove("sync/username");
            }
        }
        accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

QString SyncCredentialsDialog::serverUrl() const
{
    QString url = m_serverUrlInput->text().trimmed();
    while (url.endsWith('/')) url.chop(1);
    return url;
}

QString SyncCredentialsDialog::username() const
{
    return m_usernameInput->text().trimmed();
}

QString SyncCredentialsDialog::password() const
{
    return m_passwordInput->text();
}
