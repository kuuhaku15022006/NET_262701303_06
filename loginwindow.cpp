#include "loginwindow.h"
#include "registerwindow.h"
#include "mainwindow.h"
#include <QFormLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>
#include <QMessageBox>

LoginWindow::LoginWindow(QWidget *parent) : QWidget(parent)
{
    setWindowTitle("Chat TCP - Login");
    setMinimumSize(360, 260);
    auto *layout = new QVBoxLayout(this);
    auto *title = new QLabel("CHAT TCP");
    title->setAlignment(Qt::AlignCenter);
    auto *form = new QFormLayout;
    usernameInput = new QLineEdit(this);
    usernameInput->setPlaceholderText("Username");
    passwordInput = new QLineEdit(this);
    passwordInput->setPlaceholderText("Password");
    passwordInput->setEchoMode(QLineEdit::Password);
    form->addRow("Username:", usernameInput);
    form->addRow("Password:", passwordInput);
    auto *loginButton = new QPushButton("Login", this);
    auto *registerButton = new QPushButton("Register", this);
    layout->addWidget(title);
    layout->addLayout(form);
    layout->addWidget(loginButton);
    layout->addWidget(registerButton);
    connect(loginButton, &QPushButton::clicked, this, &LoginWindow::login);
    connect(registerButton, &QPushButton::clicked, this, &LoginWindow::openRegister);
}

void LoginWindow::login()
{
    if (usernameInput->text().trimmed().isEmpty() || passwordInput->text().isEmpty()) {
        QMessageBox::warning(this, "Login Error", "Please enter username and password.");
        return;
    }
    // TODO: Connect to NV4 TCP Client and authenticate with server.
    auto *mainWindow = new MainWindow;
    mainWindow->show();
    close();
}

void LoginWindow::openRegister()
{
    auto *window = new RegisterWindow(this);
    window->setAttribute(Qt::WA_DeleteOnClose);
    window->show();
}
