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

    t.setVariable("name", terminalName);
    t.setVariable("shiftdata",shiftData);
    t.setVariable("operator", operatorFIO);


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
    const int countRow = counterSales.size();
    t.loop("rowsf", countRow);
    for(int i=0;i<countRow;++i){
        QString number = QString::number(i);
        t.setVariable("rowsf"+number+".salCount", counterSales.at(i));
    }

    //Приходы топлива
    int countIncoming = modelIncoming->rowCount();
    bool incomingYes = countIncoming>0;
    t.setCondition("incomingYes",incomingYes);
    if(incomingYes){
        const int incRow = tdRows.size();
        t.loop("incom",incRow);
        for(int i=0;i<incRow;++i){
            QString number = QString::number(i);
            t.setVariable("incom"+number+".incomData",tdRows.at(i));
        }
    }

    //Емкости книжка
    const int bookRow = tanksBookData.size();
    t.loop("tanksbook", bookRow);
    for(int i=0;i<bookRow;++i){
        QString number = QString::number(i);
        t.setVariable("tanksbook"+number+".bookData", tanksBookData.at(i));
    }


    //Емкости факт
    const int facktRow = tanksFacktData.size();
    t.loop("tanksfackt", facktRow);
    for(int i=0;i<facktRow;++i){
        QString number = QString::number(i);
        t.setVariable("tanksfackt"+number+".facktdata", tanksFacktData.at(i));
    }

    //Отпуски по видам оплат
    t.loop("paytypesale",rowCountAP);
    for(int i=0; i<rowCountAP;++i){
        QString numberAP = QString::number(i);
        t.setVariable("paytypesale"+numberAP+".paytypename", modelActivPaytypes->data(modelActivPaytypes->index(i,1)).toString());
        t.setVariable("paytypesale"+numberAP+".paytypeSub", modelActivPaytypes->data(modelActivPaytypes->index(i,3)).toString());
        t.setVariable("paytypesale"+numberAP+".paytypeTable", tablePaytypeSale(modelActivPaytypes->data(modelActivPaytypes->index(i,0)).toInt()));
    }

    //Наличные в кассе
    t.setVariable("cashdata", tableCashData());

    response.write(t.toUtf8(),true);
}

QString ShiftReports::tableCashData()
{
    QString cashTable;
    QSqlQuery *q = new QSqlQuery(dbObj);
    q->exec("select m.svalue from migrateoptions m where m.migrateoption_id = 930");
    q->next();
    int docID = q->value(0).toInt();
    q->finish();
    if(docID<0) docID = 0;
    double sAddIn = 0;
    if(docID != 0) {
        q->prepare("F_ROUNDTO( ( SELECT SUM( D.SUMMA ) FROM CASHDOCS D WHERE D.TERMINAL_ID = C.TERMINAL_ID AND D.SHIFT_ID = C.SHIFT_ID AND D.POS_ID = C.POS_ID AND D.CASHDOCSTYPE_ID = :docID ), 2 )");
        q->bindValue(":docID", docID);
        q->exec();
        q->next();
        sAddIn = q->value(0).toDouble();
    }
    q->finish();
    q->prepare("SELECT SUM( F_ROUNDTO( SUMMA, 2 ) ) FROM SERVICECHECKS WHERE TERMINAL_ID = ^terminalID AND SHIFT_ID = :shiftID AND CHECK_TYPE = 1");
    q->bindValue(":terminalID", terminalID);
    q->bindValue(":shiftID", shiftID);
    q->exec();
    q->next();
    double collectedFromSafe = q->value(0).toDouble();

    cashTable = "<TABLE cellSpacing=0 border=1>";
    //Заголовок
    cashTable += "<tr bgColor='#C9C7CF' align=center><td rowspan=2><b>Касса</b></td><td colspan=2><b>На начало</b></td><td rowspan=2><b>Взнос</b></td><td colspan=2><b>Изъятие</b></td><td rowspan=2><b>Получено</b></td>"
                 "<td rowspan=2><b>Возврат</b></td><td rowspan=2><b>Продажи</b></td><td colspan=2><b>На конец</b></td><td  rowspan=2><b>БН оплата</b></td><td rowspan=2><b>БН возврат</b></td><td rowspan=2><b>БН продажи</b></td></tr>";
    cashTable += "<tr bgColor='#C9C7CF' align=center><td><b>В кассе</b></td><td><b>В сейфе</b></td><td><b>Из кассы</b></td><td><b>Из сейфа</b></td>"
                 "<td><b>В кассе</b></td><td><b>В сейфе</b></td></tr>";

    QString strSQL;
    bool result = false;
    bool active;
    if(posCount > 1){
        active = false;
        strSQL = QString("SELECT C.POS_ID, P.NAME AS POSNAME, C.CASHRESTIN + %1 AS CASHRESTIN, C.CASHIN - %1 AS CASHIN, "
                         "C.CASHOUT, C.CASHSALEIN, C.CASHSALEOUT, C.CASHRESTOUT, C.CASHLESSSALEIN, C.CASHLESSSALEOUT, "
                         "C2.SAFESUM AS SAFESUM_FROM, C.SAFESUM, "
                                 "CASE "
                                   "WHEN P.ISMASTER = 'T' THEN %2 "
                                   "ELSE 0.00 "
                                 "END AS COLLECT_SAFE "
                                 "FROM CASHS C "
                                  "LEFT JOIN CASHS C2 ON C2.TERMINAL_ID = C.TERMINAL_ID AND C2.SHIFT_ID = C.SHIFT_ID - 1 AND C2.POS_ID = C.POS_ID "
                                  "LEFT JOIN POSS P ON P.TERMINAL_ID = C.TERMINAL_ID AND P.POS_ID = C.POS_ID "
                                 "WHERE C.TERMINAL_ID = %3 AND C.SHIFT_ID = %4 "
                                 "ORDER BY C.POS_ID")
                .arg(sAddIn)
                .arg(collectedFromSafe)
                .arg(terminalID)
                .arg(shiftID);
    }

    cashTable += "</table>";
    return cashTable;
}

