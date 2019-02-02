#pragma once

#include "../dfgDefs.hpp"
#include "../rand.hpp"
#include "removeFile.hpp"
#include "renameFileOrDirectory.hpp"
#include "../io/OfStream.hpp"
#include <string>
#include "../io/BasicOmcByteStream.hpp"
#include "../os.hpp"

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(os) {

    // An output file for dumping complete byte collection to a destination file without overwriting existing file before all bytes have been succesfully written to an intermediate location.
    // Notable properties:
    //  -If file exists, it won't be overwritten and left in corrupted state if application terminates while writing.
    //      -In such case the original file won't be modified.
    // TODO: default intermediate memory stream should have partly contiguous storage instead of completely contiguous.
    template <class IntermediateFileStream_T = DFG_MODULE_NS(io)::DFG_CLASS_NAME(OfStream), class IntermediateMemoryStream_T = DFG_MODULE_NS(io)::BasicOmcByteStream<>>
    class OutputFile_completeOrNone
    {
    public:
        typedef std::string PathStringT;
        typedef IntermediateFileStream_T    IntermediateFileStreamT;
        typedef IntermediateMemoryStream_T  IntermediateMemoryStreamT;

        enum ErrorCode
        {
            ErrorCode_unableToRemoveDestination     = -1,
            ErrorCode_unableToMoveToDestination     = -2,
            ErrorCode_unableToWriteToDestination    = -3,
            ErrorCode_alreadyWritten                = -4
        };

        OutputFile_completeOrNone(PathStringT sPath)
            : m_pathDestination(std::move(sPath))
            , m_bAutoDiscardIntermediateFile(true)
            , m_bFinalDestinationWritten(false)
        {
        }

        ~OutputFile_completeOrNone()
        {
            if (!m_bFinalDestinationWritten)
                writeIntermediateToFinalLocation();
            if (m_bAutoDiscardIntermediateFile)
                discardIntermediate();
        }
        
        IntermediateFileStreamT& intermediateFileStream()
        {
            m_bFinalDestinationWritten = false;
            if (!m_intermediateStream.is_open())
            {
                m_pathIntermediate = generateIntermediatePath(m_pathDestination);
                DFG_ASSERT_CORRECTNESS(!m_pathIntermediate.empty());
                if (!m_pathIntermediate.empty())
                    m_intermediateStream.open(m_pathIntermediate);
            }
                
            return m_intermediateStream;
        }

        IntermediateMemoryStreamT& intermediateMemoryStream(const size_t nReserveHint = 0)
        {
            m_bFinalDestinationWritten = false;
            if (nReserveHint > 0)
                m_intermediateMemoryStream.reserve(nReserveHint);
            return m_intermediateMemoryStream;
        }

        // Returns 0 on success, otherwise ErrorCode
        int writeIntermediateToFinalLocation()
        {
            const bool bIntermediateFile = m_intermediateStream.is_open();

            if (bIntermediateFile)
            {
                // Current logics:
                //  1. Remove original from destination path.
                //  2. Move intermediate to destination path.
                // Another approach would be to move original to temp location, move intermediate to destination, remove original from temp location

                // Close the intermediate stream so that the file can be accessed.
                m_intermediateStream.close();

                // Remove current destination if exists.
                if (isPathFileAvailable(m_pathDestination, FileModeExists))
                {
                    const bool bRemoveSuccess = (0 == removeFile(m_pathDestination.c_str()));
                    if (!bRemoveSuccess)
                    {
                        m_bAutoDiscardIntermediateFile = false;
                        return ErrorCode_unableToRemoveDestination;
                    }
                }

                // Move intermediate to destination.
                const bool bRenameSuccess = (0 == renameFileOrDirectory_cstdio(m_pathIntermediate, m_pathDestination));
                if (!bRenameSuccess)
                {
                    m_bAutoDiscardIntermediateFile = false;
                    return ErrorCode_unableToMoveToDestination;
                }
            }
            else // Case: memory intermediate
            {
                const auto& bytes = m_intermediateMemoryStream.releaseData();
                const auto bWriteSuccess = DFG_MODULE_NS(io)::DFG_CLASS_NAME(OfStream)::dumpBytesToFile_overwriting(m_pathDestination, bytes.data(), bytes.size());
                if (!bWriteSuccess)
                    return ErrorCode_unableToWriteToDestination;
            }

            m_bFinalDestinationWritten = true;
            return 0;
        }

        void discardIntermediate()
        {
            m_intermediateStream.close();
            if (!m_pathIntermediate.empty())
                removeFile(m_pathIntermediate.c_str());
            m_intermediateMemoryStream.releaseData();
        }

        static PathStringT generateIntermediatePath(const PathStringT& sDestinationPath)
        {
            PathStringT s = sDestinationPath + ".part";
            if (!isPathFileAvailable(s, FileModeExists))
                return s;

            const char chars[] = "0123456789abcdefghijklmnopqrstuvwxyz";
            auto randEng = DFG_MODULE_NS(rand)::createDefaultRandEngineRandomSeeded();
            auto distrEng = DFG_MODULE_NS(rand)::makeDistributionEngineUniform<int>(&randEng, 0, DFG_COUNTOF_SZ(chars));
            const auto randChar = [&]() { return chars[distrEng()]; };

            for (int i = 0; i < 10; ++i) // Try up to 10 different combinations of random 4 character sequences: there are 36^4 = 1679616 possibilities.
            {
                char szRandPart[5] = { randChar(), randChar(), randChar(), randChar(), '\0' };
                auto sRandomAdded = s + szRandPart;
                if (!isPathFileAvailable(sRandomAdded, FileModeExists))
                    return sRandomAdded;
            }
            // Couldn't generate path -> return empty.
            return PathStringT();
        }

    public:
        PathStringT m_pathDestination;
        PathStringT m_pathIntermediate;
        IntermediateFileStreamT m_intermediateStream;
        IntermediateMemoryStreamT m_intermediateMemoryStream;
        bool m_bAutoDiscardIntermediateFile;
        bool m_bFinalDestinationWritten;
        
    }; // Class OutputFile_completeOrNone

} } // module namespace
