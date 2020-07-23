// lua_cArgParse
// C++17, declarative arguments type checking, metaprogramming.
// 
// Applicability:
//   When writing C functions that are registered and called from Lua, it is often
//   necessary to write checks on the received arguments against the required data
//   types. By declaratively describing and passing parameter types to this library,
//   it takes care of the bulk of these checks.
// 
// Basic example:
//   static int32_t test(lua_State* L) {
//       std::tuple<std::variant<int32_t, std::string>, std::optional<float>> args;
//       std::string errorStr;
//       if (!lua::cArgParse(L, args, errorStr)) {
//           luaL_error(L, errorStr.c_str());
//           return 0;
//       }
//       // Don't call lua_pop(L, lua_gettop(L)) - it's already in cArgParse.
//       return process(args);
//   }
//   ...
//   lua_register(L, "test", test);
//   assert(luaL_dostring(L, "test(123)") == LUA_OK);
//
// Author: Yurii Blok
// License: BSL-1.0
// https://github.com/yurablok/lua_cArgParse
// History:
// v0.2 23-Jul-20   Added std::map support.
// v0.1 21-Jul-20   First release.

#pragma once
#include <limits>
#include <cmath>
#include <tuple>
#include <optional>
#include <variant>
#include <string>
#include <vector>
#include <map>

extern "C" {
#   include "lua.h"
#   include "lualib.h"
#   include "lauxlib.h"
}

