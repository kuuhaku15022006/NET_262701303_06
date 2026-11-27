
#ifndef CHATWINDOW_H
#define CHATWINDOW_H

#include <QWidget>

class QLabel;
class QTextEdit;
class QLineEdit;
class QPushButton;

class ChatWindow : public QWidget
{
public:
    explicit ChatWindow(QWidget *parent = nullptr);

    void setChatTitle(const QString &title);

    QTextEdit* getMessageArea() const;
    QLineEdit* getMessageInput() const;
    QPushButton* getSendButton() const;

private:
    QLabel *chatTitle;
    QTextEdit *messageArea;
    QLineEdit *messageInput;
    QPushButton *sendButton;
};

#endif
