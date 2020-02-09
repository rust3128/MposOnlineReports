#include "database.h"
#include "global.h"
#include <QSqlError>


DataBase::DataBase(QSettings *settings, QObject *parent)
    : QObject(parent),
      dbSet(settings)
{

    db = QSqlDatabase::addDatabase("QIBASE");
    db.setHostName(dbSet->value("Server").toString());
//    db.setPort(dbSet->value("Port").toInt());
    db.setDatabaseName(dbSet->value("Database").toString());
    db.setUserName(dbSet->value("User").toString());
    db.setPassword(dbSet->value("Password").toString());
}


bool DataBase::openDatabase()
{
    qInfo() << QString("DB Connecion: %1:%2").arg(db.hostName()).arg(db.databaseName());
    if(db.open()){
        qInfo("База данных успешно открыта!");
        return true;
    } else {
        qCritical("Не удалось открыть базу данных");

        return false;
    }

}

QSqlDatabase DataBase::getDB()
{
    return  db;
}
