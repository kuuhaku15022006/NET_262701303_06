#include "registerwindow.h"
#include <QFormLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>
#include <QMessageBox>

RegisterWindow::RegisterWindow(QWidget *parent) : QWidget(parent)
{
    setWindowTitle("Chat TCP - Register");
    setMinimumSize(360, 300);
    auto *layout = new QVBoxLayout(this);
    auto *title = new QLabel("CREATE ACCOUNT");
    title->setAlignment(Qt::AlignCenter);
    auto *form = new QFormLayout;
    usernameInput = new QLineEdit(this);
    usernameInput->setPlaceholderText("Username");
    passwordInput = new QLineEdit(this);
    passwordInput->setPlaceholderText("Password");
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
    QMessageBox::information(this, "Register",
                             "Registration request is ready for server integration.");
}
