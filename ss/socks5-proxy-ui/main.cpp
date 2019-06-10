#include <QApplication>
#include <QProcess>
#include <QSystemTrayIcon>
#include <QFileIconProvider>
#include <QIcon>
#include <QDir>
#include <QDebug>
#include <QMenu>
#include <QTableWidget>
#include "ui.h"

QProcess* StartProcess()
{
    QString program = QDir::homePath() + "/c++/cpp_network_example/socks5-proxy-ui/local"; // 必须使用完整路径
    QString config = QDir::homePath() + "/proxy.conf"; // 使用完整路径
    QStringList args;
    args<<"-c"<<config;
    // qDebug()<<config;
    QProcess* p = new QProcess(qApp);
    p->start(program, args);
    return p;
}

// get program icon
QIcon programIcon()
{
    QFileInfo fileInfo(qApp->arguments().at(0));
    return QFileIconProvider().icon(fileInfo);
}

/* hide docker icon, paste the two lines in Info.plist
<key>LSUIElement</key>
<string>1</string>
*/

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QProcess* process = StartProcess();
    QObject::connect(&a, &QApplication::aboutToQuit, [process](){ process->close(); });
    QSystemTrayIcon* tray = new QSystemTrayIcon(&a);
    tray->setIcon(programIcon());
    UI* ui = new UI;
    QMenu* menu = new QMenu(nullptr);
    menu->addAction("show ui", [ui](){
        ui->show();
        ui->activateWindow();
        ui->raise(); });
    menu->addAction("quit", [](){ qApp->quit(); });
    tray->setContextMenu(menu);
    tray->show();
    //ui->show();
    return a.exec();
}
