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
    qDebug() << "object ID" << objectID << "shiftID" << shiftID;

    QByteArray language=request.getHeader("Accept-Language");
    qDebug("language=%s",qPrintable(language));
    response.setHeader("Content-Type", "text/html; charset=utf-8");
    Template t=objectsList->getTemplate("shiftreport");

    t.setVariable("terminalID", termData.at(0));
    t.setVariable("name", termData.at(1));
    t.setVariable("address", termData.at(2));

    //Таблица счетчики
    QSqlQuery *q = new QSqlQuery(dbObj);
    q->prepare("SELECT c.TERMINAL_ID, c.SHIFT_ID, F.SHORTNAME, TANKS.COLOR, c.DISPENSER_ID, c.E_FROM, c.E_TO, "
               "(c.E_TO - c.E_FROM) AS E_ADD, c.SPILLED, c.M_FROM, c.M_TO, (c.M_TO - c.M_FROM) "
               "AS M_ADD, (c.M_TO - c.M_FROM - c.E_TO + c.E_FROM) AS FAIL "
               "FROM COUNTERS c LEFT JOIN TANKS LEFT JOIN FUELS F ON F.FUEL_ID = c.FUEL_ID "
               "ON TANKS.TANK_ID = c.TANK_ID "
               "WHERE c.TERMINAL_ID = :terminalID AND c.SHIFT_ID = :shiftID "
               "ORDER BY c.TERMINAL_ID, c.SHIFT_ID, F.fuel_id, c.DISPENSER_ID");
    q->bindValue(":terminalID", terminalID);
    q->bindValue("shiftID",shiftID);
    q->exec();
    int rowCount = q->size();
    t.loop("row",rowCount);
    uint i = 0;
    while(q->next()){
        QString number = QString::number(i);
        QString shortName = q->value(2).toString();
        t.setVariable("row"+number+".shortName", shortName);
        ++i;
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
}
