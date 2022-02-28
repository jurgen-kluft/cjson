#include "xbase/x_target.h"
#include "xbase/x_runes.h"
#include "xjson/x_json.h"
#include "xjson/x_json_decode.h"

#include "xunittest/xunittest.h"

using namespace xcore;

extern unsigned char   kyria_json[];
extern unsigned int    kyria_json_len;

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
static jsonvalue_t s_members_key[] = {
    jsonvalue_t("nob", &s_default_key.m_nob), 
    jsonvalue_t("index", &s_default_key.m_index), 
    jsonvalue_t("label", &s_default_key.m_label), 
    jsonvalue_t("w", &s_default_key.m_w), 
    jsonvalue_t("h", &s_default_key.m_h),
    jsonvalue_t("capcolor", &s_default_key.m_capcolor, &s_default_key.m_capcolor_size),
    jsonvalue_t("txtcolor", &s_default_key.m_txtcolor, &s_default_key.m_txtcolor_size),
    jsonvalue_t("ledcolor", &s_default_key.m_ledcolor, &s_default_key.m_ledcolor_size),
};
// clang-format on

// implementation of the constructor for the key object
static void* json_construct_key(json_allocator_t* alloc) { return alloc->construct<key_t>(); }

// clang-format off
static jsonobject_t json_key = 
{
	"key",
	&s_default_key, 
	json_construct_key, 
	sizeof(s_members_key) / sizeof(jsonvalue_t), 
	s_members_key
};
// clang-format on

struct keygroup_t
{
    const char* m_name;       // name of this group
    float       m_x;          // x position of this group
    float       m_y;          // y position of this group
	float       m_w;          // key width
	float       m_h;          // key height
	float       m_sw;         // key spacing width
	float       m_sh;         // key spacing height
	xcore::s16  m_r;          // rows
	xcore::s16  m_c;          // columns
	xcore::s16  m_a;          // angle, -45 degrees to 45 degrees (granularity is 1 degree)
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
static jsonvalue_t s_members_keygroup[] = {
    jsonvalue_t("name", &s_default_keygroup.m_name), 
    jsonvalue_t("x", &s_default_keygroup.m_x), 
    jsonvalue_t("y", &s_default_keygroup.m_y),
	jsonvalue_t("w", &s_default_keygroup.m_w), 
	jsonvalue_t("h", &s_default_keygroup.m_h), 
	jsonvalue_t("sw", &s_default_keygroup.m_sw), 
	jsonvalue_t("sh", &s_default_keygroup.m_sh), 
	jsonvalue_t("r", &s_default_keygroup.m_r),
    jsonvalue_t("c", &s_default_keygroup.m_c),
    jsonvalue_t("a", &s_default_keygroup.m_a), 
    jsonvalue_t("capcolor", &s_default_keygroup.m_capcolor, &s_default_keygroup.m_capcolor_size), 
    jsonvalue_t("txtcolor", &s_default_keygroup.m_txtcolor, &s_default_keygroup.m_txtcolor_size), 
    jsonvalue_t("ledcolor", &s_default_keygroup.m_ledcolor, &s_default_keygroup.m_ledcolor_size), 
    jsonvalue_t("keys", &s_default_keygroup.m_keys, &s_default_keygroup.m_nb_keys, &json_key), 
};
// clang-format on

// implementation of the constructor for the keygroup object
static void* json_construct_keygroup(json_allocator_t* alloc) { return alloc->construct<keygroup_t>(); }

// clang-format off
static jsonobject_t json_keygroup = 
{
	"keygroup",
	&s_default_keygroup, 
	json_construct_keygroup, 
	sizeof(s_members_keygroup) / sizeof(jsonvalue_t), 
	s_members_keygroup
};
// clang-format on

struct keyboard_t
{
    keyboard_t()
    {
        m_nb_keygroups  = 0;
        m_keygroups     = nullptr;
        m_capcolor_size = 0;
        m_txtcolor_size = 0;
        m_ledcolor_size = 0;
        m_capcolor      = nullptr;
        m_txtcolor      = nullptr;
        m_ledcolor      = nullptr;
        m_scale         = 1.0f;
        m_w             = 81.f;
        m_h             = 81.f;
        m_sw            = 9.f;
        m_sh            = 9.f;
    }

    xcore::s16  m_nb_keygroups;
    keygroup_t* m_keygroups;

    // global caps, txt and led color, can be overriden per key
    xcore::s8 m_capcolor_size;
    xcore::s8 m_txtcolor_size;
    xcore::s8 m_ledcolor_size;
    float*     m_capcolor;
    float*     m_txtcolor;
    float*     m_ledcolor;
    float      m_scale;
    float      m_w;  // key width
    float      m_h;  // key height
    float      m_sw; // key spacing width
    float      m_sh; // key spacing height
};

static keyboard_t s_default_keyboard;

// clang-format off
static jsonvalue_t s_members_keyboard[] = {
    jsonvalue_t("scale", &s_default_keyboard.m_scale), 
    jsonvalue_t("key_width", &s_default_keyboard.m_w), 
    jsonvalue_t("key_height", &s_default_keyboard.m_h), 
    jsonvalue_t("key_spacing_x", &s_default_keyboard.m_sw), 
    jsonvalue_t("key_spacing_y", &s_default_keyboard.m_sh), 
    jsonvalue_t("capcolor", &s_default_keyboard.m_capcolor, &s_default_keyboard.m_capcolor_size), 
    jsonvalue_t("txtcolor", &s_default_keyboard.m_txtcolor, &s_default_keyboard.m_txtcolor_size), 
    jsonvalue_t("ledcolor", &s_default_keyboard.m_ledcolor, &s_default_keyboard.m_ledcolor_size), 
    jsonvalue_t("keygroups", &s_default_keyboard.m_keygroups, &s_default_keyboard.m_nb_keygroups, &json_keygroup), 
};
// clang-format on

// implementation of the constructor for the keygroup object
static void* json_construct_keyboard(json_allocator_t* alloc) { return alloc->construct<keyboard_t>(); }

// clang-format off
static jsonobject_t json_keyboard = 
{
	"keyboard",
	&s_default_keyboard, 
	json_construct_keyboard, 
	sizeof(s_members_keyboard) / sizeof(jsonvalue_t), 
	s_members_keyboard
};
// clang-format on

struct keyboard_root_t
{
    keyboard_root_t()
    {
        m_keyboard = nullptr;
    }
    keyboard_t* m_keyboard;
};

static keyboard_root_t s_default_keyboard_root;

// clang-format off
static jsonvalue_t s_members_keyboard_root[] = {
    jsonvalue_t("keyboard", &s_default_keyboard_root.m_keyboard, &json_keyboard), 
};
// clang-format on

// clang-format off
static jsonobject_t json_keyboard_root = {
    "root",
    &s_default_keyboard_root, 
    nullptr, 
    sizeof(s_members_keyboard_root) / sizeof(jsonvalue_t), 
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
            jsonobject_t* json_root = &json_keyboard_root;

			json::MemAllocLinear* alloc = json::CreateMemAllocLinear(1024 * 1024);
			json::MemAllocLinear* scratch = json::CreateMemAllocLinear(16 * 1024);
            
            char const* error_message = nullptr;
            json_decode((const char*)kyria_json, (const char*)kyria_json + kyria_json_len, json_root, &root, alloc, scratch, error_message);
        }
    }
}
UNITTEST_SUITE_END