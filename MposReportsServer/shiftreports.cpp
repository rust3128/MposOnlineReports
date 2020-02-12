#include "shiftreports.h"
#include "global.h"

ShiftReports::ShiftReports(QObject *parent) :
    HttpRequestHandler(parent)
{
    QString configFileName=searchConfigFile();
    QSettings *databaseSettings = new QSettings(configFileName,QSettings::IniFormat);
    databaseSettings->beginGroup("Database");
    db = QSqlDatabase::addDatabase("QIBASE","reports");
    db.setHostName(databaseSettings->value("Server").toString());

    db.setDatabaseName(databaseSettings->value("Database").toString());
    db.setUserName(databaseSettings->value("User").toString());
    db.setPassword(databaseSettings->value("Password").toString());

    if(!db.open()){
        qFatal("Не удалось открыть базу данных");
    }
    qInfo() << QString("DB Connecion: %1:%2").arg(db.hostName()).arg(db.databaseName());
}

ShiftReports::~ShiftReports()
{
    db.close();
    db.removeDatabase("shList");
}


void ShiftReports::service(HttpRequest &request, HttpResponse &response)
{
    HttpSession session=sessionStore->getSession(request,response,true);
    objectID = session.get("objectID").toUInt();
    shiftID = session.get("shiftID").toUInt();
    qDebug() << "object ID report" << objectID << "shiftID report" << shiftID;
    openObjectDB();
    QByteArray language=request.getHeader("Accept-Language");
    qDebug("language=%s",qPrintable(language));
    response.setHeader("Content-Type", "text/html; charset=utf-8");
    Template t=objectsList->getTemplate("shiftreport");

    t.setVariable("terminalID", termData.at(0));
    t.setVariable("name", termData.at(1));
    t.setVariable("address", termData.at(2));

    //Таблица счетчики
    const int rowCount = modelCounters->rowCount();
    t.loop("row",rowCount);
    for(int i=0;i<rowCount;++i) {
        QString number = QString::number(i);
        t.setVariable("row"+number+".shortName", modelCounters->data(modelCounters->index(i,2)).toString());
        t.setVariable("row"+number+".num", modelCounters->data(modelCounters->index(i,4)).toString());
        t.setVariable("row"+number+".beg_el",QString::number(modelCounters->data(modelCounters->index(i,5)).toDouble(),'f',2) );

    }


    response.write(t.toUtf8(),true);
}

void ShiftReports::openObjectDB()
{
    QSqlQuery *q = new QSqlQuery(db);
    q->prepare("SELECT o.server, o.objdb, o.objdbuser, o.pass, o.terminal_id, o.name, o.address from objects o where o.object_id=:objectID");
    q->bindValue(":objectID",objectID);
    if(!q->exec()){
        qCritical() << "Не могу прочитать данные о подключению к объекту";
        return;
    }
    q->next();

    terminalID = q->value(4).toUInt();

    dbObj = QSqlDatabase::database("azs"+QString::number(objectID));

//    dbObj = QSqlDatabase::addDatabase("QIBASE","azs"+QString::number(terminalID));
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
    modelCounters = new QSqlQueryModel();
    QString strSQL = QString("SELECT c.TERMINAL_ID, c.SHIFT_ID, F.SHORTNAME, TANKS.COLOR, c.DISPENSER_ID, c.E_FROM, c.E_TO, "
                              "(c.E_TO - c.E_FROM) AS E_ADD, c.SPILLED, c.M_FROM, c.M_TO, (c.M_TO - c.M_FROM) "
                              "AS M_ADD, (c.M_TO - c.M_FROM - c.E_TO + c.E_FROM) AS FAIL "
                              "FROM COUNTERS c LEFT JOIN TANKS LEFT JOIN FUELS F ON F.FUEL_ID = c.FUEL_ID "
                              "ON TANKS.TANK_ID = c.TANK_ID "
                              "WHERE c.TERMINAL_ID = %1 AND c.SHIFT_ID = %2 "
                              "ORDER BY c.TERMINAL_ID, c.SHIFT_ID, F.fuel_id, c.DISPENSER_ID")
            .arg(terminalID)
            .arg(shiftID);
    modelCounters->setQuery(strSQL,dbObj);
}
