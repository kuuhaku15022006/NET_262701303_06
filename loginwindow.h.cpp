#ifndef LOGINWINDOW_H
#define LOGINWINDOW_H

#include <QWidget>

class QLineEdit;

class LoginWindow : public QWidget
{
    Q_OBJECT

public:
    explicit LoginWindow(QWidget* parent = nullptr);

private:
    QLineEdit* usernameInput;
    QLineEdit* passwordInput;

private slots:
    void login();
};

#endif