#include "Core/Stl.hpp"

// Vanilla FTLog/LogChannel are no-ops in retail builds (LogChannel only reaches CET's console),
// so scripts get a native that writes to CyberpunkMP.log.
namespace
{
void CyberpunkMPLog(const Red::CString& aText)
{
    spdlog::info("[Script] {}", aText.c_str());
}
} // namespace

RTTI_DEFINE_GLOBALS({ RTTI_FUNCTION(CyberpunkMPLog); });