namespace utils::lua {

namespace details {

namespace {
    // https://habr.com/ru/post/318236/
    template<size_t Index, typename TCallback, typename ...TParams>
    struct foreach_t {
        static void foreach_(TCallback& callback, std::tuple<TParams...>& tuple) {
            const size_t idx = sizeof...(TParams) - Index;
            if (!callback(std::get<idx>(tuple))) {
                // first error - no sense to continue
                return;
            }
            foreach_t<Index - 1, TCallback, TParams...>::foreach_(callback, tuple);
        }
        static void foreach_(TCallback& callback, std::variant<TParams...>& variant) {
            const size_t idx = sizeof...(TParams) - Index;
            // explicit operator call due to the template parameter
            // https://stackoverflow.com/a/1762137
            using arg_t = std::decay_t<decltype(std::get<idx>(variant))>;
            if (callback.template operator()<arg_t>(variant)) {
                // first match - no sense to continue
                return;
            }
            foreach_t<Index - 1, TCallback, TParams...>::foreach_(callback, variant);
        }
    };
    template<typename TCallback, typename ...TParams>
    struct foreach_t<0, TCallback, TParams...> {
        static void foreach_(TCallback& /*callback*/, std::tuple<TParams...>& /*tuple*/) {
        }
        static void foreach_(TCallback& /*callback*/, std::variant<TParams...>& /*tuple*/) {
        }
    };
}
template<typename TCallback, typename ...TParams>
void foreach_(TCallback& callback, std::tuple<TParams...>& tuple) {
    // std::apply can't work with std::tuple<std::optional<...>>
    // std::apply can't be interrupted
    foreach_t<sizeof...(TParams), TCallback, TParams...>::foreach_(callback, tuple);
}
template<typename TCallback, typename ...TParams>
void foreach_(TCallback& callback, std::variant<TParams...>& variant) {
    foreach_t<sizeof...(TParams), TCallback, TParams...>::foreach_(callback, variant);
}

template <typename>
struct is_optional : std::false_type {};
template <typename T>
struct is_optional<std::optional<T>> : std::true_type {};

template <typename>
struct is_tuple : std::false_type {};
template <typename ...T>
struct is_tuple<std::tuple<T...>> : std::true_type {};

template <typename>
struct is_variant : std::false_type {};
template <typename ...T>
struct is_variant<std::variant<T...>> : std::true_type {};

template <typename>
struct is_vector : std::false_type {};
template <typename T>
struct is_vector<std::vector<T>> : std::true_type {};

template <typename>
struct is_map : std::false_type {};
template <typename key_t, typename value_t>
struct is_map<std::map<key_t, value_t>> : std::true_type {};

template <typename T>
struct always_false : std::false_type {};

struct LuaCArgParseMeta {
    lua_State* lua = nullptr;
    std::string* errorStr = nullptr;
    int32_t argsNumber = 0;
    int32_t argIdx = 0;
};

template <typename arg_t, typename res_t>
bool processInteger(LuaCArgParseMeta& meta, res_t& res, const bool quiet) {
    if (static_cast<bool>(lua_isinteger(meta.lua, meta.argIdx)) == false
            || lua_type(meta.lua, meta.argIdx) != LUA_TNUMBER) {
        if (!quiet) {
            *meta.errorStr = "an integer expected at arg ";
            *meta.errorStr += std::to_string(meta.argIdx);
            meta.argIdx = -1;
        }
        return false;
    }
    const lua_Integer integer64 = lua_tointeger(meta.lua, meta.argIdx);
    if (integer64 < std::numeric_limits<arg_t>::lowest()
            || integer64 > std::numeric_limits<arg_t>::max()) {
        if (!quiet) {
            *meta.errorStr = "value ";
            *meta.errorStr += std::to_string(integer64);
            *meta.errorStr += " at arg ";
            *meta.errorStr += std::to_string(meta.argIdx);
            *meta.errorStr += " is out of ";
            if constexpr (std::is_unsigned_v<arg_t>) {
                *meta.errorStr += "u";
            }
            *meta.errorStr += "int";
            *meta.errorStr += std::to_string(sizeof(arg_t) * 8);
            *meta.errorStr += "_t range";
            meta.argIdx = -1;
        }
        return false;
    }
    res = static_cast<arg_t>(integer64);
    return true;
}

template <typename arg_t, typename res_t>
bool processFloat(LuaCArgParseMeta& meta, res_t& res, const bool quiet) {
    if (static_cast<bool>(lua_isinteger(meta.lua, meta.argIdx)) == true
            || lua_type(meta.lua, meta.argIdx) != LUA_TNUMBER) {
        if (!quiet) {
            *meta.errorStr = "a number expected at arg ";
            *meta.errorStr += std::to_string(meta.argIdx);
            meta.argIdx = -1;
        }
        return false;
    }
    const lua_Number float64 = lua_tonumber(meta.lua, meta.argIdx);
    if (float64 < std::numeric_limits<arg_t>::lowest()
            || float64 > std::numeric_limits<arg_t>::max()) {
        if (!quiet) {
            *meta.errorStr = "value ";
            *meta.errorStr += std::to_string(float64);
            *meta.errorStr += " at arg ";
            *meta.errorStr += std::to_string(meta.argIdx);
            *meta.errorStr += " is out of ";
            if constexpr (sizeof(res_t) == sizeof(float)) {
                *meta.errorStr += "float";
            }
            else {
                *meta.errorStr += "double";
            }
            *meta.errorStr += " range";
            meta.argIdx = -1;
        }
        return false;
    }
    res = static_cast<arg_t>(float64);
    return true;
}

template <typename res_t>
bool processString(LuaCArgParseMeta& meta, res_t& res, const bool quiet) {
    if (lua_type(meta.lua, meta.argIdx) != LUA_TSTRING) {
        if (!quiet) {
            *meta.errorStr = "a string expected at arg ";
            *meta.errorStr += std::to_string(meta.argIdx);
            meta.argIdx = -1;
        }
        return false;
    }
    size_t len = 0;
    const char* str = lua_tolstring(meta.lua, meta.argIdx, &len);
    if (str == nullptr) {
        if (!quiet) {
            *meta.errorStr = "a string expected at arg ";
            *meta.errorStr += std::to_string(meta.argIdx);
            meta.argIdx = -1;
        }
        return false;
    }
    res = std::move(std::string(str, len));
    return true;
}

template <typename arg_t>
bool processOptional(LuaCArgParseMeta& meta, std::optional<arg_t>& optional) {
    if (meta.argIdx > meta.argsNumber) {
        --meta.argIdx;
        return true;
    }
    optional = arg_t();
    bool ok = false;
    if constexpr (std::is_integral_v<arg_t>) {
        ok = processInteger<arg_t>(meta, optional.value(), false);
    }
    else if constexpr (std::is_floating_point_v<arg_t>) {
        ok = processFloat<arg_t>(meta, optional.value(), false);
    }
    else if constexpr (std::is_same_v<arg_t, std::string>) {
        ok = processString(meta, optional.value(), false);
    }
    else if constexpr (is_variant<arg_t>::value) {
        ok = processVariant(meta, optional.value());
    }
    else {
        static_assert(always_false<arg_t>::value, "prohibited combination");
    }
    if (!ok) {
        optional = std::nullopt;
        return false;
    }
    return true;
}

template <typename arg_t, typename res_t>
bool processVector(LuaCArgParseMeta& meta, res_t& res, const bool quietInit) {
    if (lua_type(meta.lua, meta.argIdx) != LUA_TTABLE) {
        if (!quietInit) {
            *meta.errorStr = "a table expected at arg ";
            *meta.errorStr += std::to_string(meta.argIdx);
            meta.argIdx = -1;
        }
        return false;
    }
    bool ok = true;
    lua_pushnil(meta.lua);
    std::map<int32_t, arg_t> map;
    bool quiet = quietInit;
    while (lua_next(meta.lua, -2) != 0) {
        if (!ok) {
            lua_pop(meta.lua, 1);
            continue;
        }
        do {
            // key at -2 and value at -1
            const int32_t keyType = lua_type(meta.lua, -2);
            if (keyType != LUA_TNUMBER) {
                *meta.errorStr = "an integer key expected in table at arg ";
                *meta.errorStr += std::to_string(meta.argIdx);
                ok = false;
                break;
            }
            const lua_Number keyValueF = lua_tonumber(meta.lua, -2);
            lua_Number integralPart;
            if (std::modf(keyValueF, &integralPart) != 0.0) {
                *meta.errorStr = "an integer key expected in table at arg ";
                *meta.errorStr += std::to_string(meta.argIdx);
                ok = false;
                break;
            }
            const int32_t keyValue = static_cast<int32_t>(integralPart);
            if (keyValue < 1) {
                *meta.errorStr = "key value ";
                *meta.errorStr += std::to_string(keyValue);
                *meta.errorStr += " must be > 0 in table at arg ";
                *meta.errorStr += std::to_string(meta.argIdx);
                ok = false;
                break;
            }
            LuaCArgParseMeta valueMeta;
            valueMeta.lua = meta.lua;
            valueMeta.errorStr = meta.errorStr;
            valueMeta.argIdx = -1;
            arg_t arg;
            if constexpr (std::is_integral_v<arg_t>) {
                ok = processInteger<arg_t>(valueMeta, arg, quiet);
            }
            else if constexpr (std::is_floating_point_v<arg_t>) {
                ok = processFloat<arg_t>(valueMeta, arg, quiet);
            }
            else if constexpr (std::is_same_v<arg_t, std::string>) {
                ok = processString(valueMeta, arg, quiet);
            }
            else if constexpr (is_optional<arg_t>::value) {
                static_assert(always_false<arg_t>::value, "optional is not allowed in vector");
            }
            else if constexpr (is_variant<arg_t>::value) {
                static_assert(always_false<arg_t>::value, "variant is not allowed in vector");
            }
            else if constexpr (is_tuple<arg_t>::value) {
                static_assert(always_false<arg_t>::value, "tuple is not allowed in vector");
            }
            else {
                static_assert(always_false<arg_t>::value, "prohibited combination");
            }
            // If in variant && error && first iteration.
            if (quietInit && !ok && map.empty()) {
                // Revert.
                lua_settable(meta.lua, -3);
                return false;
            }
            quiet = false;
            if (ok) {
                map[keyValue] = std::move(arg);
            }
        } while (false);
        // Remove the value with keeping the key for the next iteration.
        lua_pop(meta.lua, 1);
    }
    if (!meta.errorStr->empty()) {
        meta.argIdx = -1;
        return false;
    }
    std::vector<arg_t> vector;
    vector.reserve(map.size());
    // std::transform is 5% faster but does not allow movement.
    // Maybe there is some other way?
    int32_t maxIdx = 0;
    for (auto& it : map) {
        vector.push_back(std::move(it.second));
        maxIdx = it.first;
    }
    if (maxIdx != vector.size()) {
        *meta.errorStr = "wrong key sequence in table at arg ";
        *meta.errorStr += std::to_string(meta.argIdx);
        ok = false;
    }
    if (!ok) {
        meta.argIdx = -1;
    }
    else {
        res = std::move(vector);
    }
    return ok;
}

template <typename key_t, typename value_t, typename res_t>
bool processMap(LuaCArgParseMeta& meta, res_t& res, const bool quietInit) {
    if (lua_type(meta.lua, meta.argIdx) != LUA_TTABLE) {
        if (!quietInit) {
            *meta.errorStr = "a table expected at arg ";
            *meta.errorStr += std::to_string(meta.argIdx);
            meta.argIdx = -1;
        }
        return false;
    }
    bool ok = true;
    lua_pushnil(meta.lua);
    std::map<key_t, value_t> map;
    bool quiet = quietInit;
    while (lua_next(meta.lua, -2) != 0) {
        if (!ok) {
            lua_pop(meta.lua, 1);
            continue;
        }
        do {
            // key at -2 and value at -1
            LuaCArgParseMeta parseMeta;
            parseMeta.lua = meta.lua;
            parseMeta.errorStr = meta.errorStr;
            parseMeta.argIdx = -2;
            key_t key;
            if constexpr (std::is_integral_v<key_t>) {
                ok = processInteger<key_t>(parseMeta, key, quiet);
            }
            else if constexpr (std::is_floating_point_v<key_t>) {
                ok = processFloat<key_t>(parseMeta, key, quiet);
            }
            else if constexpr (std::is_same_v<key_t, std::string>) {
                ok = processString(parseMeta, key, quiet);
            }
            else {
                static_assert(always_false<key_t>::value, "prohibited combination");
            }
            // If in variant && error && first iteration.
            if (quietInit && !ok && map.empty()) {
                // Revert.
                lua_settable(meta.lua, -3);
                return false;
            }
            if (ok) {
                parseMeta.argIdx = -1;
                value_t value;
                if constexpr (std::is_integral_v<value_t>) {
                    ok = processInteger<value_t>(parseMeta, value, quiet);
                }
                else if constexpr (std::is_floating_point_v<value_t>) {
                    ok = processFloat<value_t>(parseMeta, value, quiet);
                }
                else if constexpr (std::is_same_v<value_t, std::string>) {
                    ok = processString(parseMeta, value, quiet);
                }
                else if constexpr (is_optional<value_t>::value) {
                    static_assert(always_false<value_t>::value, "optional is not allowed in map");
                }
                else if constexpr (is_variant<value_t>::value) {
                    static_assert(always_false<value_t>::value, "variant is not allowed in map");
                }
                else if constexpr (is_tuple<value_t>::value) {
                    static_assert(always_false<value_t>::value, "tuple is not allowed in map");
                }
                else {
                    static_assert(always_false<value_t>::value, "prohibited combination");
                }
                // If in variant && error && first iteration.
                if (quietInit && !ok && map.empty()) {
                    // Revert.
                    lua_settable(meta.lua, -3);
                    return false;
                }
                if (ok) {
                    map[key] = std::move(value);
                }
            }
            quiet = false;
        } while (false);
        // Remove the value with keeping the key for the next iteration.
        lua_pop(meta.lua, 1);
    }
    if (!meta.errorStr->empty()) {
        meta.argIdx = -1;
        return false;
    }
    res = std::move(map);
    return ok;
}

struct VariantVisitor {
    LuaCArgParseMeta* meta = nullptr;
    bool success = false;

