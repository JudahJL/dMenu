#pragma once

#include "menus/Translator.h"
#include <imgui.h>
#include <nlohmann/json.hpp>

class ModSettings
{
    struct setting_base;
    struct setting_checkbox;
    struct setting_slider;
    struct setting_keymap;

    // checkboxes' control key -> checkbox
    static inline std::unordered_map<std::string, std::shared_ptr<setting_checkbox>> m_checkbox_toggle_;

public:
    static inline std::shared_ptr<setting_keymap> key_map_listening;
    static void                                   submit_input(uint32_t id);

    enum entry_type {
        kEntryType_Checkbox,
        kEntryType_Slider,
        kEntryType_Textbox,
        kEntryType_Dropdown,
        kEntryType_Text,
        kEntryType_Group,
        kEntryType_Keymap,
        kEntryType_Color,
        kEntryType_Button,
        kSettingType_Invalid
    };

    static std::string get_type_str(entry_type t);

    struct entry_base {
        struct Control {
            struct Req {
                enum ReqType {
                    kReqType_Checkbox,
                    kReqType_GameSetting
                };

                 Req() = default;
                ~Req() = default;

                ReqType            type{ kReqType_Checkbox };
                std::string        id{ "New Requirement" };
                bool               _not{ false };  // the requirement needs to be off for satisfied() to return true
                [[nodiscard]] bool satisfied() const;
            };

            enum FailAction {
                kFailAction_Disable,
                kFailAction_Hide,
            };

             Control() = default;
            ~Control() = default;

            FailAction         failAction{ kFailAction_Disable };
            std::vector<Req>   reqs;
            [[nodiscard]] bool satisfied() const;
        };

        entry_type   type{ kSettingType_Invalid };
        Translatable name;
        Translatable desc;
        Control      control;

        [[nodiscard]] constexpr virtual bool is_setting() const { return false; }

        [[nodiscard]] constexpr virtual bool is_group() const { return false; }

        entry_base() = default;

        entry_base(const entry_type type, const char* name):
            type(type), name(Translatable(name)) { }

        virtual ~entry_base() = default;
    };

    struct entry_text: entry_base {
        ImVec4 _color{ 1.0F, 1.0F, 1.0F, 1.0F };

        entry_text():
            entry_base(kEntryType_Text, "New Text") { }

        ~entry_text() override = default;
    };

    struct entry_group: entry_base {
        std::vector<std::shared_ptr<entry_base>> entries;

        entry_group():
            entry_base(kEntryType_Group, "New Group") { }

        ~entry_group() override = default;

        [[nodiscard]] constexpr bool is_group() const override { return true; }
    };

    struct setting_base: entry_base {
        std::string ini_section;
        std::string ini_id;

        std::string gameSetting;

        [[nodiscard]] constexpr bool is_setting() const override { return true; }

        setting_base() = default;

        setting_base(const entry_type a_type, const char* a_name) {
            this->type = a_type;
            this->name = Translatable(a_name);
        };

        ~setting_base() override = default;

        virtual bool reset() { return false; };
    };

    struct setting_checkbox: setting_base {
        setting_checkbox():
            setting_base(kEntryType_Checkbox, "New Checkbox") { }

        ~setting_checkbox() override = default;

        bool        value{ true };
        bool        default_value{ true };
        std::string control_id;

        bool reset() override {
            const bool changed = value != default_value;
            value              = default_value;
            return changed;
        }
    };

    struct setting_slider: setting_base {
        setting_slider():
            setting_base(kEntryType_Slider, "New Slider") { }

        ~setting_slider() override = default;

        float   value{};
        float   min{};
        float   max{ 1.0F };
        float   step{ 0.1F };
        float   default_value{};
        uint8_t precision{ 2 };  // number of decimal places

        bool reset() override {
            const bool changed = value != default_value;
            value              = default_value;
            return changed;
        }
    };

    struct setting_textbox: setting_base {
        setting_textbox():
            setting_base(kEntryType_Textbox, "New Textbox") {
        }

        ~setting_textbox() override = default;

        std::string value{};
        char*       buf{};
        std::string default_value{};

        bool reset() override {
            const bool changed = value != default_value;
            value              = default_value;
            return changed;
        }
    };

    struct setting_dropdown: setting_base {
        setting_dropdown():
            setting_base(kEntryType_Dropdown, "New Dropdown") { }

        ~setting_dropdown() override = default;

        std::vector<std::string> options;
        size_t                   value{ 0 };  // index into options
        size_t                   default_value{ 0 };

