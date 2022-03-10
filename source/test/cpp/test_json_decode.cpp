#include "xbase/x_target.h"
#include "xbase/x_memory.h"
#include "xbase/x_runes.h"
#include "xjson/x_json.h"
#include "xjson/x_json_allocator.h"
#include "xjson/x_json_decode.h"
#include "xjson/x_json_encode.h"

#include "xunittest/xunittest.h"

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
};

static key_t s_default_key;

// clang-format off
static json::JsonFieldDescr s_members_key[] = {
    json::JsonFieldDescr("nob", s_default_key.m_nob), 
    json::JsonFieldDescr("index", s_default_key.m_index), 
    json::JsonFieldDescr("label", s_default_key.m_label), 
    json::JsonFieldDescr("w", s_default_key.m_w), 
    json::JsonFieldDescr("h", s_default_key.m_h),
    json::JsonFieldDescr("cap_color", s_default_key.m_capcolor, s_default_key.m_capcolor_size),
    json::JsonFieldDescr("txt_color", s_default_key.m_txtcolor, s_default_key.m_txtcolor_size),
    json::JsonFieldDescr("led_color", s_default_key.m_ledcolor, s_default_key.m_ledcolor_size),
};

static void json_alloc_key(json::JsonAllocator* alloc, s32 n, void*& ptr) { ptr = alloc->AllocateArray<key_t>(n); }
static void json_copy_key(void* dst, s32 dst_index, void* src ) { ((key_t*)dst)[dst_index] = *(key_t*)src; }

static json::JsonTypeDescr json_key = 
{
    "key",
    &s_default_key, 
    sizeof(key_t),
    ALIGNOF(key_t),
    sizeof(s_members_key) / sizeof(json::JsonFieldDescr), 
    s_members_key
};

static json::JsonTypeFuncs json_keys_funcs = {
    json_alloc_key,
    json_copy_key,
};
// clang-format on

struct keygroup_t
{
    const char* m_name; // name of this group
    float       m_x;    // x position of this group
    float       m_y;    // y position of this group
    float       m_w;    // key width
    float       m_h;    // key height
    float       m_sw;   // key spacing width
    float       m_sh;   // key spacing height
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
};

static keygroup_t s_default_keygroup;

// clang-format off
static json::JsonFieldDescr s_members_keygroup[] = {
    json::JsonFieldDescr("name", s_default_keygroup.m_name), 
    json::JsonFieldDescr("x", s_default_keygroup.m_x), 
    json::JsonFieldDescr("y", s_default_keygroup.m_y),
    json::JsonFieldDescr("w", s_default_keygroup.m_w), 
    json::JsonFieldDescr("h", s_default_keygroup.m_h), 
    json::JsonFieldDescr("sw", s_default_keygroup.m_sw), 
    json::JsonFieldDescr("sh", s_default_keygroup.m_sh), 
    json::JsonFieldDescr("r", s_default_keygroup.m_r),
    json::JsonFieldDescr("c", s_default_keygroup.m_c),
    json::JsonFieldDescr("a", s_default_keygroup.m_a),
    json::JsonFieldDescr("cap_color", s_default_keygroup.m_capcolor, s_default_keygroup.m_capcolor_size), 
    json::JsonFieldDescr("txt_color", s_default_keygroup.m_txtcolor, s_default_keygroup.m_txtcolor_size), 
    json::JsonFieldDescr("led_color", s_default_keygroup.m_ledcolor, s_default_keygroup.m_ledcolor_size), 
    json::JsonFieldDescr("keys", s_default_keygroup.m_keys, s_default_keygroup.m_nb_keys, json_keys_funcs, json_key), 
};

static void json_alloc_keygroup(json::JsonAllocator* alloc, s32 n, void*& ptr) { ptr = alloc->AllocateArray<keygroup_t>(n); }
static void json_copy_keygroup(void* dst, s32 dst_index, void* src) { ((keygroup_t*)dst)[dst_index] = *(keygroup_t*)src; }

