#include "tcpclient.h"
#include "messagemodel.h"
#include <QAbstractSocket>
#include <QJsonDocument>
#include <QDateTime>
#include <QDataStream>
#include <QJsonArray>
#include <QStringList>
#include <QDebug>

static QString g_current_user;

TcpClient::TcpClient(QObject* parent) : QObject(parent) {
    connect(&socket_, &QTcpSocket::readyRead, this, &TcpClient::on_ready_read);
    connect(&socket_, &QTcpSocket::connected, this, &TcpClient::on_connected);
    connect(&socket_, &QTcpSocket::disconnected, this, &TcpClient::on_disconnected);
    connect(&socket_, &QTcpSocket::errorOccurred, this, &TcpClient::on_error_occurred);

    heartbeat_timer_.setInterval(10000);
    connect(&heartbeat_timer_, &QTimer::timeout, this, &TcpClient::send_heartbeat);
}

void TcpClient::connect_to_host(const QString& host, quint16 port) {
    if (socket_.state() == QAbstractSocket::ConnectedState) socket_.disconnectFromHost();
    socket_.connectToHost(host, port);
}

void TcpClient::disconnect_from_host() {
    if (socket_.state() == QAbstractSocket::ConnectedState) {
        socket_.disconnectFromHost();
    }
}

void TcpClient::on_connected() {
    heartbeat_timer_.start();
    emit connected();
}

void TcpClient::on_disconnected() {
    heartbeat_timer_.stop();
    emit disconnected();
    g_current_user.clear();
}

void TcpClient::on_error_occurred(QAbstractSocket::SocketError socket_error) {
    Q_UNUSED(socket_error);
    emit error_occurred(socket_.errorString());
}

void TcpClient::send_json(const QJsonObject& json_object) {
    QString json_type = json_object.value("type").toString();
    if (json_type == "message") {
        QString text = json_object.value("text").toString();
        if (message_model_) message_model_->add_message("me", text, QDateTime::currentDateTime());
    } else if (json_type == "login") {
        g_current_user = json_object.value("username").toString();
    } else if (json_type == "logout") {
        g_current_user.clear();
    }

    QJsonDocument doc(json_object);
    QByteArray payload = doc.toJson(QJsonDocument::Compact);
    QByteArray frame;
    QDataStream ds(&frame, QIODevice::WriteOnly);
    ds.setByteOrder(QDataStream::BigEndian);
    ds << static_cast<quint32>(payload.size());
    frame.append(payload);
    socket_.write(frame);
}

void TcpClient::on_ready_read() {
    QByteArray chunk = socket_.readAll();
    buffer_.append(chunk);
    while (buffer_.size() >= 4) {
        QDataStream ds(buffer_);
        ds.setByteOrder(QDataStream::BigEndian);
        quint32 len = 0;
        ds >> len;
        if (buffer_.size() < 4 + static_cast<int>(len)) break;
        QByteArray payload = buffer_.mid(4, len);
        buffer_.remove(0, 4 + len);
        process_frame(payload);
    }
}

void TcpClient::process_frame(const QByteArray& payload) {
    QJsonDocument doc = QJsonDocument::fromJson(payload);
    if (!doc.isObject()) return;
    QJsonObject json_obj = doc.object();
    QString type = json_obj.value("type").toString();

    if (type == "message" || type == "private") {
        QString from = json_obj.value("from").toString();
        QString text = json_obj.value("text").toString();
        qint64 timestamp = json_obj.value("ts").toVariant().toLongLong();
        QDateTime datetime = QDateTime::fromMSecsSinceEpoch(timestamp ? timestamp : QDateTime::currentMSecsSinceEpoch());
        if (from != g_current_user && message_model_) message_model_->add_message(from, text, datetime);
        emit message_received(from, text, datetime.toMSecsSinceEpoch());
    } else if (type == "login_result" || type == "register_result") {
        bool ok = json_obj.value("ok").toBool();
        QString reason = json_obj.value("reason").toString();
        QString username = json_obj.value("username").toString();
        if (type == "login_result") {
            if (ok) {
                emit login_succeeded(g_current_user.isEmpty() ? username : g_current_user);
            } else {
                emit login_failed(reason);
            }
        } else {
            if (ok) emit register_succeeded();
            else emit register_failed(reason);
        }
    } else if (type == "pong") {
        // Ignore
    } else if (json_obj.contains("users") && json_obj.value("users").isArray()) {
        QJsonArray arr = json_obj.value("users").toArray();
        QStringList users_list;
        for (const QJsonValue& v : arr) users_list << v.toString();
        emit online_users_updated(users_list);
    }
}

void TcpClient::send_heartbeat() {
    QJsonObject j;
    j["type"] = "heartbeat";
    send_json(j);
}