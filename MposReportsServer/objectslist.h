#ifndef OBJECTSLIST_H
#define OBJECTSLIST_H
#include <QSqlQuery>
#include <QSqlError>
#include "httprequesthandler.h"
#include <QSqlQueryModel>

using namespace stefanfrings;
class ObjectsList : public HttpRequestHandler
{
    Q_OBJECT
public:
    ObjectsList(QObject *parent = nullptr);
    ~ObjectsList();
    // HttpRequestHandler interface
public:
    void service(HttpRequest &request, HttpResponse &response);
private:
    void createModels();

private:
    QSqlDatabase db;
    QSqlQueryModel *modelObjects;
    uint userID;
};

#endif // OBJECTSLIST_H
