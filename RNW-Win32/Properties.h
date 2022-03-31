#pragma once
#include <unordered_map>

template<typename TPropertyEnum>
struct PropertyStorage {
    using PropertyTypes = std::variant<DWORD, float, int, std::string>;
    std::array<std::optional<PropertyTypes>, static_cast<int>(TPropertyEnum::Last)> m_properties{};
};

enum class PropertyIndex {
    Background,
    BorderRadius,
    Last,
};

template<typename TBackingType, PropertyIndex index>
struct Property {
    using type = TBackingType;
    static std::optional<TBackingType> Get(PropertyStorage<PropertyIndex>& storage) {
        const auto& v = storage.m_properties[static_cast<int>(index)];
        if (v.has_value()) {
            if (std::holds_alternative<TBackingType>(v.value())) {
                return std::get<TBackingType>(v.value());
            }
        }
        return std::nullopt;
    }

    static std::optional<TBackingType> Get(PropertyStorage<PropertyIndex>* storage) {
        return Get(*storage);
    }


    static void Set(PropertyStorage<PropertyIndex>& storage, TBackingType value) {
        storage.m_properties[static_cast<int>(index)] = value;
    }

    static void Set(PropertyStorage<PropertyIndex>* storage, TBackingType value) {
        return Set(*storage, value);
    }

    static void Clear(PropertyStorage<PropertyIndex>& storage) {
        return Set(storage, std::nullopt);
    }
};


enum class StringPropertyIndex {
    Text,
    Last,
};

template<typename TStringIndexEnum>
struct StringStorage {
    std::unordered_map<TStringIndexEnum, std::string> m_strings;
};

template<StringPropertyIndex index>
struct StringProperty {
    static std::optional<std::string> Get(StringStorage<StringPropertyIndex>& storage) {
        const auto& v = storage.m_strings.find(index);
        if (v != storage.m_strings.end()) {
            return v->second;
        }
        else {
            return std::nullopt;
        }
    }

    static std::optional<std::string> Get(StringStorage<StringPropertyIndex>* storage) {
        return Get(*storage);
    }

    static void Set(StringStorage<StringPropertyIndex>& storage, std::string_view value) {
        storage.m_strings[static_cast<int>(index)] = value;
    }

    static void Set(StringStorage<StringPropertyIndex>* storage, std::string_view value) {
        return Set(*storage, value);
    }

    static void Clear(StringStorage<StringPropertyIndex>& storage) {
        storage.m_strings.erase(static_cast<int>(index));
    }
};
