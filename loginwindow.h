#ifndef LOGINWINDOW_H
#define LOGINWINDOW_H
#include <QWidget>
class QLineEdit;
class LoginWindow : public QWidget
{
    Q_OBJECT
public:
    explicit LoginWindow(QWidget *parent = nullptr);
private slots:
    void login();
    void openRegister();
private:
    QLineEdit *usernameInput;
    QLineEdit *passwordInput;
};
#endif
