#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include "videoplayer.h"
#include <QIcon>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setWindowIcon(QIcon(":/assets/icon512.png"));

    QQmlApplicationEngine engine;
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    qmlRegisterType<VideoPlayer>("VideoPlayer", 1, 0, "VideoPlayer");

    engine.loadFromModule("videoPlayer", "Main");

    return app.exec();
}
