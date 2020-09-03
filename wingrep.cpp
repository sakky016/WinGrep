#include <filesystem>
#include <fstream>
#include <iostream>
#include <thread>
#include "wingrep.h"


//---------------------------------------------------------------------------------------------
// @name                : GetFilesToSearch
//
// @description         : Retrieves all files present in the specified search path.
//
// @returns             : Vector of files that are required to be parsed
//---------------------------------------------------------------------------------------------
vector<string> GetFilesToSearch(const GrepOptions_t& options)
{
    vector<string> files;
    if (filesystem::exists(options.searchPath))
    {
        if (options.bRecursiveSearch)
        {
            for (const auto& entry : filesystem::recursive_directory_iterator(options.searchPath))
            {
                files.push_back(entry.path().string());
            }
        }
        else
        {
            for (const auto& entry : filesystem::directory_iterator(options.searchPath))
            {
                files.push_back(entry.path().string());
            }
        }
    }

    return files;
}


//---------------------------------------------------------------------------------------------
// @name                : ProcessFile
//
// @description         : Searches the provided file for the search_string as per the options
//                        specified. If found, displays the information on console.
//
// @returns             : Count of matches found in the file.
//---------------------------------------------------------------------------------------------
long long ProcessFile(const string& file, const GrepOptions_t& options)
{
    long long results = 0;
    std::ifstream infile(file.c_str());

    std::string line;
    long lineNumber = 1;
    while (std::getline(infile, line))
    {
        if (!line.empty())
        {
            size_t foundIndex = string::npos;
            bool bFoundMatch = false;
            if (options.bCaseInsensitive)
            {
                std::string tmpSearchString(options.searchString);
                std::string tmpLine(line);
                std::transform(options.searchString.begin(), options.searchString.end(), tmpSearchString.begin(), ::toupper);
                std::transform(line.begin(), line.end(), tmpLine.begin(), ::toupper);
                foundIndex = tmpLine.find(tmpSearchString);
            }
            else
            {
                foundIndex = line.find(options.searchString);
            }

            if (foundIndex != string::npos)
            {
                bFoundMatch = true;

                // Match whole word if option is specified
                if (options.bMatchWholeWord)
                {
                    if (foundIndex == 0)
                    {
                        // Found at the beginning of the line
                        size_t wordEndIndex = foundIndex + options.searchString.size();
                        if (line[wordEndIndex] != ' ')
                        {
                            bFoundMatch = false;
                        }
                    }
                    else if (foundIndex + options.searchString.size() == line.size())
                    {
                        // Found at end of the line
                        if (line[foundIndex - 1] != ' ')
                        {
                            bFoundMatch = false;
                        }
                    }
                    else
                    {
                        // Found somewhere in the middle
                        if (line[foundIndex - 1] != ' ' || line[foundIndex + options.searchString.size()] != ' ')
                        {
                            bFoundMatch = false;
                        }

                    }
                }
            }

            // Match is found, show result on console
            if (bFoundMatch)
            {
                std::cout << file << ": " << line;
                if (options.bShowLineNumbers)
                {
                    cout << ": " << lineNumber;
                }

                cout << endl;
                results++;
            }
        }

        lineNumber++;
    }

    return results;
}