#include <filesystem>
#include <iostream>

#include "wingrep.h"

using namespace std;


//---------------------------------------------------------------------------------------------
// @name                : Usage
//
// @description         : Show command usage on console
//
// @returns             : Nothing
//---------------------------------------------------------------------------------------------
static void Usage()
{
    cout << "Usage:\n";
    cout << "WinGrep.exe [options] <path> <search_string>\n";
    cout << "Options:\n";
    cout << "  -r  : Recursive search\n";
    cout << "  -i  : Case insensitive search\n";
    cout << "  -n  : Show line numbers in search result\n";
    cout << "  -w  : Match whole word\n";
    cout << "\n";
}


//---------------------------------------------------------------------------------------------
// @name                : ProcessCommandLineArguments
//
// @description         : Parses the command line arguments and populates the options 
//                        structure required by the WinGrep application.
//
// @returns             : true if valid arguments were passed, false otherwise
//---------------------------------------------------------------------------------------------
static bool ProcessCommandLineArguments(int argc, char* argv[], GrepOptions_t & options)
{
    bool bValidArguments = true;

    if (argc < 3)
    {
        return false;
    }

    string param;
    int nextParamIndex = 1;

    // Process options
    for (int paramIndex = nextParamIndex; paramIndex < argc; paramIndex++)
    {
        param = argv[paramIndex];        
        if (param[0] == '-')
        {
            for (int j = 1; j < param.size(); j++)
            {
                if (param[j] == 'r')
                {
                    options.bRecursiveSearch = true;                    
                }
                else if (param[j] == 'i')
                {
                    options.bCaseInsensitive = true;
                }
                else if (param[j] == 'n')
                {
                    options.bShowLineNumbers = true; 
                }
                else if (param[j] == 'w')
                {
                    options.bMatchWholeWord = true;
                }
            }

            nextParamIndex++;
        }
    }

    // Search Path
    if (nextParamIndex < argc)
    {
        options.searchPath = argv[nextParamIndex];
        nextParamIndex++;
    }

    // Search string
    if (nextParamIndex < argc)
    {
        options.searchString = argv[nextParamIndex];
        nextParamIndex++;
    }

    if (options.searchPath.empty() || options.searchString.empty())
    {
        bValidArguments = false;
    }

    return bValidArguments;
}


//---------------------------------------------------------------------------------------------
//  M A I N
//---------------------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    GrepOptions_t options{};
    bool bValidArguments = ProcessCommandLineArguments(argc, argv, options);
    if (!bValidArguments)
    {
        Usage();
        return 0;
    }
    
    long long results = 0;
    if (filesystem::exists(options.searchPath))
    {
        vector<string> filesToSearch = GetFilesToSearch(options);
        for (const auto& file : filesToSearch)
        {
            results += ProcessFile(file, options);
        }

        cout << " Found " << results << " result(s)\n";
    }
    else
    {
        cout << "Invalid search path : " << options.searchPath << endl;
    }


    return 0;
}