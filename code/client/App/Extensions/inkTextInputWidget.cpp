#include "Core/Stl.hpp"
#include <RED4ext/Scripting/Natives/Generated/ink/TextInputWidget.hpp>

struct TextInputExtension : RED4ext::ink::TextInputWidget
{
    // Patch 2.5: disabled until verified, caretPosition (0x434) was reversed for 2.2
    void MoveCaret(int32_t position)
    {
        spdlog::warn("[2.5] inkTextInput::MoveCaret is disabled until caretPosition offset is verified");
    }
};

RTTI_EXPAND_CLASS(RED4ext::ink::TextInputWidget, { RTTI_METHOD_FQN(TextInputExtension::MoveCaret); });