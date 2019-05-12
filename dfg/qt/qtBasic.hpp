#pragma once

#include "../dfgBase.hpp"
#include "qtIncludeHelpers.hpp"
#include <string>

DFG_BEGIN_INCLUDE_QT_HEADERS
#include <QString>
#include <QTextStream>
DFG_END_INCLUDE_QT_HEADERS

DFG_ROOT_NS_BEGIN {

inline auto cbegin(const QString& cont) -> decltype(cont.constBegin()) {return cont.constBegin();}

inline auto cend(const QString& cont) -> decltype(cont.constEnd()) { return cont.constEnd(); }

} // root namespace

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(io) {

// TODO: test
inline void ignore(QTextStream& strm, const int nCount)
{
	for(int i = 0; i<nCount; ++i)
		strm.read(1);
}

// TODO: Check what this should be.
// TODO: test
inline int eofChar(QTextStream&) { return -1; }

// TODO: test
inline int readOne(QTextStream& strm)
{
	const auto s = strm.read(1);
	return (!s.isEmpty()) ? s[0].unicode() : eofChar(strm);
}

// TODO: test
inline auto peekNext(QTextStream& strm) -> decltype(readOne(strm))
{
	const auto pos = strm.pos();
	const auto c = readOne(strm);
	strm.seek(pos);
	return c;
}
			
// TODO: test
// TODO: revise
inline bool isStreamInReadableState(QTextStream& strm)
{
	if (strm.device())
		return strm.device()->isReadable() && !strm.atEnd();
	else
		return (!strm.atEnd() && strm.status() == QTextStream::Ok);
	//return (strm.device() != nullptr && strm.device()->isReadable() && !strm.atEnd());
}

// TODO: test
// TODO: revise
inline bool isStreamInGoodState(QTextStream& strm) { return isStreamInReadableState(strm); }

// TODO: test
inline void advance(QTextStream& strm, const std::streamsize nCount)
{
	strm.seek(strm.pos() + nCount);
}

}} // module io


DFG_ROOT_NS_BEGIN { DFG_SUB_NS(qt)
{
    // Placeholder implementation for converting QString into a 8-bit string that can given to 
    // file API expecting char* string.
    inline std::string qStringToFileApi8Bit(const QString& sPath)
    {
        return std::string(sPath.toLocal8Bit());
    }

} } // Module qt
