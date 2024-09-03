// #include <imgui_impl_dx11.h>
// #include <imgui_impl_win32.h>
#include "imgui_internal.h"
#include "imgui_stdlib.h"

#include "imgui.h"
#include "Utils.h"

namespace Utils {
    namespace imgui {
        void HoverNote(const char* text, const char* note) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
            ImGui::Text(note);
            ImGui::PopStyleColor();
            if(ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::Text(text);
                ImGui::EndTooltip();
            }
        }
    }  // namespace imgui

    std::string getFormEditorID(const RE::TESForm* a_form) {
        switch(a_form->GetFormType()) {
            case RE::FormType::Keyword:
            case RE::FormType::LocationRefType:
            case RE::FormType::Action:
            case RE::FormType::MenuIcon:
            case RE::FormType::Global:
            case RE::FormType::HeadPart:
            case RE::FormType::Race:
            case RE::FormType::Sound:
            case RE::FormType::Script:
            case RE::FormType::Navigation:
            case RE::FormType::Cell:
            case RE::FormType::WorldSpace:
            case RE::FormType::Land:
            case RE::FormType::NavMesh:
            case RE::FormType::Dialogue:
            case RE::FormType::Quest:
            case RE::FormType::Idle:
            case RE::FormType::AnimatedObject:
            case RE::FormType::ImageAdapter:
            case RE::FormType::VoiceType:
            case RE::FormType::Ragdoll:
            case RE::FormType::DefaultObject:
            case RE::FormType::MusicType:
            case RE::FormType::StoryManagerBranchNode:
            case RE::FormType::StoryManagerQuestNode:
            case RE::FormType::StoryManagerEventNode:
            case RE::FormType::SoundRecord:
                return a_form->GetFormEditorID();
            default:
            {
                static auto tweaks = GetModuleHandle(L"po3_Tweaks");
                static auto func   = reinterpret_cast<func_get_form_editor_id>(GetProcAddress(tweaks, "GetFormEditorID"));
                if(func) {
                    return func(a_form->formID);
                }
                return {};
            }
        }
    }
}  // namespace Utils

settingsLoader::~settingsLoader() {
    log();
}

/*Load a boolean value if present.*/

void settingsLoader::load(bool& settingRef, const char* key, const char* section, const bool a_bDefault) {
    if(_ini.GetValue(section, key)) {
        const bool val = _ini.GetBoolValue(section, key, a_bDefault);
        settingRef     = val;
        _loadedSettings++;
    }
}

/*Load a float value if present.*/

void settingsLoader::load(float& settingRef, const char* key, const char* section, const float a_fDefault) {
    if(_ini.GetValue(section, key)) {
        const auto val = static_cast<float>(_ini.GetDoubleValue(section, key, a_fDefault));
        settingRef     = val;
        _loadedSettings++;
    }
}

/*Load an unsigned int value if present.*/

void settingsLoader::load(uint32_t& settingRef, const char* key, const char* section, uint32_t a_uDefault) {
    if(_ini.GetValue(section, key)) {
        try {
            settingRef = std::stoul(_ini.GetValue(section, key, std::to_string(a_uDefault).c_str()));
        } catch(const std::out_of_range& e) {
            logger::warn("settingsLoader::load: {}", e.what());
            settingRef = a_uDefault;
        } catch(const std::invalid_argument& e) {
            logger::warn("settingsLoader::load: {}", e.what());
            settingRef = a_uDefault;
        }
        _loadedSettings++;
    }
}

void settingsLoader::load(spdlog::level::level_enum& settingRef, const char* key, const char* section, spdlog::level::level_enum a_uDefault) {
    if(_ini.GetValue(section, key)) {
        const auto val = static_cast<spdlog::level::level_enum>(_ini.GetLongValue(section, key, a_uDefault));
        spdlog::set_level(val);
        spdlog::flush_on(val);
        logger::trace("Changed LogLevel to {}", spdlog::level::to_short_c_str(val));
        settingRef = val;
        _loadedSettings++;
    }
}

void settingsLoader::save(const bool& settingRef, const char* key, const char* section, const char* comment) {
    if(_ini.GetValue(section, key)) {
        _ini.SetBoolValue(section, key, settingRef, comment, true);
        _savedSettings++;
    }
}

