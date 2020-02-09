#ifndef GLOBAL_H
#define GLOBAL_H
#include <QCoreApplication>
#include "filelogger.h"
#include "staticfilecontroller.h"
#include "httpsessionstore.h"

using namespace stefanfrings;

/** Redirects log messages to a file */
extern FileLogger* logger;

extern StaticFileController* staticFileController;

extern HttpSessionStore* sessionStore;

extern QString searchConfigFile();
#endif // GLOBAL_H
