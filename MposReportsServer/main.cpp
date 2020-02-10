
#include <QCoreApplication>
#include <QSettings>
#include <QSqlDatabase>


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

    QSqlDatabase db = QSqlDatabase::addDatabase("QIBASE");
    db.setHostName(databaseSettings->value("Server").toString());
//    db.setPort(dbSet->value("Port").toInt());
    db.setDatabaseName(databaseSettings->value("Database").toString());
    db.setUserName(databaseSettings->value("User").toString());
    db.setPassword(databaseSettings->value("Password").toString());

    if(!db.open()){
        qFatal("Не удалось открыть базу данных");
        return 1;
    }
    qInfo() << QString("DB Connecion: %1:%2").arg(db.hostName()).arg(db.databaseName());

    // Configure template cache
    QSettings* templateSettings=new QSettings(configFileName,QSettings::IniFormat,&a);
    templateSettings->beginGroup("templates");
    objectsList = new TemplateCache(templateSettings,&a);


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
