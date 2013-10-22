
#include "FileUtil.h"
#include <boost/foreach.hpp>

#ifdef _WIN32
#include <Windows.h>
#endif	// ifdef WIN32


namespace FileUtil
{
    
    std::string replaceBackslash(const std::string& path)
    {
        std::string npath;
        BOOST_FOREACH( char ch, path )
        {
            if (ch == 0x5c) npath += '/';
            else npath += ch;
        }
        return npath;
    }
    

#ifdef _WIN32

    
    static std::string getPath(const std::string& fileName)
    {
        char drive[_MAX_DRIVE];
        char dir[_MAX_DIR];
        char fname[_MAX_FNAME];
        char ext[_MAX_EXT];

        _splitpath_s(fileName.c_str(), drive, _MAX_DRIVE, dir, _MAX_DIR, fname, _MAX_FNAME, ext, _MAX_EXT);

        std::string path(drive);
        path += dir;
        return path;
    }


    std::vector<std::string> findPath(const std::string& pattern)
    {
        WIN32_FIND_DATA fd;

        std::string dir = getPath(pattern);

        std::vector<std::string> list;
        HANDLE handle = FindFirstFileEx(
            pattern.c_str(),
    #if _MSC_VER > 1600
            FindExInfoBasic,
    #else
            FindExInfoStandard,
    #endif
            &fd,
            FindExSearchNameMatch,
            NULL,
            0);

        if (handle != INVALID_HANDLE_VALUE)
        {
            while (true)
            {
                if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
                {
                    std::string fullPath = dir + fd.cFileName;
                    list.push_back(fullPath);
                }

                if (!FindNextFile(handle, &fd))
                {
                    if (GetLastError() == ERROR_NO_MORE_FILES)
                    {
                        break;
                    }
                    else
                    {
                        break;
                    }
                }
            }
            FindClose(handle);
        }
        return list;
    }

#endif	// ifdef WIN32

}	// namespace FileUtil

