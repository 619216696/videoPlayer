#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include "videoplayer.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

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
