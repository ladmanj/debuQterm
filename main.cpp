#include <QApplication>
#include <stdlib.h> // pro putenv
#include "MainWindow.h"

int main(int argc, char *argv[])
{
    // Fix pro softwarové vykreslování v MSYS2 (pokud stále zlobí)
    // qputenv("QT_OPENGL", "software");

    QApplication a(argc, argv);

    MainWindow w;
    w.show();

    return a.exec();
}