QString ShiftReports::tablePaytypeSale(int paytypeID)
{
    QString tableRes;
    QSqlQueryModel *saleModel = new QSqlQueryModel();

    QString strSelect = QString("SELECT F.FUEL_ID, F.SHORTNAME, F.CODENAME, T.TANK_ID, T.COLOR AS COLOR, "
                                "CL.NAME AS FIRM_NAME, "
                                "SUM(F_ROUNDTO(S.GIVE, 2)) AS GIVE, "
                                "SUM(F_ROUNDTO(S.SUMMA, 2)) AS D_SUMMA, "
                                "SUM(F_ROUNDTO(S.DISCOUNTSUMMA, 2)) AS DISCOUNT, "
                                "SUM(F_ROUNDTO(S.SUMMA - S.DISCOUNTSUMMA, 2)) AS SUMMA, "
                                "COUNT(*) AS SALES_COUNT "
                                "FROM GET_FSALES( %1, %2, %2, -1, %3, -1, -1, -1, -1 ) S "
                                "LEFT JOIN FUELS F ON F.FUEL_ID = S.FUEL_ID "
                                "LEFT JOIN TANKS T ON T.TERMINAL_ID = S.TERMINAL_ID AND T.TANK_ID = S.TANK_ID "
                                "LEFT JOIN FSALEINFOS FSL ON FSL.TERMINAL_ID = S.TERMINAL_ID AND FSL.FSALEINFO_ID = S.SALEORDER_ID "
                                "LEFT JOIN CLIENTS CL ON CL.PAYTYPE_ID = S.PAYTYPE_ID AND CL.CODE = FSL.CODE "
                                "GROUP BY F.FUEL_ID, F.SHORTNAME, CL.NAME, T.TANK_ID, T.COLOR, F.CODENAME ORDER BY F.CODENAME, F.FUEL_ID")
            .arg(terminalID)
            .arg(shiftID)
            .arg(paytypeID);
    saleModel->setQuery(strSelect,dbObj);
    const int smRowCount = saleModel->rowCount();
    tableRes = "<TABLE cellSpacing=0 border=1>";
    //Заголовок
    tableRes += "<tr bgColor='#C9C7CF' align=center><td><b>НП</b></td><td><b>Организация</b></td><td colspan=2><b>Реализация</b></td>"
                         "<td><b>Цена ср.</b></td><td><b>Сумма</b></td><td><b>Скидка</b></td><td><b>Итого</b></td><td><b>Чеков</b></td></tr>";
    tableRes += "<tr bgColor='#C9C7CF' align=center><td></td><td></td><td><b>л</b></td><td><b>кг</b></td><td></td><td></td><td></td><td></td><td></td></tr>";
    double sumWeight=0, weight;
    for(int i=0;i<smRowCount;++i){
        tableRes += "<tr>";
        tableRes += QString("<td align=center bgColor=%1><b>%2</b></td>")
                .arg(intToColor(saleModel->data(saleModel->index(i,4)).toUInt()))
                .arg(saleModel->data(saleModel->index(i,1)).toString());
        tableRes += QString("<td align=left>%1</td>").arg(saleModel->data(saleModel->index(i,5)).toString());
        tableRes += QString("<td align=right>%1</td>").arg(displayData(saleModel->data(saleModel->index(i,6))));
        weight = calcWeight(saleModel->data(saleModel->index(i,3)).toInt(),saleModel->data(saleModel->index(i,6)).toDouble());
        sumWeight +=weight;
        tableRes += QString("<td align=right>%1</td>").arg(displayData(weight));
        tableRes += QString("<td align=right>%1</td>").arg(displayData(saleModel->data(saleModel->index(i,9)).toDouble() / saleModel->data(saleModel->index(i,6)).toDouble()) );
        tableRes += QString("<td align=right>%1</td>").arg(displayData(saleModel->data(saleModel->index(i,7))));
        tableRes += QString("<td align=right>%1</td>").arg(displayData(saleModel->data(saleModel->index(i,8))));
        tableRes += QString("<td align=right>%1</td>").arg(displayData(saleModel->data(saleModel->index(i,9))));
        tableRes += QString("<td align=right>%1</td>").arg(saleModel->data(saleModel->index(i,10)).toInt());
        tableRes += "</tr>";
    }
    tableRes += "<tr>";
    tableRes += "<td><b>Итого</b></td><td></td>"+QString("<td align=right><b>%1</b></td>").arg(displayData(columnModelSum(saleModel,6)));
    tableRes += QString("<td align=right><b>%1</b></td>").arg(displayData(sumWeight))+"<td></td>";
    tableRes += QString("<td align=right><b>%1</b></td>").arg(displayData(columnModelSum(saleModel,7)));
    tableRes += QString("<td align=right><b>%1</b></td>").arg(displayData(columnModelSum(saleModel,8)));
    tableRes += QString("<td align=right><b>%1</b></td>").arg(displayData(columnModelSum(saleModel,9)));
    tableRes += QString("<td align=right><b>%1</b></td>").arg(columnModelSum(saleModel,10).toInt());
    tableRes += "</tr>";
    tableRes += "</table>";
    return  tableRes;
}

