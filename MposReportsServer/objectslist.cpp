#include "objectslist.h"
#include "global.h"



ObjectsList::ObjectsList(QObject *parent) :
    HttpRequestHandler(parent)
{
    QString configFileName=searchConfigFile();
    QSettings *databaseSettings = new QSettings(configFileName,QSettings::IniFormat);
    databaseSettings->beginGroup("Database");
    db = QSqlDatabase::addDatabase("QIBASE","objList");
    db.setHostName(databaseSettings->value("Server").toString());

    db.setDatabaseName(databaseSettings->value("Database").toString());
    db.setUserName(databaseSettings->value("User").toString());
    db.setPassword(databaseSettings->value("Password").toString());

    if(!db.open()){
        qFatal("Не удалось открыть базу данных");
    }
    qInfo() << QString("DB Connecion: %1:%2").arg(db.hostName()).arg(db.databaseName());
}

ObjectsList::~ObjectsList()
{
    db.close();
    db.removeDatabase("objList");
}


void ObjectsList::service(HttpRequest &request, HttpResponse &response)
{
    HttpSession session=sessionStore->getSession(request,response,true);
    userID = session.get("userID").toUInt();


    QSqlQuery q(db);
    q.prepare("select c.clientname from clients c "
              "INNER JOIN clientusers u ON u.client_id = c.client_id "
              "where u.user_id=:userID");
    q.bindValue(":userID",userID);
    if(!q.exec()) qDebug() << "ERROE CLIENTS NAME" << q.lastError().text();
    q.next();
    QString clientName=q.value(0).toString().trimmed();
    q.finish();
    createModels();
    const int azsCount = modelObjects->rowCount();
    QByteArray language=request.getHeader("Accept-Language");
    qDebug("language=%s",qPrintable(language));

    response.setHeader("Content-Type", "text/html; charset=utf-8");
    Template t=objectsList->getTemplate("objectslist");
    t.setVariable("clientName", clientName);
    t.loop("row",azsCount);
    for(int i=0;i<azsCount;++i){
        QString number = QString::number(i);
        QString numPp = QString::number(i+1);
        QString terminalID = modelObjects->data(modelObjects->index(i,0)).toString();
        QString name = modelObjects->data(modelObjects->index(i,1)).toString();
        QString address = modelObjects->data(modelObjects->index(i,2)).toString();
        QString objectID = modelObjects->data(modelObjects->index(i,3)).toString();
        t.setVariable("row"+number+".numPp",numPp);
        t.setVariable("row"+number+".terminalID",terminalID);
        t.setVariable("row"+number+".name",name);
        t.setVariable("row"+number+".address",address);
        t.setVariable("row"+number+".objectID",objectID);
    }
    response.write(t.toUtf8(),true);

}

void ObjectsList::createModels()
{
    modelObjects = new QSqlQueryModel();
    QString strSQL = QString("SELECT o.terminal_id, o.name, o.address, o.object_id from objects o "
                             "INNER JOIN clientusers u ON u.client_id = o.client_id "
                             "WHERE u.user_id = %1 "
                             "ORDER BY o.terminal_id").arg(userID);
    modelObjects->setQuery(strSQL,db);
}
