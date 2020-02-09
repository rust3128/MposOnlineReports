#include "objectslist.h"
#include "global.h"

#include <QSqlQuery>
#include <QSqlError>

ObjectsList::ObjectsList(QObject *parent) :
    HttpRequestHandler(parent)
{
    QString configFileName=searchConfigFile();
    QSettings *databaseSettings = new QSettings(configFileName,QSettings::IniFormat);
    databaseSettings->beginGroup("Database");
    db = new DataBase(databaseSettings,this);
    db->openDatabase();
}


void ObjectsList::service(HttpRequest &request, HttpResponse &response)
{
    HttpSession session=sessionStore->getSession(request,response,true);

    uint userID = session.get("userID").toUInt();

    qDebug() << "User ID from ObjectsList = " << userID;

    QSqlQuery q(db->getDB());

    q.prepare("select c.clientname from clients c "
              "INNER JOIN clientusers u ON u.client_id = c.client_id "
              "where u.user_id=:userID");
    q.bindValue(":userID",userID);
    if(!q.exec()) qDebug() << "ERROE CLIENTS NAME" << q.lastError().text();
    q.next();
    QString clientName=q.value(0).toString().trimmed();
    qDebug() << "Client NAme = "<< clientName;
}
