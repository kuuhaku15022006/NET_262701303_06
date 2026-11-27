#ifndef REGISTERWINDOW_H
#define REGISTERWINDOW_H
#include <QWidget>
#include <QJsonObject>
class QLineEdit;
class TCPClient;
class RegisterWindow : public QWidget
{
    Q_OBJECT
public:
    explicit RegisterWindow(QWidget *parent = nullptr);
    explicit RegisterWindow(TCPClient *client, QWidget *parent = nullptr);
private slots:
    void registerUser();
    void handleServerResponse(const QJsonObject &response);
private:
    QLineEdit *usernameInput;
    QLineEdit *passwordInput;
    QLineEdit *displayNameInput;
    TCPClient *m_client = nullptr;
    bool m_waitingRegister = false;
};
#endif
