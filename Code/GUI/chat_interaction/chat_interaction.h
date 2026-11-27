#ifndef CHAT_INTERACTION_H
#define CHAT_INTERACTION_H

#include <QWidget>
#include <QJsonObject>
#include <QString>
#include <QSet>

class TCPClient;
class QListWidget;
class QListWidgetItem;
class QLabel;
class QLineEdit;
class QPushButton;
class QToolButton;

class ChatInteraction : public QWidget
{
    Q_OBJECT
public:
    explicit ChatInteraction(TCPClient *client,
                             const QString &currentUserId,
                             QWidget *parent = nullptr);

    // conversationId có thể là UUID, vì vậy dùng QString.
    void addConversation(const QString &conversationId,
                         const QString &displayName);
    void clearConversations();
    void setConversationListVisible(bool visible);
    void selectConversationById(const QString &conversationId,
                                const QString &displayName = QString());

private slots:
    void selectConversation(QListWidgetItem *item);
    void sendMessage();
    void receiveMessage(const QJsonObject &message);
    void receiveResponse(const QJsonObject &response);
    void updateConnection(bool connected);
    void showMessageMenu(const QPoint &pos);
    void showEmojiPicker();

private:
    void setupUi();
    void requestHistory();
    void appendMessage(const QJsonObject &message);
    void startReply(const QJsonObject &message);
    void cancelReply();
    void forwardMessage(const QJsonObject &message);
    bool sendJsonRequest(const QString &type,
                         const QJsonObject &payload);
    QString messageId(const QJsonObject &message) const;
    QString conversationId(const QJsonObject &message) const;

    TCPClient *m_client = nullptr;
    QString m_userId;
    QString m_conversationId;
    QString m_replyToId;
    QSet<QString> m_renderedIds;

    QListWidget *m_conversations = nullptr;
    QLabel *m_conversationLabel = nullptr;
    QListWidget *m_messages = nullptr;
    QLabel *m_title = nullptr;
    QLabel *m_status = nullptr;
    QLabel *m_replyPreview = nullptr;
    QLineEdit *m_input = nullptr;
    QPushButton *m_send = nullptr;
    QPushButton *m_cancelReply = nullptr;
    QToolButton *m_emoji = nullptr;
};

#endif
