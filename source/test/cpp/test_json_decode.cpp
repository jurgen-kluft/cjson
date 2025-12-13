#include "ccore/c_target.h"
#include "cbase/c_memory.h"
#include "cbase/c_runes.h"
#include "cjson/c_json_parser.h"
#include "cjson/c_json_allocator.h"
#include "cjson/c_json_decode.h"
#include "cjson/c_json_encode.h"

#include "cunittest/cunittest.h"

using namespace ncore;

extern unsigned char kyria_json[];
extern unsigned int  kyria_json_len;

struct key_t
{
    key_t()
    {
        m_nob      = false;
        m_index    = 0;
        m_label    = "Q";
        m_w        = 80.0f;
        m_h        = 80.0f;
        m_capcolor = nullptr;
        m_ledcolor = nullptr;
        m_txtcolor = nullptr;
    }

    bool        m_nob;           // home-key (e.g. the F or J key)
    ncore::s16  m_index;         // index in keymap
    const char* m_label;         // label (e.g. "Q")
    float       m_w;             // key width
    float       m_h;             // key height
    ncore::s8   m_capcolor_size; // should become 3 or 4 (RGB or RGBA)
    ncore::s8   m_txtcolor_size; //
    ncore::s8   m_ledcolor_size; //
    float*      m_capcolor;      // color of the key cap, e.g. [ 0.1, 0.1, 0.1, 1.0 ]
    float*      m_txtcolor;      // color of the key label
    float*      m_ledcolor;      // color of the key led

    DCORE_CLASS_PLACEMENT_NEW_DELETE
};

template <> void njson::JsonObjectTypeRegisterFields<key_t>(key_t& base, njson::JsonFieldDescr*& members, s32& member_count)
{
    static njson::JsonFieldDescr s_members[] = {
        njson::JsonFieldDescr("nob", base.m_nob),
        njson::JsonFieldDescr("index", base.m_index),
        njson::JsonFieldDescr("label", base.m_label),
        njson::JsonFieldDescr("w", base.m_w),
        njson::JsonFieldDescr("h", base.m_h),
        njson::JsonFieldDescr("cap_color", base.m_capcolor, base.m_capcolor_size),
        njson::JsonFieldDescr("txt_color", base.m_txtcolor, base.m_txtcolor_size),
        njson::JsonFieldDescr("led_color", base.m_ledcolor, base.m_ledcolor_size),
    };
    members      = s_members;
    member_count = sizeof(s_members) / sizeof(njson::JsonFieldDescr);
}
static njson::JsonObjectTypeDeclr<key_t> json_key("key");

struct keygroup_t
{
    const char* m_name; // name of this group
    float       m_x;    // x position of this group
    float       m_y;    // y position of this group
    float       m_w;    // key width
    float       m_h;    // key height
    float       m_sw;   // key spacing width
    float       m_sh;   // key spacing height
    ncore::u16  m_enum; // enum
    ncore::s16  m_r;    // rows
    ncore::s16  m_c;    // columns
    ncore::s16  m_a;    // angle, -45 degrees to 45 degrees (granularity is 1 degree)
    ncore::s8   m_capcolor_size;
    ncore::s8   m_txtcolor_size;
    ncore::s8   m_ledcolor_size;
    float*      m_capcolor; // color of the key cap
    float*      m_txtcolor; // color of the key label
    float*      m_ledcolor; // color of the key led
    ncore::s16  m_nb_keys;  // number of keys in the array
    key_t*      m_keys;     // array of keys

    DCORE_CLASS_PLACEMENT_NEW_DELETE
};

// You can provide to and from string functions, if not the default to and from string functions are used.
//                                       Bit      0       1       2       3       4       5       6       7
static const u64               enum_values[] = {1 << 0, 1 << 1, 1 << 2, 1 << 3, 1 << 4, 1 << 5, 1 << 6, 1 << 7};
static const char*             enum_strs[]   = {"LShift", "LCtrl", "LAlt", "LCmd", "RShift", "RCtrl", "RAlt", "RCmd"};
static njson::JsonFlagsTypeDef json_keygroup_enum("keygroup_enum", sizeof(u16), alignof(u16), enum_strs, enum_values, DARRAYSIZE(enum_values));

template <> void njson::JsonObjectTypeRegisterFields<keygroup_t>(keygroup_t& base, njson::JsonFieldDescr*& members, s32& member_count)
{
    static njson::JsonFieldDescr s_members[] = {
        njson::JsonFieldDescr("name", base.m_name),
        njson::JsonFieldDescr("x", base.m_x),
        njson::JsonFieldDescr("y", base.m_y),
        njson::JsonFieldDescr("w", base.m_w),
        njson::JsonFieldDescr("h", base.m_h),
        njson::JsonFieldDescr("sw", base.m_sw),
        njson::JsonFieldDescr("sh", base.m_sh),
        njson::JsonFieldDescr("enum", base.m_enum, json_keygroup_enum),
        njson::JsonFieldDescr("r", base.m_r),
        njson::JsonFieldDescr("c", base.m_c),
        njson::JsonFieldDescr("a", base.m_a),
        njson::JsonFieldDescr("cap_color", base.m_capcolor, base.m_capcolor_size),
        njson::JsonFieldDescr("txt_color", base.m_txtcolor, base.m_txtcolor_size),
        njson::JsonFieldDescr("led_color", base.m_ledcolor, base.m_ledcolor_size),
        njson::JsonFieldDescr("keys", base.m_keys, base.m_nb_keys, json_key),
    };
    members      = s_members;
    member_count = sizeof(s_members) / sizeof(njson::JsonFieldDescr);
}
static njson::JsonObjectTypeDeclr<keygroup_t> json_keygroup("keygroup");

