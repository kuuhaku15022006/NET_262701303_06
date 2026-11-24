#include "chat_interaction.h"
#include "../../client/TCPClient.h"

#include <QAction>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonValue>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>
#include <QDateTime>
#include <QColor>

namespace {
QString valueAsString(const QJsonObject &obj, const QStringList &keys)
{
    for (const QString &key : keys) {
        const QJsonValue v = obj.value(key);
        if (v.isString()) return v.toString();
        if (v.isDouble()) return QString::number(v.toVariant().toLongLong());
    }
    return {};
}

QJsonArray messageArray(const QJsonObject &response)
{
    const QJsonObject payload = response.value("payload").toObject();
    for (const QString &key : {QStringLiteral("messages"),
                               QStringLiteral("history"),
                               QStringLiteral("items")}) {
        if (payload.value(key).isArray()) return payload.value(key).toArray();
        if (response.value(key).isArray()) return response.value(key).toArray();
    }
    const QJsonObject data = payload.value("data").toObject();
    for (const QString &key : {QStringLiteral("messages"),
                               QStringLiteral("history"),
                               QStringLiteral("items")}) {
        if (data.value(key).isArray()) return data.value(key).toArray();
    }
    return {};
}
}

ChatInteraction::ChatInteraction(TCPClient *client,
                                 const QString &currentUserId,
                                 QWidget *parent)
    : QWidget(parent), m_client(client), m_userId(currentUserId)
{
    setupUi();

    if (!m_client) {
        m_status->setText("TCPClient chưa được khởi tạo");
        return;
    }

    connect(m_client, &TCPClient::messageReceived,
            this, &ChatInteraction::receiveMessage);
    connect(m_client, &TCPClient::responseReceived,
            this, &ChatInteraction::receiveResponse);
    connect(m_client, &TCPClient::connectionStateChanged,
            this, &ChatInteraction::updateConnection);
    connect(m_client, &TCPClient::connectionError, this,
            [this](const QString &error) { m_status->setText(error); });

    updateConnection(m_client->isConnected());
}

void ChatInteraction::setupUi()
{
    setWindowTitle("Chat interactions");
    resize(950, 620);

    auto *root = new QHBoxLayout(this);
    auto *left = new QVBoxLayout;
    auto *right = new QVBoxLayout;

    auto *conversationLabel = new QLabel("Conversations");
    m_conversations = new QListWidget;
    m_conversations->setMinimumWidth(200);
    left->addWidget(conversationLabel);
    left->addWidget(m_conversations);

    auto *header = new QHBoxLayout;
    m_title = new QLabel("Select a conversation");
    m_status = new QLabel("Disconnected");
    header->addWidget(m_title, 1);
    header->addWidget(m_status);

    m_messages = new QListWidget;
    m_messages->setWordWrap(true);
    m_messages->setSpacing(5);
    m_messages->setContextMenuPolicy(Qt::CustomContextMenu);

    auto *replyRow = new QHBoxLayout;
    m_replyPreview = new QLabel;
    m_replyPreview->setWordWrap(true);
    m_replyPreview->hide();
    m_cancelReply = new QPushButton("Cancel reply");
    m_cancelReply->hide();
    replyRow->addWidget(m_replyPreview, 1);
    replyRow->addWidget(m_cancelReply);

    auto *composer = new QHBoxLayout;
    m_emoji = new QToolButton;
    m_emoji->setText("😊");
    m_input = new QLineEdit;
    m_input->setPlaceholderText("Type a message...");
    m_send = new QPushButton("Send");
    m_send->setEnabled(false);
    composer->addWidget(m_emoji);
    composer->addWidget(m_input, 1);
    composer->addWidget(m_send);

    right->addLayout(header);
    right->addWidget(m_messages, 1);
    right->addLayout(replyRow);
    right->addLayout(composer);
    root->addLayout(left, 1);
    root->addLayout(right, 3);

    connect(m_conversations, &QListWidget::itemClicked,
            this, &ChatInteraction::selectConversation);
    connect(m_send, &QPushButton::clicked,
            this, &ChatInteraction::sendMessage);
    connect(m_input, &QLineEdit::returnPressed,
            this, &ChatInteraction::sendMessage);
    connect(m_cancelReply, &QPushButton::clicked,
            this, &ChatInteraction::cancelReply);
    connect(m_emoji, &QToolButton::clicked,
            this, &ChatInteraction::showEmojiPicker);
    connect(m_messages, &QListWidget::customContextMenuRequested,
            this, &ChatInteraction::showMessageMenu);
}

