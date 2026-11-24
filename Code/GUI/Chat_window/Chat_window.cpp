
#include "Chat_Window.h"

#include "../../client/TCPClient.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QToolButton>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMenu>
#include <QInputDialog>
#include <QMessageBox>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonValue>
#include <QScrollBar>
#include <QFrame>
#include <QFont>

ChatWindow::ChatWindow(
    TCPClient *client,
    const QString &currentUserId,
    QWidget *parent
)
    : QWidget(parent),
      m_client(client),
      m_currentUserId(currentUserId)
{
    setupUI();

    if (!m_client)
        return;

    connect(m_client, &TCPClient::messageReceived,
            this, &ChatWindow::receiveMessage);

    connect(m_client, &TCPClient::responseReceived,
            this, &ChatWindow::handleResponse);

    connect(m_client, &TCPClient::connectionStateChanged,
            this, [this](bool connected) {
        m_connectionStatus->setText(
            connected ? "Connected" : "Disconnected"
        );

        m_connectionStatus->setStyleSheet(
            connected
            ? "color: green;"
            : "color: red;"
        );

        m_sendButton->setEnabled(connected);
    });

    connect(m_client, &TCPClient::connectionError,
            this, [this](const QString &error) {
        QMessageBox::warning(this, "Connection error", error);
    });
}

void ChatWindow::setupUI()
{
    setWindowTitle("TCP Chat");
    resize(1000, 650);

    auto *mainLayout = new QHBoxLayout(this);

    // =========================
    // LEFT: CONVERSATION LIST
    // =========================

    auto *leftPanel = new QVBoxLayout();

    auto *conversationTitle = new QLabel("Conversations");

    m_conversationList = new QListWidget();
    m_conversationList->setMinimumWidth(220);

    leftPanel->addWidget(conversationTitle);
    leftPanel->addWidget(m_conversationList);

    // =========================
    // RIGHT: CHAT AREA
    // =========================

    auto *rightPanel = new QVBoxLayout();

    auto *headerLayout = new QHBoxLayout();

    m_chatTitle = new QLabel("Select a conversation");
    m_chatTitle->setStyleSheet(
        "font-size: 18px; font-weight: bold;"
    );

    m_connectionStatus = new QLabel("Disconnected");
    m_connectionStatus->setStyleSheet("color: red;");

    headerLayout->addWidget(m_chatTitle);
    headerLayout->addStretch();
    headerLayout->addWidget(m_connectionStatus);

    // Message list
    m_messageList = new QListWidget();
    m_messageList->setWordWrap(true);
    m_messageList->setSpacing(6);

    m_messageList->setContextMenuPolicy(
        Qt::CustomContextMenu
    );

    connect(
        m_messageList,
        &QListWidget::customContextMenuRequested,
        this,
        &ChatWindow::showMessageMenu
    );

    // =========================
    // REPLY BAR
    // =========================

    auto *replyLayout = new QHBoxLayout();

    m_replyLabel = new QLabel();
    m_replyLabel->setWordWrap(true);
    m_replyLabel->hide();

    m_cancelReplyButton = new QPushButton("Cancel");
    m_cancelReplyButton->hide();

    replyLayout->addWidget(m_replyLabel, 1);
    replyLayout->addWidget(m_cancelReplyButton);

    connect(
        m_cancelReplyButton,
        &QPushButton::clicked,
        this,
        &ChatWindow::cancelReply
    );

    // =========================
    // MESSAGE INPUT
    // =========================

    auto *inputLayout = new QHBoxLayout();

    m_emojiButton = new QToolButton();
    m_emojiButton->setText("😊");
    m_emojiButton->setToolTip("Choose emoji");

    m_messageInput = new QLineEdit();
    m_messageInput->setPlaceholderText(
        "Type a message..."
    );

    m_sendButton = new QPushButton("Send");

    inputLayout->addWidget(m_emojiButton);
    inputLayout->addWidget(m_messageInput, 1);
    inputLayout->addWidget(m_sendButton);

    connect(
        m_sendButton,
        &QPushButton::clicked,
        this,
        &ChatWindow::sendMessage
    );

    connect(
        m_messageInput,
        &QLineEdit::returnPressed,
        this,
        &ChatWindow::sendMessage
    );

    connect(
        m_emojiButton,
        &QToolButton::clicked,
        this,
        &ChatWindow::chooseEmoji
    );

    // =========================
    // COMBINE LAYOUTS
    // =========================

    rightPanel->addLayout(headerLayout);
    rightPanel->addWidget(m_messageList, 1);
    rightPanel->addLayout(replyLayout);
    rightPanel->addLayout(inputLayout);

    mainLayout->addLayout(leftPanel, 1);
    mainLayout->addLayout(rightPanel, 3);

    m_sendButton->setEnabled(false);

    connect(
        m_conversationList,
        &QListWidget::itemClicked,
        this,
        &ChatWindow::selectConversation
    );
}

