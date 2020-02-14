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
    int rowCount = modelCounters->rowCount();
    t.loop("row",rowCount);
    for(int i=0;i<rowCount;++i) {
        QString number = QString::number(i);
        t.setVariable("row"+number+".shortName", modelCounters->data(modelCounters->index(i,0)).toString());
        t.setVariable("row"+number+".bgColor",intToColor(modelCounters->data(modelCounters->index(i,2)).toUInt()));
        t.setVariable("row"+number+".dispenser_id", modelCounters->data(modelCounters->index(i,3)).toString());
        t.setVariable("row"+number+".e_from",QString::number(modelCounters->data(modelCounters->index(i,4)).toDouble(),'f',2) );
        t.setVariable("row"+number+".e_to",displayData(modelCounters->data(modelCounters->index(i,5))));
        t.setVariable("row"+number+".e_add", displayData(modelCounters->data(modelCounters->index(i,6))));
        t.setVariable("row"+number+".sale", displayData(modelCounters->data(modelCounters->index(i,7))));
        t.setVariable("row"+number+".spilled", displayData(modelCounters->data(modelCounters->index(i,8))));
        t.setVariable("row"+number+".m_from",QString::number(modelCounters->data(modelCounters->index(i,9)).toDouble(),'f',2) );
        t.setVariable("row"+number+".m_to",QString::number(modelCounters->data(modelCounters->index(i,10)).toDouble(),'f',2) );
        t.setVariable("row"+number+".m_add",displayData(modelCounters->data(modelCounters->index(i,11))));
        t.setVariable("row"+number+".fail",displayData(modelCounters->data(modelCounters->index(i,12))));


    }
    //Отпуск по счетчикам
    const int rowCountAP = modelActivPaytypes->rowCount();
    t.loop("col", rowCountAP);
    for(int i=0;i<rowCountAP;++i) {
        QString number = QString::number(i);
        t.setVariable("col"+number+".colNamePaytype", modelActivPaytypes->data(modelActivPaytypes->index(i,1)).toString());
    }
    const int rowContSF = modelSalesFuels->rowCount();
    t.loop("rowsf", rowContSF);
    for(int i=0;i<rowContSF;++i){
        QString number = QString::number(i);
        t.setVariable("rowsf"+number+".shortName", modelSalesFuels->data(modelSalesFuels->index(i,0)).toString());
        t.setVariable("rowsf"+number+".bgColor",intToColor(modelSalesFuels->data(modelSalesFuels->index(i,2)).toUInt()));
        t.setVariable("rowsf"+number+".e_sum", displayData(modelSalesFuels->data(modelSalesFuels->index(i,3))));
        t.setVariable("rowsf"+number+".spill", displayData(modelSalesFuels->data(modelSalesFuels->index(i,4))));

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
    QString strSQL = QString("SELECT F.SHORTNAME, F.CODENAME, T.COLOR, C.DISPENSER_ID, C.E_FROM, C.E_TO, (C.E_TO - C.E_FROM) AS E_ADD, "
                             "(SELECT SUM(F_ROUNDTO(S.GIVE, 2)) FROM SALEORDERS S "
                             "WHERE S.TERMINAL_ID = C.TERMINAL_ID AND S.SHIFT_ID = C.SHIFT_ID AND S.DISPENSER_ID = C.DISPENSER_ID AND S.TRK_ID = C.TRK_ID) AS SALE, "
                             "C.SPILLED, C.M_FROM, C.M_TO, (C.M_TO - C.M_FROM) AS M_ADD, (C.M_TO - C.M_FROM - C.E_TO + C.E_FROM) AS FAIL "
                             "FROM COUNTERS C "
                             "JOIN TANKS T ON T.TERMINAL_ID = C.TERMINAL_ID AND T.TANK_ID = C.TANK_ID "
                             "LEFT JOIN FUELS F ON F.FUEL_ID = C.FUEL_ID "
                             "WHERE C.TERMINAL_ID = %1 AND C.SHIFT_ID = %2 "
                             "ORDER BY F.CODENAME, T.COLOR, C.DISPENSER_ID")
            .arg(terminalID)
            .arg(shiftID);
    modelCounters->setQuery(strSQL,dbObj);


    modelActivPaytypes = new QSqlQueryModel();
    strSQL = QString("SELECT DISTINCT S.PAYTYPE_ID, P.SHORTNAME FROM GET_FSALES( %1, %2, %2, -1, -1, -1, -1, -1, -1 ) S "
                     "LEFT JOIN PAYTYPES P ON P.PAYTYPE_ID = S.PAYTYPE_ID "
                     "ORDER BY S.TERMINAL_ID, S.SHIFT_ID, S.PAYTYPE_ID")
            .arg(terminalID)
            .arg(shiftID);
    modelActivPaytypes->setQuery(strSQL,dbObj);

    modelSalesFuels = new QSqlQueryModel();
    strSQL = QString("SELECT F.SHORTNAME, F.FUEL_ID, T.COLOR, SUM(C.E_TO - C.E_FROM) AS E_SUM, SUM(C.SPILLED) AS SPILL, F.CODENAME "
                     "FROM COUNTERS C "
                     "LEFT JOIN TANKS T ON T.TANK_ID = C.TANK_ID AND T.TERMINAL_ID=C.TERMINAL_ID "
                     "LEFT JOIN FUELS F ON F.FUEL_ID = C.FUEL_ID "
                     "WHERE C.TERMINAL_ID = %1 AND C.SHIFT_ID = %2 "
                     "GROUP BY F.FUEL_ID, F.SHORTNAME, T.COLOR, F.CODENAME "
                     "ORDER BY F.CODENAME, T.COLOR, F.FUEL_ID")
            .arg(terminalID)
            .arg(shiftID);
    modelSalesFuels->setQuery(strSQL,dbObj);

    modelSalFID = new QSqlQueryModel();
    strSQL = QString("SELECT S.FUEL_ID, S.PAYTYPE_ID, SUM( S.GIVE ) AS SUMM "
                     "FROM GET_FSALES( 3037, 2017, 2017, -1, -1, -1, -1, -1, -1 ) S "
                     "LEFT JOIN TANKS T ON T.TANK_ID = S.TANK_ID AND T.TERMINAL_ID = S.TERMINAL_ID "
                     "GROUP BY S.FUEL_ID, S.PAYTYPE_ID, T.COLOR "
                     "ORDER BY S.FUEL_ID, S.PAYTYPE_ID")
            .arg(terminalID)
            .arg(shiftID);
    modelSalFID->setQuery(strSQL,dbObj);


}

QString ShiftReports::displayData(QVariant dat)
{
    QString strReturn;
    if(dat.toInt() == 0){
        strReturn = "";
    } else if(dat.toDouble() > 0){
        strReturn = QString::number(dat.toDouble(),'f',2);
    } else if(dat.toDouble() < 0){
        strReturn = "<FONT COLOR=#FF0000>"+QString::number(dat.toDouble(),'f',2);
    }
    return strReturn;
}

QString ShiftReports::intToColor(uint col)
{
    QString color = QString("%1").arg(col, 6, 16, QLatin1Char('0'));
    QString c2 = color;
    c2[0]=color[4];
    c2[1]=color[5];
    c2[4]=color[0];
    c2[5]=color[1];
    return c2;
}
