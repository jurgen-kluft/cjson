#include "cbase/x_target.h"
#include "cbase/x_memory.h"
#include "cbase/x_runes.h"
#include "xjson/x_json.h"
#include "xjson/x_json_allocator.h"
#include "xjson/x_json_decode.h"
#include "xjson/x_json_encode.h"

#include "cunittest/cunittest.h"

using namespace xcore;

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
    xcore::s16  m_index;         // index in keymap
    const char* m_label;         // label (e.g. "Q")
    float       m_w;             // key width
    float       m_h;             // key height
    xcore::s8   m_capcolor_size; // should become 3 or 4 (RGB or RGBA)
    xcore::s8   m_txtcolor_size; //
    xcore::s8   m_ledcolor_size; //
    float*      m_capcolor;      // color of the key cap, e.g. [ 0.1, 0.1, 0.1, 1.0 ]
    float*      m_txtcolor;      // color of the key label
    float*      m_ledcolor;      // color of the key led

    XCORE_CLASS_PLACEMENT_NEW_DELETE
};

template <> void json::JsonObjectTypeRegisterFields<key_t>(key_t& base, json::JsonFieldDescr*& members, s32& member_count)
{
    static json::JsonFieldDescr s_members[] = {
        json::JsonFieldDescr("nob", base.m_nob),
        json::JsonFieldDescr("index", base.m_index),
        json::JsonFieldDescr("label", base.m_label),
        json::JsonFieldDescr("w", base.m_w),
        json::JsonFieldDescr("h", base.m_h),
        json::JsonFieldDescr("cap_color", base.m_capcolor, base.m_capcolor_size),
        json::JsonFieldDescr("txt_color", base.m_txtcolor, base.m_txtcolor_size),
        json::JsonFieldDescr("led_color", base.m_ledcolor, base.m_ledcolor_size),
    };
    members      = s_members;
    member_count = sizeof(s_members) / sizeof(json::JsonFieldDescr);
}
static json::JsonObjectTypeDeclr<key_t> json_key("key");

struct keygroup_t
{
    const char* m_name; // name of this group
    float       m_x;    // x position of this group
    float       m_y;    // y position of this group
    float       m_w;    // key width
    float       m_h;    // key height
    float       m_sw;   // key spacing width
    float       m_sh;   // key spacing height
    xcore::u16  m_enum; // enum
    xcore::s16  m_r;    // rows
    xcore::s16  m_c;    // columns
    xcore::s16  m_a;    // angle, -45 degrees to 45 degrees (granularity is 1 degree)
    xcore::s8   m_capcolor_size;
    xcore::s8   m_txtcolor_size;
    xcore::s8   m_ledcolor_size;
    float*      m_capcolor; // color of the key cap
    float*      m_txtcolor; // color of the key label
    float*      m_ledcolor; // color of the key led
    xcore::s16  m_nb_keys;  // number of keys in the array
    key_t*      m_keys;     // array of keys

    XCORE_CLASS_PLACEMENT_NEW_DELETE
};

// You can provide to and from string functions, if not the default to and from string functions are used.
//                          Bit      0       1         2       3       4        5        6       7
static const char* enum_strs[] = {"LShift", "LCtrl", "LAlt", "LCmd", "RShift", "RCtrl", "RAlt", "RCmd", nullptr};
static json::JsonEnumTypeDef json_keygroup_enum("keygroup_enum", sizeof(u16), ALIGNOF(u16), enum_strs);

