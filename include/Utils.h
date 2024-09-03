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
            if(form) {
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
        virtual bool Unk_01(void) { return false; }  // 01

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

/*Helper class to load from a simple ini file.*/
class settingsLoader
{
    CSimpleIniA _ini;
    int         _loadedSettings{};
    int         _savedSettings{};
    const char* _settingsFile;

public:
    explicit settingsLoader(const char* settingsFile):
        _settingsFile(settingsFile) {
        _ini.LoadFile(settingsFile);
        if(_ini.IsEmpty()) {
            logger::info("Warning: {} is empty.", settingsFile);
        }
    };

    ~    settingsLoader();
    /*Load a boolean value if present.*/
    void load(bool& settingRef, const char* key, const char* section = "UI", bool a_bDefault = false);
    /*Load a float value if present.*/
    void load(float& settingRef, const char* key, const char* section = "UI", float a_fDefault = 0.0F);
    /*Load an unsigned int value if present.*/
    void load(uint32_t& settingRef, const char* key, const char* section = "UI", uint32_t a_uDefault = 0);
    void load(spdlog::level::level_enum& settingRef, const char* key, const char* section = "UI", spdlog::level::level_enum a_uDefault = spdlog::level::info);

    void save(const bool& settingRef, const char* key, const char* section = "UI", const char* comment = nullptr);
    void save(const float& settingRef, const char* key, const char* section = "UI", const char* comment = nullptr);
    void save(const uint32_t& settingRef, const char* key, const char* section = "UI", const char* comment = nullptr);
    void save(const spdlog::level::level_enum& loglevelRef, const char* key, const char* section = "Debug", const char* comment = nullptr);

    void flush() const;

    /*Load an integer value if present.*/
    void load(int& settingRef, const char* key, const char* section = "UI");

    void log();
};

namespace ImGui {
    bool SliderFloatWithSteps(const char* label, float* v, float v_min, float v_max, float v_step);
    void HoverNote(const char* text, const char* note = "(?)");

    bool ToggleButton(const char* str_id, bool* v);

    bool InputTextRequired(const char* label, std::string* str, ImGuiInputTextFlags flags = 0);
    bool InputTextWithPaste(const char* label, std::string& text, const ImVec2& size = ImVec2(0, 0), bool multiline = false, ImGuiInputTextFlags flags = 0);
    bool InputTextWithPasteRequired(const char* label, std::string& text, const ImVec2& size = ImVec2(0, 0), bool multiline = false, ImGuiInputTextFlags flags = 0);

}  // namespace ImGui
