#include "ccore/c_target.h"
#include "cbase/c_memory.h"
#include "cbase/c_runes.h"
#include "cjson/c_json_parser.h"
#include "cjson/c_json_allocator.h"
#include "cjson/c_json_decoder.h"

#include "cunittest/cunittest.h"

using namespace ncore;

extern unsigned char data_kyria[];
extern unsigned int  data_kyria_len;

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

static void json_decode_key(njson::ndecoder::decoder_t* d, key_t* out_key)
{
    njson::ndecoder::result_t result = njson::ndecoder::read_object_begin(d);
    if (result.not_ok() || result.end())
        return;
    njson::ndecoder::decoder_add_member(d, "nob", &out_key->m_nob);
    njson::ndecoder::decoder_add_member(d, "index", &out_key->m_index);
    njson::ndecoder::decoder_add_member(d, "label", &out_key->m_label);
    njson::ndecoder::decoder_add_member(d, "w", &out_key->m_w);
    njson::ndecoder::decoder_add_member(d, "h", &out_key->m_h);
    njson::ndecoder::decoder_add_member(d, "cap_color", &out_key->m_capcolor, &out_key->m_capcolor_size, 4);
    njson::ndecoder::decoder_add_member(d, "txt_color", &out_key->m_txtcolor, &out_key->m_txtcolor_size, 4);
    njson::ndecoder::decoder_add_member(d, "led_color", &out_key->m_ledcolor, &out_key->m_ledcolor_size, 4);
    while (result.valid())
    {
        njson::ndecoder::field_t field = njson::ndecoder::decode_field(d);
        njson::ndecoder::decoder_decode_member(d, field);
        result = njson::ndecoder::read_object_end(d);
    }
}

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

//                                       Bit      0       1       2       3       4       5       6       7
static const u16   flag_values[] = {1 << 0, 1 << 1, 1 << 2, 1 << 3, 1 << 4, 1 << 5, 1 << 6, 1 << 7};
static const char* flag_strs[]   = {"LShift", "LCtrl", "LAlt", "LCmd", "RShift", "RCtrl", "RAlt", "RCmd"};

static void json_decode_keygroup(njson::ndecoder::decoder_t* d, keygroup_t* out_keygroup)
{
    njson::ndecoder::result_t result = njson::ndecoder::read_object_begin(d);
    if (result.not_ok() || result.end())
        return;

    njson::ndecoder::decoder_add_member(d, "name", &out_keygroup->m_name);
    njson::ndecoder::decoder_add_member(d, "x", &out_keygroup->m_x);
    njson::ndecoder::decoder_add_member(d, "y", &out_keygroup->m_y);
    njson::ndecoder::decoder_add_member(d, "w", &out_keygroup->m_w);
    njson::ndecoder::decoder_add_member(d, "h", &out_keygroup->m_h);
    njson::ndecoder::decoder_add_member(d, "sw", &out_keygroup->m_sw);
    njson::ndecoder::decoder_add_member(d, "sh", &out_keygroup->m_sh);
    njson::ndecoder::decoder_add_flag_member(d, "enum", flag_strs, flag_values, DARRAYSIZE(flag_values), &out_keygroup->m_enum);
    njson::ndecoder::decoder_add_member(d, "r", &out_keygroup->m_r);
    njson::ndecoder::decoder_add_member(d, "c", &out_keygroup->m_c);
    njson::ndecoder::decoder_add_member(d, "a", &out_keygroup->m_a);
    njson::ndecoder::decoder_add_member(d, "cap_color", &out_keygroup->m_capcolor, &out_keygroup->m_capcolor_size, 4);
    njson::ndecoder::decoder_add_member(d, "txt_color", &out_keygroup->m_txtcolor, &out_keygroup->m_txtcolor_size, 4);
    njson::ndecoder::decoder_add_member(d, "led_color", &out_keygroup->m_ledcolor, &out_keygroup->m_ledcolor_size, 4);

    while (result.valid())
    {
        njson::ndecoder::field_t field = njson::ndecoder::decode_field(d);
        if (!njson::ndecoder::decoder_decode_member(d, field))
        {
            if (njson::ndecoder::field_equal(field, "keys"))
            {
                i32                       array_size;
                njson::ndecoder::result_t result = njson::ndecoder::read_array_begin(d, array_size);
                if (result.valid())
                {
                    out_keygroup->m_nb_keys = (ncore::s16)array_size;
                    out_keygroup->m_keys    = d->m_DecoderAllocator->AllocateArray<key_t>(array_size);
                    i32 array_index         = 0;
                    while (result.valid())
                    {
                        if (array_index < array_size)
                            json_decode_key(d, &out_keygroup->m_keys[array_index]);
                        array_index++;
                        result = njson::ndecoder::read_array_end(d);
                    }
                }
            }
        }
        result = njson::ndecoder::read_object_end(d);
    }
}

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
        m_key_w = 81.f;
        m_key_h = 81.f;
        m_key_spacing_x = 9.f;
        m_key_spacing_y = 9.f;
    }

    const char* m_name; // name of this keyboard

    ncore::s16  m_nb_keygroups;
    keygroup_t* m_keygroups;

    // global caps, txt and led color, can be overriden per key
    float m_capcolor[4];
    float m_txtcolor[4];
    float m_ledcolor[4];
    float m_scale;
    float m_key_w;         // key width
    float m_key_h;         // key height
    float m_key_spacing_x; // key spacing width
    float m_key_spacing_y; // key spacing height

    DCORE_CLASS_PLACEMENT_NEW_DELETE
};