// Add a conversation to the left panel
void ChatWindow::addConversation(
    int id,
    const QString &name
)
{
    auto *item = new QListWidgetItem(name);

    item->setData(Qt::UserRole, id);

    m_conversationList->addItem(item);
}

void ChatWindow::setConversations(
    const QList<QPair<int, QString>> &conversations
)
{
    m_conversationList->clear();

    for (const auto &conversation : conversations) {
        addConversation(
            conversation.first,
            conversation.second
        );
    }
}

// Select a conversation
void ChatWindow::selectConversation(
    QListWidgetItem *item
)
{
    if (!item)
        return;

    m_currentConversationId =
        item->data(Qt::UserRole).toInt();

    m_chatTitle->setText(item->text());

    m_messageList->clear();
    cancelReply();

    loadHistory(m_currentConversationId);
}

// Request conversation history from server
void ChatWindow::loadHistory(int conversationId)
{
    if (!m_client || !m_client->isConnected())
        return;

    m_client->getHistory(conversationId, 50, 0);
}

// Send a message
void ChatWindow::sendMessage()
{
    if (!m_client || !m_client->isConnected()) {
        QMessageBox::warning(
            this,
            "Error",
            "You are not connected to the server."
        );
        return;
    }

    if (m_currentConversationId < 0) {
        QMessageBox::information(
            this,
            "Information",
            "Please select a conversation first."
        );
        return;
    }

    const QString content =
        m_messageInput->text().trimmed();

    if (content.isEmpty())
        return;

    // Send message or reply through existing TCPClient API
    m_client->sendMessage(
        m_currentConversationId,
        content,
        m_replyToMessageId
    );

    m_messageInput->clear();
    cancelReply();
}

// Receive a new message from TCPClient
void ChatWindow::receiveMessage(
    const QJsonObject &message
)
{
    const int conversationId =
        message.value("conversation_id").toInt(-1);

    // Ignore messages belonging to another conversation
    if (conversationId != m_currentConversationId)
        return;

    displayMessage(message);
}

// Display one message
void ChatWindow::displayMessage(
    const QJsonObject &message
)
{
    const QString messageId =
        message.value("id").toString();

    const QString senderId =
        message.value("sender_id").toString();

    const QString content =
        message.value("content").toString();

    const QString replyTo =
        message.value("reply_to").toString();

    const QString forwardedFrom =
        message.value("forwarded_from").toString();

    const QString timestamp =
        message.value("timestamp").toString();

    const bool isMine =
        senderId == m_currentUserId;

    QString displayText;

    if (!forwardedFrom.isEmpty()) {
        displayText += "[Forwarded message]\n";
    }

    if (!replyTo.isEmpty()) {
        displayText += "[Reply to message: "
                    + replyTo + "]\n";
    }

    displayText += isMine ? "You: " : senderId + ": ";
    displayText += content;

    if (!timestamp.isEmpty()) {
        displayText += "\n" + timestamp;
    }

    auto *item = new QListWidgetItem(displayText);

    // Save message ID for Reply and Forward
    bool ok = false;
    const int numericId = messageId.toInt(&ok);

    item->setData(Qt::UserRole, ok ? numericId : -1);
    item->setData(Qt::UserRole + 1, message);

    if (isMine) {
        item->setTextAlignment(Qt::AlignRight);
        item->setBackground(QColor("#D9FDD3"));
    } else {
        item->setTextAlignment(Qt::AlignLeft);
        item->setBackground(QColor("#FFFFFF"));
    }

    m_messageList->addItem(item);
    m_messageList->scrollToBottom();
}

