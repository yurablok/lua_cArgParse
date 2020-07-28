# lua_cArgParse

C++17, declarative arguments type checking, metaprogramming.

### Applicability:

When writing C functions that are registered and called from Lua, it is often
necessary to write checks on the received arguments against the required data types.
By declaratively describing and passing parameter types to this library, it takes
care of the bulk of these checks.

### Basic example:

```cpp
static int32_t test(lua_State* L) {
    std::tuple<std::variant<int32_t, std::string>, std::optional<float>> args;
    std::string errorStr;
    if (!utils::lua::cArgParse(L, args, errorStr)) {
        luaL_error(L, errorStr.c_str());
        return 0;
    }
    // Don't call lua_pop(L, lua_gettop(L)) - it's already in cArgParse.
    return process(args);
}
...
lua_register(L, "test", test);
assert(luaL_dostring(L, "test(123)") == LUA_OK);
```
More examples can be found in [tests/test.cpp](https://github.com/yurablok/lua_cArgParse/blob/master/tests/tests.cpp).

### What's supported:

- `std::tuple`
- `std::variant` (cannot contain: `std::optional`, `std::tuple`, `std::variant`)
- `std::optional`
- `(u)int(8|16|32|64)_t`, `float`, `double`
- `std::string`
- `std::vector` (cannot contain: `std::optional`, `std::tuple`)
- `std::map` (cannot contain: `std::optional`, `std::tuple`)
- TODO: `std::tuple` in `std::tuple`
- TODO: maybe `std::tuple` in `std::vector` if `std::vector` not in `std::variant`
- TODO: use of Reflection far in the future

### Principle of usage:
1. Describe the types of a function parameters in `std::tuple` or `std::variant`
   using supported constructs.
2. Call `cArgParse` with passing to it these `std::tuple` or `std::variant` and
   check the arguments parsing result through the returned ErrorString or Boolean.
   An empty ErrorString means successful parsing.
3. If there was a parsing error occured, handle the error string as you want -
   pass to `luaL_error`, pass to a logger, etc.
