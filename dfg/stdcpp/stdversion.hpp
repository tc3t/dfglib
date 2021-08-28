// Compatibility header for places that need to include <version> when available

#pragma once

#if __cplusplus > 201703L
    #include <version>
#endif
