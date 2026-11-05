#include "loginwindow.h"
#include "mainwindow.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QMessageBox>

LoginWindow::LoginWindow(QWidget* parent)
    : QWidget(parent)
{
    setWindowTitle("Chat TCP - Login");
    resize(350, 250);

    auto* layout = new QVBoxLayout(this);

    auto* title = new QLabel("CHAT TCP - LOGIN");
    title->setAlignment(Qt::AlignCenter);

    usernameInput = new QLineEdit(this);
    usernameInput->setPlaceholderText("Username");

    passwordInput = new QLineEdit(this);
    passwordInput->setPlaceholderText("Password");
    passwordInput->setEchoMode(QLineEdit::Password);

    auto* loginButton = new QPushButton("Login", this);

    layout->addWidget(title);
    layout->addWidget(usernameInput);
    layout->addWidget(passwordInput);
    layout->addWidget(loginButton);

    connect(loginButton, &QPushButton::clicked,
        this, &LoginWindow::login);
}

void LoginWindow::login()
{
    if (usernameInput->text().trimmed().isEmpty()
        || passwordInput->text().isEmpty()) {
        QMessageBox::warning(
            this, "Login Error",
            "Please enter username and password."
        );
        return;
    }

    // TODO: Integrate with NV4 TCP Client.
    auto* window = new MainWindow;
    window->show();
    close();
}