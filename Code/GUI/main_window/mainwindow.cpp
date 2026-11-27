#include "mainwindow.h"
#include "../chat_interaction/chat_interaction.h"
#include "../../client/TCPClient.h"
#include <QHBoxLayout>
#include <QJsonArray>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(QWidget *parent)
    : MainWindow(nullptr, QString(), QString(), parent)
{
}

MainWindow::MainWindow(TCPClient *client,
                       const QString &userId,
                       const QString &username,
                       QWidget *parent)
    : QMainWindow(parent)
    , m_client(client)
    , m_userId(userId)
    , m_username(username)
{
    setWindowTitle(m_username.isEmpty()
                   ? QStringLiteral("Chat TCP")
                   : QStringLiteral("Chat TCP - ") + m_username);
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
    right->addWidget(new QLabel(m_username.isEmpty() ? QStringLiteral("Sample User") : QStringLiteral("User: ") + m_username));
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

    if (m_client) {
        messages->hide();
        messageInput->hide();
        sendButton->hide();

        m_chatWidget = new ChatInteraction(m_client, m_userId, this);
        m_chatWidget->setConversationListVisible(false);
        right->addWidget(m_chatWidget, 1);

        connect(m_client, &TCPClient::responseReceived,
                this, &MainWindow::handleServerResponse);
        if (m_client->isConnected()) {
            m_client->getConversations();
        }
    }

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
    if (m_chatWidget) {
        m_chatWidget->selectConversationById(item->data(Qt::UserRole).toString(), item->text());
    }
}

void MainWindow::sendMessage()
{
    if (messageInput->text().trimmed().isEmpty())
        return;
    // TODO: NV6 sends the message through NV4 TCP Client.
    messageInput->clear();
}

void MainWindow::handleServerResponse(const QJsonObject &response)
{
    if (response.value(QStringLiteral("request_type")).toString() != QStringLiteral("get_conversations")
        || !response.value(QStringLiteral("ok")).toBool())
        return;

    const QJsonArray arr = response.value(QStringLiteral("payload")).toObject()
                               .value(QStringLiteral("conversations")).toArray();
    if (arr.isEmpty())
        return;

    conversationList->clear();
    if (m_chatWidget)
        m_chatWidget->clearConversations();

    for (const QJsonValue &val : arr) {
        const QJsonObject cObj = val.toObject();
        const int id = cObj.value(QStringLiteral("id")).toInt();
        const QString idStr = QString::number(id);
        const QString name = cObj.value(QStringLiteral("name")).toString();

        auto *item = new QListWidgetItem(name);
        item->setData(Qt::UserRole, id);
        conversationList->addItem(item);

        if (m_chatWidget)
            m_chatWidget->addConversation(idStr, name);
    }

    if (conversationList->count() > 0) {
        conversationList->setCurrentRow(0);
        selectConversation(conversationList->item(0));
    }
}
