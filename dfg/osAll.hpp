#pragma once

#include "os.hpp"
#include "os/fileSize.hpp"
#include "os/memoryMappedFile.hpp"
#include "os/removeFile.hpp"
#include "os/renameFileOrDirectory.hpp"
#include "os/TemporaryFileStream.hpp"

#ifdef _WIN32
	#include "os/win.hpp"
#endif
