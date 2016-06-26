#pragma once

#include "dfgDefs.hpp"
#include "ReadOnlySzParam.hpp"
#include <fstream>
#include "os/fileSize.hpp"

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
		#ifdef _WIN32
				auto p = func(NULL, 0);
				std::basic_string<Char_T> s;
				if (p)
					s = p;
				free(p);
				return std::move(s);
		#else
				// Note: getcwd(NULL, 0) might not be defined as it is for _getcwd. See documentation.
		#error Not implemented. See getcdw.
		#endif
	}
} // DFG_DETAIL_NS

// TODO: test
inline std::string getCurrentWorkingDirectoryC()
{
	return DFG_DETAIL_NS::getCurrentWorkingDirectoryImpl<char>(&_getcwd);
}

// TODO: test
inline std::wstring getCurrentWorkingDirectoryW()
{
	return DFG_DETAIL_NS::getCurrentWorkingDirectoryImpl<wchar_t>(&_wgetcwd);
}

#if defined(_WIN32)
	// TODO: test
	inline int setCurrentWorkingDirectory(ConstCharPtr pPath)
	{
		return (pPath) ? _chdir(pPath) : -1;
	}

	// TODO: test
	inline int setCurrentWorkingDirectory(ConstWCharPtr pPath)
	{
		return (pPath) ? _wchdir(pPath) : -1;
	}
#endif //defined(_WIN32)

#if defined(_WIN32)
	// Checks whether file or folder exists and whether it has the given mode.
	// TODO: test
	enum FileMode {FileModeExists = 0, FileModeRead = 4, FileModeWrite = 2, FileModeReadWrite = 6};
	inline bool isPathFileAvailable(ConstCharPtr pszFilePath, FileMode fm) {return (_access(pszFilePath, fm) == 0);}
	inline bool isPathFileAvailable(ConstWCharPtr pszFilePath, FileMode fm) {return (_waccess(pszFilePath, fm) == 0);}

	// Returns true iff. pszPath is a filename(i.e. not an absolute or relative path), false otherwise.
	// TODO: test
	inline bool isPathFilePath(ConstCharPtr pszPath) {return (PathIsFileSpecA(pszPath) != 0);}
	inline bool isPathFilePath(ConstWCharPtr pszPath) {return (PathIsFileSpecW(pszPath) != 0);}

	// If extension is found, returns pointer to "." preceding the extension.
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
	template <class Char_T>
	inline ConstCharPtr pathFilename(const Char_T* const pszPath)
	{
		std::basic_string<Char_T> str(pszPath); // Redundant copy
		const auto nPos = str.find_last_of("\\/");
		if (nPos != str.npos)
			return pszPath + nPos + 1;
		else
			return pszPath; // No / nor \ found, assume that the whole string is the filename.
	}

	namespace DFG_DETAIL_NS
	{
		inline DWORD getModuleFileName(HMODULE hModule, char* pOut, DWORD nBufferSizeInChars) {return ::GetModuleFileNameA(hModule, pOut, nBufferSizeInChars);}
		inline DWORD getModuleFileName(HMODULE hModule, wchar_t* pOut, DWORD nBufferSizeInChars) {return ::GetModuleFileNameW(hModule, pOut, nBufferSizeInChars);}
	};

	template <class Char_T>
	inline std::basic_string<Char_T> getExecutableFilePath()
	{
		typedef std::basic_string<Char_T> StringT;
		Char_T buffer[MAX_PATH];
		const DWORD nLength = DFG_DETAIL_NS::getModuleFileName(NULL, buffer, CountOf(buffer));
		if (nLength == DFG_COUNTOF(buffer))
		{
			std::vector<Char_T> buf(10*DFG_COUNTOF(buffer));
			const DWORD nLength = DFG_DETAIL_NS::getModuleFileName(NULL, &buf[0], buf.size());
			if (nLength == buf.size())
				buf[0] = '\0';
			return StringT(&buf[0]);
		}
		else
			return StringT(buffer);
	}

#endif // _WIN32



}} // module os
