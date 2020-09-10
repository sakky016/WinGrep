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
    cout << "  -r        : Recursive search\n";
    cout << "  -i        : Case insensitive search\n";
    cout << "  -n        : Show line numbers in search result\n";
    cout << "  -w        : Match whole word\n";
    cout << "  -j <num>  : Uses <num> number of worker threads to search.\n"
            "              If nothing or an invalid value is specified then hardware_concurrency\n"
            "              is used to determine number of worker threads\n";
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
                else if (param[j] == 'j')
                {
                    nextParamIndex++;
                    string threadsStr(argv[nextParamIndex]);
                    if (threadsStr.empty())
                    {
                        bValidArguments = false;
                    }
                    else
                    {
                        try
                        {
                            options.threadsToUse = stoi(threadsStr);
                        }
                        catch (invalid_argument er)
                        {
                            options.threadsToUse = 0;
                        }
                    }
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
    
    //cout << "Search Path: " << options.searchPath << endl;
    //cout << "Search String: " << options.searchString << endl;

    long long results = 0;
    results = GetSearchResults(options);
    cout << " Found " << results << " result(s)\n";


    return 0;
}