#pragma once

#include <RED4ext/Scripting/Natives/Generated/game/ui/CharacterCustomizationSystem.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/ui/CharacterCustomizationState.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/ui/CharacterCustomizationPreset.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/data/Item_Record.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/data/AttachmentSlot_Record.hpp>
#include <RED4ext/Scripting/Natives/Generated/ink/GameSettingsResource.hpp>
#include <RED4ext/Scripting/Natives/CResource.hpp>
#include <RED4ext/Scripting/Natives/Generated/world/RuntimeSystemEntityAppearanceChanger.hpp>
#include "Game/Utils.h"

using namespace Red;

constexpr auto CreateHandle_CharacterCustomizationState = Core::RawFunc<2710710832UL, 
    Handle<game::ui::CharacterCustomizationState> * (*)(Handle<game::ui::CharacterCustomizationState> *)>();


constexpr auto CharacterCustomizationState_InitFromPreset = Core::RawFunc<1993027039UL, 
    Handle<game::ui::CharacterCustomizationState> * (*)(game::ui::CharacterCustomizationState *, Handle<game::ui::CharacterCustomizationPreset> *)>();

constexpr auto CharacterCustomizationState_Serialize = Core::RawFunc<108403442UL, 
    void (*)(game::ui::CharacterCustomizationState *, CMPStream *)>();

// constexpr auto gameTransactionSystem_AddItemToSlot = Core::RawFunc<56564470UL, 
//     void (*)(game::TransactionSystem *, const AddItemToSlotContext *)>();

// Patch 2.5: the appearance getters used to be called through vtable slots reversed for 2.2.
// They are now resolved by address hash, these are the same functions ArchiveXL hooks on 2.5
// (CharacterCustomizationState_Get*Appearances1).
using AppearanceKeys = DynArray<world::EntityAppearanceChangeParameter::Key>;

constexpr auto CharacterCustomizationState_GetHeadAppearances = Core::RawFunc<4051123539UL,
    void (*)(game::ui::ICharacterCustomizationState *, CName, bool, AppearanceKeys &)>();

constexpr auto CharacterCustomizationState_GetBodyAppearances = Core::RawFunc<4014159024UL,
    void (*)(game::ui::ICharacterCustomizationState *, CName, bool, AppearanceKeys &)>();

constexpr auto CharacterCustomizationState_GetArmsAppearances = Core::RawFunc<110771572UL,
    void (*)(game::ui::ICharacterCustomizationState *, CName, bool, AppearanceKeys &)>();

// Patch 2.5: the isBodyGenderMale field offset is not verified, go through RTTI instead
inline bool IsBodyGenderMale(game::ui::CharacterCustomizationState * aState)
{
    bool isMale = true;
    if (!Red::CallVirtual(aState, "IsBodyGenderMale", isMale))
        spdlog::warn("IsBodyGenderMale is not callable through RTTI, assuming male body");
    return isMale;
}

constexpr auto GetGameSettings = Core::RawFunc<394336209UL,
    ink::GameSettingsResource const * (*)(void)>();

// constexpr auto ItemID_SetUniqueCounterValue = Core::RawFunc<1713639304UL, void (*)(ItemID *)>();

constexpr auto ItemID_Create = Core::RawFunc<3020231866UL, ItemID * (*)(ItemID *, TweakDBID, int64_t seed)>();

constexpr auto GetItemRecord = Core::RawFunc<3236045910UL, 
    Handle<game::data::Item_Record> * (Handle<game::data::Item_Record>::*)(TweakDBID)>();

constexpr auto GetPlacementSlot = Core::RawFunc<1934369954UL, 
    Handle<game::data::AttachmentSlot_Record> * (game::data::Item_Record::*)(Handle<game::data::AttachmentSlot_Record> *, int32_t)>();

constexpr auto EvalArrayTweakDBID = Core::RawFunc<523315084UL, 
    DynArray<TweakDBID> const & (game::data::Val::*)(void)>();

constexpr auto EvalCName = Core::RawFunc<3648983380UL, 
    CName const & (game::data::Val::*)(void)>();

constexpr auto ResolveSuffix = Core::RawFunc<4124714348UL, 
    CString * (*)(void *, CString *, Handle<game::data::Item_Record> &, bool isMale)>();

constexpr auto ResolveVisualTags = Core::RawFunc<1337337421UL, 
    CString * (*)(CName, CString *, Handle<game::data::Item_Record> &)>();

constexpr auto GetItemAppearanceName = Core::RawFunc<3029088864UL, 
    CString * (*)(CString *, Handle<game::Object> const &, Handle<game::Object> const &, Handle<game::data::Item_Record> const &, ItemID const &)>();

template <class T>
struct Span 
{
    T * start;
    T * end;
};

constexpr auto ScheduleSynchronizedAppearanceChanges = Core::RawFunc<386815609UL, 
    void (*)(world::RuntimeSystemEntityAppearanceChanger *, 
        WeakHandle<game::Object> &, 
        Span<world::EntityAppearanceChangeParameter::Key> * old_keys,
        Span<world::EntityAppearanceChangeParameter::Key> * new_keys,
        std::function<void (void)> const &,
        uint32_t //io::EAsyncPriority
    )>();

// CharacterCustomizationSystem
// THandle<game::ui::CharacterCustomizationState> unk78

// could use game::ui::CharacterCustomizationState::Serialize to pass data to server

inline Handle<game::ui::CharacterCustomizationState> * GetCustomizationState(game::ui::CharacterCustomizationSystem * self) 
{
    return reinterpret_cast<Handle<game::ui::CharacterCustomizationState> *>((uintptr_t)self + 0x78);
}
