void sendMessage(const QJsonObject &message);

signals:
    void messageReceived(const QJsonObject &message);