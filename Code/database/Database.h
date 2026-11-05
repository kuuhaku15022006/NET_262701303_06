
#ifndef DATABASE_H
#define DATABASE_H

#include <QSqlDatabase>

class Database
{
public:
    bool connect();
    bool initialize();

private:
    QSqlDatabase db;
};

#endif
