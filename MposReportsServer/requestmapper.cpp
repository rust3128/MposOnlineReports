#include "requestmapper.h"
#include "global.h"


RequestMapper::RequestMapper(QObject *parent) :
    HttpRequestHandler(parent)
{
    userID = 0;
    modelUsers = new QSqlQueryModel(this);
    modelUsers->setQuery("SELECT c.user_id, c.username, c.userpass FROM clientusers c where c.isactive = 'true'");
}


void RequestMapper::service(HttpRequest &request, HttpResponse &response)
{
    HttpSession session=sessionStore->getSession(request,response,true);

    QByteArray username=request.getParameter("username");
    QByteArray password=request.getParameter("password");
    QByteArray objectID=request.getParameter("objectID");

    qDebug("username=%s",username.constData());
    qDebug("password=%s",password.constData());
    qDebug("objectID=%s",objectID.constData());

    if(username.length() !=0){
        userID=getUserID(QString(username), QString(password));
    }

    QByteArray path=request.getPath();
    qDebug("RequestMapper: path=%s",path.data());

    if(path == "/"){
        response.redirect("/login.html");
        return;
    } else if (path=="/login.html") {
        staticFileController->service(request,response);
    } else if (path == "/objects" && userID > 0){
        session.set("userID",userID);
        objectsList.service(request,response);
    } else if(path == "/shifts" && objectID.length()>0) {
        session.set("objectID",objectID);
        shiftList.service(request,response);
    }
    else {
        response.setStatus(404,"Not found");
        response.write("The URL is wrong, no such document.",true);
    }

    qDebug("RequestMapper: finished request");
}

uint RequestMapper::getUserID(QString login, QString pass)
{
    uint userID = 0;
    const int rowCount = modelUsers->rowCount();
    for(int row=0; row<rowCount; ++row){
        if(modelUsers->data(modelUsers->index(row,1)).toString() == login){
            if(modelUsers->data(modelUsers->index(row,2)).toString() == pass){
                userID = modelUsers->data(modelUsers->index(row,0)).toUInt();
                break;
            }
        }
    }
    return userID;

}
