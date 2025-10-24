#pragma once
#include <memory>
#include "IDatabase.h"
#include "ConfigManager.h"

class DatabaseFactory 
{
public:
	static std::unique_ptr<IDatabase> create(const ConfigManager& cm);
};