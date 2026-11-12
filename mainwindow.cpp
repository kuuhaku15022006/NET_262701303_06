#include "mainwindow.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setWindowTitle("Chat TCP");
    resize(900, 600);
    auto *central = new QWidget(this);
    setCentralWidget(central);
    auto *mainLayout = new QHBoxLayout(central);

    auto *left = new QVBoxLayout;
    left->addWidget(new QLabel("Conversations"));
    conversationList = new QListWidget;
    auto *c1 = new QListWidgetItem("Conversation 1");
    c1->setData(Qt::UserRole, 1);
    auto *c2 = new QListWidgetItem("Conversation 2");
    c2->setData(Qt::UserRole, 2);
    conversationList->addItem(c1);
    conversationList->addItem(c2);
    left->addWidget(conversationList);

    auto *right = new QVBoxLayout;
    right->addWidget(new QLabel("Avatar"));
    right->addWidget(new QLabel("Sample User"));
    auto *messages = new QTextEdit;
    messages->setReadOnly(true);
    messages->setPlaceholderText("Message history will appear here.");
    right->addWidget(messages, 1);

    auto *input = new QHBoxLayout;
    messageInput = new QLineEdit;
    messageInput->setPlaceholderText("Type a message...");
    auto *sendButton = new QPushButton("Send");
    input->addWidget(messageInput);
    input->addWidget(sendButton);
    right->addLayout(input);

    mainLayout->addLayout(left, 1);
    mainLayout->addLayout(right, 3);

    connect(conversationList, &QListWidget::itemClicked,
            this, &MainWindow::selectConversation);
    connect(sendButton, &QPushButton::clicked,
            this, &MainWindow::sendMessage);
    connect(messageInput, &QLineEdit::returnPressed,
            this, &MainWindow::sendMessage);
}

void MainWindow::selectConversation(QListWidgetItem *item)
{
    const int id = item->data(Qt::UserRole).toInt();
    emit conversationSelected(id);
    // TODO: NV4 loads conversation history.
}

void MainWindow::sendMessage()
{
    if (messageInput->text().trimmed().isEmpty())
        return;
    // TODO: NV6 sends the message through NV4 TCP Client.
    messageInput->clear();
}
