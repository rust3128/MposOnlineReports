#ifndef REQUESTMAPPER_H
#define REQUESTMAPPER_H
#include "httprequesthandler.h"
#include "objectslist.h"
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
};

#endif // REQUESTMAPPER_H
