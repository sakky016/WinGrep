#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <thread>
#include "wingrep.h"
#include <queue>

const string EXTENSION_EXCLUSION_LIST[] = { "zip", "obj", "so", "dll" };
const string EXCLUDE_PATH_CONTAINING[] = { "test" };


//---------------------------------------------------------------------------------------------
// Static variables used by the worker threads
//---------------------------------------------------------------------------------------------
static vector<thread> g_workerThreads;
static queue<string> g_pathsToProcess;
static bool g_searchComplete = false;
static long long g_results = 0;
static mutex g_mtx;


//---------------------------------------------------------------------------------------------
// IsValidExtension
//---------------------------------------------------------------------------------------------
static bool IsValidExtension(const string& filepath)
{
    size_t extensionStart = filepath.rfind('.');
    if (extensionStart != string::npos)
    {
        string extension = filepath.substr(extensionStart + 1, filepath.size());
        for (const auto& excludedExtension : EXTENSION_EXCLUSION_LIST)
        {
            if (excludedExtension == extension)
            {
                return false;
            }
        }
    }
    else
    {
        // No extension found
        return false;
    }

}


//---------------------------------------------------------------------------------------------
// IsValidFileToSearch
//---------------------------------------------------------------------------------------------
static bool IsValidFileToSearch(const string & filepath)
{
    
    if (!IsValidExtension(filepath))
    {
        return false;
    }

    return true;
}


//---------------------------------------------------------------------------------------------
// IsPathInExclusionList
//---------------------------------------------------------------------------------------------
static bool IsPathInExclusionList(const string & filepath)
{
    for (const auto& excludePath : EXCLUDE_PATH_CONTAINING)
    {
        if (filepath.find(excludePath) != string::npos)
        {
            return true;
        }
    }

    return false;
}


//---------------------------------------------------------------------------------------------
// DisplayConstraintInfo
//---------------------------------------------------------------------------------------------
static void DisplayConstraintInfo()
{
    cout << "\n--------------------------------------------------------------------\n";
    cout << "Skipping directories containing following word(s):\n";
    for (const auto& excludePath : EXCLUDE_PATH_CONTAINING)
    {
        cout << excludePath << endl;
    }

    cout << "\nSkipping files with following extension(s):\n";
    for (const auto& excludedExtension : EXTENSION_EXCLUSION_LIST)
    {
        cout << excludedExtension << endl;
    }
    cout << "--------------------------------------------------------------------\n";
}


//---------------------------------------------------------------------------------------------
// @name                : AddPathToProcess
//
// @description         : Adds a path to the queue of directory paths that needs to be 
//                        processed by worker threads.
//
// @returns             : Nothing
//---------------------------------------------------------------------------------------------
static void AddPathToProcess(const string& path)
{
    g_mtx.lock();
    g_pathsToProcess.push(path);
    g_mtx.unlock();
}


//---------------------------------------------------------------------------------------------
// @name                : GetPathToProcess
//
// @description         : Fetches the directory that needs processing from front of the queue.
//
// @returns             : Directory path
//---------------------------------------------------------------------------------------------
static string GetPathToProcess()
{
    string path;
    g_mtx.lock();
    if (!g_pathsToProcess.empty())
    {
        path = g_pathsToProcess.front();
        g_pathsToProcess.pop();
    }
    g_mtx.unlock();

    return path;
}


//---------------------------------------------------------------------------------------------
// @name                : SearchLine
//
// @description         : Looks for the search string in the line. Does the searching on the
//                        basis of options provided.
//
// @returns             : true if match found.
//---------------------------------------------------------------------------------------------
static bool SearchLine(const string& line, const GrepOptions_t& options)
{
    bool bFoundMatch = false;
    size_t foundIndex = string::npos;
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

    return bFoundMatch;
}


//---------------------------------------------------------------------------------------------
// @name                : DisplaySearchResult
//
// @description         : Show search result on console as per specified options.
//
// @returns             : Nothing
//---------------------------------------------------------------------------------------------
static void DisplaySearchResult(const string& file, const string& line, int lineNumber, const GrepOptions_t& options)
{
    g_mtx.lock();
    std::cout << file;
    if (options.bShowLineNumbers)
    {
        cout << ":" << lineNumber;
    }
    std::cout << "  " << line << endl;
    g_mtx.unlock();
}


//---------------------------------------------------------------------------------------------
// @name                : ProcessFile
//
// @description         : Searches the provided file for the search_string as per the options
//                        specified. If found, displays the information on console.
//
// @returns             : Count of matches found in the file.
//---------------------------------------------------------------------------------------------
static long long ProcessFile(const string& file, const GrepOptions_t& options)
{
    long long results = 0;
    if (!IsValidFileToSearch(file))
    {
        return results;
    }


    string line;
    long lineNumber = 1;
    ifstream infile(file.c_str());
    while (getline(infile, line))
    {
        if (!line.empty())
        {
            bool bFoundMatch = SearchLine(line, options);

            // Match is found, show result on console
            if (bFoundMatch)
            {
                DisplaySearchResult(file, line, lineNumber, options);
                results++;
            }
        }

        lineNumber++;
    }

    return results;
}