        bool reset() override {
            const bool changed{ value != default_value };
            value = default_value;
            return changed;
        }
    };

    struct setting_color: setting_base {
        setting_color():
            setting_base(kEntryType_Color, "New Color") { }

        ~setting_color() override = default;

        bool reset() override {
            const bool changed{ color.x != default_color.x || color.y != default_color.y || color.z != default_color.z || color.w != default_color.w };
            color = default_color;
            return changed;
        }

        ImVec4 color{ 0.0F, 0.0F, 0.0F, 1.0F };
        ImVec4 default_color;
    };

    struct setting_keymap: setting_base {
        setting_keymap():
            setting_base(kEntryType_Keymap, "New Keymap") { }

        ~setting_keymap() override = default;

        uint32_t           value{ 0 };
        uint32_t           default_value{ 0 };
        static const char* keyid_to_str(uint32_t key_id);
    };

    struct entry_button: entry_base {
        entry_button():
            entry_base(kEntryType_Button, "New Button") { }

        ~entry_button() override = default;

        std::string id;

        [[nodiscard]] constexpr bool is_setting() const override {
            return false;
        }
    };

    /* Settings of one mod, represented by one .json file and serialized to one .ini file.*/
    struct mod_setting {
        std::string                              name;
        std::vector<std::shared_ptr<entry_base>> entries;
        std::string                              ini_path;
        std::string                              json_path;

        std::vector<std::function<void()>> callbacks;
    };

    /* Settings of all mods*/
    static inline std::vector<std::shared_ptr<mod_setting>> mods;

    static inline std::unordered_set<std::shared_ptr<mod_setting>> json_dirty_mods;  // mods whose changes need to be flushed to .json file. i.e. author has changed its setting
    static inline std::unordered_set<std::shared_ptr<mod_setting>> ini_dirty_mods;   // mods whose changes need to be flushed to .ini or gamesetting. i.e.  user has changed its setting

    static void show();                                                              // called by imgui per tick

    /* Load settings config from .json files and saved settings from .ini files*/
    static void init();

private:
    /* Load a single mod from .json file*/

    /* Read everything in group_json and populate entries*/
    static std::shared_ptr<entry_base>  load_json_non_group(const nlohmann::json& json);
    static std::shared_ptr<entry_group> load_json_group(const nlohmann::json& group_json);
    static std::shared_ptr<entry_base>  load_json_entry(const nlohmann::json& json);
    static void                         load_json(const std::filesystem::path& a_path);

    static void populate_non_group_json(const std::shared_ptr<entry_base>& group, nlohmann::json& group_json);
    static void populate_group_json(const std::shared_ptr<entry_group>& group, nlohmann::json& group_json);
    static void populate_entry_json(const std::shared_ptr<entry_base>& entry, nlohmann::json& entry_json);

    static void flush_json(const std::shared_ptr<mod_setting>& mod);

    static void get_all_settings(const std::shared_ptr<mod_setting>& mod, std::vector<std::shared_ptr<ModSettings::setting_base>>& r_vec);
    static void get_all_entries(const std::shared_ptr<mod_setting>& mod, std::vector<std::shared_ptr<ModSettings::entry_base>>& r_vec);

    static void load_ini(const std::shared_ptr<mod_setting>& mod);
    static void flush_ini(const std::shared_ptr<mod_setting>& mod);

    static void flush_game_setting(const std::shared_ptr<mod_setting>& mod);

    static void insert_game_setting(const std::shared_ptr<mod_setting>& mod);

public:
    static void save_all_game_setting();
    static void insert_all_game_setting();
    static bool api_register_for_setting_update(std::string a_mod, std::function<void()> a_callback) = delete;

    static void send_all_settings_update_event();

private:
    static void show_reload_translation_button();
    static void show_save_button();
    static void show_cancel_button();
    static void show_save_json_button();

    static void show_buttons_window();

    static void show_mod_setting(const std::shared_ptr<mod_setting>& mod);
    static void show_entry_edit(const std::shared_ptr<entry_base>& base, const std::shared_ptr<mod_setting>& mod);
    static void show_entry(const std::shared_ptr<entry_base>& base, const std::shared_ptr<mod_setting>& mod);
    static void show_entries(std::vector<std::shared_ptr<entry_base>>& entries, const std::shared_ptr<mod_setting>& mod);

    static void send_settings_update_event(std::string_view modName);
    static void send_mod_callback_event(std::string_view mod_name, std::string_view str_arg);

    static inline bool edit_mode{ false };
};