static json::JsonTypeDescr json_keygroup = {
    "keygroup",
    &s_default_keygroup, 
    sizeof(keygroup_t),
    ALIGNOF(keygroup_t),
    sizeof(s_members_keygroup) / sizeof(json::JsonFieldDescr), 
    s_members_keygroup
};

static json::JsonTypeFuncs json_keygroup_funcs = {
    json_alloc_keygroup,
    json_copy_keygroup,
};
// clang-format on

static const float sColorDarkGrey[] = {0.1f, 0.1f, 0.1f, 1.0f};
static const float sColorWhite[]    = {1.0f, 1.0f, 1.0f, 1.0f};
static const float sColorBlue[]     = {0.0f, 0.0f, 1.0f, 1.0f};

struct keyboard_t
{
    keyboard_t()
    {
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
};

static keyboard_t s_default_keyboard;

// clang-format off
static json::JsonFieldDescr s_members_keyboard[] = {
    json::JsonFieldDescr("name", s_default_keyboard.m_name),
    json::JsonFieldDescr("scale", s_default_keyboard.m_scale), 
    json::JsonFieldDescr("key_width", s_default_keyboard.m_w), 
    json::JsonFieldDescr("key_height", s_default_keyboard.m_h), 
    json::JsonFieldDescr("key_spacing_x", s_default_keyboard.m_sw), 
    json::JsonFieldDescr("key_spacing_y", s_default_keyboard.m_sh), 
    json::JsonFieldDescr("cap_color", s_default_keyboard.m_capcolor, 4), 
    json::JsonFieldDescr("txt_color", s_default_keyboard.m_txtcolor, 4), 
    json::JsonFieldDescr("led_color", s_default_keyboard.m_ledcolor, 4), 
    json::JsonFieldDescr("keygroups", s_default_keyboard.m_keygroups, s_default_keyboard.m_nb_keygroups, json_keygroup_funcs, json_keygroup), 
};
// clang-format on

// implementation of the constructor for the keygroup object
static void json_construct_keyboard(json::JsonAllocator* alloc, s32 n, void*& ptr) { ptr = alloc->AllocateArray<keyboard_t>(n); }
static void json_copy_keyboard(void* dst, s32 dst_index, void* src) { ((keyboard_t*)dst)[dst_index] = *(keyboard_t*)src; }

// clang-format off
static json::JsonTypeDescr json_keyboard = 
{
    "keyboard",
    &s_default_keyboard, 
    sizeof(keyboard_t),
    ALIGNOF(keyboard_t),
    sizeof(s_members_keyboard) / sizeof(json::JsonFieldDescr), 
    s_members_keyboard
};

static json::JsonTypeFuncs json_keyboard_funcs = {
    json_construct_keyboard,
    json_copy_keyboard,
};

struct keyboard_root_t
{
    keyboard_root_t() { m_keyboard = nullptr; }
    keyboard_t* m_keyboard;
};

static keyboard_root_t s_default_keyboard_root;

static json::JsonFieldDescr s_members_keyboard_root[] = {
    json::JsonFieldDescr("keyboard", s_default_keyboard_root.m_keyboard, json_keyboard_funcs, json_keyboard), 
};
static json::JsonTypeDescr json_keyboard_root = {
    "root", 
    &s_default_keyboard_root, 
    sizeof(keyboard_root_t),
    ALIGNOF(keyboard_root_t),
    sizeof(s_members_keyboard_root) / sizeof(json::JsonFieldDescr), 
    s_members_keyboard_root
};
// clang-format on

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
            json_root.m_descr    = &json_keyboard_root;
            json_root.m_instance = &root;

            json::JsonAllocator* alloc   = json::CreateAllocator(1024 * 1024);
            json::JsonAllocator* scratch = json::CreateAllocator(64 * 1024);

            char const* error_message = nullptr;
            bool        ok            = json::JsonDecode((const char*)kyria_json, (const char*)kyria_json + kyria_json_len, json_root, alloc, scratch, error_message);
            CHECK_TRUE(ok);

            scratch->Reset();

            char* json_text = alloc->m_Cursor;
            ok = json::JsonEncode(json_root, json_text, json_text + alloc->m_Size, error_message);
        }
    }
}
UNITTEST_SUITE_END