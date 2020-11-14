/*
Initial author: Convery (tcn@ayria.se)
Started: 2020-10-30
License: MIT

Each SIMD library has their own 'issues'.
NLohmann does not work well with u8string.
SIMDJSON treats all but the largest ints as signed.
*/

#pragma once
#include <memory>
#include <Stdinclude.hpp>
#include "Bytebuffer.hpp"
#include "Stringconv.hpp"
#include "Utilities/Internal/Misc.hpp"

#pragma warning(push)
#pragma warning(disable: 4702)
namespace JSON
{
    using Type_t = enum { Null, Bool, Float, Signedint, Unsignedint, String, Object, Array };
    using Object_t = std::unordered_map<std::string, struct Value_t>;
    using Array_t = std::vector<struct Value_t>;

    // Helper for type deduction.
    template<typename T> constexpr Type_t toType()
    {
        if constexpr (std::is_same_v<T, std::u8string>) return String;
        if constexpr (std::is_same_v<T, Object_t>) return Object;
        if constexpr (std::is_same_v<T, Array_t>) return Array;
        if constexpr (std::is_floating_point_v<T>) return Float;
        if constexpr (std::is_integral_v<T>)
        {
            if constexpr (std::is_signed_v<T>)
                return Signedint;
            return Unsignedint;
        }
        if constexpr (std::is_same_v<T, bool>) return Bool;

        return Null;
    }

    // Generic value wrapper.
    #define asPtr(Type) std::static_pointer_cast<Type>(Value)
    struct Value_t
    {
        Type_t Type{};
        std::shared_ptr<void> Value;

        //
        std::string dump() const
        {
            switch (Type)
            {

                case String: return va("\"%s\"", Encoding::toNarrow(*asPtr(std::u8string)).c_str());
                case Unsignedint: return va("%llu", *asPtr(uint64_t));
                case Signedint: return va("%lli", *asPtr(int64_t));
                case Bool: return *asPtr(bool) ? "true" : "false";
                case Float: return va("%f", *asPtr(double));
                case Null: return "null";

                case Object:
                {
                    std::string Result{ "{" };
                    for (const auto &[lKey, lValue] : *asPtr(Object_t))
                    {
                        Result.append(va("\"%*s\":", lKey.size(), lKey.data()));
                        Result.append(lValue.dump());
                        Result.append(",");
                    }
                    if (!asPtr(Object_t)->empty()) Result.pop_back();
                    Result.append("}");
                    return Result;
                }
                case Array:
                {
                    std::string Result{ "[" };
                    for (const auto &lValue : *asPtr(Array_t))
                    {
                        Result.append(lValue.dump());
                        Result.append(",");
                    }
                    if (!asPtr(Array_t)->empty()) Result.pop_back();
                    Result.append("]");
                    return Result;
                }

                default: return {};
            }
        }

        //
        bool update(const Value_t &Input)
        {
            if (Type == Null)
            {
                *this = Input;
                return true;
            }

            if (Type != Input.Type)
                return false;

            switch (Input.Type)
            {
                case Unsignedint: *this = Input; break;
                case Signedint: *this = Input; break;
                case String: *this = Input; break;
                case Array: *this = Input; break;
                case Float: *this = Input; break;
                case Bool: *this = Input; break;
                case Null: break;

                case Object:
                {
                    const auto Delta = std::static_pointer_cast<Object_t>(Input.Value);
                    const auto Current = asPtr(Object_t);

                    for (const auto &[lKey, lValue] : *Delta)
                    {
                        (*Current)[lKey].update(lValue);
                    }

                    break;
                }
                default:
                    return false;
            }

            return true;
        }

        //
        bool contains(std::string_view Key) const
        {
            if (Type != Object) return false;
            return asPtr(Object_t)->contains(Key.data());
        }
        bool empty() const
        {
            if (Type == String) return asPtr(std::u8string)->empty();
            if (Type == Object) return asPtr(Object_t)->empty();
            if (Type == Array) return asPtr(Array_t)->empty();
            return true;
        }