    template <typename arg_t, typename ...args_t>
    bool operator()(std::variant<args_t...>& arg) {
        using T = arg_t;
#ifdef _DEBUG
        const auto lua_type_test = lua_typename(meta->lua, lua_type(meta->lua, meta->argIdx));
        (void)lua_type_test;
#endif // _DEBUG
        if constexpr (std::is_integral_v<T>) {
            success = processInteger<T>(*meta, arg, true);
        }
        else if constexpr (std::is_floating_point_v<T>) {
            success = processFloat<T>(*meta, arg, true);
        }
        else if constexpr (std::is_same_v<T, std::string>) {
            success = processString(*meta, arg, true);
        }
        else if constexpr (std::is_same_v<T, std::nullptr_t>) {
            return false;
        }
        else if constexpr (is_vector<T>::value) {
            success = processVector<typename T::value_type>(*meta, arg, true);
            if (meta->argIdx == -1) {
                // Error, abort processing.
                return true;
            }
        }
        else if constexpr (is_map<T>::value) {
            success = processMap<typename T::key_type, typename T::mapped_type>(
                *meta, arg, true);
            if (meta->argIdx == -1) {
                // Error, abort processing.
                return true;
            }
        }
        else if constexpr (is_optional<T>::value) {
            static_assert(always_false<T>::value, "optional is not allowed in variant");
        }
        else if constexpr (is_variant<T>::value) {
            static_assert(always_false<T>::value, "variant is not allowed in variant");
        }
        else if constexpr (is_tuple<T>::value) {
            static_assert(always_false<T>::value, "tuple is not allowed in variant");
        }
        else {
            static_assert(always_false<T>::value, "prohibited combination");
        }
        return success;
    }
};

struct TupleVisitor {
    LuaCArgParseMeta* meta = nullptr;
    bool isOnlyOptionalAllowed = false;

