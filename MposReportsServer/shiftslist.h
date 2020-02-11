#ifndef SHIFTSLIST_H
#define SHIFTSLIST_H
#include "httprequesthandler.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlQueryModel>


using namespace stefanfrings;

class ShiftsList : public HttpRequestHandler
{
    Q_OBJECT
public:
    ShiftsList(QObject *parent = nullptr);
    ~ShiftsList();
    // HttpRequestHandler interface
public:
    void service(HttpRequest &request, HttpResponse &response);
private:
    void openObjectDB();
private:
    QSqlDatabase db;
    QSqlDatabase dbObj;
    QSqlQueryModel *modelShifts;
    uint userID;
    uint objectID;
    QStringList termData;
};

#endif // SHIFTSLIST_H
