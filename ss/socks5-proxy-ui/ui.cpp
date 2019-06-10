#include <chrono>
#include "ui.h"
#include "ui_ui.h"

UI::UI(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::UI)
{
    ui->setupUi(this);
    QStringList headers;
    model_ = new Socks5Table(conns_);
    ui->tableView->setModel(model_);
    ui->tableView->setColumnWidth(2, 250);
    ui->tableView->setColumnWidth(3, 150);
    socket_ = new QUdpSocket(this);
    socket_->bind(QHostAddress::LocalHost, 12345);
    connect(socket_, SIGNAL(readyRead()), this, SLOT(udpDataReady()));

    int w = 0;
    for(int i = 0; i < ui->tableView->horizontalHeader()->count(); ++i)
    {
        w += ui->tableView->horizontalHeader()->sectionSize(i);
    }
    this->resize(w+30, this->height());
}

UI::~UI()
{
    delete ui;
}

QTableView *UI::tv()
{
    return ui->tableView;
}

void UI::udpDataReady()
{
    QByteArray buffer;
    buffer.resize(socket_->pendingDatagramSize());

    QHostAddress sender;
    quint16 senderPort;
    socket_->readDatagram(buffer.data(), buffer.size(), &sender, &senderPort);
    Package p = FromJSon(buffer);
    if(p.fd < 0) // delete
    {
        auto it = conns_.con_map_.find(-p.fd);
        if(it != conns_.con_map_.end())
        {
            int d = std::distance(conns_.con_map_.begin(), it);
            conns_.con_map_.erase(it);
            model_->removeRow(d, QModelIndex());
        }
    }
    else
    {
        auto it = conns_.con_map_.find(p.fd);
        if(it == conns_.con_map_.end())
        {
            auto pair = conns_.con_map_.insert(std::make_pair(p.fd, p));
            int d = std::distance(conns_.con_map_.begin(), pair.first);
            model_->insertRow(d, QModelIndex());
        }
        else
        {
            int d = std::distance(conns_.con_map_.begin(), it);
            if(! p.domain.isEmpty())
            {
                it->second.domain = p.domain;
            }

            if(p.time != 0)
            {
                it->second.time = p.time;
            }

            if(! p.ip.isEmpty())
            {
                it->second.ip = p.ip;
            }

            if(p.port != 0)
            {
                it->second.port = p.port;
            }

            if(p.sended > 0)
            {
                it->second.sended = p.sended;
            }

            if(p.recved > 0)
            {
                it->second.recved = p.recved;
            }
            QModelIndex id1 = model_->index(d, 0, QModelIndex());
            QModelIndex id2 = model_->index(d, 8, QModelIndex());
            model_->dataChanged(id1, id2);
        }
    }
    //qDebug()<<conns_.con_map_.size()<<", "<<QString::fromUtf8(buffer);
}

void UI::closeEvent(QCloseEvent *event)
{
    this->hide();
    event->ignore();
}
