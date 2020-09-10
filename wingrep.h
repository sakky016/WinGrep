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
    int threadsToUse;
    std::string searchPath;
    std::string searchString;
}GrepOptions_t;

long long GetSearchResults(const GrepOptions_t& options);