#ifndef REGISTERWINDOW_H
#define REGISTERWINDOW_H
#include <QWidget>
class QLineEdit;
class RegisterWindow : public QWidget
{
    Q_OBJECT
public:
    explicit RegisterWindow(QWidget *parent = nullptr);
private slots:
    void registerUser();
private:
    QLineEdit *usernameInput;
    QLineEdit *passwordInput;
    QLineEdit *displayNameInput;
};
#endif
