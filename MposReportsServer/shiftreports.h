#ifndef SHIFTREPORTS_H
#define SHIFTREPORTS_H
#include "httprequesthandler.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlQueryModel>


using namespace stefanfrings;

class ShiftReports : public HttpRequestHandler
{
    Q_OBJECT
public:
    ShiftReports(QObject *parent=nullptr);
    ~ShiftReports();

    // HttpRequestHandler interface
public:
    void service(HttpRequest &request, HttpResponse &response);
private:
    void openObjectDB();
    QString displayData(QVariant dat);
    QString intToColor(uint col);
    QVariant columnModelSum(QSqlQueryModel *mod, int column);
    QString tablePaytypeSale(int paytypeID);
    double calcWeight(int tankID, double give);
    QString tableCashData();
private:
    QSqlDatabase db;
    QSqlDatabase dbObj;
    uint objectID;
    uint shiftID;
    uint terminalID;
    int rowCountAP;
    QString terminalName;
    QString shiftData;
    QString operatorFIO;
    int posCount;

    QSqlQueryModel *modelCounters;
    QSqlQueryModel *modelActivPaytypes;
    QSqlQueryModel *modelSalesFuels;
    QSqlQueryModel *modelSalFID;
    QSqlQueryModel *modelIncoming;
    QSqlQueryModel *modelTanksBook;
    QSqlQueryModel *modelTanksFackt;
    QSqlQueryModel *modelTankSaldos;
    QStringList counterSales;
    QStringList tdRows;
    QStringList tanksBookData;
    QStringList tanksFacktData;


};

#endif // SHIFTREPORTS_H
