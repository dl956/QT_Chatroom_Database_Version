#include "messagemodel.h"
#include <QDebug>

MessageModel::MessageModel(QObject* parent) : QAbstractListModel(parent) {}

int MessageModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return items_.size();
}

QVariant MessageModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid()) return {};
    const ChatMessageItem& chat_item = items_.at(index.row());
    switch (role) {
    case SenderRole: return chat_item.sender;
    case TextRole:   return chat_item.text;
    case TimeRole:   return chat_item.time.toString(Qt::ISODate);
    }
    return {};
}

QHash<int, QByteArray> MessageModel::roleNames() const {
    QHash<int, QByteArray> role_name_hash;
    role_name_hash[SenderRole] = "sender";
    role_name_hash[TextRole] = "text";
    role_name_hash[TimeRole] = "time";
    return role_name_hash;
}

void MessageModel::add_message(const QString& sender, const QString& text, const QDateTime& time) {
    qDebug() << "[MessageModel] add_message sender=" << sender << "text=" << text << "time=" << time.toString();
    beginInsertRows(QModelIndex(), items_.size(), items_.size());
    items_.append({sender, text, time});
    endInsertRows();
    qDebug() << "[MessageModel] new rowCount=" << items_.size();
}