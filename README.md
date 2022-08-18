# json parser

A JSON parser as well as a JSON decoder that can read directly into C structures using JSON type declarations.
These declarations can then also be used again to write JSON, see [test_json_decode.cpp](https://github.com/jurgen-kluft/xjson/blob/main/source/test/cpp/test_json_decode.cpp) for an example.

- UTF-8
- Lexer
- Parser
- Decoder (reading JSON)
- Encoder (writing JSON)

Uses a simple forward/linear allocator.

## Dependencies

- cbase (types & allocator)

