#pragma once

#include <RED4ext/Map.hpp>

namespace Game
{
enum class EScriptDefinition : uint8_t
{
    Type = 0,
    Class,
    Constant, // Values for Enums and Bitfields
    Enum,
    Bitfield,
    Function,
    Parameter,
    LocalVar,
    Property,
    SourceFile,
};

enum class EScriptVisibility : uint8_t
{
    Public = 0,
    Protected,
    Private,
};

struct ScriptDefinition
{
    virtual Red::Memory::IAllocator* GetAllocator() = 0; // 00
    virtual ~ScriptDefinition() = 0;                     // 08
    virtual EScriptDefinition GetType() = 0;             // 10
    virtual EScriptVisibility GetVisibility() = 0;       // 18
    virtual ScriptDefinition* GetParent() = 0;           // 20
    virtual Red::CName GetName() = 0;                    // 28
    virtual bool IsNative() = 0;                         // 30

    Red::CName name;     // 08
    uint64_t unk10;      // 10
    uint8_t unk18[0x28]; // 18 - patch 2.5, layouts below follow Codeware v1.21.0
};

enum class EScriptType : uint8_t
{
    Primitive = 0,
    Class, // Class/Enum/Bitfield
    StrongHandle,
    WeakHandle,
    DynArray,
    StaticArray,
    ScriptRef,
};

struct ScriptType : ScriptDefinition
{
    Red::CBaseRTTIType* rttiType; // 40
    ScriptDefinition* innerType;  // 48
    int32_t arraySize;            // 50
    EScriptType metaType;         // 54
};
RED4EXT_ASSERT_OFFSET(ScriptType, innerType, 0x48);
RED4EXT_ASSERT_OFFSET(ScriptType, metaType, 0x54);

struct ScriptProperty : ScriptDefinition
{
    struct Flags
    {
        uint16_t isNative : 1;     // 00
        uint16_t b01 : 1;          // 01
        uint16_t b02 : 1;          // 02
        uint16_t isConst : 1;      // 03
        uint16_t b04 : 1;          // 04
        uint16_t b05 : 1;          // 05
        uint16_t isPersistent : 1; // 06
        uint16_t b07 : 1;          // 07
        uint16_t b08 : 1;          // 08
    };

    Red::CProperty* rttiProp;                          // 40
    ScriptDefinition* parent;                          // 48
    Flags flags;                                       // 50
    Red::Map<Red::CString, Red::CString> defaults;     // 58 - engine map, not the project's Map alias
    EScriptVisibility visibility;                      // 80
    Red::HashMap<Red::CName, Red::CString> attributes; // 88
    ScriptType* type;                                  // B8
};
RED4EXT_ASSERT_OFFSET(ScriptProperty, type, 0xB8);

struct ScriptClass;
struct ScriptFunction;

struct ScriptParameter : ScriptDefinition
{
    struct Flags
    {
        uint8_t isOptional : 1;     // 00
        uint8_t isOut : 1;          // 01
        uint8_t isShortCircuit : 1; // 02 - Got from redscript
        uint8_t isConst : 1;        // 03
    };

    Red::CProperty* rttiProp; // 40
    ScriptFunction* function; // 48
    ScriptType* type;         // 50
    Flags flags;              // 58
};

struct ScriptLocalVar : ScriptDefinition
{
    Red::CProperty* rttiProp; // 40
    ScriptFunction* function; // 48
    ScriptType* type;         // 50
    uint8_t unk58;            // 58
};

struct ScriptFileInfo : ScriptDefinition
{
    uint32_t crc;      // 40
    uint32_t index;    // 44
    uint32_t hash;     // 48 - Murmur3 of path
    Red::CString path; // 50
};

struct ScriptFunction : ScriptDefinition
{
    struct Flags
    {
        uint32_t isStatic : 1;     // 00
        uint32_t isExec : 1;       // 01
        uint32_t isTimer : 1;      // 02
        uint32_t isFinal : 1;      // 03
        uint32_t isNative : 1;     // 04
        uint32_t b05 : 1;          // 05
        uint32_t b06 : 1;          // 06
        uint32_t isEvent : 1;      // 07
        uint32_t b08 : 1;          // 08
        uint32_t b09 : 1;          // 09
        uint32_t b10 : 1;          // 0A
        uint32_t b11 : 1;          // 0B
        uint32_t b12 : 1;          // 0C
        uint32_t b13 : 1;          // 0D
        uint32_t b14 : 1;          // 0E
        uint32_t isConst : 1;      // 0F
        uint32_t isThreadsafe : 1; // 10
        uint32_t isScripted : 1;   // 11 - Seems pointless, for all functions isScripted != isNative
    };

