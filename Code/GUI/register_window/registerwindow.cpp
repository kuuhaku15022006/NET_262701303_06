#include "registerwindow.h"
#include "../../client/TCPClient.h"
#include <QCoreApplication>
#include <QFileInfo>
#include <QFormLayout>
#include <QLineEdit>
#include <QProcess>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>
#include <QLabel>
#include <QMessageBox>

RegisterWindow::RegisterWindow(QWidget *parent)
    : RegisterWindow(nullptr, parent)
{
}

RegisterWindow::RegisterWindow(TCPClient *client, QWidget *parent)
    : QWidget(parent), m_client(client)
{
    setWindowFlag(Qt::Window, true);
    setWindowTitle("Chat TCP - Register");
    setMinimumSize(360, 300);
    auto *layout = new QVBoxLayout(this);
    auto *title = new QLabel("CREATE ACCOUNT");
    title->setAlignment(Qt::AlignCenter);
    auto *form = new QFormLayout;
    usernameInput = new QLineEdit(this);
    usernameInput->setPlaceholderText("Username");
    passwordInput = new QLineEdit(this);
    passwordInput->setPlaceholderText("Password (min 8 chars)");
    passwordInput->setEchoMode(QLineEdit::Password);
    displayNameInput = new QLineEdit(this);
    displayNameInput->setPlaceholderText("Display name");
    form->addRow("Username:", usernameInput);
    form->addRow("Password:", passwordInput);
    form->addRow("Display name:", displayNameInput);
    auto *button = new QPushButton("Register", this);
    layout->addWidget(title);
    layout->addLayout(form);
    layout->addWidget(button);
    connect(button, &QPushButton::clicked, this, &RegisterWindow::registerUser);

    if (m_client) {
        connect(m_client, &TCPClient::responseReceived,
                this, &RegisterWindow::handleServerResponse);
        connect(m_client, &TCPClient::connectionError, this, [this](const QString &err) {
            if (m_waitingRegister) {
                m_waitingRegister = false;
                QMessageBox::warning(this, "Connection Error", err);
            }
        });
    }
}

void RegisterWindow::registerUser()
{
    if (usernameInput->text().trimmed().isEmpty() ||
        passwordInput->text().isEmpty() ||
        displayNameInput->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Register Error", "Please fill in all fields.");
        return;
    }
    // TODO: Send register request through NV4 TCP Client.
    if (!m_client) {
        QMessageBox::information(this, "Register",
                                 "Registration request is ready for server integration.");
        return;
    }

    m_waitingRegister = true;
    const QString user = usernameInput->text().trimmed();
    const QString pass = passwordInput->text();
    const QString disp = displayNameInput->text().trimmed();

    if (!m_client->isConnected()) {
        const QString appDir = QCoreApplication::applicationDirPath();
        for (const QString &p : {appDir + QStringLiteral("/ChatServer.exe"),
                                 appDir + QStringLiteral("/../ChatServer.exe")}) {
            if (QFileInfo::exists(p)) {
                QProcess::startDetached(p, {});
                break;
            }
        }

        QTimer::singleShot(250, this, [this, user, pass, disp]() {
            QObject *ctx = new QObject(this);
            connect(m_client, &TCPClient::connected, ctx, [this, ctx, user, pass, disp]() {
                ctx->deleteLater();
                if (m_waitingRegister)
                    m_client->registerUser(user, pass, disp);
            });
            m_client->connectToServer(QStringLiteral("127.0.0.1"), 5555);
        });
        return;
    }

    m_client->registerUser(user, pass, disp);
}

void RegisterWindow::handleServerResponse(const QJsonObject &response)
{
    if (!m_waitingRegister)
        return;
    const QString reqType = response.value(QStringLiteral("request_type")).toString();
    if (!reqType.isEmpty() && reqType != QStringLiteral("register"))
        return;

    m_waitingRegister = false;
    if (response.value(QStringLiteral("ok")).toBool()) {
        QMessageBox::information(this, "Register", "Đăng ký tài khoản thành công!");
        close();
    } else {
        const QString err = response.value(QStringLiteral("error")).toString();
        QMessageBox::warning(this, "Register Error",
                             err.isEmpty() ? QStringLiteral("Đăng ký thất bại.") : err);
    }
}