    template<typename arg_t>
    bool operator()(arg_t&& arg) {
        using T = std::decay_t<decltype(arg)>;
        meta->errorStr->clear();
        ++meta->argIdx;
#ifdef _DEBUG
        const auto lua_type_test = lua_typename(meta->lua, lua_type(meta->lua, meta->argIdx));
        (void)lua_type_test;
#endif // _DEBUG
        if (isOnlyOptionalAllowed) {
            if constexpr (!is_optional<T>::value) {
                *meta->errorStr = "optional must be last";
                meta->argIdx = -1;
                return false;
            }
        }
        if constexpr (std::is_integral_v<T>) {
            return processInteger<T>(*meta, arg, false);
        }
        else if constexpr (std::is_floating_point_v<T>) {
            return processFloat<T>(*meta, arg, false);
        }
        else if constexpr (std::is_same_v<T, std::string>) {
            return processString(*meta, arg, false);
        }
        else if constexpr (is_variant<T>::value) {
            return processVariant(*meta, arg);
        }
        else if constexpr (is_optional<T>::value) {
            isOnlyOptionalAllowed = true;
            return processOptional(*meta, arg);
        }
        else if constexpr (is_vector<T>::value) {
            return processVector<typename T::value_type>(*meta, arg, false);
        }
        else if constexpr (is_map<T>::value) {
            return processMap<typename T::key_type, typename T::mapped_type>(
                *meta, arg, false);
        }
        else {
            static_assert(always_false<T>::value, "prohibited combination");
        }
    }
};

template <typename ...args_t>
bool processVariant(LuaCArgParseMeta& meta, std::variant<args_t...>& variant) {
    if (lua_type(meta.lua, meta.argIdx) == LUA_TNONE) {
        *meta.errorStr = "wrong arguments number";
        return false;
    }
    VariantVisitor visitor;
    visitor.meta = &meta;
    foreach_(visitor, variant);
    if (!visitor.success && meta.errorStr->empty()) {
        *meta.errorStr = "no suitable variant";
    }
    return visitor.success;
}

template <typename ...args_t>
bool processTuple(LuaCArgParseMeta& meta, std::tuple<args_t...>& tuple) {
    TupleVisitor visitor;
    visitor.meta = &meta;
    foreach_(visitor, tuple);
    return meta.argIdx == meta.argsNumber && meta.errorStr->empty();
}

} // namespace details


template <typename ...args_t>
bool cArgParse(lua_State* lua, std::tuple<args_t...>& args, std::string& errorStr) {
    errorStr.clear();
    details::LuaCArgParseMeta meta;
    meta.lua = lua;
    meta.errorStr = &errorStr;
    meta.argsNumber = lua_gettop(lua);
    meta.argIdx = 0;
    const bool ok = details::processTuple(meta, args);
    lua_pop(lua, meta.argsNumber);
    if (ok) {
        return true;
    }
    if (!errorStr.empty()) {
        return false;
    }
    if (meta.argIdx != meta.argsNumber) {
        errorStr = "wrong arguments number";
        return false;
    }
    return true;
}
template <typename ...args_t>
std::tuple<args_t...> cArgParse(lua_State* lua, std::string& errorStr) {
    std::tuple<args_t...> args;
    cArgParse(lua, args, errorStr);
    return std::move(args);
}
template <typename ...args_t>
bool cArgParse(lua_State* lua, std::variant<args_t...>& args, std::string& errorStr) {
    errorStr.clear();
    details::LuaCArgParseMeta meta;
    meta.lua = lua;
    meta.errorStr = &errorStr;
    meta.argsNumber = lua_gettop(lua);
    meta.argIdx = 1;
    // since only simple types are allowed in a variant
    if (meta.argIdx != meta.argsNumber) {
        errorStr = "wrong arguments number";
        return false;
    }
    const bool ok = details::processVariant(meta, args);
    lua_pop(lua, meta.argsNumber);
    if (ok) {
        return true;
    }
    if (!errorStr.empty()) {
        return false;
    }
    return true;
}
template <typename ...args_t>
std::variant<args_t...> cArgParse(lua_State* lua, std::string& errorStr) {
    std::variant<args_t...> args;
    cArgParse(lua, args, errorStr);
    return std::move(args);
}

} // namespace utils::lua
