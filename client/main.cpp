#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "tcpclient.h"
#include "messagemodel.h"

int main(int argc, char** argv) {
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;

    TcpClient tcp_client;
    MessageModel message_model;
    tcp_client.set_message_model(&message_model);

    engine.rootContext()->setContextProperty("tcp_client", &tcp_client);
    engine.rootContext()->setContextProperty("message_model", &message_model);

    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));
    if (engine.rootObjects().isEmpty()) return -1;
    return app.exec();
}