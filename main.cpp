#include <QCoreApplication>
#include <QDir>
#include <QLockFile>
#include <signal.h>
#include "controller.h"

static void signalHandler(int)
{
    QCoreApplication::quit();
}

static int start(const QCoreApplication &application, const QString &configFile, const QString &lockFile)
{
    QFile config(configFile);
    QLockFile lock(lockFile);
    int result = EXIT_RESTART;

    if (!configFile.isEmpty() && !config.open(QFile::ReadOnly))
    {
        printf("Startup failed, unable to open configurantion file \"%s\"\n", configFile.toUtf8().constData());
        return EXIT_FAILURE;
    }

    if (!application.arguments().contains("-f") && !lock.tryLock(1000))
    {
        printf("Startup failed, unable to create lock file \"%s\"\n", lockFile.toUtf8().constData());
        return EXIT_FAILURE;
    }

    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    while (result == EXIT_RESTART)
    {
        Controller controller(configFile);
        QObject::connect(&application, &QCoreApplication::aboutToQuit, &controller, &Controller::quit);
        result = application.exec();
    }

    return result;
}

int main(int argc, char **argv)
{
    QList <QString> list = {"-c", "-l"};
    QCoreApplication application(argc, argv);
    QString configFile, lockFile;

    if (application.arguments().contains("-h"))
    {
        printf("\n"
               "  -c <file>   use the specified configuraton file\n"
               "  -l <file>   use the specified lock file\n"
               "  -f          force start (ignore lock file)\n"
               "  -v          print application version\n"
               "  -h          print this help\n"
               "\n");

        return EXIT_SUCCESS;
    }

    if (application.arguments().contains("-v"))
    {
        printf("%s %s\n", application.applicationName().toUtf8().constData(), SERVICE_VERSION);
        return EXIT_SUCCESS;
    }

    for (int i = 0; i < argc; i++)
    {
        switch (list.indexOf(application.arguments().value(i)))
        {
            case 0: configFile = application.arguments().value(++i); break;
            case 1: lockFile = application.arguments().value(++i); break;
        }
    }

    return start(application, configFile, lockFile.isEmpty() ? QString("%1/%2.lock").arg(QDir::tempPath(), application.applicationName()) : lockFile);
}
