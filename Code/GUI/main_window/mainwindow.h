#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QMainWindow>
#include <QJsonObject>
class QListWidget;
class QListWidgetItem;
class QLineEdit;
class TCPClient;
class ChatInteraction;
class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    explicit MainWindow(TCPClient *client,
                        const QString &userId,
                        const QString &username = QString(),
                        QWidget *parent = nullptr);
signals:
    void conversationSelected(int conversationId);
private slots:
    void selectConversation(QListWidgetItem *item);
    void sendMessage();
    void handleServerResponse(const QJsonObject &response);
private:
    QListWidget *conversationList;
    QLineEdit *messageInput;
    TCPClient *m_client = nullptr;
    ChatInteraction *m_chatWidget = nullptr;
    QString m_userId;
    QString m_username;
};
#endif
