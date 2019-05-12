#pragma once

#include "dfgDefs.hpp"
#include "ReadOnlySzParam.hpp"
#include <fstream>
#include "os/fileSize.hpp"
#include "dfgBase.hpp"

#ifdef _WIN32
	#include <Shlwapi.h>
	#include <io.h>
	#include <direct.h> // _getcwd, _chdir

	#pragma comment(lib, "shlwapi")
#else
	#include <unistd.h> // For getcdw
#endif // _WIN32

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(os) {

namespace DFG_DETAIL_NS
{
	template <class Char_T, class Func_T>
	inline std::basic_string<Char_T> getCurrentWorkingDirectoryImpl(Func_T func)
	{
		auto p = func(NULL, 0);
		std::basic_string<Char_T> s;
		if (p)
			s = p;
		free(p);
		return std::move(s);
	}
} // DFG_DETAIL_NS

// TODO: test
inline std::string getCurrentWorkingDirectoryC()
{
#ifdef _WIN32
	return DFG_DETAIL_NS::getCurrentWorkingDirectoryImpl<char>(&_getcwd);
#else
    return DFG_DETAIL_NS::getCurrentWorkingDirectoryImpl<char>(&getcwd);
#endif
}

#ifdef _WIN32
inline std::wstring getCurrentWorkingDirectoryW()
{
	return DFG_DETAIL_NS::getCurrentWorkingDirectoryImpl<wchar_t>(&_wgetcwd);
}
#endif

#if defined(_WIN32)
	// TODO: test
	inline int setCurrentWorkingDirectory(DFG_CLASS_NAME(StringViewSzC) path)
	{
		return _chdir(path.c_str());
	}

	// TODO: test
	inline int setCurrentWorkingDirectory(DFG_CLASS_NAME(StringViewSzW) path)
	{
		return _wchdir(path.c_str());
	}
#else // non-Windows
    inline int setCurrentWorkingDirectory(DFG_CLASS_NAME(StringViewSzC) path)
    {
        return chdir(path.c_str());
    }
#endif //defined(_WIN32)

    enum FileMode { FileModeExists = 0, FileModeRead = 4, FileModeWrite = 2, FileModeReadWrite = 6 };
#if defined(_WIN32)
	// Checks whether file or folder exists and whether it has the given mode.
	// TODO: test
	inline bool isPathFileAvailable(DFG_CLASS_NAME(StringViewSzC) filePath, FileMode fm) {return (_access(filePath.c_str(), fm) == 0);}
	inline bool isPathFileAvailable(DFG_CLASS_NAME(StringViewSzW) filePath, FileMode fm) {return (_waccess(filePath.c_str(), fm) == 0);}
#else
    inline bool isPathFileAvailable(DFG_CLASS_NAME(StringViewSzC) filePath, FileMode fm) { return (access(filePath.c_str(), fm) == 0); }
    inline bool isPathFileAvailable(DFG_CLASS_NAME(StringViewSzW) filePath, FileMode fm)
    {
        return isPathFileAvailable(DFG_MODULE_NS(io)::pathStrToFileApiFriendlyPath(filePath.c_str()), fm);
    }
#endif

#if _WIN32
	// Returns true iff. pszPath is a filename(i.e. not an absolute or relative path), false otherwise.
	// TODO: test
	inline bool isPathFilePath(ConstCharPtr pszPath) {return (PathIsFileSpecA(pszPath) != 0);}
	inline bool isPathFilePath(ConstWCharPtr pszPath) {return (PathIsFileSpecW(pszPath) != 0);}

	// If extension is found, returns pointer to the last "." found (e.g. "test.tar.gz" returns ".gz").
	// If no extension is found, returns pointer to the trailing null character.
	// [in] psz : Pointer to null terminated path.
	// Win: Maximum length of string pointer to by psz is MAX_PATH.
	// Win: "valid file extension cannot contain a space".
	// Win: Needs "shlwapi.lib".
	// Win: Path can use either '\' or '/' (not sure whether mixture would work)
	// TODO: test
	inline ConstCharPtr pathFindExtension(const ConstCharPtr psz)	{return PathFindExtensionA(psz);}
	inline ConstWCharPtr pathFindExtension(const ConstWCharPtr psz)	{return PathFindExtensionW(psz);}

    // Returns pointer to the beginning of filename in given path string using the assumption that
    // the file name begins from the first character after the last directory separator / or \.
    // If no directory separator is found, the given path is assumed to be filename as whole.
    // If given parameter is null, returns empty string.
    template <class Char_T>
    inline const Char_T* pathFilename(const Char_T* const pszPath)
    {
        if (!pszPath)
            return DFG_STRING_LITERAL_BY_CHARTYPE(Char_T, "");
        const Char_T* pszFilenamePart = pszPath;
        for (auto p = pszPath; *p != '\0'; ++p)
        {
            if (*p == '\\' || *p == '/')
                pszFilenamePart = p + 1;
        }
        return pszFilenamePart;
    }

	namespace DFG_DETAIL_NS
	{
        inline DWORD getModuleFileName(HMODULE hModule, char* pOut, DWORD nBufferSizeInChars)    { return ::GetModuleFileNameA(hModule, pOut, nBufferSizeInChars); }
        inline DWORD getModuleFileName(HMODULE hModule, wchar_t* pOut, DWORD nBufferSizeInChars) { return ::GetModuleFileNameW(hModule, pOut, nBufferSizeInChars); }
	};

    template <class Char_T>
    inline std::basic_string<Char_T> getExecutableFilePath()
    {
        typedef std::basic_string<Char_T> StringT;
        Char_T buffer[MAX_PATH];
        const DWORD nLength = DFG_DETAIL_NS::getModuleFileName(NULL, buffer, DFG_COUNTOF(buffer));
        if (nLength == DFG_COUNTOF(buffer))
        {
            std::vector<Char_T> buf(10*DFG_COUNTOF(buffer));
            const DWORD nLength2 = DFG_DETAIL_NS::getModuleFileName(NULL, &buf[0], static_cast<DWORD>(buf.size()));
            if (nLength2 == buf.size())
                buf[0] = '\0';
            return StringT(&buf[0]);
        }
        else
            return StringT(buffer);
    }

#endif // _WIN32



}} // module os
