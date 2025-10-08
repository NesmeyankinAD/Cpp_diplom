#pragma once
#include <string>

class IDatabase 
{
public:
	virtual ~IDatabase() = default;

	// Установить соединение по строке подключения
	virtual bool connect(const std::string& connectionString) = 0;

	// Отключиться
	virtual void disconnect() = 0;

	// Статус подключения
	virtual bool isConnected() const = 0;

	// Выполнить SQL-запрос и вернуть результат как строку
	virtual std::string query(const std::string& sql) = 0;
};