void ChatInteraction::addConversation(const QString &id, const QString &name)
{
    if (id.trimmed().isEmpty()) return;
    for (int i = 0; i < m_conversations->count(); ++i) {
        if (m_conversations->item(i)->data(Qt::UserRole).toString() == id)
            return;
    }
    auto *item = new QListWidgetItem(name);
    item->setData(Qt::UserRole, id);
    m_conversations->addItem(item);
}

void ChatInteraction::clearConversations()
{
    m_conversations->clear();
    m_messages->clear();
    m_renderedIds.clear();
    m_conversationId.clear();
    m_title->setText("Select a conversation");
    cancelReply();
}

void ChatInteraction::selectConversation(QListWidgetItem *item)
{
    if (!item) return;
    m_conversationId = item->data(Qt::UserRole).toString();
    m_title->setText(item->text());
    m_messages->clear();
    m_renderedIds.clear();
    cancelReply();
    requestHistory();
}

bool ChatInteraction::sendJsonRequest(const QString &type,
                                      const QJsonObject &payload)
{
    if (!m_client || !m_client->isConnected()) {
        QMessageBox::warning(this, "Disconnected",
                             "Connect to the server before sending.");
        return false;
    }
    QJsonObject request;
    request.insert("type", type);
    request.insert("payload", payload);
    return m_client->sendRequest(request) >= 0;
}

void ChatInteraction::requestHistory()
{
    if (m_conversationId.isEmpty()) return;
    QJsonObject payload;
    payload.insert("conversation_id", m_conversationId);
    payload.insert("limit", 50);
    payload.insert("offset", 0);
    sendJsonRequest("get_history", payload);
}

void ChatInteraction::sendMessage()
{
    const QString text = m_input->text().trimmed();
    if (text.isEmpty()) return;
    if (m_conversationId.isEmpty()) {
        QMessageBox::information(this, "Chat", "Select a conversation first.");
        return;
    }

    QJsonObject payload;
    payload.insert("conversation_id", m_conversationId);
    payload.insert("content", text);
    payload.insert("reply_to", m_replyToId.isEmpty()
                                  ? QJsonValue(QJsonValue::Null)
                                  : QJsonValue(m_replyToId));

    if (sendJsonRequest("send_message", payload)) {
        // Chỉ render khi server broadcast message_received,
        // tránh hiển thị trùng với event từ server.
        m_input->clear();
        cancelReply();
    }
}

QString ChatInteraction::messageId(const QJsonObject &message) const
{
    return valueAsString(message, {"id", "message_id"});
}

QString ChatInteraction::conversationId(const QJsonObject &message) const
{
    return valueAsString(message, {"conversation_id", "conversationId"});
}

void ChatInteraction::receiveMessage(const QJsonObject &message)
{
    const QString convId = conversationId(message);
    if (!convId.isEmpty() && convId != m_conversationId) return;
    appendMessage(message);
}

void ChatInteraction::receiveResponse(const QJsonObject &response)
{
    const QJsonArray messages = messageArray(response);
    if (messages.isEmpty()) return;

    const QString responseConv = valueAsString(
        response.value("payload").toObject(), {"conversation_id"});
    if (!responseConv.isEmpty() && responseConv != m_conversationId) return;

    m_messages->clear();
    m_renderedIds.clear();
    for (const QJsonValue &value : messages)
        if (value.isObject()) appendMessage(value.toObject());
}