// Handle response from server
void ChatWindow::handleResponse(
    const QJsonObject &response
)
{
    const QJsonObject payload =
        response.value("payload").toObject();

    // Expected format:
    // {
    //   "type": "response",
    //   "payload": {
    //      "messages": [...]
    //   }
    // }

    const QJsonArray messages =
        payload.value("messages").toArray();

    if (messages.isEmpty())
        return;

    m_messageList->clear();

    for (const QJsonValue &value : messages) {
        if (value.isObject())
            displayMessage(value.toObject());
    }
}

// Right-click on a message
void ChatWindow::showMessageMenu(
    const QPoint &position
)
{
    QListWidgetItem *item =
        m_messageList->itemAt(position);

    if (!item)
        return;

    QMenu menu(this);

    QAction *replyAction =
        menu.addAction("Reply");

    QAction *forwardAction =
        menu.addAction("Forward");

    QAction *selectedAction =
        menu.exec(
            m_messageList->viewport()->mapToGlobal(position)
        );

    const QJsonObject message =
        item->data(Qt::UserRole + 1).toJsonObject();

    if (selectedAction == replyAction) {
        setReplyMessage(message);
    }
    else if (selectedAction == forwardAction) {
        forwardSelectedMessage();
    }
}

// Set the message being replied to
void ChatWindow::setReplyMessage(
    const QJsonObject &message
)
{
    const QString messageId =
        message.value("id").toString();

    bool ok = false;
    const int id = messageId.toInt(&ok);

    if (!ok || id <= 0) {
        QMessageBox::warning(
            this,
            "Reply",
            "Message ID is not compatible with the current TCPClient API."
        );
        return;
    }

    m_replyToMessageId = id;

    const QString content =
        message.value("content").toString();

    m_replyLabel->setText(
        "Replying to: " + content
    );

    m_replyLabel->show();
    m_cancelReplyButton->show();

    m_messageInput->setFocus();
}

// Cancel reply mode
void ChatWindow::cancelReply()
{
    m_replyToMessageId = -1;

    m_replyLabel->clear();
    m_replyLabel->hide();

    m_cancelReplyButton->hide();
}

// Forward a selected message
void ChatWindow::forwardSelectedMessage()
{
    QListWidgetItem *item =
        m_messageList->currentItem();

    if (!item)
        return;

    const int messageId =
        item->data(Qt::UserRole).toInt();

    if (messageId <= 0) {
        QMessageBox::warning(
            this,
            "Forward",
            "Invalid message ID."
        );
        return;
    }

    bool ok = false;

    const int targetConversationId =
        QInputDialog::getInt(
            this,
            "Forward message",
            "Target conversation ID:",
            1,
            1,
            999999,
            1,
            &ok
        );

    if (!ok)
        return;

    if (!m_client || !m_client->isConnected())
        return;

    m_client->forwardMessage(
        messageId,
        targetConversationId
    );

    QMessageBox::information(
        this,
        "Forward",
        "Forward request has been sent."
    );
}

// Simple emoji picker
void ChatWindow::chooseEmoji()
{
    QMenu menu(this);

    const QStringList emojis = {
        "😀", "😃", "😄", "😁",
        "😂", "🤣", "😊", "😍",
        "🥰", "😎", "😭", "😡",
        "👍", "👎", "❤️", "🔥",
        "🎉", "👏", "🙏", "💯"
    };

    for (const QString &emoji : emojis) {
        QAction *action = menu.addAction(emoji);

        connect(
            action,
            &QAction::triggered,
            this,
            [this, emoji]() {
                m_messageInput->insert(emoji);
                m_messageInput->setFocus();
            }
        );
    }

    menu.exec(
        m_emojiButton->mapToGlobal(
            QPoint(0, m_emojiButton->height())
        )
    );
}
