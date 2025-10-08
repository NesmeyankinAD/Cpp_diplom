#pragma once
#include <string>

class IDatabase 
{
public:
	virtual ~IDatabase() = default;

	// ���������� ���������� �� ������ �����������
	virtual bool connect(const std::string& connectionString) = 0;

	// �����������
	virtual void disconnect() = 0;

	// ������ �����������
	virtual bool isConnected() const = 0;

	// ��������� SQL-������ � ������� ��������� ��� ������
	virtual std::string query(const std::string& sql) = 0;
};