template <> void json::JsonObjectTypeRegisterFields<keygroup_t>(keygroup_t& base, json::JsonFieldDescr*& members, s32& member_count)
{
    static json::JsonFieldDescr s_members[] = {
        json::JsonFieldDescr("name", base.m_name),
        json::JsonFieldDescr("x", base.m_x),
        json::JsonFieldDescr("y", base.m_y),
        json::JsonFieldDescr("w", base.m_w),
        json::JsonFieldDescr("h", base.m_h),
        json::JsonFieldDescr("sw", base.m_sw),
        json::JsonFieldDescr("sh", base.m_sh),
        json::JsonFieldDescr("enum", base.m_enum, json_keygroup_enum),
        json::JsonFieldDescr("r", base.m_r),
        json::JsonFieldDescr("c", base.m_c),
        json::JsonFieldDescr("a", base.m_a),
        json::JsonFieldDescr("cap_color", base.m_capcolor, base.m_capcolor_size),
        json::JsonFieldDescr("txt_color", base.m_txtcolor, base.m_txtcolor_size),
        json::JsonFieldDescr("led_color", base.m_ledcolor, base.m_ledcolor_size),
        json::JsonFieldDescr("keys", base.m_keys, base.m_nb_keys, json_key),
    };
    members      = s_members;
    member_count = sizeof(s_members) / sizeof(json::JsonFieldDescr);
}
static json::JsonObjectTypeDeclr<keygroup_t> json_keygroup("keygroup");

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
        xcore::copy(m_capcolor, sColorDarkGrey);
        xcore::copy(m_txtcolor, sColorWhite);
        xcore::copy(m_ledcolor, sColorBlue);
        m_scale = 1.0f;
        m_w     = 81.f;
        m_h     = 81.f;
        m_sw    = 9.f;
        m_sh    = 9.f;
    }

    const char* m_name; // name of this keyboard

    xcore::s16  m_nb_keygroups;
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

    XCORE_CLASS_PLACEMENT_NEW_DELETE
};

template <> void json::JsonObjectTypeRegisterFields<keyboard_t>(keyboard_t& base, json::JsonFieldDescr*& members, s32& member_count)
{
    static json::JsonFieldDescr s_members[] = {
        json::JsonFieldDescr("name", base.m_name),
        json::JsonFieldDescr("scale", base.m_scale),
        json::JsonFieldDescr("key_width", base.m_w),
        json::JsonFieldDescr("key_height", base.m_h),
        json::JsonFieldDescr("key_spacing_x", base.m_sw),
        json::JsonFieldDescr("key_spacing_y", base.m_sh),
        json::JsonFieldDescr("cap_color", base.m_capcolor, 4),
        json::JsonFieldDescr("txt_color", base.m_txtcolor, 4),
        json::JsonFieldDescr("led_color", base.m_ledcolor, 4),
        json::JsonFieldDescr("keygroups", base.m_keygroups, base.m_nb_keygroups, json_keygroup),
    };
    members      = s_members;
    member_count = sizeof(s_members) / sizeof(json::JsonFieldDescr);
}
static json::JsonObjectTypeDeclr<keyboard_t> json_keyboard("keyboard");

struct keyboard_root_t
{
    keyboard_root_t() { m_keyboard = nullptr; }
    keyboard_t* m_keyboard;

    XCORE_CLASS_PLACEMENT_NEW_DELETE
};

template <> void json::JsonObjectTypeRegisterFields<keyboard_root_t>(keyboard_root_t& base, json::JsonFieldDescr*& members, s32& member_count)
{
    static json::JsonFieldDescr s_members[] = {
        json::JsonFieldDescr("keyboard", base.m_keyboard, json_keyboard),
    };
    members      = s_members;
    member_count = sizeof(s_members) / sizeof(json::JsonFieldDescr);
}

static json::JsonObjectTypeDeclr<keyboard_root_t> json_keyboards_root("root");

UNITTEST_SUITE_BEGIN(xjson_decode)
{
    UNITTEST_FIXTURE(decode)
    {
        UNITTEST_FIXTURE_SETUP() {}
        UNITTEST_FIXTURE_TEARDOWN() {}

        UNITTEST_TEST(test)
        {
            keyboard_root_t root;

            json::JsonObject json_root;
            json_root.m_descr    = &json_keyboards_root;
            json_root.m_instance = &root;

            json::JsonAllocator* alloc   = json::CreateAllocator(1024 * 1024);
            json::JsonAllocator* scratch = json::CreateAllocator(64 * 1024);

            char const* error_message = nullptr;
            bool        ok            = json::JsonDecode((const char*)kyria_json, (const char*)kyria_json + kyria_json_len, json_root, alloc, scratch, error_message);
            CHECK_TRUE(ok);

            scratch->Reset();

            char* json_text = alloc->m_Cursor;
            ok              = json::JsonEncode(json_root, json_text, json_text + alloc->m_Size, error_message);

            json::DestroyAllocator(alloc);
            json::DestroyAllocator(scratch);
        }
    }
}
UNITTEST_SUITE_END