//---------------------------------------------------------------------------------------------
// @name                : GetFilesToSearch
//
// @description         : Retrieves all files present in the specified search path.
//
// @returns             : Vector of files that are required to be parsed
//---------------------------------------------------------------------------------------------
static void ProcessFiles(const string& searchPath, const GrepOptions_t& options)
{
    if (filesystem::exists(searchPath) /*&& !IsPathInExclusionList(searchPath)*/)
    {
        for (const auto& entry : filesystem::directory_iterator(searchPath))
        {
            if (filesystem::is_directory(entry) && options.bRecursiveSearch)
            {
                //g_mtx.lock();
                //cout << "Looking in " << entry.path().string() << "\n";
                //g_mtx.unlock();
                AddPathToProcess(entry.path().string());
            }
            else
            {
                string file = entry.path().string();
                g_results += ProcessFile(file, options);
            }
        }
    }
}

//---------------------------------------------------------------------------------------------
// @name                : DirectoryProcessingWorkerThread
//
// @description         : Worker thread which runs continuously to process a directory. Once 
//                        all g_searchComplete becomes true, this worker terminates execution.
//                        g_searchComplete becomes true when there are no more paths to process.
//
// @returns             : Nothing
//---------------------------------------------------------------------------------------------
static void DirectoryProcessingWorkerThread(const GrepOptions_t& options)
{
    long long directoriesProcessed = 0;
    //g_mtx.lock();
    //cout << "Worker thread " << this_thread::get_id() << " spawned\n";
    //g_mtx.unlock();

    while (!g_searchComplete)
    {
        string path = GetPathToProcess();
        if (!path.empty())
        {
            ProcessFiles(path, options);
            directoriesProcessed++;
        }
    }

    g_mtx.lock();
    //cout << "Worker thread " << this_thread::get_id() << " processed " << directoriesProcessed << " locations\n";
    g_mtx.unlock();
}


//---------------------------------------------------------------------------------------------
// @name                : MarkSearchComplete
//
// @description         : Check if all the directories in the work queue have been processed.
//                        When done, it marks the search complete.
//
// @returns             : Nothing
//---------------------------------------------------------------------------------------------
static void MarkSearchComplete()
{
    while (1)
    {
        // Check if all the locations have been searched
        if (g_pathsToProcess.empty())
        {
            g_mtx.lock();
            g_searchComplete = true;
            g_mtx.unlock();
            break;
        }
    }
}


//---------------------------------------------------------------------------------------------
// @name                : SpawnWorkerThreads
//
// @description         : Creates multiple worker threads each of which is responsible for 
//                        processing a directory.
//
// @returns             : Nothing
//---------------------------------------------------------------------------------------------
static void SpawnWorkerThreads(const GrepOptions_t& options)
{
    unsigned int workerThreadsCount = 0;
    unsigned int hardwareConcurrency = thread::hardware_concurrency();
    if (options.threadsToUse <= 0 || options.threadsToUse > hardwareConcurrency)
    {
        workerThreadsCount = hardwareConcurrency;
    }
    else
    {
        workerThreadsCount = options.threadsToUse;
    }

    for (unsigned int i = 0; i < workerThreadsCount; i++)
    {
        thread workerThread(DirectoryProcessingWorkerThread, options);
        g_workerThreads.push_back(move(workerThread));
    }

    cout << "Spawned " << workerThreadsCount << " worker threads\n";
}


//---------------------------------------------------------------------------------------------
// @name                : WaitForWorkerThreadsToComplete
//
// @description         : Join all the worker threads
//
// @returns             : Nothing
//---------------------------------------------------------------------------------------------
static void WaitForWorkerThreadsToComplete()
{
    for (auto& workerThread : g_workerThreads)
    {
        if (workerThread.joinable())
        {
            workerThread.join();
        }
    }
}


//---------------------------------------------------------------------------------------------
// @name                : GetSearchResults
//
// @description         : This is the core function which processes the search string in the
//                        provided path as per the options(flags) specified.
//
// @returns             : Number of matching search results
//---------------------------------------------------------------------------------------------
long long GetSearchResults(const GrepOptions_t& options)
{
    if (filesystem::exists(options.searchPath))
    {
        SpawnWorkerThreads(options);
        ProcessFiles(options.searchPath, options);
        MarkSearchComplete();
        WaitForWorkerThreadsToComplete();
    }
    else
    {
        cout << "Invalid search path : " << options.searchPath << endl;
    }

    //DisplayConstraintInfo();
    return g_results;
}


