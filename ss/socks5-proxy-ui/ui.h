#ifndef UI_H
#define UI_H

#include <QWidget>
#include <QStandardItemModel>
#include <QUdpSocket>
#include <QCloseEvent>
#include <QTableView>
#include "socks5table.h"

namespace Ui {
class UI;
}

class UI : public QWidget
{
    Q_OBJECT

public:
    explicit UI(QWidget *parent = 0);
    ~UI() override;
    QTableView *tv();
public slots:
    void udpDataReady();
private:
    Ui::UI *ui;
    Socks5Table* model_;
    Connections conns_;
    QUdpSocket* socket_;

    // QWidget interface
protected:
    void closeEvent(QCloseEvent *event) override;
};

#endif // UI_H