    Red::CBaseFunction* rttiFunc;                      // 40
    ScriptClass* parent;                               // 48
    Red::CName shortName;                              // 50
    Red::DynArray<ScriptParameter*> params;            // 58
    Red::DynArray<ScriptLocalVar*> localVars;          // 68
    Red::HashMap<Red::CName, Red::CString> attributes; // 78
    ScriptType* returnType;                            // A8 - not verified for 2.5
    uint8_t unkB0[0xF8 - 0xB0];                        // B0 - 2.5 inserted another hash map at B8
    // Patch 2.5: verified in game, all "static native" functions have 0x04000011 here.
    // Codeware v1.21.0 keeps the 2.2 tail (flags at C0), which is wrong for 2.5.
    Flags flags;                                       // F8
    // operatorName, visibility, code, sourceFile... are not verified for 2.5 and unused
};
RED4EXT_ASSERT_OFFSET(ScriptFunction, params, 0x58);
RED4EXT_ASSERT_OFFSET(ScriptFunction, flags, 0xF8);

struct ScriptClass : ScriptDefinition
{
    struct Flags
    {
        uint8_t isNative : 1;     // 00
        uint8_t isImportOnly : 1; // 01
        uint8_t isAbstract : 1;   // 02
        uint8_t isFinal : 1;      // 03
        uint8_t isStruct : 1;     // 04
    };

    Red::CClass* rttiClass;                            // 40
    ScriptClass* parent;                               // 48
    Red::DynArray<ScriptProperty*> properties;         // 50
    Red::DynArray<ScriptProperty*> overrides;          // 60
    uint8_t unk70[0x20];                               // 70
    Red::DynArray<ScriptFunction*> functions;          // 90
    Red::HashMap<Red::CName, Red::CString> attributes; // A0
    EScriptVisibility visibility;                      // D0
    Flags flags;                                       // D1
};
RED4EXT_ASSERT_OFFSET(ScriptClass, parent, 0x48);
RED4EXT_ASSERT_OFFSET(ScriptClass, properties, 0x50);
RED4EXT_ASSERT_OFFSET(ScriptClass, functions, 0x90);
RED4EXT_ASSERT_OFFSET(ScriptClass, flags, 0xD1);

struct ScriptBundle
{
    ScriptBundle();
    ~ScriptBundle();

    void* unk00;                                                   // 00
    Red::HashMap<Red::CName, ScriptDefinition*> definitionsByName; // 08
    Red::HashMap<Red::CName, ScriptFunction*> functionsByName;     // 38
    Red::HashMap<Red::CName, ScriptType*> typesByName;             // 68
    Red::DynArray<ScriptFileInfo*> files;                          // 98
    Red::DynArray<ScriptFunction*> globals;                        // A8
    Red::DynArray<void*> enums;                                    // B8
    Red::DynArray<void*> bitfields;                                // C8
    Red::DynArray<ScriptClass*> classes;                           // D8
    Red::DynArray<ScriptType*> types;                              // E8
    Red::DynArray<Red::CString> strings;                           // F8
    // the remaining fields (unk100, typesLock, ...) are not verified for patch 2.5
};
RED4EXT_ASSERT_OFFSET(ScriptBundle, definitionsByName, 0x08);
RED4EXT_ASSERT_OFFSET(ScriptBundle, classes, 0xD8);

struct StringView
{
    const char* str;
    uint32_t length;
};

struct ScriptReport
{
    virtual ~ScriptReport() = default;
    virtual void AddValidationError(StringView&) = 0;

    bool fillErrors;
};
} // namespace Game