static void json_decode_keyboard(njson::ndecoder::decoder_t* d, keyboard_t* out_keyboard)
{
    njson::ndecoder::result_t result = njson::ndecoder::read_object_begin(d);
    if (result.not_ok() || result.end())
        return;

    njson::ndecoder::decoder_add_member(d, "name", &out_keyboard->m_name);
    njson::ndecoder::decoder_add_member(d, "cap_color", out_keyboard->m_capcolor, 4);
    njson::ndecoder::decoder_add_member(d, "txt_color", out_keyboard->m_txtcolor, 4);
    njson::ndecoder::decoder_add_member(d, "led_color", out_keyboard->m_ledcolor, 4);
    njson::ndecoder::decoder_add_member(d, "scale", &out_keyboard->m_scale);
    njson::ndecoder::decoder_add_member(d, "key_width", &out_keyboard->m_key_w);
    njson::ndecoder::decoder_add_member(d, "key_height", &out_keyboard->m_key_h);
    njson::ndecoder::decoder_add_member(d, "key_spacing_x", &out_keyboard->m_key_spacing_x);
    njson::ndecoder::decoder_add_member(d, "key_spacing_y", &out_keyboard->m_key_spacing_y);
    while (result.valid())
    {
        njson::ndecoder::field_t field = njson::ndecoder::decode_field(d);
        if (!njson::ndecoder::decoder_decode_member(d, field))
        {
            if (njson::ndecoder::field_equal(field, "keygroups"))
            {
                i32                       array_size;
                njson::ndecoder::result_t result = njson::ndecoder::read_array_begin(d, array_size);
                if (result.valid())
                {
                    out_keyboard->m_nb_keygroups = (ncore::s16)array_size;
                    out_keyboard->m_keygroups    = d->m_DecoderAllocator->AllocateArray<keygroup_t>(array_size);
                    i32 array_index              = 0;
                    while (result.valid())
                    {
                        if (array_index < array_size)
                            json_decode_keygroup(d, &out_keyboard->m_keygroups[array_index]);
                        array_index++;
                        result = njson::ndecoder::read_array_end(d);
                    }
                }
            }
        }
        result = njson::ndecoder::read_object_end(d);
    }
}

struct keyboard_root_t
{
    keyboard_root_t() { m_keyboard = nullptr; }
    keyboard_t* m_keyboard;

    DCORE_CLASS_PLACEMENT_NEW_DELETE
};

static void json_decode_keyboard_root(njson::ndecoder::decoder_t* d, keyboard_root_t* out_root)
{
    njson::ndecoder::result_t result = njson::ndecoder::read_object_begin(d);
    if (result.not_ok() || result.end())
        return;

    while (result.valid())
    {
        njson::ndecoder::field_t field = njson::ndecoder::decode_field(d);
        if (njson::ndecoder::field_equal(field, "keyboard"))
        {
            out_root->m_keyboard = d->m_DecoderAllocator->Allocate<keyboard_t>();
            json_decode_keyboard(d, out_root->m_keyboard);
        }
        result = njson::ndecoder::read_object_end(d);
    }
}

