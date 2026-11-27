#ifndef LOGINWINDOW_H
#define LOGINWINDOW_H
#include <QWidget>
#include <QJsonObject>
class QLineEdit;
class TCPClient;
class LoginWindow : public QWidget
{
    Q_OBJECT
public:
    explicit LoginWindow(QWidget *parent = nullptr);
private slots:
    void login();
    void openRegister();
    void handleServerResponse(const QJsonObject &response);
private:
    QLineEdit *usernameInput;
    QLineEdit *passwordInput;
    TCPClient *m_client = nullptr;
    bool m_waitingLogin = false;
};
#endif
