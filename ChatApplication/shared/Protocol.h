
#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <QByteArray>
#include <QJsonObject>

namespace Protocol
{
    QByteArray encode(const QJsonObject &data);

    bool decode(
        const QByteArray &raw,
        QJsonObject &result,
        QString *error = nullptr
    );
}

#endif
