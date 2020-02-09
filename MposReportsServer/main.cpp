
#include <QCoreApplication>
#include <QSettings>


#include "httplistener.h"
#include "global.h"
#include "requestmapper.h"
#include "DataBase/database.h"

using namespace stefanfrings;

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    a.setApplicationName("MPosOnlineReports");
    a.setOrganizationName("RustSoft");

    // Ищем конфигурационный файл
    QString configFileName=searchConfigFile();

    // Configure logging
    QSettings* logSettings=new QSettings(configFileName,QSettings::IniFormat,&a);
    logSettings->beginGroup("logging");
    logger=new FileLogger(logSettings,10000,&a);
    logger->installMsgHandler();
    // Log the library version
    qDebug("QtWebApp has version %s",getQtWebAppLibVersion());

    // Получаем настройки базы данных
    QSettings *databaseSettings = new QSettings(configFileName,QSettings::IniFormat,&a);
    databaseSettings->beginGroup("Database");
    DataBase *db = new DataBase(databaseSettings,&a);
    if(!db->openDatabase()){
        qFatal("Не могу открвыть базу даных! Ааварийное завершение работы.");
        return 1;
    }

    // Static file controller
    QSettings* fileSettings=new QSettings(configFileName,QSettings::IniFormat,&a);
    fileSettings->beginGroup("files");
    staticFileController=new StaticFileController(fileSettings,&a);

    // Session store
    QSettings* sessionSettings=new QSettings(configFileName,QSettings::IniFormat,&a);
    sessionSettings->beginGroup("sessions");
    sessionStore=new HttpSessionStore(sessionSettings,&a);

    // HTTP server
    QSettings* listenerSettings=new QSettings(configFileName,QSettings::IniFormat,&a);
    listenerSettings->beginGroup("listener");
    new HttpListener(listenerSettings,new RequestMapper(&a),&a);

    return a.exec();
}