        //
        template<typename T> T value(std::string_view Key, T Defaultvalue) const
        {
            if constexpr (!std::is_convertible_v<Value_t, T>) return Defaultvalue;
            if (Type != Object) return Defaultvalue;

            if (!asPtr(Object_t)->contains(Key.data())) return Defaultvalue;
            return asPtr(Object_t)->at(Key.data());
        }

        //
        template<typename T> operator T() const
        {
            switch (Type)
            {
                case Object: if constexpr (std::is_same_v<T, Object_t>) return *asPtr(Object_t);
                case Unsignedint: if constexpr (std::is_integral_v<T>) return *asPtr(uint64_t);
                case Float: if constexpr (std::is_floating_point_v<T>) return *asPtr(double);
                case Array: if constexpr (std::is_same_v<T, Array_t>) return *asPtr(Array_t);
                case Signedint: if constexpr (std::is_integral_v<T>) return *asPtr(int64_t);
                case Bool: if constexpr (std::is_same_v<T, bool>) return *asPtr(bool);
                case String:
                {
                    if constexpr (Internal::isDerived<T, std::basic_string>{} ||
                        Internal::isDerived<T, std::basic_string_view>{})
                    {
                        if constexpr (std::is_same_v<typename T::value_type, wchar_t>)
                            return Encoding::toWide(*asPtr(std::u8string));

                        if constexpr (std::is_same_v<typename T::value_type, char>)
                            return Encoding::toNarrow(*asPtr(std::u8string));

                        if constexpr (std::is_same_v<typename T::value_type, char8_t>)
                            return *asPtr(std::u8string);
                    }
                }
                default: return {};
            }
        }
        template<typename T> T &get()
        {
            if (Type == Null) *this = Value_t(T());
            return *asPtr(T);
        };
        template<typename T> T get() const
        {
            return *this;
        }

        //
        Value_t operator[](std::string_view Key) const
        {
            return value(Key, Value_t());
        }
        Value_t &operator[](std::string_view Key)
        {
            if (Type == Null) *this = Object_t();
            if (Type != Object) return *this;
            return asPtr(Object_t)->at(Key.data());
        }

        //
        template<typename T> Value_t(const T &Input)
        {
            if constexpr (std::is_same_v<T, Value_t>) *this = Input;
            else
            {
                Type = toType<T>();

                if constexpr (std::is_integral_v<T> && !std::is_signed_v<T>) Value = std::make_shared<uint64_t>(Input);
                if constexpr (std::is_integral_v<T> && std::is_signed_v<T>) Value = std::make_shared<int64_t>(Input);
                if constexpr (std::is_floating_point_v<T>) Value = std::make_shared<double>(Input);
                if constexpr (std::is_same_v<T, bool>) Value = std::make_shared<bool>(Input);

                if constexpr (Internal::isDerived<T, std::basic_string>{} ||
                    Internal::isDerived<T, std::basic_string_view>{})
                {
                    Type = Type_t::String;
                    Value = std::make_shared<std::u8string>(Encoding::toUTF8(Input));
                }

                if constexpr (Internal::isDerived<T, std::unordered_map>{})
                {
                    Object_t Object(Input.size());
                    for (const auto &[Key, _Value] : Input)
                        Object[Key] = _Value;

                    Type = Type_t::Object;
                    Value = std::make_shared<Object_t>(std::move(Object));
                }
                if constexpr (Internal::isDerived<T, std::vector>{})
                {
                    Array_t Array(Input.size());
                    for (const auto &[Index, _Value] : Enumerate(Input))
                        Array[Index] = _Value;

                    Type = Type_t::Array;
                    Value = std::make_shared<Array_t>(std::move(Array));
                }

                assert(Type != Null);
            }
        }
        Value_t() = default;
    };
    #undef asPtr