/*
{
    "keyboard":
    {
        "scale": 42,
        "key_width": 1,
        "key_height": 2,
        "key_spacing_x": 0.125,
        "key_spacing_y": 0.126,
        "cap_color": [ 25,25,25,255 ],
        "led_color": [ 251,252,253,254 ],
        "txt_color": [ 241,242,243,244 ],
        "keygroups": [
            {
                "name": "left thumb",
                "x": 0.75,
                "y": 1.55,
                "w": -1.2,
                "h": -1.5,
                "sw": -1.7,
                "sh": -1.9,
                "enum": "RShift|RCtrl|RAlt|LShift|LCtrl|LAlt",
                "cap_color": [ 25,26,26,255 ],
                "led_color": [ 31,32.33,255 ],
                "txt_color": [ 61,62,63,255 ],
                "c": 2,
                "r": 3,
                "a": 0,
                "keys": [
                    { "label": "Esc", "index": 0, "nob": false, "cap_color": [ 25,25,25,255 ], "led_color": [ 255,255,255,255 ], "txt_color": [ 255,255,255,255 ], "w": 1, "h": 1 },
                    { "label": "Q", "index": 1, "nob": false, "cap_color": [ 25,25,25,255 ], "led_color": [ 255,255,255,255 ], "txt_color": [ 255,255,255,255 ], "w": 1, "h": 1 },
                    { "label": "Caps", "index": 12, "nob": false, "cap_color": [ 25,25,25,255 ], "led_color": [ 255,255,255,255 ], "txt_color": [ 255,255,255,255 ], "w": 1, "h": 1 },
                    { "label": "A", "index": 13, "nob": false, "cap_color": [ 25,25,25,255 ], "led_color": [ 255,255,255,255 ], "txt_color": [ 255,255,255,255 ], "w": 1, "h": 1 },
                    { "label": "Shift", "index": 24, "nob": false, "cap_color": [ 25,25,25,255 ], "led_color": [ 255,255,255,255 ], "txt_color": [ 255,255,255,255 ], "w": 1, "h": 1 },
                    { "label": "Z", "index": 25, "nob": false, "cap_color": [ 25,25,25,255 ], "led_color": [ 255,255,255,255 ], "txt_color": [ 255,255,255,255 ], "w": 1, "h": 1 }
                ]
            },
            {
                "name": "right thumb",
                "x": 7.875,
                "y": 4.125,
                "w": -1,
                "h": -1,
                "sw": -1,
                "sh": -1,
                "enum": "RShift|RCtrl|RAlt|RCmd|LShift|LCtrl|LAlt|LCmd",
                "cap_color": [ 71,72.73,255 ],
                "led_color": [ 91,92,93,255 ],
                "txt_color": [ 111,112,113,255 ],
                "c": 1,
                "r": 2,
                "a": 40,
                "keys": [
                    { "label": "Left", "index": 31, "nob": false, "cap_color": [ 1,2.3,255 ], "led_color": [ 4,5,6,255 ], "txt_color": [ 7,8,9,255 ], "w": 1, "h": 2 },
                    { "label": "Down", "index": 44, "nob": true, "cap_color": [ 10,11,12,255 ], "led_color": [ 13,14,15,255 ], "txt_color": [ 16,17,18,255 ], "w": 4, "h": 5 }
                ]
            }
        ]
    }
}
*/
UNITTEST_SUITE_BEGIN(json_decoder)
{
    UNITTEST_FIXTURE(decode)
    {
        UNITTEST_FIXTURE_SETUP() {}
        UNITTEST_FIXTURE_TEARDOWN() {}

        UNITTEST_ALLOCATOR;

        UNITTEST_TEST(test)
        {
            keyboard_root_t root;

            njson::JsonAllocator alloc;
            njson::JsonAllocator scratch;
            alloc.Init(Allocator, 1024 * 1024, "json allocator");
            scratch.Init(Allocator, 64 * 1024, "json scratch allocator");

            njson::ndecoder::decoder_t* decoder = njson::ndecoder::create_decoder(&scratch, &alloc, (const char*)data_kyria, (const char*)data_kyria + data_kyria_len);
            json_decode_keyboard_root(decoder, &root);

            // TODO: validate keyboard_root_t and all members
            CHECK_EQUAL(true, root.m_keyboard != nullptr);
            CHECK_EQUAL(42.0f, root.m_keyboard->m_scale);
            CHECK_EQUAL(1, root.m_keyboard->m_key_w);
            CHECK_EQUAL(2, root.m_keyboard->m_key_h);
            CHECK_EQUAL(0.125f, root.m_keyboard->m_key_spacing_x);
            CHECK_EQUAL(0.126f, root.m_keyboard->m_key_spacing_y);
            CHECK_EQUAL(25.0f, root.m_keyboard->m_capcolor[0]);
            CHECK_EQUAL(25.0f, root.m_keyboard->m_capcolor[1]);
            CHECK_EQUAL(25.0f, root.m_keyboard->m_capcolor[2]);
            CHECK_EQUAL(255.0f, root.m_keyboard->m_capcolor[3]);

            CHECK_EQUAL(2, root.m_keyboard->m_nb_keygroups);
            CHECK_EQUAL(6, root.m_keyboard->m_keygroups[0].m_nb_keys);
            CHECK_EQUAL(2, root.m_keyboard->m_keygroups[1].m_nb_keys);

            alloc.Destroy();
            scratch.Destroy();
        }
    }
}
UNITTEST_SUITE_END
