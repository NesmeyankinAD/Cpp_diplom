#include "ConfigManager.h"
#include "DatabaseFactory.h"

int main_1() 
{
    ConfigManager cm("config.ini");

    auto db = DatabaseFactory::create(cm);

    if (!db->connect("")) 
    {
        return 1;
    }

    auto res = db->query("SELECT 1");
    // std::cout << res << std::endl;

    db->disconnect();
    return 0;
}


#include <iostream>
#include "ConfigManager.h"
#include "DatabaseFactory.h"
#include "IDatabase.h"

int main() 
{
    try 
    {
        // ���� � ini-����� � �����������
        const std::string iniPath = "config.ini";

        // �������� ������������ � ���������� �� ����� �������
        ConfigManager cm(iniPath);
        std::unique_ptr<IDatabase> db = DatabaseFactory::create(cm);

        if (!db) 
        {
            std::cerr << "Failed to create database instance." << std::endl;
            return 1;
        }

        // ����������� (������ ������ �������� ������������� ����� ConfigManager)
        if (!db->connect("")) 
        {
            std::cerr << "Database connection failed." << std::endl;
            return 1;
        }

        // ������ �������� �������: ��������� ������ ��
        std::string resp = db->query("SELECT 'OK' AS status;");
        std::cout << "DB response:\n" << resp << std::endl;

        // �� ���������� ����� ������� ������ �������� (���������� � �.�.)
        db->disconnect();
    }
    catch (const std::exception& ex) 
    {
        std::cerr << "Error: " << ex.what() << std::endl;
        return 1;
    }

    return 0;
}