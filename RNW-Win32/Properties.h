#pragma once
#include <unordered_map>
using Thickness = unsigned short;
using Size = std::array<Thickness, 2>;
enum class TextAlign {
    Left = TA_LEFT,
    Right = TA_RIGHT,
    Baseline = TA_BASELINE,
    Center = TA_CENTER,
};

inline bool operator&(TextAlign a, TextAlign b) {
    return static_cast<std::underlying_type_t<TextAlign>>(a) & static_cast<std::underlying_type_t<TextAlign>>(b);
}

template<typename TPropertyEnum>
struct PropertyStorage {
private:
    union PropertyTypes {
        std::monostate _;
        COLORREF colorRef;
        float f;
        int i;
        uint16_t u16;
        bool b;
        Size size;
        TextAlign textAlign;
    };
    static constexpr auto properties_count = static_cast<int>(TPropertyEnum::Last);
    std::array<PropertyTypes, properties_count> m_properties{};
    std::array<uint8_t, (properties_count + 1) / 2> m_types{};
    static_assert(sizeof(PropertyTypes) <= 4, "Property type got too big, consider using sparse storage instead");

    template<typename T>
    struct type_index {};

    template<> struct type_index<std::monostate> { static constexpr uint8_t value = 0; };
    template<> struct type_index<COLORREF> { static constexpr uint8_t value = 1; };
    template<> struct type_index<float> { static constexpr uint8_t value = 2; };
    template<> struct type_index<int> { static constexpr uint8_t value = 3; };
    template<> struct type_index<bool> { static constexpr uint8_t value = 4; };
    template<> struct type_index<Size> { static constexpr uint8_t value = 5; };
    template<> struct type_index<uint16_t> { static constexpr uint8_t value = 6; };
    template<> struct type_index<TextAlign> { static constexpr uint8_t value = 7; };
    // NB: type indices can go until 15

    template<typename T>
    static constexpr uint8_t type_index_v = type_index<T>::value;

public:
    template<typename T>
    void set(size_t index, T value) {
        if constexpr (std::is_same_v<T, std::monostate>) {
            m_properties[index]._ = {};
        }
        else if constexpr (std::is_same_v<T, COLORREF>) {
            m_properties[index].colorRef = value;
        }
        else if constexpr (std::is_same_v<T, float>) {
            m_properties[index].f = value;
        }
        else if constexpr (std::is_same_v<T, uint16_t>) {
            m_properties[index].u16 = value;
        }
        else if constexpr (std::is_same_v<T, int>) {
            m_properties[index].i = value;
        }
        else if constexpr (std::is_same_v<T, bool>) {
            m_properties[index].b = value;
        }
        else if constexpr (std::is_same_v<T, Size>) {
            m_properties[index].size = value;
        }
        else if constexpr (std::is_same_v<T, TextAlign>) {
            m_properties[index].textAlign = value;
        }
        else {
            static_assert(sizeof(T) == 0, "unknown type");
        }

        auto v = m_types[index / 2];
        constexpr auto type_idx = type_index_v<T>;
        static_assert(type_idx < 16, "type indices must fit in a nibble");
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
            if constexpr (type_idx == type_index_v<std::monostate>) return std::nullopt;
            else if constexpr (type_idx == type_index_v<COLORREF>) return v.colorRef;
            else if constexpr (type_idx == type_index_v<float>) return v.f;
            else if constexpr (type_idx == type_index_v<int>) return v.i;
            else if constexpr (type_idx == type_index_v<bool>) return v.b;
            else if constexpr (type_idx == type_index_v<Size>) return v.size;
            else if constexpr (type_idx == type_index_v<uint16_t>) return v.u16;
            else if constexpr (type_idx == type_index_v<TextAlign>) return v.textAlign;
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
    Foreground,

    BorderRadius,
    BorderColor,
    BorderWidth,

    OnMouseEnter,
    OnMouseLeave,
    OnPress,

    FontSize,
    TextAlign,

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
    FontFamily,
    Last,
};

template<typename TStringIndexEnum>
struct StringStorage {
    std::unordered_map<TStringIndexEnum, std::wstring> m_strings;
};

template<StringPropertyIndex index>
struct StringProperty {
    using type = std::wstring_view;

    static std::optional<std::wstring> Get(StringStorage<StringPropertyIndex>& storage) {
        const auto& v = storage.m_strings.find(index);
        if (v != storage.m_strings.end()) {
            return v->second;
        }
        else {
            return std::nullopt;
        }
    }

    static std::optional<std::wstring> Get(StringStorage<StringPropertyIndex>* storage) {
        return Get(*storage);
    }

    static void Set(StringStorage<StringPropertyIndex>& storage, std::wstring_view value) {
        storage.m_strings[index] = value;
    }

    static void Set(StringStorage<StringPropertyIndex>* storage, std::wstring_view value) {
        return Set(*storage, value);
    }

    static void Clear(StringStorage<StringPropertyIndex>& storage) {
        storage.m_strings.erase(index);
    }
};

