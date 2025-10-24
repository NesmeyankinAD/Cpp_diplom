#include "DatabaseFactory.h"
#include "PostgresDatabase.h"

std::unique_ptr<IDatabase> DatabaseFactory::create(const ConfigManager& cm) 
{
    return std::make_unique<PostgresDatabase>(cm.getDBHost(),
                                              cm.getDBPort(),
                                              cm.getDBName(),
                                              cm.getDBUser(),
                                              cm.getDBPassword());
}