#ifndef OBJECTSLIST_H
#define OBJECTSLIST_H
#include "DataBase/database.h"
#include "httprequesthandler.h"

using namespace stefanfrings;
class ObjectsList : public HttpRequestHandler
{
    Q_OBJECT
public:
    ObjectsList(QObject *parent = nullptr);

    // HttpRequestHandler interface
public:
    void service(HttpRequest &request, HttpResponse &response);
private:
    DataBase *db;
};

#endif // OBJECTSLIST_H
