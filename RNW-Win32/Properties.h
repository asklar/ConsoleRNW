#pragma once
#include <unordered_map>

template<typename TPropertyEnum>
struct PropertyStorage {
private:
    union PropertyTypes {
        std::monostate _;
        DWORD d;
        float f;
        int i;
        bool b;
    };
    static constexpr auto properties_count = static_cast<int>(TPropertyEnum::Last);
    std::array<PropertyTypes, properties_count> m_properties{};
    std::array<uint8_t, (properties_count + 1) / 2> m_types{};
    static_assert(sizeof(PropertyTypes) <= 4, "Property type got too big, consider using sparse storage instead");

    template<typename T>
    struct type_index {};

    template<> struct type_index<std::monostate> { static constexpr uint8_t value = 0; };
    template<> struct type_index<DWORD> { static constexpr uint8_t value = 1; };
    template<> struct type_index<float> { static constexpr uint8_t value = 2; };
    template<> struct type_index<int> { static constexpr uint8_t value = 3; };
    template<> struct type_index<bool> { static constexpr uint8_t value = 4; };

    template<typename T>
    static constexpr uint8_t type_index_v = type_index<T>::value;

public:
    template<typename T>
    void set(size_t index, T value) {
        if constexpr (std::is_same_v<T, std::monostate>) {
            m_properties[index]._ = {};
        }
        else if constexpr (std::is_same_v<T, DWORD>) {
            m_properties[index].d = value;
        }
        else if constexpr (std::is_same_v<T, float>) {
            m_properties[index].f = value;
        }
        else if constexpr (std::is_same_v<T, int>) {
            m_properties[index].i = value;
        }
        else if constexpr (std::is_same_v<T, bool>) {
            m_properties[index].b = value;
        }
        else {
            static_assert(sizeof(T) == 0, "unknown type");
        }

        auto v = m_types[index / 2];
        auto type_idx = type_index_v<T>;
        if (index % 2 == 0) {
            v = (v & 0xf0) | type_idx;
        }
        else {
            v = (v & 0x0f) | (type_idx << 4);
        }
        m_types[index / 2] = v;
    }

    uint8_t get_type_index(size_t index) {
        auto v = m_types[index / 2];
        return (index % 2 == 0) ? (v & 0xf) : (v >> 4);
    }

    template<typename T>
    std::optional<T> get(size_t index) {
        auto v = m_properties[index];
        constexpr auto type_idx = type_index_v<T>;
        if (get_type_index(index) == type_idx) {
            if constexpr (type_idx == 0) return std::nullopt;
            else if constexpr (type_idx == 1) return v.d;
            else if constexpr (type_idx == 2) return v.f;
            else if constexpr (type_idx == 3) return v.i;
            else if constexpr (type_idx == 4) return v.b;
            else
                static_assert(sizeof(T) == 0, "unknown type");
        }
        else {
            return std::nullopt;
        }
    }

};

enum class PropertyIndex {
    Background,
    BorderRadius,
    OnMouseEnter,
    OnMouseLeave,
    Last,
};

template<typename TBackingType, PropertyIndex index>
struct Property {
    using type = TBackingType;
    static std::optional<TBackingType> Get(PropertyStorage<PropertyIndex>& storage) {
        return storage.get<TBackingType>(static_cast<int>(index));
    }

    static std::optional<TBackingType> Get(PropertyStorage<PropertyIndex>* storage) {
        return Get(*storage);
    }


    static void Set(PropertyStorage<PropertyIndex>& storage, TBackingType value) {
        storage.set(static_cast<int>(index), value);
    }

    static void Set(PropertyStorage<PropertyIndex>* storage, TBackingType value) {
        return Set(*storage, value);
    }

    static void Clear(PropertyStorage<PropertyIndex>& storage) {
        return Set(storage, std::monostate{});
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

