QJsonObject resp;
resp["request_id"] = req["request_id"];
resp["status"] = "ok";
resp["type"] = req["type"];

QJsonObject ev;
ev["type"] = "new_message";
ev["content"] = "xin chao tu server";

// Hai dòng JSON gửi dính nhau trong một lần write
QByteArray data = QJsonDocument(resp).toJson(QJsonDocument::Compact) + "\n"
                + QJsonDocument(ev).toJson(QJsonDocument::Compact) + "\n";
socket->write(data);