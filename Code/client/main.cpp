
#include <QApplication>
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QTextEdit>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QWidget window;
    window.setWindowTitle("Chat Application");
    window.resize(1000, 650);

    auto *mainLayout = new QHBoxLayout(&window);

    // Left panel: contacts
    auto *contactPanel = new QVBoxLayout();

    auto *contactTitle = new QLabel("Contacts");
    auto *contactList = new QListWidget();

    contactList->addItem("🙂 Alice");
    contactList->addItem("🧑 Bob");
    contactList->addItem("👤 Charlie");

    contactPanel->addWidget(contactTitle);
    contactPanel->addWidget(contactList);

    // Right panel: chat
    auto *chatPanel = new QVBoxLayout();

    auto *chatTitle = new QLabel("Chat with Alice");
    auto *messageArea = new QTextEdit();
    messageArea->setReadOnly(true);

    auto *inputLayout = new QHBoxLayout();
    auto *messageInput = new QLineEdit();
    auto *sendButton = new QPushButton("Send");

    messageInput->setPlaceholderText("Type a message...");

    inputLayout->addWidget(messageInput);
    inputLayout->addWidget(sendButton);

    chatPanel->addWidget(chatTitle);
    chatPanel->addWidget(messageArea);
    chatPanel->addLayout(inputLayout);

    mainLayout->addLayout(contactPanel, 1);
    mainLayout->addLayout(chatPanel, 3);

    // Demonstration only: append message locally
    QObject::connect(sendButton, &QPushButton::clicked,
                     [&]() {
        QString text = messageInput->text().trimmed();

        if (!text.isEmpty()) {
            messageArea->append("You: " + text);
            messageInput->clear();
        }
    });

    QObject::connect(messageInput, &QLineEdit::returnPressed,
                     sendButton, &QPushButton::click);

    window.show();

    return app.exec();
}
