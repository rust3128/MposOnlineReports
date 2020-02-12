#ifndef REQUESTMAPPER_H
#define REQUESTMAPPER_H
#include "httprequesthandler.h"
#include "objectslist.h"
#include "shiftslist.h"
#include "shiftreports.h"
#include <QSqlQueryModel>

using namespace stefanfrings;

class RequestMapper : public HttpRequestHandler
{
    Q_OBJECT
public:
    RequestMapper(QObject *parent = nullptr);

    // HttpRequestHandler interface
public:
    void service(HttpRequest &request, HttpResponse &response);

private:
    uint getUserID(QString login, QString pass);
private:
    QSqlQueryModel *modelUsers;
    uint userID;
    ObjectsList objectsList;
    ShiftsList shiftList;
    ShiftReports shiftReport;
};

#endif // REQUESTMAPPER_H
