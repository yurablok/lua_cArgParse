#include <iostream>
#include <cassert>

#ifdef _MSC_VER
#   pragma comment(lib, "lua.lib")
#endif
//extern "C" {
//#   include "../lua/lua.h"
//#   include "../lua/lualib.h"
//#   include "../lua/lauxlib.h"
//}

#include "../lua_cArgParse.hpp"

//static void dumpstack(lua_State* L) {
//    printf("//==--\n");
//    int top = lua_gettop(L);
//    for (int i = 1; i <= top; i++) {
//        printf("%d\t%s\t", i, luaL_typename(L, i));
//        switch (lua_type(L, i)) {
//        case LUA_TNUMBER:
//            printf("%g\n", lua_tonumber(L, i));
//            break;
//        case LUA_TSTRING:
//            printf("%s\n", lua_tostring(L, i));
//            break;
//        case LUA_TBOOLEAN:
//            printf("%s\n", (lua_toboolean(L, i) ? "true" : "false"));
//            break;
//        case LUA_TNIL:
//            printf("%s\n", "nil");
//            break;
//        default:
//            printf("%p\n", lua_topointer(L, i));
//            break;
//        }
//    }
//    printf("\\\\==--\n");
//}

bool contains(const std::string_view source, const std::string_view pattern) {
    if (source.find(pattern) == std::string_view::npos) {
        //std::cout << source << std::endl;
        return false;
    }
    return true;
}

