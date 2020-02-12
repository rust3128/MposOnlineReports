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
private:
    QSqlDatabase db;
    QSqlDatabase dbObj;
    uint objectID;
    uint shiftID;
    uint terminalID;
    QStringList termData;
    QSqlQueryModel *modelCounters;
};

#endif // SHIFTREPORTS_H