static const float sColorDarkGrey[] = {0.1f, 0.1f, 0.1f, 1.0f};
static const float sColorWhite[]    = {1.0f, 1.0f, 1.0f, 1.0f};
static const float sColorBlue[]     = {0.0f, 0.0f, 1.0f, 1.0f};

struct keyboard_t
{
    keyboard_t()
    {
        m_name         = "Kyria";
        m_nb_keygroups = 0;
        m_keygroups    = nullptr;
        ncore::g_copy(m_capcolor, sColorDarkGrey);
        ncore::g_copy(m_txtcolor, sColorWhite);
        ncore::g_copy(m_ledcolor, sColorBlue);
        m_scale = 1.0f;
        m_w     = 81.f;
        m_h     = 81.f;
        m_sw    = 9.f;
        m_sh    = 9.f;
    }

    const char* m_name; // name of this keyboard

    ncore::s16  m_nb_keygroups;
    keygroup_t* m_keygroups;

    // global caps, txt and led color, can be overriden per key
    float m_capcolor[4];
    float m_txtcolor[4];
    float m_ledcolor[4];
    float m_scale;
    float m_w;  // key width
    float m_h;  // key height
    float m_sw; // key spacing width
    float m_sh; // key spacing height

    DCORE_CLASS_PLACEMENT_NEW_DELETE
};

template <> void njson::JsonObjectTypeRegisterFields<keyboard_t>(keyboard_t& base, njson::JsonFieldDescr*& members, s32& member_count)
{
    static njson::JsonFieldDescr s_members[] = {
        njson::JsonFieldDescr("name", base.m_name),
        njson::JsonFieldDescr("scale", base.m_scale),
        njson::JsonFieldDescr("key_width", base.m_w),
        njson::JsonFieldDescr("key_height", base.m_h),
        njson::JsonFieldDescr("key_spacing_x", base.m_sw),
        njson::JsonFieldDescr("key_spacing_y", base.m_sh),
        njson::JsonFieldDescr("cap_color", base.m_capcolor, 4),
        njson::JsonFieldDescr("txt_color", base.m_txtcolor, 4),
        njson::JsonFieldDescr("led_color", base.m_ledcolor, 4),
        njson::JsonFieldDescr("keygroups", base.m_keygroups, base.m_nb_keygroups, json_keygroup),
    };
    members      = s_members;
    member_count = sizeof(s_members) / sizeof(njson::JsonFieldDescr);
}
static njson::JsonObjectTypeDeclr<keyboard_t> json_keyboard("keyboard");

struct keyboard_root_t
{
    keyboard_root_t() { m_keyboard = nullptr; }
    keyboard_t* m_keyboard;

    DCORE_CLASS_PLACEMENT_NEW_DELETE
};

template <> void njson::JsonObjectTypeRegisterFields<keyboard_root_t>(keyboard_root_t& base, njson::JsonFieldDescr*& members, s32& member_count)
{
    static njson::JsonFieldDescr s_members[] = {
        njson::JsonFieldDescr("keyboard", base.m_keyboard, json_keyboard),
    };
    members      = s_members;
    member_count = sizeof(s_members) / sizeof(njson::JsonFieldDescr);
}

static njson::JsonObjectTypeDeclr<keyboard_root_t> json_keyboards_root("root");

UNITTEST_SUITE_BEGIN(json_decode)
{
    UNITTEST_FIXTURE(decode)
    {
        UNITTEST_FIXTURE_SETUP() {}
        UNITTEST_FIXTURE_TEARDOWN() {}

        UNITTEST_ALLOCATOR;

        UNITTEST_TEST(test)
        {
            keyboard_root_t root;

            njson::JsonObject json_root;
            json_root.m_descr    = &json_keyboards_root;
            json_root.m_instance = &root;

            njson::JsonAllocator alloc;
            njson::JsonAllocator scratch;
            alloc.Init(Allocator, 1024 * 1024, "json allocator");
            scratch.Init(Allocator, 64 * 1024, "json scratch allocator");

            char const* error_message = nullptr;
            bool        ok            = njson::JsonDecode((const char*)kyria_json, (const char*)kyria_json + kyria_json_len, json_root, &alloc, &scratch, error_message);
            CHECK_TRUE(ok);

            scratch.Reset();

            char* json_text     = alloc.m_Pointer + alloc.m_Size;
            char* json_text_end = alloc.m_Pointer + alloc.m_Capacity;
            ok                  = njson::JsonEncode(json_root, json_text, json_text_end, error_message);

            alloc.Destroy();
            scratch.Destroy();
        }
    }
}
UNITTEST_SUITE_END