void ChatInteraction::appendMessage(const QJsonObject &message)
{
    const QString id = messageId(message);
    if (!id.isEmpty() && m_renderedIds.contains(id)) return;

    const QString convId = conversationId(message);
    if (!convId.isEmpty() && convId != m_conversationId) return;

    const QString sender = valueAsString(message, {"sender_id", "senderId"});
    const QString content = valueAsString(message, {"content", "message"});
    const QString replyTo = valueAsString(message, {"reply_to", "replyTo"});
    const QString forwarded = valueAsString(message, {"forwarded_from", "forwardedFrom"});
    const QString timestamp = valueAsString(message, {"timestamp", "created_at"});
    const bool mine = !m_userId.isEmpty() && sender == m_userId;

    QString display;
    if (!forwarded.isEmpty()) display += "↪ Forwarded\n";
    if (!replyTo.isEmpty()) display += "↳ Reply to: " + replyTo + "\n";
    display += mine ? "You: " : sender + ": ";
    display += content;
    if (!timestamp.isEmpty()) {
        const QDateTime dt = QDateTime::fromString(timestamp, Qt::ISODate);
        display += "\n" + (dt.isValid()
            ? dt.toLocalTime().toString("dd/MM/yyyy HH:mm") : timestamp);
    }

    auto *item = new QListWidgetItem(display);
    item->setData(Qt::UserRole, id);
    item->setData(Qt::UserRole + 1, message);
    item->setTextAlignment(mine ? Qt::AlignRight : Qt::AlignLeft);
    item->setBackground(mine ? QColor("#DDF3DD") : QColor("#F0F0F0"));
    m_messages->addItem(item);
    m_messages->scrollToBottom();
    if (!id.isEmpty()) m_renderedIds.insert(id);
}

void ChatInteraction::showMessageMenu(const QPoint &pos)
{
    QListWidgetItem *item = m_messages->itemAt(pos);
    if (!item) return;
    m_messages->setCurrentItem(item);
    const QJsonObject message = item->data(Qt::UserRole + 1).toJsonObject();

    QMenu menu(this);
    QAction *reply = menu.addAction("Reply");
    QAction *forward = menu.addAction("Forward");
    QAction *chosen = menu.exec(m_messages->viewport()->mapToGlobal(pos));
    if (chosen == reply) startReply(message);
    else if (chosen == forward) forwardMessage(message);
}

void ChatInteraction::startReply(const QJsonObject &message)
{
    const QString id = messageId(message);
    if (id.isEmpty()) {
        QMessageBox::warning(this, "Reply", "This message has no valid ID.");
        return;
    }
    m_replyToId = id;
    m_replyPreview->setText("Replying to: " +
        valueAsString(message, {"content", "message"}).left(160));
    m_replyPreview->show();
    m_cancelReply->show();
    m_input->setFocus();
}

void ChatInteraction::cancelReply()
{
    m_replyToId.clear();
    m_replyPreview->clear();
    m_replyPreview->hide();
    m_cancelReply->hide();
}

void ChatInteraction::forwardMessage(const QJsonObject &message)
{
    const QString id = messageId(message);
    if (id.isEmpty()) {
        QMessageBox::warning(this, "Forward", "This message has no valid ID.");
        return;
    }

    QStringList names, ids;
    for (int i = 0; i < m_conversations->count(); ++i) {
        QListWidgetItem *item = m_conversations->item(i);
        const QString targetId = item->data(Qt::UserRole).toString();
        if (!targetId.isEmpty() && targetId != m_conversationId) {
            names << item->text();
            ids << targetId;
        }
    }
    if (ids.isEmpty()) {
        QMessageBox::information(this, "Forward", "No target conversation available.");
        return;
    }

    bool ok = false;
    const QString selected = QInputDialog::getItem(
        this, "Forward message", "Target conversation:", names, 0, false, &ok);
    if (!ok) return;

    const int index = names.indexOf(selected);
    if (index < 0) return;

    QJsonObject payload;
    payload.insert("message_id", id);
    payload.insert("target_conversation_id", ids.at(index));
    sendJsonRequest("forward_message", payload);
}

void ChatInteraction::showEmojiPicker()
{
    const QStringList emojis = {
        "😀", "😂", "😊", "😍", "🥰", "😎", "😭", "😡",
        "👍", "👎", "❤️", "🔥", "🎉", "👏", "🙏", "💯"
    };
    QMenu menu(this);
    for (const QString &emoji : emojis) {
        QAction *action = menu.addAction(emoji);
        connect(action, &QAction::triggered, this, [this, emoji]() {
            m_input->insert(emoji);
            m_input->setFocus();
        });
    }
    menu.exec(m_emoji->mapToGlobal(QPoint(0, m_emoji->height())));
}

void ChatInteraction::updateConnection(bool connected)
{
    m_status->setText(connected ? "Connected" : "Disconnected");
    m_status->setStyleSheet(connected ? "color: green;" : "color: red;");
    m_send->setEnabled(connected);
}
