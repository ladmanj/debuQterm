#include <QApplication>
#include <stdlib.h>
#include "MainWindow.h"

int main(int argc, char *argv[])
{
    Q_INIT_RESOURCE(debuQterm);

    QApplication a(argc, argv);

    MainWindow w;
    w.show();
    w.setWindowIcon(QIcon(":/res/debuQterm.png"));

    return a.exec();
}
