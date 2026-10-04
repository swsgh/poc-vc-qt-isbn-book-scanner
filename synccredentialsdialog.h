#ifndef SYNCCREDENTIALSDIALOG_H
#define SYNCCREDENTIALSDIALOG_H

#include <QDialog>
#include <QString>

class QCheckBox;
class QLineEdit;

class SyncCredentialsDialog : public QDialog
{
public:
    explicit SyncCredentialsDialog(bool registering, QWidget *parent = nullptr);

    QString serverUrl() const;
    QString username() const;
    QString password() const;

private:
    bool m_registering;
    QLineEdit *m_serverUrlInput = nullptr;
    QLineEdit *m_usernameInput = nullptr;
    QLineEdit *m_passwordInput = nullptr;
    QLineEdit *m_confirmationInput = nullptr;
    QCheckBox *m_rememberUsername = nullptr;
};

#endif // SYNCCREDENTIALSDIALOG_H