void settingsLoader::save(const float& settingRef, const char* key, const char* section, const char* comment) {
    if(_ini.GetValue(section, key)) {
        _ini.SetDoubleValue(section, key, settingRef, comment, true);
        _savedSettings++;
    }
}

void settingsLoader::save(const uint32_t& settingRef, const char* key, const char* section, const char* comment) {
    if(_ini.GetValue(section, key)) {
        _ini.SetDoubleValue(section, key, settingRef, comment, false);
        _savedSettings++;
    }
}

void settingsLoader::save(const spdlog::level::level_enum& loglevelRef, const char* key, const char* section, const char* comment) {
    if(_ini.GetValue(section, key)) {
        _ini.SetLongValue(section, key, loglevelRef, comment, false, true);
        _savedSettings++;
    }
}

/*Load an integer value if present.*/

void settingsLoader::load(int& settingRef, const char* key, const char* section) {
    if(_ini.GetValue(section, key)) {
        const int val = static_cast<int>(_ini.GetDoubleValue(section, key));
        settingRef    = val;
        _loadedSettings++;
    }
}

void settingsLoader::log() {
    logger::info("Loaded {} settings, saved {} settings from {}.", _loadedSettings, _savedSettings, _settingsFile);
}

void settingsLoader::flush() const {
    if(const auto error{ _ini.SaveFile(_settingsFile) }; error == SI_FAIL) {
        logger::info("Failed to save settings file {}", _settingsFile);
    }
}

namespace ImGui {
    bool SliderFloatWithSteps(const char* label, float* v, const float v_min, const float v_max, const float v_step) {
        char text_buf[64] = {};
        ImFormatString(text_buf, IM_ARRAYSIZE(text_buf), "%g", *v);

        // Map from [v_min,v_max] to [0,N]
        const int  countValues   = static_cast<int>((v_max - v_min) / v_step);
        int        v_i           = static_cast<int>((*v - v_min) / v_step);
        const bool value_changed = SliderInt(label, &v_i, 0, countValues, text_buf);

        // Remap from [0,N] to [v_min,v_max]
        *v = v_min + static_cast<float>(v_i) * v_step;
        return value_changed;
    }

