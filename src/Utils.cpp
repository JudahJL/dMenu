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
            default: {
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

namespace ImGui {
    bool SliderFloatWithSteps(const char* label, float& v, const float v_min, const float v_max, const float v_step) {
        char text_buf[64]{};
        ImFormatString(text_buf, std::size(text_buf), "%g", v);

        // Map from [v_min,v_max] to [0,N]
        const int  countValues   = static_cast<int>((v_max - v_min) / v_step);
        int        v_i           = static_cast<int>((v - v_min) / v_step);
        const bool value_changed = SliderInt(label, &v_i, 0, countValues, text_buf);

        // Remap from [0,N] to [v_min,v_max]
        v = v_min + static_cast<float>(v_i) * v_step;
        return value_changed;
    }

    void HoverNote(const char* text, const char* note) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5F, 0.5F, 0.5F, 1.0F));
        ImGui::Text(note);
        ImGui::PopStyleColor();
        if(ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text(text);
            ImGui::EndTooltip();
        }
    }

    bool ToggleButton(const char* str_id, bool* v) {
        bool         ret{ false };
        const ImVec2 p{ ImGui::GetCursorScreenPos() };
        ImDrawList*  draw_list{ ImGui::GetWindowDrawList() };

        const float height{ ImGui::GetFrameHeight() };
        const float width{ height * 1.55F };
        const float radius{ height * 0.50F };

        ImGui::InvisibleButton(str_id, ImVec2(width, height));
        if(ImGui::IsItemClicked()) {
            *v  = !*v;
            ret = true;
        }

        float t{ *v ? 1.0F : 0.0F };

        ImGuiContext& g{ *GImGui };
        if(g.LastActiveId == g.CurrentWindow->GetID(str_id)) {
            constexpr float ANIM_SPEED{ 0.08F };
            const float     t_anim{ ImSaturate(g.LastActiveIdTimer / ANIM_SPEED) };
            t = *v ? (t_anim) : (1.0F - t_anim);
        }

        const ImU32 col_bg = ImGui::IsItemHovered() ?
                           ImGui::GetColorU32(ImLerp(ImVec4(0.78F, 0.78F, 0.78F, 1.0F), ImVec4(0.64F, 0.83F, 0.34F, 1.0F), t)) :
                           ImGui::GetColorU32(ImLerp(ImVec4(0.85F, 0.85F, 0.85F, 1.0F), ImVec4(0.56F, 0.83F, 0.26F, 1.0F), t));

        draw_list->AddRectFilled(p, ImVec2(p.x + width, p.y + height), col_bg, height * 0.5F);
        draw_list->AddCircleFilled(ImVec2(p.x + radius + t * (width - radius * 2.0F), p.y + radius), radius - 1.5F, IM_COL32(255, 255, 255, 255));

        return ret;
    }

    bool InputTextRequired(const char* label, std::string* str, ImGuiInputTextFlags flags) {
        const bool empty = str->empty();
        if(empty) {
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(1.0F, 0.0F, 0.0F, 0.2F));
            ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(1.0F, 0.0F, 0.0F, 0.2F));
            ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(1.0F, 0.0F, 0.0F, 0.2F));
        }
        const bool ret = ImGui::InputText(label, str, flags);
        if(empty) {
            ImGui::PopStyleColor(3);
        }
        return ret;
    }

    // Callback function to handle resizing the std::string buffer
    static int InputTextCallback(ImGuiInputTextCallbackData* data) {
        auto* str{ static_cast<std::string*>(data->UserData) };
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
        const bool result{ (multiline) ?
                                ImGui::InputTextMultiline(label, &text[0], text.capacity() + 1, size, flags, InputTextCallback, &text) :
                                ImGui::InputText(label, &text[0], text.capacity() + 1, flags, InputTextCallback, &text) };

        // Check if the InputText is hovered and the right mouse button is clicked
        if(ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
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
        bool empty{ text.empty() };
        if(empty) {
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(1.0F, 0.0F, 0.0F, 0.2F));
            ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(1.0F, 0.0F, 0.0F, 0.2F));
            ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(1.0F, 0.0F, 0.0F, 0.2F));
        }
        const bool result{ multiline ?
            ImGui::InputTextMultiline(label, &text[0], text.capacity() + 1, size, flags, InputTextCallback, &text) :
            ImGui::InputText(label, &text[0], text.capacity() + 1, flags, InputTextCallback, &text) };

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

settingsLoader::settingsLoader(const char* settingsFile):
    _settingsFile(settingsFile) {
    _ini.SetUnicode();

    _ini.LoadFile(settingsFile);
    if(_ini.IsEmpty()) [[unlikely]] {
        logger::info("Warning: {} is empty.", settingsFile);
    }
}

settingsLoader::~settingsLoader() {
    if(_savedSettings > 0) [[likely]] {
        // Save the INI file and check the result
        const auto result = _ini.SaveFile(_settingsFile);

        if(result == SI_FAIL) [[unlikely]] {
            logger::error("Failed to save settings file: {}", _settingsFile);
        } else [[likely]] {
            logger::info("Successfully saved settings file: {}", _settingsFile);
        }
    }
    logger::info("Loaded {} settings, saved {} settings from {}.", _loadedSettings, _savedSettings, _settingsFile);
}