    inline std::string Dump(const Value_t &Value) { return Value.dump(); }
    inline std::string Dump(const Array_t &Value) { return Value_t(Value).dump(); }
    inline std::string Dump(const Object_t &Value) { return Value_t(Value).dump(); }

    inline Value_t Parse(std::string_view JSONString)
    {
        Object_t Result;

        // Implementation dependent.
        if (!JSONString.empty())
        {
            #if defined(HAS_SIMDJSON)
            static simdjson::dom::parser Parser;
            simdjson::dom::object Object;

            if (!Parser.parse(JSONString).get(Object))
            {
                for (const auto &[Key, Value] : Object)
                {
                    const std::function<Value_t(const simdjson::dom::element &)>
                        Parse = [&Parse](const simdjson::dom::element &Item) -> Value_t
                    {
                        switch (Item.type())
                        {
                            case simdjson::dom::element_type::STRING: return Encoding::toUTF8(Item.get_string());
                            case simdjson::dom::element_type::DOUBLE: return Item.get_double().first;
                            case simdjson::dom::element_type::UINT64: return Item.get_uint64().first;
                            case simdjson::dom::element_type::INT64: return Item.get_int64().first;
                            case simdjson::dom::element_type::BOOL: return Item.get_bool().first;

                            case simdjson::dom::element_type::OBJECT:
                            {
                                Object_t Object; Object.reserve(Item.get_object().size());
                                for (const auto &[Key, Value] : Item.get_object())
                                {
                                    Object.emplace(Key, Parse(Value));
                                }
                                return Object;
                            }
                            case simdjson::dom::element_type::ARRAY:
                            {
                                Array_t Array; Array.reserve(Item.get_array().size());
                                for (const auto &Subitem : Item.get_array())
                                {
                                    Array.push_back(Parse(Subitem));
                                }
                                return Array;
                            }

                            case simdjson::dom::element_type::NULL_VALUE: return {};
                        }

                        // WTF??
                        assert(false);
                        return {};
                    };

                    Result.emplace(Key, Parse(Value));
                }
            }

            #elif defined(HAS_NLOHMANN)
            try
            {
                const auto Object = nlohmann::json::parse(JSONString.data(), nullptr, true, true);
                for (const auto &[Key, Value] : Object.items())
                {
                    const std::function<Value_t(const nlohmann::json &)>
                        Parse = [&Parse](const nlohmann::json &Item) -> Value_t
                    {
                        if (Item.is_string()) return Encoding::toUTF8(Item.get<std::string>());
                        if (Item.is_number_float()) return Item.get<double>();
                        if (Item.is_number_unsigned()) return Item.get<uint64_t>();
                        if (Item.is_number_integer()) return Item.get<int64_t>();
                        if (Item.is_boolean()) return Item.get<bool>();
                        if (Item.is_object())
                        {
                            Object_t Object; Object.reserve(Item.size());
                            for (const auto &[Key, Value] : Item.items())
                            {
                                Object.emplace(Key, Parse(Value));
                            }
                            return Object;
                        }
                        if (Item.is_array())
                        {
                            Array_t Array; Array.reserve(Item.size());
                            for (const auto &Subitem : Item)
                            {
                                Array.push_back(Parse(Subitem));
                            }
                            return Array;
                        }

                        // WTF??
                        assert(false);
                        return {};
                    };

                    Result.emplace(Key, Parse(Value));
                }
            } catch (const std::exception &e)
            {
                (void)e; Debugprint(e.what());
            };
            #else
            static_assert(false, "No JSON parser available.");
            #endif
        }

        return Result;
    }
    inline Value_t Parse(const std::string &JSONString)
    {
        if (JSONString.empty()) return {};
        return Parse(std::string_view(JSONString));
    }
    inline Value_t Parse(const char *JSONString)
    {
        if (!JSONString) return {};
        return Parse(std::string(JSONString));
    }
}
#pragma warning(pop)