
#ifndef CHATWINDOW_H
#define CHATWINDOW_H

#include <QWidget>
#include <QJsonObject>

class TCPClient;
class QListWidget;
class QListWidgetItem;
class QLabel;
class QLineEdit;
class QPushButton;
class QToolButton;
class QMenu;

class ChatWindow : public QWidget
{
    Q_OBJECT

public:
    explicit ChatWindow(TCPClient *client,
                        const QString &currentUserId,
                        QWidget *parent = nullptr);

    void setConversations(const QList<QPair<int, QString>> &conversations);

private slots:
    void selectConversation(QListWidgetItem *item);
    void sendMessage();
    void receiveMessage(const QJsonObject &message);
    void handleResponse(const QJsonObject &response);
    void showMessageMenu(const QPoint &position);
    void chooseEmoji();

private:
    void setupUI();
    void loadHistory(int conversationId);
    void displayMessage(const QJsonObject &message);
    void setReplyMessage(const QJsonObject &message);
    void cancelReply();
    void forwardSelectedMessage();
    void addConversation(int id, const QString &name);

    TCPClient *m_client = nullptr;

    QListWidget *m_conversationList = nullptr;
    QListWidget *m_messageList = nullptr;

    QLabel *m_chatTitle = nullptr;
    QLabel *m_connectionStatus = nullptr;
    QLabel *m_replyLabel = nullptr;

    QLineEdit *m_messageInput = nullptr;
    QPushButton *m_sendButton = nullptr;
    QPushButton *m_cancelReplyButton = nullptr;
    QToolButton *m_emojiButton = nullptr;

    int m_currentConversationId = -1;
    int m_replyToMessageId = -1;

    QString m_currentUserId;
};

#endif
