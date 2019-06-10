#include <QJsonDocument>
#include <QJsonObject>
#include <algorithm>
#include <iterator>
#include <QDebug>
#include <chrono>
#include "socks5table.h"

Package FromJSon(const QString str)
{
    QJsonDocument document = QJsonDocument::fromJson(str.toUtf8());
    QJsonObject data = document.object();
    Package p;
    p.fd = data["fd"].toInt();
    p.time = data["start_time"].toInt();
    p.domain = data["domain"].toString();
    p.ip = data["ip"].toString();
    p.port = data["port"].toInt();
    p.sended = data["sended"].toInt();
    p.recved = data["recved"].toInt();
    return p;
}

Socks5Table::Socks5Table(Connections& conns, QObject *parent) : QAbstractTableModel(parent),
    conns_(conns)
{
    time_seconds_ = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    timer_ = new QTimer(this);
    connect(timer_, &QTimer::timeout, [this]() {
        time_seconds_ = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count();

        auto it = conns_.con_map_.begin();
        for(int i = 0; it != conns_.con_map_.end(); ++it, ++i)
        {
            QModelIndex index = this->index(i, 1, QModelIndex());
            this->dataChanged(index, index);
        }
    });
    timer_->start(1000);
}

QVariant Socks5Table::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(role == Qt::DisplayRole)
    {
        if(orientation != Qt::Horizontal)
            return QVariant();

        if(section == 0)
            return QString("fd");
        else if(section == 1)
            return QString("time");
        else if(section == 2)
            return QString("domain");
        else if(section == 3)
            return QString("ip");
        else if(section == 4)
            return QString("port");
        else if(section == 5)
            return QString("sended");
        else if(section == 6)
            return QString("recved");
    }

    return QVariant();
}

int Socks5Table::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return conns_.con_map_.size();
}

int Socks5Table::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return 7;
}

QVariant Socks5Table::data(const QModelIndex &index, int role) const
{
    if(role == Qt::DisplayRole)
    {
        const int row = index.row();
        const int col = index.column();

        if(row + 1 > int(conns_.con_map_.size()))
            return QVariant();

        if(col >= 7)
            return QVariant();

        auto it = conns_.con_map_.begin();
        std::advance(it, row);

        Package& p = it->second;
        if(col == 0)
            return p.fd;
        else if(col == 1)
        {
            return FormatTime(p.time);
        }
        else if(col == 2)
            return p.domain;
        else if(col == 3)
            return p.ip;
        else if(col == 4)
            return p.port;
        else if(col == 5)
            return p.sended;
        else if(col ==6)
            return p.recved;
    }

    return QVariant();
}

bool Socks5Table::insertRows(int row, int count, const QModelIndex &parent)
{
    beginInsertRows(parent, row, row + count - 1);

    endInsertRows();
    return true;
}

bool Socks5Table::insertColumns(int column, int count, const QModelIndex &parent)
{
    beginInsertColumns(parent, column, column + count - 1);

    endInsertColumns();
    return true;
}

bool Socks5Table::removeRows(int row, int count, const QModelIndex &parent)
{
    beginRemoveRows(parent, row, row + count - 1);

    endRemoveRows();
    return true;
}

bool Socks5Table::removeColumns(int column, int count, const QModelIndex &parent)
{
    beginRemoveColumns(parent, column, column + count - 1);

    endRemoveColumns();
    return true;
}

QString Socks5Table::FormatTime(int t) const
{
    int d = time_seconds_ - t;
    if(d <= 0)
    {
        return QString("00:00");
    }

    int sec = d % 60;
    int min = d / 60;
    if(min > 60)
    {
        min = min % 60;
        int h = min / 60;
        QString str;
        str.sprintf("%02d:%02d:%02d", h, min, sec);
        return str;
    }
    QString str;
    str.sprintf("%02d:%02d", min, sec);
    return str;
}
