#ifndef SOCKS5TABLE_H
#define SOCKS5TABLE_H

#include <map>
#include <QAbstractTableModel>
#include <QTimer>

class Package
{
public:
    int fd;
    int time;
    QString domain;
    QString ip;
    int port;
    int sended;
    int recved;
};

Package FromJSon(const QString str);

class Connections
{
public:
    std::map<int, Package> con_map_;
};

class Socks5Table : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit Socks5Table(Connections& conns, QObject *parent = nullptr);

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    // Add data:
    bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
    bool insertColumns(int column, int count, const QModelIndex &parent = QModelIndex()) override;

    // Remove data:
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
    bool removeColumns(int column, int count, const QModelIndex &parent = QModelIndex()) override;
private:
    QString FormatTime(int t) const;
private:
    Connections& conns_;
    int time_seconds_; // 秒数
    QTimer* timer_;
};

#endif // SOCKS5TABLE_H
