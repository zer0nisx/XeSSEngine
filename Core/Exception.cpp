#include "Exception.h"
#include "Logger.h"

#ifdef _WIN32
#include <comdef.h>
#endif

namespace XeSS {

void ThrowIfFailed(long hr, const std::string& message) {
    if (FAILED(hr)) {
#ifdef _WIN32
        _com_error err(hr);
        std::string errorMsg = message + " (HRESULT: 0x" +
            std::to_string(hr) + ", " + std::string(err.ErrorMessage()) + ")";
#else
        std::string errorMsg = message + " (Error code: " + std::to_string(hr) + ")";
#endif

        XESS_ERROR("HRESULT Error: {}", errorMsg);
        throw GraphicsException(errorMsg);
    }
}

} // namespace XeSS
