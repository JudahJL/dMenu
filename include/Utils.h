#pragma once

#include <SimpleIni.h>

inline size_t strl(const char* a_str) {
    if(a_str == nullptr)
        return 0;

    size_t len = 0;
    while(a_str[len] != '\0' && len < 0x7F'FF'FF'FF) {
        len++;
    }
    return len;
}

namespace Utils {
    namespace imgui {
        void HoverNote(const char* text, const char* note = "*");
    }  // namespace imgui

    template<class T>
    void loadUsefulPlugins(std::unordered_set<RE::TESFile*>& mods) {
        for(auto* form : RE::TESDataHandler::GetSingleton()->GetFormArray<T>()) {
            if(form) [[likely]] {
                if(!mods.contains(form->GetFile())) {
                    mods.insert(form->GetFile());
                }
            }
        }
    };

    using func_get_form_editor_id = const char* (*)(std::uint32_t);

    std::string getFormEditorID(const RE::TESForm* a_form);

    class Setting
    {
        template<typename T>
        friend RE::Setting* MakeSetting(const char* a_name, T a_data);

        friend RE::Setting* MakeSetting(char* a_name);

        using Type = RE::Setting::Type;

        explicit Setting(char* a_name):
            name(a_name){};

        template<typename T>
        Setting(const char* a_name, T a_data)
            requires(std::same_as<T, const char*> ||
                     std::same_as<T, bool> ||
                     std::same_as<T, int> ||
                     std::same_as<T, unsigned int> ||
                     std::same_as<T, float>)
        {
            std::size_t nameLen = std::strlen(a_name) + sizeof('\0');

            if(nameLen < MAX_PATH) {
                name = new char[nameLen];
                strcpy_s(name, nameLen, a_name);

                if constexpr(std::is_same_v<T, const char*>) {
                    if(GetType() == Type::kString) {
                        std::size_t dataLen = std::strlen(a_data) + sizeof('\0');
                        if(dataLen < MAX_PATH) {
                            data.s = new char[dataLen];
                            strcpy_s(data.s, dataLen, a_data);

                            return;
                        }
                    }
                } else if constexpr(std::is_same_v<T, bool>) {
                    if(GetType() == Type::kBool) {
                        data.b = a_data;

                        return;
                    }
                } else if constexpr(std::is_same_v<T, int>) {
                    if(GetType() == Type::kSignedInteger) {
                        data.i = a_data;

                        return;
                    }
                } else if constexpr(std::is_same_v<T, unsigned int>) {
                    if(GetType() == Type::kUnsignedInteger) {
                        data.u = a_data;

                        return;
                    }
                } else if constexpr(std::is_same_v<T, float>) {
                    if(GetType() == Type::kFloat) {
                        data.f = a_data;

                        return;
                    }
                }

                delete[] name;
                name = nullptr;
            }
        }

        virtual ~Setting()  // 00
        {
            if(name) {
                if(GetType() == Type::kString) {
                    delete[] data.s;
                }

                delete[] name;
            }
        }

        // For the virtual table auto-generation.
        virtual bool Unk_01() { return false; }  // 01

        [[nodiscard]] Type GetType() const {
            return reinterpret_cast<const RE::Setting*>(this)->GetType();
        }

        // members
        RE::Setting::Data data{};  // 08
        char*             name;    // 10
    };

    static_assert(sizeof(Setting) == sizeof(RE::Setting));

    template<typename T>
    RE::Setting* MakeSetting(const char* a_name, T a_data) {
        return reinterpret_cast<RE::Setting*>(new Setting{ a_name, a_data });
    }

    inline RE::Setting* MakeSetting(char* a_name) {
        return reinterpret_cast<RE::Setting*>(new Setting{ a_name });
    }

}  // namespace Utils

template<class T>
T lexical_cast(const std::string& a_str, bool a_hex = false) {
    const auto base{ a_hex ? 16 : 10 };

    if constexpr(std::is_floating_point_v<T>) {
        return static_cast<T>(std::stof(a_str));
    } else if constexpr(std::is_signed_v<T>) {
        return static_cast<T>(std::stoi(a_str, nullptr, base));
    } else if constexpr(sizeof(T) == sizeof(std::uint64_t)) {
        return static_cast<T>(std::stoull(a_str, nullptr, base));
    } else {
        return static_cast<T>(std::stoul(a_str, nullptr, base));
    }
}

/*Helper class to load from a simple ini file.*/
class settingsLoader
{
public:
     settingsLoader() = delete;
    ~settingsLoader();

    explicit settingsLoader(const char* settingsFile);

    template<class T>
    void update(T& a_value, const char* a_section, const char* a_key, const char* a_comment) {
        if constexpr(std::is_same_v<bool, T>) {
            _ini.SetBoolValue(a_section, a_key, a_value, a_comment);
            ++_savedSettings;
        } else if constexpr(std::is_floating_point_v<T>) {
            _ini.SetDoubleValue(a_section, a_key, a_value, a_comment);
            ++_savedSettings;
        } else if constexpr(std::is_arithmetic_v<T> || std::is_enum_v<T>) {
            _ini.SetValue(a_section, a_key, std::to_string(a_value).c_str(), a_comment);
            ++_savedSettings;
        } else {
            _ini.SetValue(a_section, a_key, a_value.c_str(), a_comment);
            ++_savedSettings;
        }
    }

    template<class T>
    void load(T& a_value, const char* a_section, const char* a_key) {
        if constexpr(std::is_same_v<bool, T>) {
            a_value = _ini.GetBoolValue(a_section, a_key, a_value);
            ++_loadedSettings;
        } else if constexpr(std::is_floating_point_v<T>) {
            a_value = static_cast<T>(_ini.GetDoubleValue(a_section, a_key, a_value));
            ++_loadedSettings;
        } else if constexpr(std::is_arithmetic_v<T> || std::is_enum_v<T>) {
            a_value = lexical_cast<T>(_ini.GetValue(a_section, a_key, std::to_string(a_value).c_str()));
            if constexpr(std::is_same<T, spdlog::level::level_enum>()) {
                spdlog::set_level(a_value);
                spdlog::flush_on(a_value);
            }
            ++_loadedSettings;
        } else {
            a_value = _ini.GetValue(a_section, a_key, a_value.c_str());
            ++_loadedSettings;
        }
    }

private:
    CSimpleIniA _ini;
    int         _loadedSettings{};
    int         _savedSettings{};
    const char* _settingsFile;
};

namespace ImGui {
    bool SliderFloatWithSteps(const char* label, float& v, float v_min, float v_max, float v_step);
    void HoverNote(const char* text, const char* note = "(?)");

    bool ToggleButton(const char* str_id, bool* v);

    bool InputTextRequired(const char* label, std::string* str, ImGuiInputTextFlags flags = ImGuiInputTextFlags_None);
    bool InputTextWithPaste(const char* label, std::string& text, const ImVec2& size = ImVec2(0, 0), bool multiline = false, ImGuiInputTextFlags flags = ImGuiInputTextFlags_None);
    bool InputTextWithPasteRequired(const char* label, std::string& text, const ImVec2& size = ImVec2(0, 0), bool multiline = false, ImGuiInputTextFlags flags = ImGuiInputTextFlags_None);

}  // namespace ImGui
