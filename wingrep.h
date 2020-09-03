#pragma once

#include <string>
#include <vector>

using namespace std;

typedef struct GrepOptions_tag
{
    bool bRecursiveSearch;
    bool bCaseInsensitive;
    bool bShowLineNumbers;
    bool bMatchWholeWord;
    std::string searchPath;
    std::string searchString;
}GrepOptions_t;

vector<string> GetFilesToSearch(const GrepOptions_t & options);
long long ProcessFile(const string& file, const GrepOptions_t& options);