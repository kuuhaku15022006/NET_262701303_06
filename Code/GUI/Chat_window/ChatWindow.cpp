
#include "ChatWindow.h"

#include <QLabel>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>

ChatWindow::ChatWindow(QWidget *parent)
    : QWidget(parent)
{
    auto *mainLayout = new QVBoxLayout(this);

    // Chat title
    chatTitle = new QLabel("Chat");
    chatTitle->setStyleSheet(
        "font-size: 18px; font-weight: bold; padding: 10px;"
    );

    // Message display
    messageArea = new QTextEdit(this);
    messageArea->setReadOnly(true);
    messageArea->setPlaceholderText("Messages will appear here...");

    // Message input
    auto *inputLayout = new QHBoxLayout();

    messageInput = new QLineEdit(this);
    messageInput->setPlaceholderText("Type a message...");

    sendButton = new QPushButton("Send", this);

    inputLayout->addWidget(messageInput);
    inputLayout->addWidget(sendButton);

    // Main layout
    mainLayout->addWidget(chatTitle);
    mainLayout->addWidget(messageArea);
    mainLayout->addLayout(inputLayout);
}

void ChatWindow::setChatTitle(const QString &title)
{
    chatTitle->setText(title);
}

QTextEdit* ChatWindow::getMessageArea() const
{
    return messageArea;
}

QLineEdit* ChatWindow::getMessageInput() const
{
    return messageInput;
}

QPushButton* ChatWindow::getSendButton() const
{
    return sendButton;
}
