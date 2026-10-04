#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("ISBNBookScanner");
    app.setOrganizationName("Bookshelf");

    MainWindow window;
    window.show();
    return app.exec();
}
