
#ifndef CONNECTIONMANAGER_H
#define CONNECTIONMANAGER_H

#include <QObject>     
#include <QTcpSocket>   
#include <QHash>        
#include <QString>     

class ConnectionManager : public QObject
{
    Q_OBJECT

public:
    explicit ConnectionManager(QObject* parent = nullptr);

    bool addConnection(const QString& userId,
        QTcpSocket* socket);

    void removeConnection(const QString& userId);

    void removeSocket(QTcpSocket* socket);

    bool sendToUser(const QString& userId,
        const QByteArray& message);

    QTcpSocket* getSocket(const QString& userId) const;

    int connectionCount() const;

private:
    QHash<QString, QTcpSocket*> connections;
};

#endif // CONNECTIONMANAGER_H
