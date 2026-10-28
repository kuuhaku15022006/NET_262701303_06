
#include "protocol.h"

#include <QJsonDocument>
#include <QJsonParseError>

namespace Protocol
{
    QByteArray encode(const QJsonObject &data)
    {
        return QJsonDocument(data).toJson(
            QJsonDocument::Compact
        );
    }

    bool decode(
        const QByteArray &raw,
        QJsonObject &result,
        QString *error
    )
    {
        QJsonParseError parseError;

        QJsonDocument document =
            QJsonDocument::fromJson(raw, &parseError);

        if (parseError.error != QJsonParseError::NoError) {
            if (error) {
                *error = parseError.errorString();
            }
            return false;
        }

        if (!document.isObject()) {
            if (error) {
                *error = "JSON phải là một object";
            }
            return false;
        }

        result = document.object();
        return true;
    }
}
