#pragma once
#include <QObject>
#include <QTcpSocket>
#include <QTimer>
#include <QJsonObject>
#include <QStringList>

class MessageModel;

class TcpClient : public QObject {
    Q_OBJECT
public:
    explicit TcpClient(QObject* parent = nullptr);
    Q_INVOKABLE void connect_to_host(const QString& host, quint16 port);
    Q_INVOKABLE void disconnect_from_host();
    Q_INVOKABLE void send_json(const QJsonObject& json_object);
    void set_message_model(MessageModel* model) { message_model_ = model; }

signals:
    void connected();
    void disconnected();
    void error_occurred(const QString&);

    void login_succeeded(const QString& username);
    void login_failed(const QString& reason);
    void register_succeeded();
    void register_failed(const QString& reason);

    void online_users_updated(const QStringList& users);
    void message_received(const QString& from, const QString& text, qint64 timestamp);

private slots:
    void on_ready_read();
    void on_connected();
    void on_disconnected();
    void on_error_occurred(QAbstractSocket::SocketError);

    void send_heartbeat();

private:
    void process_frame(const QByteArray& payload);
    QTcpSocket socket_;
    QByteArray buffer_;
    MessageModel* message_model_ = nullptr;
    QTimer heartbeat_timer_;
};