double ShiftReports::calcWeight(int tankID, double give)
{
    if(give <= 0.005) {
        return 0;
    }
    double totalGive =0;
    double weightSub =0;
    for(int i=0;i<modelTankSaldos->rowCount();++i){
        if(modelTankSaldos->data(modelTankSaldos->index(i,0)).toInt() == tankID){
            totalGive = modelTankSaldos->data(modelTankSaldos->index(i,1)).toDouble();
            weightSub = modelTankSaldos->data(modelTankSaldos->index(i,2)).toDouble();
            break;
        }
    }
    if(totalGive < 0.005)
        return 0;
    return give * (weightSub / totalGive);
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
    q->finish();
    QSqlQuery *qo = new QSqlQuery(dbObj);
    qo->prepare("SELECT NAME FROM TERMINALS WHERE TERMINAL_ID = :terminalID");
    qo->bindValue(":terminalID",terminalID);
    qo->exec();
    qo->next();
    terminalName = qo->value(0).toString().trimmed();
    qo->finish();
    qo->prepare("SELECT s.DATOPEN, s.DATCLOSE, o.fio, s.ZNUMBER FROM SHIFTS s "
                "left join operators o ON o.operator_id = s.operator_id "
                "WHERE s.TERMINAL_ID = :terminalID AND s.SHIFT_ID = :shiftID");
    qo->bindValue(":terminalID", terminalID);
    qo->bindValue(":shiftID",shiftID);
    qo->exec();
    qo->next();
    shiftData = QString("Отчет за смену № %1, Z-отчет № %2, %3 - %4")
            .arg(shiftID)
            .arg(qo->value(3).toString())
            .arg(qo->value(0).toDateTime().toString("dd.MM.yyyy hh:mm:ss"))
            .arg(qo->value(1).toDateTime().toString("dd.MM.yyyy hh:mm:ss"));
    operatorFIO = qo->value(2).toString().trimmed();
    qo->finish();
    qo->prepare("SELECT count(*) FROM CASHS WHERE TERMINAL_ID = :terminalID AND SHIFT_ID = :shiftID");
    qo->bindValue(":terminalID", terminalID);
    qo->bindValue(":shiftID",shiftID);
    qo->exec();
    qo->next();
    posCount = q->value(0).toInt();


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
    strSQL = QString("SELECT DISTINCT S.PAYTYPE_ID AS PAYTYPE_NEW, P.NAME, P.FUELTYPE, PP.NAME AS POS_PAYTYPE_NAME "
                     "FROM GET_FSALES( %1, %2, %2, -1, -1, -1, -1, -1, -1 ) S "
                          "LEFT JOIN PAYTYPES P ON P.PAYTYPE_ID = S.PAYTYPE_ID "
                          "LEFT JOIN POSPAYTYPES PP ON PP.POSPAYTYPE_ID = P.FUELTYPE "
                     "WHERE S.PAYTYPE_ID <> 0 "
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
                     "FROM GET_FSALES( %1, %2, %2, -1, -1, -1, -1, -1, -1 ) S "
                     "LEFT JOIN TANKS T ON T.TANK_ID = S.TANK_ID AND T.TERMINAL_ID = S.TERMINAL_ID "
                     "GROUP BY S.FUEL_ID, S.PAYTYPE_ID, T.COLOR "
                     "ORDER BY S.FUEL_ID, S.PAYTYPE_ID")
            .arg(terminalID)
            .arg(shiftID);
    modelSalFID->setQuery(strSQL,dbObj);
    counterSales.clear();
    QString tempStr = "<td align=center bgColor='#C9C7CF'><b>НП</td><td align=center bgColor='#C9C7CF'><b>По счетч</td><td align=center bgColor='#C9C7CF'><b>Ав пролив</td>";
    rowCountAP = modelActivPaytypes->rowCount();
    for(int i=0;i<rowCountAP;++i) {
        tempStr += "<td align=center bgColor='#C9C7CF'><b>"+modelActivPaytypes->data(modelActivPaytypes->index(i,1)).toString()+"</td>";
    }
    tempStr += "<td align=center bgColor='#C9C7CF'><b>Погреш</td>";
    counterSales.append(tempStr);
    tempStr.clear();
    const int rowContSF = modelSalesFuels->rowCount();
    const int rowCountPt = modelActivPaytypes->rowCount();
    const int rowCountFID = modelSalFID->rowCount();
    uint fuelid;
    uint paytypeId;
    QList<double> summCount;
    for(int i=0;i<3+rowCountPt;++i){
        summCount.push_back(0);
    }
    for(int i=0;i<rowContSF;++i){
        tempStr = QString("<td align=right bgColor=%1><b>%2</b></td>")
                .arg(intToColor(modelSalesFuels->data(modelSalesFuels->index(i,2)).toUInt()))
                .arg(modelSalesFuels->data(modelSalesFuels->index(i,0)).toString());
        tempStr += "<td align=right>"+displayData(modelSalesFuels->data(modelSalesFuels->index(i,3)))+"</td>";
        summCount[0] += modelSalesFuels->data(modelSalesFuels->index(i,3)).toDouble();
        tempStr += "<td align=right>"+displayData(modelSalesFuels->data(modelSalesFuels->index(i,4)))+"</td>";
        summCount[1] += modelSalesFuels->data(modelSalesFuels->index(i,4)).toDouble();
        fuelid = modelSalesFuels->data(modelSalesFuels->index(i,1)).toUInt();
        for(int p=0;p<rowCountPt;++p){
            paytypeId = modelActivPaytypes->data(modelActivPaytypes->index(p,0)).toUInt();
            QString sale = "<td align=right>"+displayData(0)+"</td>";
            for(int j=0;j<rowCountFID;j++){
                if((modelSalFID->data(modelSalFID->index(j,0)).toUInt() == fuelid) &&  (modelSalFID->data(modelSalFID->index(j,1)).toUInt() == paytypeId) ){
                    sale = "<td align=right>"+displayData(modelSalFID->data(modelSalFID->index(j,2)))+"</td>";
                    summCount[p+2] += modelSalFID->data(modelSalFID->index(j,2)).toDouble();
                    break;
                }
            }
            tempStr += sale;

        }
        tempStr += "<td align=right>"+displayData(modelSalesFuels->data(modelSalesFuels->index(i,4)))+"</td>";
        summCount[summCount.size()-1] += modelSalesFuels->data(modelSalesFuels->index(i,4)).toDouble();
        counterSales.append(tempStr);
    }
    tempStr = "<td align =right><b>Итого</b></td>";
    for(int i = 0; i<summCount.size();++i){
        tempStr += "<td align=right><b>"+displayData(summCount.at(i))+"</b></td>";
    }
    counterSales.append(tempStr);

    //Приходы топлива
    modelIncoming = new QSqlQueryModel();
    strSQL=QString("SELECT F.SHORTNAME, F.CODENAME, T.TANK_ID, T.COLOR, I.DOCAMOUNT, I.DOCTEMPERATURE, I.DOCDENSITY, "
                   "G1.FUELHEIGHT AS FUELHEIGHT_FROM, G1.FUELAMOUNT AS FUELAMOUNT_FROM, G1.TEMPERATURE AS TEMPERATURE_FROM, "
                   "G1.DENSITY AS DENSITY_FROM, G2.FUELHEIGHT AS FUELHEIGHT_TO, G2.FUELAMOUNT AS FUELAMOUNT_TO, "
                   "G2.TEMPERATURE AS TEMPERATURE_TO, G2.DENSITY AS DENSITY_TO, I.REALAMOUNT,  I.REALTEMPERATURE, "
                   "I.REALDENSITY, (I.DOCAMOUNT - F_ROUNDTO( I.GIVE, 2 ) - G2.FUELAMOUNT + G1.FUELAMOUNT) AS FAILLITERS, "
                   "((I.DOCAMOUNT - F_ROUNDTO( I.GIVE, 2 )) * I.DOCDENSITY - G2.FUELAMOUNT * G2.DENSITY + G1.FUELAMOUNT * G1.DENSITY) AS FAILKILOGRAMS, I.GIVE AS GIVE "
                   "FROM INCOMINGTANKS I LEFT JOIN TANKS T ON T.TERMINAL_ID = I.TERMINAL_ID AND T.TANK_ID = I.TANK_ID "
                   "LEFT JOIN FUELS F ON F.FUEL_ID = I.FUEL_ID "
                   "LEFT JOIN GAUGINGS G1 ON G1.TERMINAL_ID = I.TERMINAL_ID AND G1.GAUGING_ID = I.GAUGINGFROM_ID "
                   "LEFT JOIN GAUGINGS G2 ON G2.TERMINAL_ID = I.TERMINAL_ID AND G2.GAUGING_ID = I.GAUGINGTO_ID "
                   "WHERE I.TERMINAL_ID = %1 AND I.SHIFT_ID = %2 and i.docamount >0 "
                   "ORDER BY t.terminal_id, F.codename, T.tank_id")
            .arg(terminalID)
            .arg(shiftID);
    modelIncoming->setQuery(strSQL,dbObj);
    tdRows.clear();
    tdRows.append("<td colspan=2></td><td align=center bgColor='#C9C7CF' colspan=3><b>По накладной</b></td><td align=center bgColor='#C9C7CF' colspan=4><b>Замер до</b></td><td align=center bgColor='#C9C7CF' colspan=4><b>Замер после</b></td>"
                  "<td align=center  bgColor='#C9C7CF' colspan=3><b>Фактически</b></td><td align=center  bgColor='#C9C7CF' colspan=2><b>Потери</b></td><td  bgColor='#C9C7CF' align=center><b>Отпуск</b></td>");
    tempStr = "<td align=center bgColor='#C9C7CF'><b>НП</td><td align=center bgColor='#C9C7CF'><b>№</td>";
    tempStr += "<td align=center bgColor='#C9C7CF'><b>л</td><td align=center bgColor='#C9C7CF'><b>Темп</td><td align=center bgColor='#C9C7CF'><b>Плот</td>";
    tempStr += "<td align=center bgColor='#C9C7CF'><b>мм</td><td align=center bgColor='#C9C7CF'><b>л</td><td align=center bgColor='#C9C7CF'><b>Темп</td><td align=center bgColor='#C9C7CF'><b>Плот</td>";
    tempStr += "<td align=center bgColor='#C9C7CF'><b>мм</td><td align=center bgColor='#C9C7CF'><b>л</td><td align=center bgColor='#C9C7CF'><b>Темп</td><td align=center bgColor='#C9C7CF'><b>Плот</td>";
    tempStr += "<td align=center bgColor='#C9C7CF'><b>л</td><td align=center bgColor='#C9C7CF'><b>Темп</td><td align=center bgColor='#C9C7CF'><b>Плот</td>";
    tempStr += "<td align=center bgColor='#C9C7CF'><b>л</td><td align=center bgColor='#C9C7CF'><b>Кг</td><td align=center bgColor='#C9C7CF'><b>л</td>";
    tdRows.append(tempStr);
    for(int i=0;i<modelIncoming->rowCount();++i){
        tempStr.clear();
        tempStr = QString("<td align=right bgColor=%1><b>%2</b></td> <td align=right>%3</td>")
                .arg(intToColor(modelIncoming->data(modelIncoming->index(i,3)).toUInt()))
                .arg(modelIncoming->data(modelIncoming->index(i,0)).toString())
                .arg(modelIncoming->data(modelIncoming->index(i,2)).toString());
        for(int j=4; j<modelIncoming->columnCount()-3;++j){
            tempStr += QString("<td align=right>%1</td>").arg(modelIncoming->data(modelIncoming->index(i,j)).toString());
        }
        tempStr += QString("<td align=right>%1</td>").arg(displayData(modelIncoming->data(modelIncoming->index(i,18))));
        if(modelIncoming->data(modelIncoming->index(i,10)).toInt() == 0){
            tempStr += QString("<td align=right></td>");
        } else {
            tempStr += QString("<td align=right>%1</td>").arg(displayData(modelIncoming->data(modelIncoming->index(i,19))));
        }
        tempStr += QString("<td align=right>%1</td>").arg(displayData(modelIncoming->data(modelIncoming->index(i,20))));
        tdRows.append(tempStr);
    }
    tempStr = QString("<td align =right><b>Итого</b></td><td></td><td align =right><b>%1</b></td>")
            .arg(displayData(columnModelSum(modelIncoming,4)));
    for(int i=0;i<10;++i)
        tempStr += "<td></td>";
    tempStr += QString("<td align =right><b>%1</b></td>").arg(displayData(columnModelSum(modelIncoming,15)));
    tempStr += "<td></td><td></td><td></td>";
    tempStr += QString("<td align =right><b>%1</b></td>").arg(displayData(0));
    tempStr += QString("<td align =right><b>%1</b></td>").arg(displayData(columnModelSum(modelIncoming,20)));
    tdRows.append(tempStr);

    //Емкости Книга
    modelTanksBook = new QSqlQueryModel();
    strSQL = QString("SELECT F.SHORTNAME, T.TANK_ID, T.COLOR, TS.PRICE, F_ROUNDTO(TS.BOOKFROM, 2) AS BOOKFROM, TS.BOOKADD, TS.BOOKSUB, "
                     "F_ROUNDTO(TS.BOOKTO, 2) AS BOOKTO, TS.WEIGHT_FROM, "
                     "( TS.WEIGHT_ADD + TS.WEIGHT_ADDSUB ) AS WEIGHT_ADD, "
                     "( TS.WEIGHT_SUB + TS.WEIGHT_ADDSUB ) AS WEIGHT_SUB, "
                     "TS.WEIGHT_TO, F.CODENAME, TS.DOCS_CORR, "
                     "ROUND( COALESCE( TS.DOCS_CORR * G.DENSITY, 0 ), 2 ) AS WEIGHT_DOCS_CORR "
                   "FROM V_TANKSALDOS_EX TS "
                        "LEFT JOIN TANKS T ON T.TERMINAL_ID = TS.TERMINAL_ID AND T.TANK_ID = TS.TANK_ID "
                        "LEFT JOIN FUELS F ON F.FUEL_ID = TS.FUEL_ID "
                        "LEFT JOIN GAUGINGS G ON G.TERMINAL_ID = TS.TERMINAL_ID AND G.GAUGING_ID = TS.GAUGINGTO_ID "
                   "WHERE TS.TERMINAL_ID = %1 AND TS.SHIFT_ID = %2 "
                   "ORDER BY F.CODENAME, T.COLOR, T.TANK_ID")
            .arg(terminalID)
            .arg(shiftID);

    modelTanksBook->setQuery(strSQL, dbObj);
    tanksBookData.clear();
    tanksBookData.append("<td bgColor='#C9C7CF' colspan=2></td><td align=center bgColor='#C9C7CF'><b>Цена</b></td><td align=center bgColor='#C9C7CF' colspan=3><b>Начало</b></td>"
                         "<td align=center bgColor='#C9C7CF' colspan=2><b>Приход</b></td><td align=center bgColor='#C9C7CF' colspan=2><b>Расход</b></td><td align=center bgColor='#C9C7CF'><b>Корр</b>"
                         "<td align=center bgColor='#C9C7CF' colspan=3><b>Конец</b></td><td align=center bgColor='#C9C7CF' colspan=2><b>Погреш</b></td>");
    tanksBookData.append("<td align=center bgColor='#C9C7CF'><b>НП</td><td align=center bgColor='#C9C7CF'><b>№</td><td align=center bgColor='#C9C7CF'><b>грн</td>"
                         "<td align=center bgColor='#C9C7CF'><b>мм</td><td align=center bgColor='#C9C7CF'><b>л</td><td align=center bgColor='#C9C7CF'><b>кг</td>"
                         "<td align=center bgColor='#C9C7CF'><b>л</td><td align=center bgColor='#C9C7CF'><b>кг</td><td align=center bgColor='#C9C7CF'><b>л</td><td align=center bgColor='#C9C7CF'><b>кг</td>"
                         "<td align=center bgColor='#C9C7CF'><b>л</td><td align=center bgColor='#C9C7CF'><b>мм</td><td align=center bgColor='#C9C7CF'><b>л</td><td align=center bgColor='#C9C7CF'><b>кг</td>"
                         "<td align=center bgColor='#C9C7CF'><b>л</td><td align=center bgColor='#C9C7CF'><b>кг</td>");
    for(int i=0;i<modelTanksBook->rowCount();++i){
        tempStr.clear();
        tempStr = QString("<td align=right bgColor=%1><b>%2</b></td><td align=right>%3</td><td align=right>%4</td><td></td>")
                .arg(intToColor(modelTanksBook->data(modelTanksBook->index(i,2)).toUInt()))
                .arg(modelTanksBook->data(modelTanksBook->index(i,0)).toString())
                .arg(modelTanksBook->data(modelTanksBook->index(i,1)).toString())
                .arg(modelTanksBook->data(modelTanksBook->index(i,3)).toString());
        for(int j=4;j<7;++j){
            tempStr += QString("<td align=right>%1</td><td align=right>%2</td>")
                    .arg(displayData(modelTanksBook->data(modelTanksBook->index(i,j))))
                    .arg(displayData(modelTanksBook->data(modelTanksBook->index(i,j+4))));
        }
        tempStr += QString("<td align=right>%1</td><td></td>").arg(displayData(modelTanksBook->data(modelTanksBook->index(i,13))));
        tempStr += QString("<td align=right>%1</td><td align=right>%2</td>")
                .arg(displayData(modelTanksBook->data(modelTanksBook->index(i,7))))
                .arg(displayData(modelTanksBook->data(modelTanksBook->index(i,11))));
        double pogr = modelTanksBook->data(modelTanksBook->index(i,7)).toDouble()
                - modelTanksBook->data(modelTanksBook->index(i,4)).toDouble()
                - modelTanksBook->data(modelTanksBook->index(i,5)).toDouble()
                + modelTanksBook->data(modelTanksBook->index(i,6)).toDouble()
                - modelTanksBook->data(modelTanksBook->index(i,13)).toDouble();
        tempStr += QString("<td align=right>%1</td>").arg(displayData(pogr));
        pogr = modelTanksBook->data(modelTanksBook->index(i,11)).toDouble()
                        - modelTanksBook->data(modelTanksBook->index(i,8)).toDouble()
                        - modelTanksBook->data(modelTanksBook->index(i,9)).toDouble()
                        + modelTanksBook->data(modelTanksBook->index(i,10)).toDouble()
                        - modelTanksBook->data(modelTanksBook->index(i,14)).toDouble();
        tempStr += QString("<td align=right>%1</td>").arg(displayData(pogr));
        tanksBookData.append(tempStr);
    }

    modelTanksFackt = new QSqlQueryModel();
    strSQL = QString("SELECT F.SHORTNAME, T.TANK_ID, T.COLOR, "
                     "GF.FUELHEIGHT AS FUELHEIGHT_FROM, GF.FUELAMOUNT AS FUELAMOUNT_FROM, "
                     "F_ROUNDTO((SELECT SUM(I.REALAMOUNT) FROM INCOMINGTANKS I WHERE I.TERMINAL_ID = C.TERMINAL_ID AND I.SHIFT_ID = C.SHIFT_ID AND I.TANK_ID = C.TANK_ID),2) AS ADD_FROM_INCOMING, "
                     "F_ROUNDTO((SELECT SUM(S2.GIVE) FROM SALEORDERS S2 WHERE S2.TERMINAL_ID = T.TERMINAL_ID AND S2.SHIFT_ID = C.SHIFT_ID AND S2.TANK_ID = T.TANK_ID),2) AS SALE, "
                     "GL.FUELHEIGHT AS FUELHEIGHT_TO,  GL.FUELAMOUNT AS FUELAMOUNT_TO, "
                     "F_ROUNDTO((SELECT SUM(S1.GIVE) FROM SALEORDERS S1 WHERE S1.TERMINAL_ID = C.TERMINAL_ID AND S1.SHIFT_ID = C.SHIFT_ID AND S1.PAYTYPE_ID = 0 AND S1.TANK_ID = C.TANK_ID),2) AS ADD_FROM_SALES, "
                     "F.CODENAME "
                     "FROM TANKSALDOS C "
                     "LEFT JOIN TANKS T ON T.TERMINAL_ID = C.TERMINAL_ID AND T.TANK_ID = C.TANK_ID "
                     "LEFT JOIN FUELS F ON F.FUEL_ID = C.FUEL_ID "
                     "LEFT JOIN GAUGINGS GF ON GF.TERMINAL_ID = C.TERMINAL_ID AND GF.GAUGING_ID = C.GAUGINGFROM_ID "
                     "LEFT JOIN GAUGINGS GL ON GL.TERMINAL_ID = T.TERMINAL_ID AND GL.GAUGING_ID = C.GAUGINGTO_ID "
                     "WHERE C.TERMINAL_ID = %1 AND C.SHIFT_ID = %2 "
                     "ORDER BY F.CODENAME, T.COLOR, T.TANK_ID")
            .arg(terminalID)
            .arg(shiftID);
    modelTanksFackt->setQuery(strSQL,dbObj);

    tanksFacktData.clear();
    tanksFacktData.append("<td bgColor='#C9C7CF' colspan=2><td align=center bgColor='#C9C7CF' colspan=2><b>Начало</b></td>"
                         "<td align=center bgColor='#C9C7CF'><b>Приход</b></td><td align=center bgColor='#C9C7CF'><b>Расход</b><td align=center bgColor='#C9C7CF' colspan=2><b>Конец</b></td>"
                         "<td align=center bgColor='#C9C7CF'><b>Погреш</b></td>");
    tanksFacktData.append("<td align=center bgColor='#C9C7CF'><b>НП</td><td align=center bgColor='#C9C7CF'><b>№</td>"
                          "<td align=center bgColor='#C9C7CF'><b>мм</td><td align=center bgColor='#C9C7CF'><b>л</td><td align=center bgColor='#C9C7CF'><b>л</td><td align=center bgColor='#C9C7CF'><b>л</td>"
                          "<td align=center bgColor='#C9C7CF'><b>мм</td><td align=center bgColor='#C9C7CF'><b>л</td><td align=center bgColor='#C9C7CF'><b>л</td>");
    for(int i=0;i<modelTanksFackt->rowCount();++i){
        tempStr = QString("<td align=right bgColor=%1><b>%2</b></td> <td align=right>%3</td>")
                .arg(intToColor(modelTanksFackt->data(modelTanksFackt->index(i,2)).toUInt()))
                .arg(modelTanksFackt->data(modelTanksFackt->index(i,0)).toString())
                .arg(modelTanksFackt->data(modelTanksFackt->index(i,1)).toString());
        for(int j=3;j<9;++j){
            tempStr += QString("<td align=right>%1</td>")
                    .arg(displayData(modelTanksFackt->data(modelTanksFackt->index(i,j))));
        }
        double pogr = modelTanksFackt->data(modelTanksFackt->index(i,8)).toDouble()
                - modelTanksFackt->data(modelTanksFackt->index(i,4)).toDouble()
                - modelTanksFackt->data(modelTanksFackt->index(i,5)).toDouble()
                + modelTanksFackt->data(modelTanksFackt->index(i,6)).toDouble();
        tempStr += QString("<td align=right>%1</td>").arg(displayData(pogr));
        tanksFacktData.append(tempStr);
    }

    //Model TankSaldos
    modelTankSaldos = new QSqlQueryModel();
    strSQL = QString("SELECT TANK_ID, COALESCE(BOOKSUB,0) - COALESCE(BOOKADDSUB,0) AS BOOKSUB, WEIGHT_SUB FROM TANKSALDOS "
                     "WHERE TERMINAL_ID=%1 AND SHIFT_ID=%2")
            .arg(terminalID)
            .arg(shiftID);
    modelTankSaldos->setQuery(strSQL,dbObj);




}

QString ShiftReports::displayData(QVariant dat)
{
    QString strReturn;
    if(dat.toInt() == 0){
        strReturn = " ";
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

QVariant ShiftReports::columnModelSum(QSqlQueryModel *mod, int column)
{
    const int rowCount = mod->rowCount();
    double sum =0.0;
    for(int i =0;i<rowCount;++i){
        sum += mod->data(mod->index(i,column)).toDouble();
    }
    return sum;
}

