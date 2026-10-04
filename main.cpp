#include "mainwindow.h"
#include <QGuiApplication>
#include <QQmlApplicationEngine>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setApplicationName("ISBNBookScanner");
    app.setOrganizationName("Bookshelf");

    QQmlApplicationEngine engine;
    MainWindow applicationController(engine);
    if (engine.rootObjects().isEmpty()) {
        return -1;
    }
    return app.exec();
}
