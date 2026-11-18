#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QMainWindow>
class QListWidget;
class QListWidgetItem;
class QLineEdit;
class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
signals:
    void conversationSelected(int conversationId);
private slots:
    void selectConversation(QListWidgetItem *item);
    void sendMessage();
private:
    QListWidget *conversationList;
    QLineEdit *messageInput;
};
#endif
