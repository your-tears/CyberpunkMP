#include "Core/Stl.hpp"

// The main menu starts the world and the in-world HUD connects once it is up. Script objects do
// not survive the switch between the two game sessions, so the request is kept on the native side.
namespace
{
bool s_autoConnectRequested = false;

void CyberpunkMPRequestAutoConnect()
{
    s_autoConnectRequested = true;
}

bool CyberpunkMPConsumeAutoConnect()
{
    const bool requested = s_autoConnectRequested;
    s_autoConnectRequested = false;
    return requested;
}
} // namespace

RTTI_DEFINE_GLOBALS({
    RTTI_FUNCTION(CyberpunkMPRequestAutoConnect);
    RTTI_FUNCTION(CyberpunkMPConsumeAutoConnect);
});
