#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include "ConfigManager.h"
#include "IDatabase.h"

class Spider
{
public:
    Spider(const ConfigManager& cm, IDatabase* db);

    // ������ �����
    void start();

private:
    ConfigManager cm_;
    IDatabase* db_;

    std::string startPage_;
    int maxDepth_;

    // ��������������� ������
    bool fetchPage(const std::string& url, std::string& content, std::vector<std::string>& links);
    std::string stripHtml(const std::string& html);
    std::unordered_map<std::string, int> indexWords(const std::string& text);

    // ���� ������
    int upsertDocumentId(const std::string& url);
    int upsertWordId(const std::string& word);
    void storeDocumentWords(int docId, const std::unordered_map<std::string, int>& freqs);

    std::string sqlEscape(const std::string& s);

    // ������� ���������� ������ �� ������ ������
    std::string extractDomain(const std::string& url) const;
    bool isSameDomain(const std::string& url) const;

    // �����: ������� �� ��� �������� �������
    void resetDatabaseForCurrentRun();
};