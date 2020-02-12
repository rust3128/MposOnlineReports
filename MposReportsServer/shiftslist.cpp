#include "shiftslist.h"
#include "global.h"

ShiftsList::ShiftsList(QObject *parent) :
    HttpRequestHandler(parent)
{
    QString configFileName=searchConfigFile();
    QSettings *databaseSettings = new QSettings(configFileName,QSettings::IniFormat);
    databaseSettings->beginGroup("Database");
    db = QSqlDatabase::addDatabase("QIBASE","shList");
    db.setHostName(databaseSettings->value("Server").toString());

    db.setDatabaseName(databaseSettings->value("Database").toString());
    db.setUserName(databaseSettings->value("User").toString());
    db.setPassword(databaseSettings->value("Password").toString());

    if(!db.open()){
        qFatal("Не удалось открыть базу данных");
    }
    qInfo() << QString("DB Connecion: %1:%2").arg(db.hostName()).arg(db.databaseName());
}

ShiftsList::~ShiftsList()
{
    db.close();
    db.removeDatabase("shList");
}


void ShiftsList::service(HttpRequest &request, HttpResponse &response)
{
    HttpSession session=sessionStore->getSession(request,response,true);
    objectID = session.get("objectID").toUInt();
    qDebug() << "object ID" << objectID;
    openObjectDB();
    const int shiftCount = modelShifts->rowCount();
    QByteArray language=request.getHeader("Accept-Language");
    qDebug("language=%s",qPrintable(language));
    response.setHeader("Content-Type", "text/html; charset=utf-8");
    Template t=objectsList->getTemplate("shiftslist");
    t.setVariable("terminalID", termData.at(0));
    t.setVariable("name", termData.at(1));
    t.setVariable("address", termData.at(2));

    t.loop("row",shiftCount);
    for(int i=0;i<shiftCount;++i){
        QString number = QString::number(i);
        QString shiftID = modelShifts->data(modelShifts->index(i,0)).toString();
        QString znum = modelShifts->data(modelShifts->index(i,1)).toString();
        QString datOpen = modelShifts->data(modelShifts->index(i,2)).toDateTime().toString("dd-MM-yyyy hh:mm:ss");
        QString datClose = modelShifts->data(modelShifts->index(i,3)).toDateTime().toString("dd-MM-yyyy hh:mm:ss");
        QString oper = modelShifts->data(modelShifts->index(i,4)).toString();
        t.setVariable("row"+number+".shiftID",shiftID);
        t.setVariable("row"+number+".znum",znum);
        t.setVariable("row"+number+".datopen",datOpen);
        t.setVariable("row"+number+".datclose",datClose);
        t.setVariable("row"+number+".operator",oper);
    }
     response.write(t.toUtf8(),true);
}

void ShiftsList::openObjectDB()
{
    QSqlQuery *q = new QSqlQuery(db);
    q->prepare("SELECT o.server, o.objdb, o.objdbuser, o.pass, o.terminal_id, o.name, o.address from objects o where o.object_id=:objectID");
    q->bindValue(":objectID",objectID);
    if(!q->exec()){
        qCritical() << "Не могу прочитать данные о подключению к объекту";
        return;
    }
    q->next();


    dbObj = QSqlDatabase::addDatabase("QIBASE","azs"+QString::number(objectID));
    dbObj.setHostName(q->value(0).toString());
    dbObj.setDatabaseName(q->value(1).toString());
    dbObj.setUserName(q->value(2).toString());
    dbObj.setPassword(q->value(3).toString());
    if(!dbObj.open()){
        qCritical() << "Не могу отрыть базу данных АЗС" << dbObj.lastError().text() ;
        return;
    }
   termData.clear();
   termData.append(q->value(4).toString());
   termData.append(q->value(5).toString());
   termData.append(q->value(6).toString());
   q->finish();
   modelShifts = new QSqlQueryModel();
   modelShifts->setQuery("SELECT first 30 s.shift_id, s.znumber, s.datopen, s.datclose, TRIM(o.fio) from shifts s "
                         "INNER JOIN operators o ON o.operator_id=s.operator_id "
                         "order by s.shift_id DESC",dbObj);




}