int main() {
    lua_State* lua = luaL_newstate();
    using namespace utils;

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    struct TestEmptyTuple {
        static int32_t test(lua_State* lua) {
            std::tuple<> args;
            std::string errorStr;
            if (!lua::cArgParse(lua, args, errorStr)) {
                luaL_error(lua, errorStr.c_str());
                return 0;
            }
            return 0;
        }
    };
    lua_register(lua, "test", TestEmptyTuple::test);
    assert(luaL_dostring(lua, "test()") == LUA_OK);

    assert(luaL_dostring(lua, "test(123)") != LUA_OK);
    assert(contains((lua_tostring(lua, -1)), "wrong arguments number"));

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    struct TestSingleU16 {
        static int32_t test(lua_State* lua) {
            std::tuple<uint16_t> args;
            std::string errorStr;
            if (!lua::cArgParse(lua, args, errorStr)) {
                luaL_error(lua, errorStr.c_str());
                return 0;
            }
            if (std::get<0>(args) != 123) {
                luaL_error(lua, "arg 1 != 123");
                return 0;
            }
            return 0;
        }
    };
    lua_register(lua, "test", TestSingleU16::test);
    assert(luaL_dostring(lua, "test(123)") == LUA_OK);

    assert(luaL_dostring(lua, "test()") != LUA_OK);
    assert(contains((lua_tostring(lua, -1)), "an integer expected at arg 1"));

    assert(luaL_dostring(lua, "test(123, 456)") != LUA_OK);
    assert(contains((lua_tostring(lua, -1)), "wrong arguments number"));

    assert(luaL_dostring(lua, "test(123.456)") != LUA_OK);
    assert(contains((lua_tostring(lua, -1)), "an integer expected at arg 1"));

    assert(luaL_dostring(lua, "test(\"123\")") != LUA_OK);
    assert(contains((lua_tostring(lua, -1)), "an integer expected at arg 1"));

    assert(luaL_dostring(lua, "test(-123)") != LUA_OK);
    assert(contains((lua_tostring(lua, -1)), "arg 1 is out of uint16_t range"));

    assert(luaL_dostring(lua, "test(123456)") != LUA_OK);
    assert(contains((lua_tostring(lua, -1)), "arg 1 is out of uint16_t range"));

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    struct TestBasicOptional {
        static int32_t test(lua_State* lua) {
            std::tuple<std::optional<float>> args;
            std::string errorStr;
            if (!lua::cArgParse(lua, args, errorStr)) {
                luaL_error(lua, errorStr.c_str());
                return 0;
            }
            if (std::get<0>(args).has_value()) {
                if (std::get<0>(args) != std::numeric_limits<float>::lowest()) {
                    luaL_error(lua, "arg 1 != float::lowest");
                    return 0;
                }
            }
            return 0;
        }
    };
    lua_register(lua, "test", TestBasicOptional::test);
    assert(luaL_dostring(lua, "test()") == LUA_OK);

    std::string cmd = "test(";
    cmd += std::to_string(std::numeric_limits<float>::lowest());
    cmd += ")";
    assert(luaL_dostring(lua, cmd.c_str()) == LUA_OK);

    cmd = "test(";
    cmd += std::to_string(std::numeric_limits<double>::lowest());
    cmd += ")";
    assert(luaL_dostring(lua, cmd.c_str()) != LUA_OK);
    assert(contains((lua_tostring(lua, -1)), "arg 1 is out of"));

    assert(luaL_dostring(lua, "test(\"str\")") != LUA_OK);
    assert(contains((lua_tostring(lua, -1)), "a number expected at arg 1"));

    assert(luaL_dostring(lua, "test(123.0, 456.0)") != LUA_OK);
    assert(contains((lua_tostring(lua, -1)), "wrong arguments number"));

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    struct TestTwoOptionals {
        static int32_t test(lua_State* lua) {
            std::tuple<std::optional<std::string>, std::optional<float>> args;
            std::string errorStr;
            if (!lua::cArgParse(lua, args, errorStr)) {
                luaL_error(lua, errorStr.c_str());
                return 0;
            }
            if (std::get<0>(args).has_value()) {
                if (std::get<0>(args) != "str") {
                    luaL_error(lua, "arg 1 != \"str\"");
                    return 0;
                }
            }
            if (std::get<1>(args).has_value()) {
                if (!std::get<0>(args).has_value()) {
                    luaL_error(lua, "arg 1 nullopt, arg 2 value");
                    return 0;
                }
                if (std::get<1>(args) != 123.456f) {
                    luaL_error(lua, "arg 2 != 123.456f");
                    return 0;
                }
            }
            return 0;
        }
    };
    lua_register(lua, "test", TestTwoOptionals::test);
    assert(luaL_dostring(lua, "test()") == LUA_OK);
    assert(luaL_dostring(lua, "test(\"str\")") == LUA_OK);
    assert(luaL_dostring(lua, "test(\"str\", 123.456)") == LUA_OK);

    assert(luaL_dostring(lua, "test(\"\")") != LUA_OK);
    assert(contains((lua_tostring(lua, -1)), "arg 1 != \"str\""));

    assert(luaL_dostring(lua, "test(123.456)") != LUA_OK);
    assert(contains((lua_tostring(lua, -1)), "a string expected at arg 1"));

    assert(luaL_dostring(lua, "test(123.456, \"str\")") != LUA_OK);
    assert(contains((lua_tostring(lua, -1)), "a string expected at arg 1"));

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    struct TestOptionalNotLast {
        static int32_t test(lua_State* lua) {
            std::tuple<std::optional<std::string>, float> args;
            std::string errorStr;
            if (!lua::cArgParse(lua, args, errorStr)) {
                luaL_error(lua, errorStr.c_str());
                return 0;
            }
            return 0;
        }
    };
    lua_register(lua, "test", TestOptionalNotLast::test);
    assert(luaL_dostring(lua, "test()") != LUA_OK);
    assert(contains((lua_tostring(lua, -1)), "optional must be last"));

    assert(luaL_dostring(lua, "test(\"str\", 123.456)") != LUA_OK);
    assert(contains((lua_tostring(lua, -1)), "optional must be last"));

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    //struct TestOptionalInOptional {
    //    static int32_t test(lua_State* lua) {
    //        std::tuple<std::optional<std::optional<std::string>>> args;
    //        std::string errorStr;
    //        lua::cArgParse(lua, args, errorStr); // compile-time error
    //        return 0;
    //    }
    //};

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    struct TestBasicVariant {
        static int32_t test(lua_State* lua) {
            std::variant<std::nullptr_t, uint16_t, int32_t, std::string> args;
            std::string errorStr;
            if (!lua::cArgParse(lua, args, errorStr)) {
                if (args.index() != 0) {
                    luaL_error(lua, "args.index() != 0");
                }
                else {
                    luaL_error(lua, errorStr.c_str());
                }
                return 0;
            }
            if (args.index() == 1) {
                if (std::get<uint16_t>(args) != 123) {
                    luaL_error(lua, "arg 1 != 123");
                    return 0;
                }
            }
            else if (args.index() == 3) {
                if (std::get<std::string>(args) != "str") {
                    luaL_error(lua, "arg 1 != \"str\"");
                    return 0;
                }
            }
            return 0;
        }
    };
    lua_register(lua, "test", TestBasicVariant::test);
    assert(luaL_dostring(lua, "test(123)") == LUA_OK);
    assert(luaL_dostring(lua, "test(\"str\")") == LUA_OK);

    assert(luaL_dostring(lua, "test(123.456)") != LUA_OK);
    assert(contains((lua_tostring(lua, -1)), "no suitable variant"));

    assert(luaL_dostring(lua, "test()") != LUA_OK);
    assert(contains((lua_tostring(lua, -1)), "wrong arguments number"));

    assert(luaL_dostring(lua, "test(123, 456)") != LUA_OK);
    assert(contains((lua_tostring(lua, -1)), "wrong arguments number"));

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    struct TestNullptrOnlyVariant {
        static int32_t test(lua_State* lua) {
            std::variant<std::nullptr_t> args;
            std::string errorStr;
            if (!lua::cArgParse(lua, args, errorStr)) {
                luaL_error(lua, errorStr.c_str());
                return 0;
            }
            return 0;
        }
    };
    lua_register(lua, "test", TestNullptrOnlyVariant::test);
    assert(luaL_dostring(lua, "test()") != LUA_OK);
    assert(contains((lua_tostring(lua, -1)), "wrong arguments number"));

    assert(luaL_dostring(lua, "test(123)") != LUA_OK);
    assert(contains((lua_tostring(lua, -1)), "no suitable variant"));

    assert(luaL_dostring(lua, "test(123, 456)") != LUA_OK);
    assert(contains((lua_tostring(lua, -1)), "wrong arguments number"));

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    struct TestVariousIntVariant {
        static int32_t test(lua_State* lua) {
            std::variant<double, int8_t, int16_t, int32_t, int64_t> args;
            std::string errorStr;
            if (!lua::cArgParse(lua, args, errorStr)) {
                luaL_error(lua, errorStr.c_str());
                return 0;
            }
            switch (args.index()) {
            case 1:
                if (std::get<int8_t>(args) != INT8_MIN) {
                    luaL_error(lua, "arg 1 != INT8_MIN");
                }
                break;
            case 2:
                if (std::get<int16_t>(args) != INT16_MIN) {
                    luaL_error(lua, "arg 1 != INT16_MIN");
                }
                break;
            case 3:
                if (std::get<int32_t>(args) != INT32_MIN) {
                    luaL_error(lua, "arg 1 != INT32_MIN");
                }
                break;
            case 4:
                if (std::get<int64_t>(args) != INT64_MIN + 1) {
                    luaL_error(lua, "arg 1 != INT64_MIN + 1");
                }
                break;
            default:
                if (std::get<double>(args) != INT64_MIN) {
                    luaL_error(lua, "arg 1 != INT64_MIN");
                }
                break;
            }
            return 0;
        }
    };
    lua_register(lua, "test", TestVariousIntVariant::test);
    cmd = "test("; cmd += std::to_string(INT8_MIN); cmd += ")";
    assert(luaL_dostring(lua, cmd.c_str()) == LUA_OK);
    cmd = "test("; cmd += std::to_string(INT16_MIN); cmd += ")";
    assert(luaL_dostring(lua, cmd.c_str()) == LUA_OK);
    cmd = "test("; cmd += std::to_string(INT32_MIN); cmd += ")";
    assert(luaL_dostring(lua, cmd.c_str()) == LUA_OK);
    // WTF Lua Behaviour
    cmd = "test("; cmd += std::to_string(INT64_MIN + 1); cmd += ")";
    assert(luaL_dostring(lua, cmd.c_str()) == LUA_OK);
    cmd = "test("; cmd += std::to_string(INT64_MIN); cmd += ")";
    assert(luaL_dostring(lua, cmd.c_str()) == LUA_OK);

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    struct TestVariantInTuple {
        static int32_t test(lua_State* lua) {
            std::tuple<
                std::variant<float, int32_t>,
                std::variant<int32_t, float>,
                std::optional<std::variant<std::string, char>>
            > args;
            std::string errorStr;
            if (!lua::cArgParse(lua, args, errorStr)) {
                luaL_error(lua, errorStr.c_str());
                return 0;
            }
            const auto& arg1 = std::get<0>(args);
            if (arg1.index() == 0) {
                if (std::get<0>(arg1) != 12.34f) {
                    luaL_error(lua, "arg1 != 123");
                    return 0;
                }
            }
            else { // arg1.index() == 1
                if (std::get<1>(arg1) != 123) {
                    luaL_error(lua, "arg1 != 123");
                    return 0;
                }
            }
            const auto& arg2 = std::get<1>(args);
            if (arg2.index() == 0) {
                if (std::get<0>(arg2) != 456) {
                    luaL_error(lua, "arg2 != 456");
                    return 0;
                }
            }
            else { // arg1.index() == 1
                if (std::get<1>(arg2) != 56.78f) {
                    luaL_error(lua, "arg2 != 56.78");
                    return 0;
                }
            }
            const auto& arg3 = std::get<2>(args);
            if (arg3 && arg3->index() == 0) {
                if (std::get<0>(*arg3) != "str") {
                    luaL_error(lua, "arg3 != \"str\"");
                    return 0;
                }
            }
            else if (arg3 && arg3->index() == 1) {
                if (std::get<1>(*arg3) != 0) {
                    luaL_error(lua, "arg3 != 0");
                    return 0;
                }
            }
            return 0;
        }
    };
    lua_register(lua, "test", TestVariantInTuple::test);
    assert(luaL_dostring(lua, "test(123, 456)") == LUA_OK);
    assert(luaL_dostring(lua, "test(12.34, 56.78)") == LUA_OK);
    assert(luaL_dostring(lua, "test(12.34, 456, \"str\")") == LUA_OK);
    assert(luaL_dostring(lua, "test(123, 56.78, 0)") == LUA_OK);

    assert(luaL_dostring(lua, "test()") != LUA_OK);
    assert(contains((lua_tostring(lua, -1)), "wrong arguments number"));

    assert(luaL_dostring(lua, "test(\"str\")") != LUA_OK);
    assert(contains((lua_tostring(lua, -1)), "no suitable variant"));

    assert(luaL_dostring(lua, "test(123)") != LUA_OK);
    assert(contains((lua_tostring(lua, -1)), "wrong arguments number"));

    assert(luaL_dostring(lua, "test(123, \"str\")") != LUA_OK);
    assert(contains((lua_tostring(lua, -1)), "no suitable variant"));

    assert(luaL_dostring(lua, "test(123, 456, 456)") != LUA_OK);
    assert(contains((lua_tostring(lua, -1)), "no suitable variant"));

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    struct TestBasicVector {
        static int32_t test(lua_State* lua) {
            std::tuple<std::vector<int16_t>> args;
            std::string errorStr;
            if (!lua::cArgParse(lua, args, errorStr)) {
                luaL_error(lua, errorStr.c_str());
                return 0;
            }
            if (std::get<0>(args).size() != 2) {
                luaL_error(lua, "args.size() != 2");
                return 0;
            }
            if (std::get<0>(args) != std::vector<int16_t>({ 123, 456 })) {
                luaL_error(lua, "args != { 123, 456 }");
                return 0;
            }
            return 0;
        }
    };
    lua_register(lua, "test", TestBasicVector::test);
    assert(luaL_dostring(lua, "test({ 123, 456 })") == LUA_OK);
    assert(luaL_dostring(lua, "test({ [1] = 123, [2] = 456 })") == LUA_OK);
    assert(luaL_dostring(lua, "test({ [2] = 456, [1] = 123 })") == LUA_OK);
    
    assert(luaL_dostring(lua, "test({ })") != LUA_OK);
    assert(contains((lua_tostring(lua, -1)), "args.size() != 2"));
    
    assert(luaL_dostring(lua, "test({ [0] = 123, [1] = 456 })") != LUA_OK);
    assert(contains((lua_tostring(lua, -1)), "key value 0 must be > 0 in table at arg 1"));
    
    assert(luaL_dostring(lua, "test({ [1] = 123, [3] = 456 })") != LUA_OK);
    assert(contains((lua_tostring(lua, -1)), "wrong key sequence in table at arg 1"));
    
    assert(luaL_dostring(lua, "test({ [123.456] = 1 })") != LUA_OK);
    assert(contains((lua_tostring(lua, -1)), "an integer key expected in table at arg 1"));
    
    assert(luaL_dostring(lua, "test({ [\"str\"] = 1 })") != LUA_OK);
    assert(contains((lua_tostring(lua, -1)), "an integer key expected in table at arg 1"));

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    struct TestVectorInVariant {
        static int32_t test(lua_State* lua) {
            std::variant<
                uint16_t,
                std::string,
                std::vector<uint16_t>,
                std::vector<std::string>
            > args;
            std::string errorStr;
            if (!lua::cArgParse(lua, args, errorStr)) {
                luaL_error(lua, errorStr.c_str());
                return 0;
            }
            switch (args.index()) {
            case 0:
                if (std::get<uint16_t>(args) != 123) {
                    luaL_error(lua, "args != 123");
                }
                break;
            case 1:
                if (std::get<std::string>(args) != "str") {
                    luaL_error(lua, "args != \"str\"");
                }
                break;
            case 2:
                if (std::get<std::vector<uint16_t>>(args)
                        != std::vector<uint16_t>({ 123, 456 })) {
                    luaL_error(lua, "args != { 123, 456 }");
                }
                break;
            case 3:
            default:
                if (std::get<std::vector<std::string>>(args)
                        != std::vector<std::string>({ "str", "rts" })) {
                    luaL_error(lua, "args != { \"str\", \"trs\" }");
                }
                break;
            }
            return 0;
        }
    };
    lua_register(lua, "test", TestVectorInVariant::test);
    assert(luaL_dostring(lua, "test(123)") == LUA_OK);
    assert(luaL_dostring(lua, "test(\"str\")") == LUA_OK);
    assert(luaL_dostring(lua, "test({ 123, 456 })") == LUA_OK);
    assert(luaL_dostring(lua, "test({ \"str\", \"rts\" })") == LUA_OK);

    assert(luaL_dostring(lua, "test({ 123, \"str\" })") != LUA_OK);
    assert(contains((lua_tostring(lua, -1)), "an integer expected at arg -1"));

    assert(luaL_dostring(lua, "test({ \"str\", 123 })") != LUA_OK);
    assert(contains((lua_tostring(lua, -1)), "a string expected at arg -1"));

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    struct TestBasicMap {
        static int32_t test(lua_State* lua) {
            std::tuple<std::map<std::string, std::string>> args;
            std::string errorStr;
            if (!lua::cArgParse(lua, args, errorStr)) {
                luaL_error(lua, errorStr.c_str());
                return 0;
            }
            auto& map = std::get<0>(args);
            if (map.size() != 2) {
                luaL_error(lua, "map.size() != 2");
                return 0;
            }
            if (map["str"] != "123") {
                luaL_error(lua, "map[\"str\"] != \"123\"");
                return 0;
            }
            if (map["rts"] != "456") {
                luaL_error(lua, "map[\"rts\"] != \"456\"");
                return 0;
            }
            return 0;
        }
    };
    lua_register(lua, "test", TestBasicMap::test);
    assert(luaL_dostring(lua, "test({ [\"str\"] = \"123\", [\"rts\"] = \"456\" })") == LUA_OK);

    assert(luaL_dostring(lua, "test({ \"123\" })") != LUA_OK);
    assert(contains((lua_tostring(lua, -1)), "a string expected at arg -2"));

    assert(luaL_dostring(lua, "test({ [\"str\"] = 123 })") != LUA_OK);
    assert(contains((lua_tostring(lua, -1)), "a string expected at arg -1"));

    assert(luaL_dostring(lua, "test(123)") != LUA_OK);
    assert(contains((lua_tostring(lua, -1)), "a table expected at arg 1"));

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    struct TestMapOrVector {
        static int32_t test(lua_State* lua) {
            std::variant<std::map<std::string, int32_t>, std::vector<int32_t>> args;
            std::string errorStr;
            if (!lua::cArgParse(lua, args, errorStr)) {
                luaL_error(lua, errorStr.c_str());
                return 0;
            }
            switch (args.index()) {
            case 0: {
                auto& map = std::get<0>(args);
                if (map.size() != 1) {
                    luaL_error(lua, "map.size() != 1");
                    return 0;
                }
                if (map["str"] != 123) {
                    luaL_error(lua, "map[\"str\"] != 123");
                    return 0;
                }
                break;
            }
            case 1: {
                auto& vec = std::get<1>(args);
                if (vec.size() != 1) {
                    luaL_error(lua, "vec.size() != 1");
                    return 0;
                }
                if (vec[0] != 456) {
                    luaL_error(lua, "vec[0] != 456");
                    return 0;
                }
                break;
            }
            }
            return 0;
        }
    };
    lua_register(lua, "test", TestMapOrVector::test);
    assert(luaL_dostring(lua, "test({ [\"str\"] = 123 })") == LUA_OK);
    assert(luaL_dostring(lua, "test({ 456 })") == LUA_OK);

    assert(luaL_dostring(lua, "test(123)") != LUA_OK);
    assert(contains((lua_tostring(lua, -1)), "no suitable variant"));

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    struct TestVectorInMapInVariant {
        static int32_t test(lua_State* lua) {
            std::variant<
                std::map<std::string, std::vector<int64_t>>,
                std::map<std::string, std::vector<double>>
            > args;
            std::string errorStr;
            if (!lua::cArgParse(lua, args, errorStr)) {
                luaL_error(lua, errorStr.c_str());
                return 0;
            }
            switch (args.index()) {
            case 0: {
                auto& map = std::get<0>(args);
                if (map.size() != 2) {
                    luaL_error(lua, "map.size() != 2");
                    return 0;
                }
                if (map["123"] != std::vector<int64_t>({ 1, 2, 3 })) {
                    luaL_error(lua, "map[\"123\"] != { 1, 2, 3 }");
                    return 0;
                }
                if (map["456"] != std::vector<int64_t>({ 4, 5, 6 })) {
                    luaL_error(lua, "map[\"456\"] != { 4, 5, 6 }");
                    return 0;
                }
                break;
            }
            case 1: {
                auto& map = std::get<1>(args);
                if (map.size() != 2) {
                    luaL_error(lua, "map.size() != 2");
                    return 0;
                }
                if (map["123"] != std::vector<double>({ 1, 2, 3 })) {
                    luaL_error(lua, "map[\"123\"] != { 1, 2, 3 }");
                    return 0;
                }
                if (map["456"] != std::vector<double>({ 4, 5, 6 })) {
                    luaL_error(lua, "map[\"456\"] != { 4, 5, 6 }");
                    return 0;
                }
                break;
            }
            }
            return 0;
        }
    };
    lua_register(lua, "test", TestVectorInMapInVariant::test);
    assert(luaL_dostring(lua, "test({"
        "[\"123\"] = { 1.0, 2.0, 3.0 },"
        "[\"456\"] = { 4.0, 5.0, 6.0 }"
    "})") == LUA_OK);
    assert(luaL_dostring(lua, "test({"
        "[\"123\"] = { 1, 2, 3 },"
        "[\"456\"] = { 4, 5, 6 }"
    "})") == LUA_OK);

    assert(luaL_dostring(lua, "test({"
        "[\"123\"] = { 1.0, 2.0, 3.0 },"
        "[\"456\"] = { 4, 5, 6 }"
    "})") != LUA_OK);
    assert(contains((lua_tostring(lua, -1)), "a number expected at arg -1")
        || contains((lua_tostring(lua, -1)), "an integer expected at arg -1"));

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    struct TestVectorInVariantInMap {
        static int32_t test(lua_State* lua) {
            std::tuple<std::map<std::string, std::variant<
                std::vector<int64_t>, std::vector<double>
            >>> args;
            std::string errorStr;
            if (!lua::cArgParse(lua, args, errorStr)) {
                luaL_error(lua, errorStr.c_str());
                return 0;
            }
            auto& map = std::get<0>(args);
            if (map.size() != 2) {
                luaL_error(lua, "map.size() != 2");
                return 0;
            }
            if (std::get<0>(map["123"]) != std::vector<int64_t>({ 1, 2, 3 })) {
                luaL_error(lua, "map[\"123\"] != { 1, 2, 3 }");
                return 0;
            }
            if (std::get<1>(map["456"]) != std::vector<double>({ 4, 5, 6 })) {
                luaL_error(lua, "map[\"456\"] != { 4, 5, 6 }");
                return 0;
            }
            return 0;
        }
    };
    lua_register(lua, "test", TestVectorInVariantInMap::test);
    assert(luaL_dostring(lua, "test({"
        "[\"123\"] = { 1, 2, 3 },"
        "[\"456\"] = { 4.0, 5.0, 6.0 }"
    "})") == LUA_OK);

    assert(luaL_dostring(lua, "test({"
        "[\"123\"] = { 1, 2.0, 3 },"
        "[\"456\"] = { 4.0, 5, 6.0 }"
    "})") != LUA_OK);
    assert(contains((lua_tostring(lua, -1)), "a number expected at arg -1")
        || contains((lua_tostring(lua, -1)), "an integer expected at arg -1"));

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    lua_close(lua);
    std::cout << "success" << std::endl;
    return 0;
}