    void HoverNote(const char* text, const char* note) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
        ImGui::Text(note);
        ImGui::PopStyleColor();
        if(ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text(text);
            ImGui::EndTooltip();
        }
    }

    bool ToggleButton(const char* str_id, bool* v) {
        bool        ret       = false;
        ImVec2      p         = ImGui::GetCursorScreenPos();
        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        const float height = ImGui::GetFrameHeight();
        const float width  = height * 1.55f;
        const float radius = height * 0.50f;

        ImGui::InvisibleButton(str_id, ImVec2(width, height));
        if(ImGui::IsItemClicked()) {
            *v  = !*v;
            ret = true;
        }

        float t = *v ? 1.0f : 0.0f;

        ImGuiContext& g = *GImGui;
        if(g.LastActiveId == g.CurrentWindow->GetID(str_id)) {
            constexpr float ANIM_SPEED = 0.08f;
            const float     t_anim     = ImSaturate(g.LastActiveIdTimer / ANIM_SPEED);
            t                          = *v ? (t_anim) : (1.0f - t_anim);
        }

        ImU32 col_bg;
        if(ImGui::IsItemHovered()) {
            col_bg = ImGui::GetColorU32(ImLerp(ImVec4(0.78f, 0.78f, 0.78f, 1.0f), ImVec4(0.64f, 0.83f, 0.34f, 1.0f), t));
        } else {
            col_bg = ImGui::GetColorU32(ImLerp(ImVec4(0.85f, 0.85f, 0.85f, 1.0f), ImVec4(0.56f, 0.83f, 0.26f, 1.0f), t));
        }

        draw_list->AddRectFilled(p, ImVec2(p.x + width, p.y + height), col_bg, height * 0.5f);
        draw_list->AddCircleFilled(ImVec2(p.x + radius + t * (width - radius * 2.0f), p.y + radius), radius - 1.5f, IM_COL32(255, 255, 255, 255));

        return ret;
    }

    bool InputTextRequired(const char* label, std::string* str, ImGuiInputTextFlags flags) {
        const bool empty = str->empty();
        if(empty) {
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(1.0f, 0.0f, 0.0f, 0.2f));
            ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(1.0f, 0.0f, 0.0f, 0.2f));
            ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(1.0f, 0.0f, 0.0f, 0.2f));
        }
        const bool ret = ImGui::InputText(label, str, flags);
        if(empty) {
            ImGui::PopStyleColor(3);
        }
        return ret;
    }

    // Callback function to handle resizing the std::string buffer
    static int InputTextCallback(ImGuiInputTextCallbackData* data) {
        auto* str = static_cast<std::string*>(data->UserData);
        if(data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
            str->resize(data->BufTextLen);
            data->Buf = &(*str)[0];
        }
        return 0;
    }

    bool InputTextWithPaste(const char* label, std::string& text, const ImVec2& size, const bool multiline, ImGuiInputTextFlags flags) {
        ImGui::PushID(&text);
        // Add the ImGuiInputTextFlags_CallbackResize flag to allow resizing the std::string buffer
        flags |= ImGuiInputTextFlags_CallbackResize;

        // Call the InputTextWithCallback function with the std::string buffer and callback function
        bool result;
        if(multiline) {
            result = ImGui::InputTextMultiline(label, &text[0], text.capacity() + 1, size, flags, InputTextCallback, &text);
        } else {
            result = ImGui::InputText(label, &text[0], text.capacity() + 1, flags, InputTextCallback, &text);
        }

        // Check if the InputText is hovered and the right mouse button is clicked
        if(ImGui::IsItemHovered() && ImGui::IsMouseClicked(1)) {
            // Set the context menu to be shown
            ImGui::OpenPopup("InputTextContextMenu");
        }

        // Display the context menu
        if(ImGui::BeginPopup("InputTextContextMenu")) {
            if(ImGui::MenuItem("Copy")) {
                // Copy the selected text to the clipboard
                if(const char* selected_text = text.c_str(); selected_text) {
                    ImGui::SetClipboardText(selected_text);
                }
            }
            if(ImGui::MenuItem("Paste")) {
                // Read the clipboard content

                if(const char* clipboard = ImGui::GetClipboardText(); clipboard) {
                    // Insert the clipboard content into the text buffer
                    text.append(clipboard);
                }
            }
            ImGui::EndPopup();
        }
        ImGui::PopID();
        return result;
    }

    bool InputTextWithPasteRequired(const char* label, std::string& text, const ImVec2& size, const bool multiline, ImGuiInputTextFlags flags) {
        ImGui::PushID(&text);
        // Add the ImGuiInputTextFlags_CallbackResize flag to allow resizing the std::string buffer
        flags |= ImGuiInputTextFlags_CallbackResize;

        // Call the InputTextWithCallback function with the std::string buffer and callback function
        bool empty = text.empty();
        if(empty) {
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(1.0f, 0.0f, 0.0f, 0.2f));
            ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(1.0f, 0.0f, 0.0f, 0.2f));
            ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(1.0f, 0.0f, 0.0f, 0.2f));
        }
        bool result;
        if(multiline) {
            result = ImGui::InputTextMultiline(label, &text[0], text.capacity() + 1, size, flags, InputTextCallback, &text);

        } else {
            result = ImGui::InputText(label, &text[0], text.capacity() + 1, flags, InputTextCallback, &text);
        }
        if(empty) {
            ImGui::PopStyleColor(3);
        }

        // Check if the InputText is hovered and the right mouse button is clicked
        if(ImGui::IsItemHovered() && ImGui::IsMouseClicked(1)) {
            // Set the context menu to be shown
            ImGui::OpenPopup("InputTextContextMenu");
        }

        // Display the context menu
        if(ImGui::BeginPopup("InputTextContextMenu")) {
            if(ImGui::MenuItem("Copy")) {
                // Copy the selected text to the clipboard
                if(const char* selected_text = text.c_str(); selected_text) {
                    ImGui::SetClipboardText(selected_text);
                }
            }
            if(ImGui::MenuItem("Paste")) {
                // Read the clipboard content

                if(const char* clipboard = ImGui::GetClipboardText(); clipboard) {
                    // Insert the clipboard content into the text buffer
                    text.append(clipboard);
                }
            }
            ImGui::EndPopup();
        }
        ImGui::PopID();
        return result;
    }
}  // namespace ImGui
