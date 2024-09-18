#include <imgui_internal.h>
#include <imgui_impl_dx11.h>
// #include <imgui_impl_win32.h>
#include <imgui_stdlib.h>

#include <SimpleIni.h>
#include "menus/ModSettings.h"

#include "Utils.h"

// #include "menus/Settings.h"

inline bool ModSettings::entry_base::Control::Req::satisfied() const {
    bool val{ false };
    switch(type) {
        case kReqType_Checkbox: {
            const auto it{ ModSettings::m_checkbox_toggle_.find(id) };
            if(it == ModSettings::m_checkbox_toggle_.end()) {
                return false;
            }
            val = it->second->value;
        } break;
        case kReqType_GameSetting: {
            const auto gsc{ RE::GameSettingCollection::GetSingleton() };
            const auto setting{ gsc->GetSetting(id.c_str()) };
            if(!setting) {
                return false;
            }
            val = setting->GetBool();
        } break;
    }
    return this->_not ? !val : val;
}

inline bool ModSettings::entry_base::Control::satisfied() const {
    return std::ranges::all_of(reqs.begin(), reqs.end(), [](const Req& r) {
        return r.satisfied();
    });
    // for(const auto& req : reqs) {
    //     if(!req.satisfied()) {
    //         return false;
    //     }
    // }
    // return true;
}

// using json = nlohmann::json;

void ModSettings::send_all_settings_update_event() {
    for(const auto& mod : mods) {
        send_settings_update_event(mod->name);
    }
}

void ModSettings::show_reload_translation_button() {
    if(ImGui::Button("Reload Translation")) {
        Translator::ReLoadTranslations();
    }
}

// not used anymore, we auto save.
void ModSettings::show_save_button() {
    const bool unsaved_changes{ !ini_dirty_mods.empty() };

    if(unsaved_changes) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2F, 0.7F, 0.3F, 1.0F));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3F, 0.8F, 0.4F, 1.0F));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4F, 0.9F, 0.5F, 1.0F));
    }

    if(ImGui::Button("Save")) {
        for(const auto& mod_setting : ini_dirty_mods) {
            flush_ini(mod_setting);
            flush_game_setting(mod_setting);
            for(const auto& callback : mod_setting->callbacks) {
                callback();
            }
            send_settings_update_event(mod_setting->name);
        }
        ini_dirty_mods.clear();
    }

    if(unsaved_changes) {
        ImGui::PopStyleColor(3);
    }
}

void ModSettings::show_cancel_button() {
    // bool unsaved_changes = !ini_dirty_mods.empty();

    if(ImGui::Button("Cancel")) {
        for(const auto& mod : ini_dirty_mods) {
            load_ini(mod);
        }
        ini_dirty_mods.clear();
    }
}

void ModSettings::show_save_json_button() {
    const bool unsaved_changes = !json_dirty_mods.empty();

    if(unsaved_changes) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2F, 0.7F, 0.3F, 1.0F));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3F, 0.8f, 0.4F, 1.0F));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4F, 0.9F, 0.5F, 1.0F));
    }

    if(ImGui::Button("Save Config")) {
        for(const auto& mod : json_dirty_mods) {
            flush_json(mod);
        }
        json_dirty_mods.clear();
    }

    if(unsaved_changes) {
        ImGui::PopStyleColor(3);
    }
}

void ModSettings::show_buttons_window() {
    const ImVec2 main_window_size{ ImGui::GetWindowSize() };
    const ImVec2 main_window_pos{ ImGui::GetWindowPos() };

    const ImVec2 buttons_window_size{ ImVec2(main_window_size.x * 0.15F, main_window_size.y * 0.1F) };

    ImGui::SetNextWindowPos(ImVec2(main_window_pos.x + main_window_size.x, main_window_pos.y + main_window_size.y - buttons_window_size.y));
    ImGui::SetNextWindowSize(buttons_window_size);  // Adjust the height as needed
    ImGui::Begin("Buttons", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
    show_save_button();
    ImGui::SameLine();
    show_cancel_button();
    if(edit_mode) {
        show_save_json_button();
    }
    ImGui::End();
}

void ModSettings::show_entry_edit(const std::shared_ptr<entry_base>& entry, const std::shared_ptr<mod_setting>& mod) {
    ImGui::PushID(entry.get());

    bool edited{ false };

    // Show input fields to edit the setting name and ini id
    if(ImGui::InputTextWithPasteRequired("Name", entry->name.def)) {
        edited = true;
    }
    if(ImGui::InputTextWithPaste("Description", entry->desc.def, ImVec2(ImGui::GetCurrentWindow()->Size.x * 0.8F, ImGui::GetTextLineHeight() * 3.0F), true, ImGuiInputTextFlags_AutoSelectAll)) {
        edited = true;
    }

    // Show fields specific to the selected setting type
    switch(const entry_type current_type{ entry->type }; current_type) {
        case kEntryType_Checkbox: {
            const auto checkbox{ std::dynamic_pointer_cast<setting_checkbox>(entry) };
            if(ImGui::Checkbox("Default", &checkbox->default_value)) {
                edited = true;
            }
            const std::string old_control_id{ checkbox->control_id };
            if(ImGui::InputTextWithPaste("Control ID", checkbox->control_id)) {  // on change of control id
                if(!old_control_id.empty()) {                                    // remove old control id
                    m_checkbox_toggle_.erase(old_control_id);
                }
                if(!checkbox->control_id.empty()) {
                    m_checkbox_toggle_[checkbox->control_id] = checkbox;  // update control id
                }
                edited = true;
            }
            break;
        }
        case kEntryType_Slider: {
            const auto slider{ std::dynamic_pointer_cast<setting_slider>(entry) };
            if(ImGui::InputFloat("Default", &slider->default_value)) {
                edited = true;
            }
            if(ImGui::InputFloat("Min", &slider->min)) {
                edited = true;
            }
            if(ImGui::InputFloat("Max", &slider->max)) {
                edited = true;
            }
            if(ImGui::InputFloat("Step", &slider->step)) {
                edited = true;
            }
            break;
        }
        case kEntryType_Textbox: {
            const auto textbox{ std::dynamic_pointer_cast<setting_textbox>(entry) };
            if(ImGui::InputTextWithPaste("Default", textbox->default_value)) {
                edited = true;
            }
            break;
        }
        case kEntryType_Dropdown: {
            const auto dropdown{ std::dynamic_pointer_cast<setting_dropdown>(entry) };
            ImGui::Text("Dropdown options");
            if(ImGui::BeginChild("##dropdown_items", ImVec2(0.0F, 200.0F), true, ImGuiWindowFlags_AlwaysAutoResize)) {
                constexpr size_t step{};
                constexpr size_t step_fast{ 100 };
                if(ImGui::InputScalar("Default", ImGuiDataType_U64, &dropdown->default_value, &step, &step_fast)) {
                    edited = true;
                }
                if(ImGui::Button("Add")) {
                    dropdown->options.emplace_back();
                    edited = true;
                }
                for(size_t i{}; i < dropdown->options.size(); i++) {
                    ImGui::PushID(static_cast<int>(i));
                    if(ImGui::InputText("Option", &dropdown->options[i])) {
                        edited = true;
                    }
                    ImGui::SameLine();
                    if(ImGui::Button("-")) {
                        dropdown->options.erase(dropdown->options.begin() + static_cast<decltype(dropdown->options)::difference_type>(i));
                        if(dropdown->value == i) {  // erased current value
                            dropdown->value = 0;    // reset to 1st value
                        }
                        edited = true;
                    }
                    ImGui::PopID();
                }
            }
            ImGui::EndChild();
            break;
        }
        case kEntryType_Text: {
            // color palette to set text color
            ImGui::Text("Text color");
            if(ImGui::BeginChild("##text_color", ImVec2(0.0F, 100.0F), true, ImGuiWindowFlags_AlwaysAutoResize)) {
                const auto text{ std::dynamic_pointer_cast<entry_text>(entry) };
                std::array colorArray{ text->_color.x, text->_color.y, text->_color.z, text->_color.w };
                if(ImGui::ColorEdit4("Color", colorArray.data())) {
                    text->_color = ImVec4(colorArray[0], colorArray[1], colorArray[2], colorArray[3]);
                    edited       = true;
                }
                ImGui::EndChild();
            }
            break;
        }
        case kEntryType_Color: {
            // color palette to set text color
            ImGui::Text("Color");
            if(ImGui::BeginChild("##color", ImVec2(0.0F, 100.0F), true, ImGuiWindowFlags_AlwaysAutoResize)) {
                const auto color{ std::dynamic_pointer_cast<setting_color>(entry) };
                std::array colorArray{ color->default_color.x, color->default_color.y, color->default_color.z, color->default_color.w };
                if(ImGui::ColorEdit4("Default Color", colorArray.data())) {
                    color->default_color = ImVec4(colorArray[0], colorArray[1], colorArray[2], colorArray[3]);
                    edited               = true;
                }
                ImGui::EndChild();
            }
            break;
        }
        case kEntryType_Keymap: {
            // color palette to set text color
            ImGui::Text("Keymap");
            if(ImGui::BeginChild("##keymap", ImVec2(0.0F, 100.0F), true, ImGuiWindowFlags_AlwaysAutoResize)) {
                const auto keymap{ std::dynamic_pointer_cast<setting_keymap>(entry) };
                if(ImGui::InputScalar("Default Key ID", ImGuiDataType_U32, &keymap->default_value)) {
                    edited = true;
                }
                ImGui::EndChild();
            }
            break;
        }
        case kEntryType_Button: {
            ImGui::Text("Button");
            if(ImGui::BeginChild("##button", ImVec2(0.0F, 100.0F), true, ImGuiWindowFlags_AlwaysAutoResize)) {
                const auto button{ std::dynamic_pointer_cast<entry_button>(entry) };
                if(ImGui::InputText("ID", &button->id)) {
                    edited = true;
                }
                ImGui::EndChild();
            }
        } break;
        default:
            break;
    }

    ImGui::Text("Control");
    // choose fail action
    constexpr std::array fail_actions{ "Disable", "Hide" };
    if(ImGui::BeginCombo("Fail Action", fail_actions[static_cast<size_t>(entry->control.failAction)])) {
        for(size_t i{}; i < fail_actions.size(); i++) {
            const bool is_selected{ static_cast<size_t>(entry->control.failAction) == i };

            if(ImGui::Selectable(fail_actions[i], is_selected)) {
                entry->control.failAction = static_cast<entry_base::Control::FailAction>(i);
            }
            if(is_selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    if(ImGui::Button("Add")) {
        entry->control.reqs.emplace_back();
        edited = true;
    }

    if(ImGui::BeginChild("##control_requirements", ImVec2(0.0F, 100.0F), true, ImGuiWindowFlags_AlwaysAutoResize)) {
        // set the number of columns to 3

        for(size_t i{}; i < entry->control.reqs.size(); i++) {
            auto& req{ entry->control.reqs[i] };  // get a reference to the requirement at index i
            ImGui::PushID(&req);
            constexpr int num_columns{ 3 };
            ImGui::Columns(num_columns, nullptr, false);
            // set the width of each column to be the same
            ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() * 0.5F);
            ImGui::SetColumnWidth(1, ImGui::GetWindowWidth() * 0.25F);
            ImGui::SetColumnWidth(2, ImGui::GetWindowWidth() * 0.25F);

            if(ImGui::InputTextWithPaste("id", req.id)) {
                edited = true;
            }

            // add the second column
            ImGui::NextColumn();
            static constexpr std::array req_types{ "Checkbox", "GameSetting" };
            if(ImGui::BeginCombo("Type", req_types[static_cast<size_t>(req.type)])) {
                for(size_t ii{}; ii < req_types.size(); ii++) {
                    const bool is_selected{ static_cast<size_t>(req.type) == ii };
                    if(ImGui::Selectable(req_types[ii], is_selected)) {
                        req.type = static_cast<entry_base::Control::Req::ReqType>(ii);
                    }
                    if(is_selected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }

            // add the third column
            ImGui::NextColumn();
            if(ImGui::Checkbox("not", &req._not)) {
                edited = true;
            }
            ImGui::SameLine();
            if(ImGui::Button("-")) {
                entry->control.reqs.erase(entry->control.reqs.begin() + static_cast<decltype(entry->control.reqs)::difference_type>(i));  // erase the requirement at index i
                edited = true;
                i--;                                                                                                                      // update the loop index to account for the erased element
            }
            ImGui::Columns(1);

            ImGui::PopID();
        }

        ImGui::EndChild();
    }

    ImGui::Text("Localization");
    if(ImGui::BeginChild((std::string(entry->name.def) + "##Localization").c_str(), ImVec2(0.0F, 100.0F), true, ImGuiWindowFlags_AlwaysAutoResize)) {
        if(ImGui::InputTextWithPaste("Name", entry->name.key)) {
            edited = true;
        }
        if(ImGui::InputTextWithPaste("Description", entry->desc.key)) {
            edited = true;
        }
        ImGui::EndChild();
    }

    if(entry->is_setting()) {
        const auto setting{ std::dynamic_pointer_cast<setting_base>(entry) };
        ImGui::Text("Serialization");
        if(ImGui::BeginChild((std::string(setting->name.def) + "##serialization").c_str(), ImVec2(0.0F, 100.0F), true, ImGuiWindowFlags_AlwaysAutoResize)) {
            if(ImGui::InputTextWithPasteRequired("ini ID", setting->ini_id)) {
                edited = true;
            }

            if(ImGui::InputTextWithPasteRequired("ini Section", setting->ini_section)) {
                edited = true;
            }

            if(ImGui::InputTextWithPaste("Game Setting", setting->gameSetting)) {
                edited = true;
            }
            if(setting->type == kEntryType_Slider) {
                if(!setting->gameSetting.empty()) {
                    switch(setting->gameSetting[0]) {
                        case 'f':
                        case 'i':
                        case 'u':
                            break;
                        default:
                            ImGui::TextColored(ImVec4(1.0F, 0.0F, 0.0F, 1.0F), "For sliders, game setting must start with f, i, or u for float, int, or uint respectively for type specification.");
                            break;
                    }
                }
            }
            ImGui::EndChild();
        }
    }
    if(edited) {
        json_dirty_mods.insert(mod);
    }
    //ImGui::EndChild();
    ImGui::PopID();
}

void ModSettings::show_entry(const std::shared_ptr<entry_base>& entry, const std::shared_ptr<mod_setting>& mod) {  // NOLINT(*-no-recursion)
    ImGui::PushID(entry.get());
    bool edited{ false };

    // check if all control requirements are met
    const bool available{ entry->control.satisfied() };
    if(!available) {
        switch(entry->control.failAction) {
            case entry_base::Control::FailAction::kFailAction_Hide:
                if(!edit_mode) {  // use disable fail action under edit mode
                    ImGui::PopID();
                    return;
                }
            default:
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5F, 0.5F, 0.5F, 1.0F));
                ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5F);
                break;
        }
    }

    const float width{ ImGui::GetContentRegionAvail().x * 0.5F };
    switch(entry->type) {
        case kEntryType_Checkbox: {
            const auto checkbox{ std::dynamic_pointer_cast<setting_checkbox>(entry) };

            if(ImGui::Checkbox(checkbox->name.get(), &checkbox->value)) {
                edited = true;
            }
            if(ImGui::IsItemHovered()) {
                if(ImGui::IsKeyPressed(ImGuiKey_R)) {
                    edited |= checkbox->reset();
                }
            }

            if(!checkbox->desc.empty()) {
                ImGui::SameLine();
                ImGui::HoverNote(checkbox->desc.get());
            }

        } break;

        case kEntryType_Slider: {
            const auto slider{ std::dynamic_pointer_cast<setting_slider>(entry) };

            // Set the width of the slider to a fraction of the available width
            ImGui::SetNextItemWidth(width);
            if(ImGui::SliderFloatWithSteps(slider->name.get(), slider->value, slider->min, slider->max, slider->step)) {
                edited = true;
            }

            if(ImGui::IsItemHovered()) {
                if(ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_LeftArrow))) {
                    if(slider->value > slider->min) {
                        slider->value -= slider->step;
                        edited         = true;
                    }
                }
                if(ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_RightArrow))) {
                    if(slider->value < slider->max) {
                        slider->value += slider->step;
                        edited         = true;
                    }
                }

                if(ImGui::IsKeyPressed(ImGuiKey_R)) {
                    edited |= slider->reset();
                }
            }

            if(!slider->desc.empty()) {
                ImGui::SameLine();
                ImGui::HoverNote(slider->desc.get());
            }
        } break;

        case kEntryType_Textbox: {
            const auto textbox{ std::dynamic_pointer_cast<setting_textbox>(entry) };

            ImGui::SetNextItemWidth(width);
            if(ImGui::InputText(textbox->name.get(), &textbox->value)) {
                edited = true;
            }

            if(!textbox->desc.empty()) {
                ImGui::SameLine();
                ImGui::HoverNote(textbox->desc.get());
            }

            if(ImGui::IsItemHovered()) {
                if(ImGui::IsKeyPressed(ImGuiKey_R)) {
                    edited |= textbox->reset();
                }
            }

        } break;

        case kEntryType_Dropdown: {
            const auto                      dropdown{ std::dynamic_pointer_cast<setting_dropdown>(entry) };
            const auto                      name{ dropdown->name.get() };
            auto                            selected{ dropdown->value };
            const std::vector<std::string>& options{ dropdown->options };
            const auto                      preview_value{ selected < options.size() ? options[selected].c_str() : "" };
            ImGui::SetNextItemWidth(width);
            if(ImGui::BeginCombo(name, preview_value)) {
                for(size_t i{}; i < options.size(); i++) {
                    const bool is_selected{ selected == i };

                    if(ImGui::Selectable(options[i].c_str(), is_selected)) {
                        selected        = i;
                        dropdown->value = selected;
                        edited          = true;
                    }

                    if(is_selected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }

                ImGui::EndCombo();
            }

            if(ImGui::IsItemHovered()) {
                if(ImGui::IsKeyPressed(ImGuiKey_R)) {
                    edited |= dropdown->reset();
                }
            }

            if(!dropdown->desc.empty()) {
                ImGui::SameLine();
                ImGui::HoverNote(dropdown->desc.get());
            }
        } break;
        case kEntryType_Text: {
            const auto t{ std::dynamic_pointer_cast<entry_text>(entry) };
            ImGui::TextColored(t->_color, t->name.get());
        } break;
        case kEntryType_Group: {
            const auto g{ std::dynamic_pointer_cast<entry_group>(entry) };
            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.0F, 0.0F, 0.0F, 0.0F));
            if(ImGui::CollapsingHeader(g->name.get())) {
                if(ImGui::IsItemHovered() && !g->desc.empty()) {
                    ImGui::SetTooltip(g->desc.get());
                }
                ImGui::PopStyleColor();
                show_entries(g->entries, mod);
            } else {
                ImGui::PopStyleColor();
            }
        } break;
        case kEntryType_Keymap: {
            const auto        k{ std::dynamic_pointer_cast<setting_keymap>(entry) };
            const std::string hash{ std::to_string(reinterpret_cast<unsigned long long>(reinterpret_cast<void**>(k.get()))) };
            if(ImGui::Button("Remap")) {
                ImGui::OpenPopup(hash.c_str());
                key_map_listening = k;
            }
            ImGui::SameLine();
            if(ImGui::Button("Unmap")) {
                k->value = 0;
                edited   = true;
            }
            ImGui::SameLine();
            ImGui::Text("%s:", k->name.get());
            ImGui::SameLine();
            ImGui::Text(setting_keymap::keyid_to_str(k->value));

            if(!k->desc.empty()) {
                ImGui::SameLine();
                ImGui::HoverNote(k->desc.get());
            }

            if(ImGui::BeginPopupModal(hash.c_str())) {
                ImGui::Text("Enter the key you wish to map");
                if(key_map_listening == nullptr) {
                    edited = true;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
        } break;
        case kEntryType_Color: {
            ImGui::SetNextItemWidth(width);
            const auto color{ std::dynamic_pointer_cast<setting_color>(entry) };
            std::array color_array{ color->color.x, color->color.y, color->color.z, color->color.w };
            if(ImGui::ColorEdit4(color->name.get(), color_array.data())) {
                color->color = ImVec4(color_array[0], color_array[1], color_array[2], color_array[3]);
                edited       = true;
            }
            if(ImGui::IsItemHovered()) {
                if(ImGui::IsKeyPressed(ImGuiKey_R)) {
                    edited |= color->reset();
                }
            }
            if(!color->desc.empty()) {
                ImGui::SameLine();
                ImGui::HoverNote(color->desc.get());
            }
        } break;
        case kEntryType_Button: {
            const auto b{ std::dynamic_pointer_cast<entry_button>(entry) };
            if(ImGui::Button(b->name.get())) {
                // trigger button callback
                constexpr auto custom_event_name{ "dmenu_buttonCallback" };
                send_mod_callback_event(custom_event_name, b->id);
            }
            if(!b->desc.empty()) {
                ImGui::SameLine();
                ImGui::HoverNote(b->desc.get());
            }
        } break;
        default:
            break;
    }
    if(!available) {  // disabled before
        ImGui::PopStyleVar();
        ImGui::PopItemFlag();
        ImGui::PopStyleColor();
    }
    if(edited) {
        ini_dirty_mods.insert(mod);
    }
    ImGui::PopID();
}

void ModSettings::show_entries(std::vector<std::shared_ptr<entry_base>>& entries, const std::shared_ptr<mod_setting>& mod) {  // NOLINT(*-no-recursion)
    ImGui::PushID(&entries);
    bool edited{ false };

    for(auto it{ entries.begin() }; it != entries.end(); ++it) {
        const auto& entry{ *it };

        ImGui::PushID(entry.get());

        ImGui::Indent();
        // edit entry
        bool entry_deleted{ false };
        bool all_entries_deleted{ false };

        if(edit_mode) {
            if(ImGui::ArrowButton("##up", ImGuiDir_Up)) {
                if(it != entries.begin()) {
                    std::iter_swap(it, it - 1);
                    edited = true;
                }
            }
            ImGui::SameLine();
            if(ImGui::ArrowButton("##down", ImGuiDir_Down)) {
                if(it != entries.end() - 1) {
                    std::iter_swap(it, it + 1);
                    edited = true;
                }
            }
            ImGui::SameLine();

            if(ImGui::Button("Edit")) {  // Get the size of the current window
                ImGui::OpenPopup("Edit Setting");
                const ImVec2 windowSize{ ImGui::GetWindowSize() };
                // Set the size of the pop-up to be proportional to the window size
                const ImVec2 popupSize(windowSize.x * 0.5F, windowSize.y * 0.5F);
                ImGui::SetNextWindowSize(popupSize);
            }
            if(ImGui::BeginPopup("Edit Setting", ImGuiWindowFlags_AlwaysAutoResize)) {
                // Get the size of the current window
                show_entry_edit(entry, mod);
                ImGui::EndPopup();
            }

            ImGui::SameLine();

            // delete  button
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0F, 0.0F, 0.0F, 1.0F));
            if(ImGui::Button("Delete")) {
                // delete entry
                ImGui::OpenPopup("Delete Confirmation");
                // move popup mousepos
                const ImVec2 mousePos{ ImGui::GetMousePos() };
                ImGui::SetNextWindowPos(mousePos);
            }
            if(ImGui::BeginPopupModal("Delete Confirmation", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::Text("Are you sure you want to delete this setting?");
                ImGui::Separator();

                if(ImGui::Button("Yes", ImVec2(120, 0))) {
                    if(entry->type == entry_type::kEntryType_Checkbox) {
                        const auto checkbox{ std::dynamic_pointer_cast<setting_checkbox>(entry) };
                        m_checkbox_toggle_.erase(checkbox->control_id);
                    }
                    const bool should_decrement{ it != entries.begin() };
                    it = entries.erase(it);
                    if(should_decrement) {
                        --it;
                    }
                    if(it == entries.end()) {
                        all_entries_deleted = true;
                    }
                    edited        = true;
                    entry_deleted = true;
                    // TODO: delete everything in a group if deleting group.
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SetItemDefaultFocus();
                ImGui::SameLine();
                if(ImGui::Button("No", ImVec2(120, 0))) {
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
            ImGui::PopStyleColor();
            ImGui::SameLine();
        }

        if(!entry_deleted) {
            show_entry(entry, mod);
        }

        ImGui::Unindent();
        ImGui::PopID();
        if(all_entries_deleted) {
            break;
        }
    }

    // add entry
    if(edit_mode) {
        if(ImGui::Button("New Entry")) {
            // correct popup position
            ImGui::SetNextWindowPos(ImGui::GetCursorScreenPos());
            ImGui::OpenPopup("Add Entry");
        }
        if(ImGui::BeginPopup("Add Entry")) {
            if(ImGui::Selectable("Checkbox")) {
                auto checkbox{ std::make_shared<setting_checkbox>() };
                entries.emplace_back(std::move(checkbox));
                edited = true;
                logger::info("added entry");
            }
            if(ImGui::Selectable("Slider")) {
                auto slider{ std::make_shared<setting_slider>() };
                entries.emplace_back(std::move(slider));
                edited = true;
                logger::info("added entry");
            }
            if(ImGui::Selectable("Textbox")) {
                auto textbox{ std::make_shared<setting_textbox>() };
                entries.emplace_back(std::move(textbox));
                edited = true;
                logger::info("added entry");
            }
            if(ImGui::Selectable("Dropdown")) {
                auto dropdown{ std::make_shared<setting_dropdown>() };
                dropdown->options.emplace_back("Option 0");
                dropdown->options.emplace_back("Option 1");
                dropdown->options.emplace_back("Option 2");
                entries.emplace_back(std::move(dropdown));
                edited = true;
                logger::info("added entry");
            }
            if(ImGui::Selectable("Keymap")) {
                auto keymap{ std::make_shared<setting_keymap>() };
                entries.emplace_back(std::move(keymap));
                edited = true;
                logger::info("added entry");
            }
            if(ImGui::Selectable("Text")) {
                auto text{ std::make_shared<entry_text>() };
                entries.emplace_back(std::move(text));
                edited = true;
                logger::info("added entry");
            }
            if(ImGui::Selectable("Group")) {
                auto group{ std::make_shared<entry_group>() };
                entries.emplace_back(std::move(group));
                edited = true;
                logger::info("added entry");
            }
            if(ImGui::Selectable("Color")) {
                auto color{ std::make_shared<setting_color>() };
                entries.emplace_back(std::move(color));
                edited = true;
                logger::info("added entry");
            }
            if(ImGui::Selectable("Button")) {
                auto button{ std::make_shared<entry_button>() };
                entries.emplace_back(std::move(button));
                edited = true;
                logger::info("added entry");
            }
            ImGui::EndPopup();
        }
    }
    if(edited) {
        json_dirty_mods.insert(mod);
    }
    ImGui::PopID();
}

inline void ModSettings::send_settings_update_event(const std::string_view modName) {
    const auto event_source{ SKSE::GetModCallbackEventSource() };
    if(!event_source) [[unlikely]] {
        return;
    }
    const SKSE::ModCallbackEvent callback_event{ "dmenu_updateSettings", modName };
    event_source->SendEvent(&callback_event);
}

void ModSettings::send_mod_callback_event(const std::string_view mod_name, const std::string_view str_arg) {
    const auto event_source{ SKSE::GetModCallbackEventSource() };
    if(!event_source) [[unlikely]] {
        return;
    }
    const SKSE::ModCallbackEvent callback_event{ mod_name.data(), str_arg.data() };
    event_source->SendEvent(&callback_event);
}

void ModSettings::show_mod_setting(const std::shared_ptr<mod_setting>& mod) {
    show_entries(mod->entries, mod);
}

void ModSettings::submit_input(const uint32_t id) {
    if(!key_map_listening) [[unlikely]] {
        return;
    }

    key_map_listening->value = id;
    key_map_listening        = nullptr;
}

inline std::string ModSettings::get_type_str(const entry_type t) {
    switch(t) {
        case entry_type::kEntryType_Checkbox:
            return "checkbox"s;
        case entry_type::kEntryType_Slider:
            return "slider"s;
        case entry_type::kEntryType_Textbox:
            return "textbox"s;
        case entry_type::kEntryType_Dropdown:
            return "dropdown"s;
        case entry_type::kEntryType_Text:
            return "text"s;
        case entry_type::kEntryType_Group:
            return "group"s;
        case entry_type::kEntryType_Color:
            return "color"s;
        case entry_type::kEntryType_Keymap:
            return "keymap"s;
        case entry_type::kEntryType_Button:
            return "button"s;
        default:
            return "invalid"s;
    }
}

void ModSettings::show() {
    // a button on the rhs of this same line
    ImGui::SameLine(ImGui::GetWindowWidth() - 100.0F);  // Move cursor to the right side of the window
    ImGui::ToggleButton("Edit Config", &edit_mode);

    // Set window padding and item spacing
    constexpr float padding{ 8.0F };
    constexpr float spacing{ 8.0F };
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(padding, padding));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(spacing, spacing));

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0F, 1.0F, 1.0F, 1.0F));
    for(const auto& mod : mods) {
        if(ImGui::CollapsingHeader(mod->name.c_str())) {
            show_mod_setting(mod);
        }
    }
    ImGui::PopStyleColor();

    ImGui::PopStyleVar(2);

    show_buttons_window();
}

static constexpr auto settings_dir{ R"(Data\SKSE\Plugins\dmenu\customSettings)" };

void ModSettings::init() {
    // Load all mods from the "Mods" directory

    logger::info("Loading .json configurations...");
    namespace fs = std::filesystem;
    for(const auto& file : fs::directory_iterator(settings_dir)) {
        if(file.path().extension() == ".json") {
            load_json(file.path());
        }
    }

    logger::info("Loading .ini config serializations...");
    // Read serialized settings from the .ini file for each mod
    for(const auto& mod : mods) {
        load_ini(mod);
        insert_game_setting(mod);
        flush_game_setting(mod);
    }
    logger::info("Mod settings initialized");
}

std::shared_ptr<ModSettings::entry_base> ModSettings::load_json_non_group(const nlohmann::json& json) {
    std::shared_ptr<ModSettings::entry_base> e;
    auto                                     type_str{ json.contains("type") ? json["type"].get<std::string>() : "" };
    auto&                                    json_default_ref{ json.contains("default") ? json["default"] : nullptr };
    auto&                                    json_style_ref{ json.contains("style") ? json["style"] : nullptr };
    if(type_str == "checkbox"s) {
        auto scb{ std::make_shared<setting_checkbox>() };
        scb->value         = json.contains("default") ? json_default_ref.get<bool>() : false;
        scb->default_value = scb->value;
        if(json.contains("control")) {
            if(auto& json_control_ref{ json["control"] }; json_control_ref.contains("id")) {
                scb->control_id                     = json_control_ref["id"].get<std::string>();
                m_checkbox_toggle_[scb->control_id] = scb;
            }
        }
        e = std::move(scb);
    } else if(type_str == "slider"s) {
        auto ssl{ std::make_shared<setting_slider>() };
        ssl->value         = json.contains("default") ? json_default_ref.get<float>() : 0;
        ssl->min           = json_style_ref["min"].get<float>();
        ssl->max           = json_style_ref["max"].get<float>();
        ssl->step          = json_style_ref["step"].get<float>();
        ssl->default_value = ssl->value;
        e                  = std::move(ssl);
    } else if(type_str == "textbox"s) {
        auto stb{ std::make_shared<setting_textbox>() };
        // size_t buf_size = json.contains("size") ? json["size"].get<size_t>() : 64;
        stb->value         = json.contains("default") ? json_default_ref.get<std::string>() : "";
        stb->default_value = stb->value;
        e                  = std::move(stb);
    } else if(type_str == "dropdown"s) {
        auto sdd{ std::make_shared<setting_dropdown>() };
        sdd->value = json.contains("default") ? json_default_ref.get<size_t>() : 0;
        for(auto& option_json : json["options"]) {
            sdd->options.push_back(option_json.get<std::string>());
        }
        sdd->default_value = sdd->value;
        e                  = std::move(sdd);
    } else if(type_str == "text"s) {
        auto et{ std::make_shared<entry_text>() };
        if(json.contains("style") && json_style_ref.contains("color")) {
            auto& json_style_color_ref{ json_style_ref["color"] };
            et->_color = ImVec4(json_style_color_ref["r"].get<float>(), json_style_color_ref["g"].get<float>(), json_style_color_ref["b"].get<float>(), json_style_color_ref["a"].get<float>());
        }
        e = std::move(et);
    } else if(type_str == "color"s) {
        auto sc{ std::make_shared<setting_color>() };
        sc->default_color = ImVec4(json_default_ref["r"].get<float>(), json_default_ref["g"].get<float>(), json_default_ref["b"].get<float>(), json_default_ref["a"].get<float>());
        sc->color         = sc->default_color;
        e                 = std::move(sc);
    } else if(type_str == "keymap"s) {
        auto skm{ std::make_shared<setting_keymap>() };
        skm->default_value = json_default_ref.get<decltype(skm->default_value)>();
        skm->value         = skm->default_value;
        e                  = std::move(skm);
    } else if(type_str == "button"s) {
        auto sb{ std::make_shared<entry_button>() };
        sb->id = json["id"].get<std::string>();
        e      = std::move(sb);
    } else [[unlikely]] {
        logger::info("Unknown setting type: {}", type_str);
        return nullptr;
    }

    if(e->is_setting()) {
        const auto s{ std::dynamic_pointer_cast<setting_base>(e) };
        if(json.contains("gameSetting")) {
            s->gameSetting = json["gameSetting"].get<std::string>();
        }
        {
            auto& json_ini_ref{ json["ini"] };
            s->ini_section = json_ini_ref["section"].get<std::string>();
            s->ini_id      = json_ini_ref["id"].get<std::string>();
        }
    }
    return e;
}

std::shared_ptr<ModSettings::entry_group> ModSettings::load_json_group(const nlohmann::json& group_json) {  // NOLINT(*-no-recursion)
    const auto group{ std::make_shared<entry_group>() };
    for(const auto& entry_json : group_json["entries"]) {
        if(const auto entry{ load_json_entry(entry_json) }; entry) {
            group->entries.emplace_back(entry);
        }
    }
    return group;
}

std::shared_ptr<ModSettings::entry_base> ModSettings::load_json_entry(const nlohmann::json& entry_json) {  // NOLINT(*-no-recursion)
    auto entry{ entry_json["type"].get<std::string>() == "group" ? load_json_group(entry_json) : load_json_non_group(entry_json) };

    if(!entry) [[unlikely]] {
        logger::error("Failed to load json entry.");
        return nullptr;
    }

    const auto& entry_json_text_ref{ entry_json["text"] };

    entry->name.def = entry_json_text_ref["name"].get<std::string>();
    if(entry_json_text_ref.contains("desc")) {
        entry->desc.def = entry_json_text_ref["desc"].get<std::string>();
    }

    if(entry_json.contains("translation")) {
        auto& entry_json_translation_ref{ entry_json["translation"] };
        if(entry_json_translation_ref.contains("name")) {
            entry->name.key = entry_json_translation_ref["name"].get<std::string>();
        }
        if(entry_json_translation_ref.contains("desc")) {
            entry->desc.key = entry_json_translation_ref["desc"].get<std::string>();
        }
    }
    entry->control.failAction = entry_base::Control::kFailAction_Disable;

    if(entry_json.contains("control")) {
        const auto& entry_json_control_ref{ entry_json["control"] };
        if(entry_json_control_ref.contains("requirements")) {
            for(const auto& req_json : entry_json_control_ref["requirements"]) {
                entry_base::Control::Req req;
                req.id   = req_json["id"].get<std::string>();
                req._not = !req_json["value"].get<bool>();
                const auto req_type{ req_json["type"].get<std::string>() };
                if(req_type == "checkbox") {
                    req.type = entry_base::Control::Req::ReqType::kReqType_Checkbox;
                } else if(req_type == "gameSetting") {
                    req.type = entry_base::Control::Req::ReqType::kReqType_GameSetting;
                } else {
                    logger::info("Error: unknown requirement type: {}", req_type);
                    continue;
                }
                entry->control.reqs.emplace_back(std::move(req));
            }
        }
        if(entry_json_control_ref.contains("failAction")) {
            const auto failAction{ entry_json_control_ref["failAction"].get<std::string>() };
            if(failAction == "disable") {
                entry->control.failAction = entry_base::Control::kFailAction_Disable;
            } else if(failAction == "hide") {
                entry->control.failAction = entry_base::Control::kFailAction_Hide;
            }
        }
    }

    return entry;
}

void ModSettings::load_json(const std::filesystem::path& a_path) {
    // Load the JSON file for this mod
    std::ifstream json_file(a_path);
    if(!json_file.is_open()) {
        // Handle error opening file
        return;
    }

    // Parse the JSON file
    nlohmann::json mod_json;
    try {
        json_file >> mod_json;
    } catch(const nlohmann::json::exception&) {
        // Handle error parsing JSON
        return;
    }
    // Create a mod_setting object to hold the settings for this mod
    auto mod{ std::make_shared<mod_setting>() };

    // name is .json's name
    mod->name      = a_path.stem().string();
    mod->json_path = std::format("{}\\{}", settings_dir, a_path.filename().string());
    try {
        if(mod_json.contains("ini")) {
            mod->ini_path = mod_json["ini"].get<std::string>();
        } else {
            mod->ini_path = std::format("{}\\ini\\{}.ini", settings_dir, mod->name);
        }

        for(const auto& entry_json : mod_json["data"]) {
            if(const auto entry{ load_json_entry(entry_json) }; entry) {
                mod->entries.emplace_back(entry);
            }
        }

        logger::info("Loaded mod {}", mod->name);
        // Add the mod to the list of mods
        mods.emplace_back(std::move(mod));
    } catch(const nlohmann::json::exception& e) {
        // Handle error parsing JSON
        logger::info("Exception parsing {} : {}", a_path.string(), e.what());
    }
}

void ModSettings::populate_non_group_json(const std::shared_ptr<entry_base>& entry, nlohmann::json& json) {
    if(!entry->is_setting()) {
        switch(entry->type) {
            case kEntryType_Text: {
                const auto& color{ std::dynamic_pointer_cast<entry_text>(entry)->_color };
                auto&       json_style_ref{ json["style"]["color"] };
                json_style_ref["r"] = color.x;
                json_style_ref["g"] = color.y;
                json_style_ref["b"] = color.z;
                json_style_ref["a"] = color.w;
            } break;
            case kEntryType_Button: {
                const auto button{ std::dynamic_pointer_cast<entry_button>(entry) };
                json["id"] = button->id;
            } break;
            default:
                break;
        }
        return;  // no need to continue, the following fields are only for settings
    }

    const auto setting{ std::dynamic_pointer_cast<setting_base>(entry) };
    auto&      json_default_ref{ json["Default"] };
    {
        auto& json_ini_ref{ json["ini"] };
        json_ini_ref["section"] = setting->ini_section;
        json_ini_ref["id"]      = setting->ini_id;
    }
    if(!setting->gameSetting.empty()) {
        json["gameSetting"] = std::dynamic_pointer_cast<setting_base>(entry)->gameSetting;
    }

    switch(setting->type) {
        case kEntryType_Checkbox: {
            const auto cb_setting{ std::dynamic_pointer_cast<setting_checkbox>(entry) };
            json_default_ref = cb_setting->default_value;
            if(!cb_setting->control_id.empty()) {
                json["control"]["id"] = cb_setting->control_id;
            }
        } break;

        case kEntryType_Slider: {
            const auto slider_setting{ std::dynamic_pointer_cast<setting_slider>(entry) };
            json_default_ref = slider_setting->default_value;
            auto& json_style_ref{ json["style"] };
            json_style_ref["min"]  = slider_setting->min;
            json_style_ref["max"]  = slider_setting->max;
            json_style_ref["step"] = slider_setting->step;
        } break;

        case kEntryType_Textbox: {
            const auto textbox_setting{ std::dynamic_pointer_cast<setting_textbox>(entry) };
            json_default_ref = textbox_setting->default_value;
        } break;

        case kEntryType_Dropdown: {
            const auto dropdown_setting{ std::dynamic_pointer_cast<setting_dropdown>(entry) };
            json_default_ref = dropdown_setting->default_value;
            for(auto& option : dropdown_setting->options) {
                json["options"].push_back(option);
            }
        } break;

        case kEntryType_Color: {
            const auto& color_setting{ std::dynamic_pointer_cast<setting_color>(entry)->default_color };
            json_default_ref["r"] = color_setting.x;
            json_default_ref["g"] = color_setting.y;
            json_default_ref["b"] = color_setting.z;
            json_default_ref["a"] = color_setting.w;
        } break;

        case kEntryType_Keymap: {
            const auto keymap_setting{ std::dynamic_pointer_cast<setting_keymap>(entry) };
            json_default_ref = keymap_setting->default_value;
        } break;

        default:
            break;
    }
}

void ModSettings::populate_group_json(const std::shared_ptr<entry_group>& group, nlohmann::json& group_json) {  // NOLINT(*-no-recursion)
    for(const auto& entry : group->entries) {
        nlohmann::json entry_json;
        populate_entry_json(entry, entry_json);
        group_json["entries"].emplace_back(std::move(entry_json));
    }
}

void ModSettings::populate_entry_json(const std::shared_ptr<entry_base>& entry, nlohmann::json& entry_json) {  // NOLINT(*-no-recursion)
    // common fields for entry
    auto& entry_json_text_ref{ entry_json["text"] };
    auto& entry_json_translation{ entry_json["translation"] };

    entry_json_text_ref["name"]    = entry->name.def;
    entry_json_text_ref["desc"]    = entry->desc.def;
    entry_json_translation["name"] = entry->name.key;
    entry_json_translation["desc"] = entry->desc.key;
    entry_json["type"]             = get_type_str(entry->type);

    auto& control_json{ entry_json["control"] };

    for(const auto& req : entry->control.reqs) {
        nlohmann::json req_json;
        std::string    req_type;
        switch(req.type) {
            case entry_base::Control::Req::kReqType_Checkbox:
                req_type = "checkbox";
                break;
            case entry_base::Control::Req::kReqType_GameSetting:
                req_type = "gameSetting";
                break;
            default:
                req_type = "ERRORTYPE";
                break;
        }
        req_json["type"]  = req_type;
        req_json["value"] = !req._not;
        req_json["id"]    = req.id;
        control_json["requirements"].emplace_back(std::move(req_json));
    }
    switch(entry->control.failAction) {
        case entry_base::Control::kFailAction_Disable:
            control_json["failAction"] = "disable";
            break;
        case entry_base::Control::kFailAction_Hide:
            control_json["failAction"] = "hide";
            break;
        default:
            control_json["failAction"] = "ERROR";
            break;
    }

    if(entry->is_group()) {
        populate_group_json(std::dynamic_pointer_cast<entry_group>(entry), entry_json);
    } else {
        populate_non_group_json(entry, entry_json);
    }
}

/* Serialize config to .json*/
void ModSettings::flush_json(const std::shared_ptr<mod_setting>& mod) {
    nlohmann::json mod_json;
    mod_json["name"] = mod->name;
    mod_json["ini"]  = mod->ini_path;

    nlohmann::json data_json;

    for(const auto& entry : mod->entries) {
        nlohmann::json entry_json;
        populate_entry_json(entry, entry_json);
        data_json.emplace_back(std::move(entry_json));
    }

    std::ofstream json_file(mod->json_path);
    if(!json_file.is_open()) {
        // Handle error opening file
        logger::info("error: failed to open {}", mod->json_path);
        return;
    }

    mod_json["data"] = std::move(data_json);

    try {
        json_file << mod_json;
    } catch(const nlohmann::json::exception& e) {
        // Handle error parsing JSON
        logger::error("Exception dumping {} : {}", mod->json_path, e.what());
    }

    insert_game_setting(mod);
    flush_game_setting(mod);

    logger::info("Saved config for {}", mod->name);
}

void ModSettings::get_all_settings(const std::shared_ptr<mod_setting>& mod, std::vector<std::shared_ptr<ModSettings::setting_base>>& r_vec) {
    std::stack<std::shared_ptr<entry_group>> group_stack;
    for(const auto& entry : mod->entries) {
        if(entry->is_group()) {
            group_stack.push(std::dynamic_pointer_cast<entry_group>(entry));
        } else if(entry->is_setting()) {
            r_vec.push_back(std::dynamic_pointer_cast<setting_base>(entry));
        }
    }
    while(!group_stack.empty()) {  // get all groups
        const auto group{ group_stack.top() };
        group_stack.pop();
        for(const auto& entry : group->entries) {
            if(entry->is_group()) {
                group_stack.push(std::dynamic_pointer_cast<entry_group>(entry));
            } else if(entry->is_setting()) {
                r_vec.push_back(std::dynamic_pointer_cast<setting_base>(entry));
            }
        }
    }
}

void ModSettings::get_all_entries(const std::shared_ptr<mod_setting>& mod, std::vector<std::shared_ptr<ModSettings::entry_base>>& r_vec) {
    std::stack<std::shared_ptr<entry_group>> group_stack;
    for(const auto& entry : mod->entries) {
        if(entry->is_group()) {
            group_stack.push(std::dynamic_pointer_cast<entry_group>(entry));
        }
        r_vec.push_back(entry);
    }
    while(!group_stack.empty()) {  // get all groups
        const auto group{ group_stack.top() };
        group_stack.pop();
        for(const auto& entry : group->entries) {
            if(entry->is_group()) {
                group_stack.push(std::dynamic_pointer_cast<entry_group>(entry));
            }
            r_vec.push_back(entry);
        }
    }
}

void ModSettings::load_ini(const std::shared_ptr<mod_setting>& mod) {
    // Create the path to the ini file for this mod
    logger::info("loading .ini for {}", mod->name);
    // Load the ini file
    CSimpleIniA ini;
    ini.SetUnicode();
    const SI_Error rc{ ini.LoadFile(mod->ini_path.c_str()) };
    if(rc != SI_OK) [[unlikely]] {
        // Handle error loading file
        logger::info(".ini file for {} not found. Creating a new .ini file.", mod->name);
        flush_ini(mod);
        return;
    }

    std::vector<std::shared_ptr<ModSettings::setting_base>> settings;
    get_all_settings(mod, settings);
    // Iterate through each setting in the group
    for(const auto& setting_ptr : settings) {
        if(setting_ptr->ini_id.empty() || setting_ptr->ini_section.empty()) {
            logger::error("Undefined .ini serialization for setting {}; failed to load value.", setting_ptr->name.def);
            continue;
        }
        // Get the value of this setting from the ini file
        std::string value{};
        bool        use_default{ false };
        if(ini.KeyExists(setting_ptr->ini_section.c_str(), setting_ptr->ini_id.c_str())) {
            value = ini.GetValue(setting_ptr->ini_section.c_str(), setting_ptr->ini_id.c_str(), "");
        } else {
            logger::info("Value not found for setting {} in .ini file. Using default entry value.", setting_ptr->name.def);
            use_default = true;
        }
        if(use_default) {
            // Convert the value to the appropriate type and assign it to the setting
            switch(setting_ptr->type) {
                case kEntryType_Checkbox: {
                    const auto settings_checkbox_ptr{ std::dynamic_pointer_cast<setting_checkbox>(setting_ptr) };
                    settings_checkbox_ptr->value = settings_checkbox_ptr->default_value;
                } break;

                case kEntryType_Slider: {
                    const auto settings_slider_ptr{ std::dynamic_pointer_cast<setting_slider>(setting_ptr) };
                    settings_slider_ptr->value = settings_slider_ptr->default_value;
                } break;

                case kEntryType_Textbox: {
                    const auto settings_textbox_ptr{ std::dynamic_pointer_cast<setting_textbox>(setting_ptr) };
                    settings_textbox_ptr->value = settings_textbox_ptr->default_value;
                } break;

                case kEntryType_Dropdown: {
                    const auto settings_dropdown_ptr{ std::dynamic_pointer_cast<setting_dropdown>(setting_ptr) };
                    settings_dropdown_ptr->value = settings_dropdown_ptr->default_value;
                } break;

                case kEntryType_Color: {
                    const auto  setting_color_ptr{ std::dynamic_pointer_cast<setting_color>(setting_ptr) };
                    auto&       setting_color_ref{ setting_color_ptr->color };
                    const auto& setting_default_color_ref{ setting_color_ptr->default_color };

                    setting_color_ref.x = setting_default_color_ref.x;
                    setting_color_ref.y = setting_default_color_ref.y;
                    setting_color_ref.z = setting_default_color_ref.z;
                    setting_color_ref.w = setting_default_color_ref.w;
                } break;

                case kEntryType_Keymap: {
                    const auto settings_keymap_ptr{ std::dynamic_pointer_cast<setting_keymap>(setting_ptr) };
                    settings_keymap_ptr->value = settings_keymap_ptr->default_value;
                } break;

                default:
                    break;
            }
        } else {
            try {
                // Convert the value to the appropriate type and assign it to the setting
                switch(setting_ptr->type) {
                    case kEntryType_Checkbox: {
                        std::dynamic_pointer_cast<setting_checkbox>(setting_ptr)->value = (value == "true");
                    } break;

                    case kEntryType_Slider: {
                        std::dynamic_pointer_cast<setting_slider>(setting_ptr)->value = std::stof(value);

                    } break;

                    case kEntryType_Textbox: {
                        std::dynamic_pointer_cast<setting_textbox>(setting_ptr)->value = value;
                    } break;

                    case kEntryType_Dropdown: {
                        std::dynamic_pointer_cast<setting_dropdown>(setting_ptr)->value = std::stoi(value);
                    } break;

                    case kEntryType_Color: {
                        const auto colUInt{ std::stoul(value) };
                        auto&      setting_color_ref{ std::dynamic_pointer_cast<setting_color>(setting_ptr)->color };
                        setting_color_ref.x = static_cast<float>(colUInt >> IM_COL32_R_SHIFT & 0xFF) / 255.0F;
                        setting_color_ref.y = static_cast<float>(colUInt >> IM_COL32_G_SHIFT & 0xFF) / 255.0F;
                        setting_color_ref.z = static_cast<float>(colUInt >> IM_COL32_B_SHIFT & 0xFF) / 255.0F;
                        setting_color_ref.w = static_cast<float>(colUInt >> IM_COL32_A_SHIFT & 0xFF) / 255.0F;
                    } break;

                    case kEntryType_Keymap: {
                        std::dynamic_pointer_cast<setting_keymap>(setting_ptr)->value = std::stoi(value);
                    } break;

                    default:
                        break;
                }
            } catch(const std::invalid_argument& e) {
                logger::error("Invalid input value: {}", e.what());
            } catch(const std::out_of_range& e) {
                logger::error("Invalid input value: {}", e.what());
            }
        }
    }
    logger::info(".ini loaded.");
}

/* Flush changes to MOD into its .ini file*/
void ModSettings::flush_ini(const std::shared_ptr<mod_setting>& mod) {
    // Create a SimpleIni object to write to the ini file
    CSimpleIniA ini;
    ini.SetUnicode();

    std::vector<std::shared_ptr<ModSettings::setting_base>> settings;
    get_all_settings(mod, settings);
    for(const auto& setting : settings) {
        if(setting->ini_id.empty() || setting->ini_section.empty()) {
            logger::error("Undefined .ini serialization for setting {}; failed to save value.", setting->name.def);
            continue;
        }
        // Get the value of this setting from the ini file
        std::string value{};
        switch(setting->type) {
            case kEntryType_Checkbox: {
                value = std::dynamic_pointer_cast<setting_checkbox>(setting)->value ? "true" : "false";
            } break;

            case kEntryType_Slider: {
                value = std::to_string(std::dynamic_pointer_cast<setting_slider>(setting)->value);
            } break;

            case kEntryType_Textbox: {
                value = std::dynamic_pointer_cast<setting_textbox>(setting)->value;
            } break;

            case kEntryType_Dropdown: {
                value = std::to_string(std::dynamic_pointer_cast<setting_dropdown>(setting)->value);
            } break;

            case kEntryType_Color: {
                // from imvec4 float to imu32
                const auto& sc{ std::dynamic_pointer_cast<setting_color>(setting)->color };
                // Step 1: Extract components
                const ImU32 col{ IM_COL32(sc.x * 255.0F, sc.y * 255.0F, sc.z * 255.0F, sc.w * 255.0F) };
                value = std::to_string(col);
            } break;

            case kEntryType_Keymap: {
                value = std::to_string(std::dynamic_pointer_cast<setting_keymap>(setting)->value);
            } break;

            default:
                break;
        }
        ini.SetValue(setting->ini_section.c_str(), setting->ini_id.c_str(), value.c_str());
    }
    // Save the ini file
    if(ini.SaveFile(mod->ini_path.c_str()) != SI_OK) {
        logger::error("Failed to save .ini file for {} in .ini file.", mod->name);
    }
}

/* Flush changes to MOD's settings to game settings.*/
void ModSettings::flush_game_setting(const std::shared_ptr<mod_setting>& mod) {
    std::vector<std::shared_ptr<ModSettings::setting_base>> settings;
    get_all_settings(mod, settings);
    const auto gsc{ RE::GameSettingCollection::GetSingleton() };
    if(!gsc) [[unlikely]] {
        logger::info("Game setting collection not found when trying to save game setting.");
        return;
    }
    for(const auto& setting : settings) {
        if(setting->gameSetting.empty()) {
            continue;
        }
        auto* s{ gsc->GetSetting(setting->gameSetting.c_str()) };
        if(!s) [[unlikely]] {
            logger::info("Error: Game setting not found when trying to save game setting {} for setting {}", setting->gameSetting, setting->name.def);
            return;
        }
        switch(setting->type) {
            case kEntryType_Checkbox: {
                s->data.b = std::dynamic_pointer_cast<setting_checkbox>(setting)->value;
            } break;
            case kEntryType_Slider: {
                const float val{ std::dynamic_pointer_cast<setting_slider>(setting)->value };
                switch(s->GetType()) {
                    case RE::Setting::Type::kUnsignedInteger:
                        s->data.u = static_cast<uint32_t>(val);
                        break;
                    case RE::Setting::Type::kSignedInteger:
                        s->data.i = (static_cast<std::int32_t>(val));
                        break;
                    case RE::Setting::Type::kFloat:
                        s->data.f = val;
                        break;
                    default:
                        logger::error("Game setting variable for slider has bad type prefix. Prefix it with f(float), i(integer), or u(unsigned integer) to specify the game setting type.");
                        break;
                }
            } break;
            case kEntryType_Textbox: {
                s->data.s = (std::dynamic_pointer_cast<setting_textbox>(setting)->value.data());
            } break;
            case kEntryType_Dropdown: {
                //s->SetUnsignedInteger(dynamic_cast<setting_dropdown*>(setting_ptr)->value);
                s->data.i = static_cast<int32_t>(std::dynamic_pointer_cast<setting_dropdown>(setting)->value);
            } break;
            case kEntryType_Keymap: {
                s->data.i = static_cast<int32_t>(std::dynamic_pointer_cast<setting_keymap>(setting)->value);
            } break;
            case kEntryType_Color: {
                const auto& sc{ std::dynamic_pointer_cast<setting_color>(setting)->color };
                const ImU32 col{ IM_COL32(sc.x * 255.0F, sc.y * 255.0F, sc.z * 255.0F, sc.w * 255.0F) };
                s->data.u = col;
            } break;

            default:
                break;
        }
    }
}

void ModSettings::insert_game_setting(const std::shared_ptr<mod_setting>& mod) {
    std::vector<std::shared_ptr<ModSettings::entry_base>> entries;
    get_all_entries(mod, entries);  // must get all entries to inject settings for the entry's control requirements
    const auto gsc{ RE::GameSettingCollection::GetSingleton() };
    if(!gsc) [[unlikely]] {
        logger::info("Game setting collection not found when trying to insert game setting.");
        return;
    }
    for(const auto& entry : entries) {
        if(entry->is_setting()) {
            const auto es{ std::dynamic_pointer_cast<ModSettings::setting_base>(entry) };
            // insert setting setting
            if(!es->gameSetting.empty()) {                      // gamesetting mapping
                if(gsc->GetSetting(es->gameSetting.c_str())) {  // setting already exists
                    return;
                }

                RE::Setting* s = Utils::MakeSetting(es->gameSetting.data());
                gsc->InsertSetting(s);
                if(!gsc->GetSetting(es->gameSetting.c_str())) {
                    logger::info("logger::error: Failed to insert game setting.");
                }
            }
        }

        // inject setting for setting's req
        for(auto& req : entry->control.reqs) {
            if(req.type == entry_base::Control::Req::kReqType_GameSetting) {
                if(!gsc->GetSetting(req.id.c_str())) {
                    RE::Setting* s{ Utils::MakeSetting(req.id.data()) };
                    gsc->InsertSetting(s);
                }
            }
        }
    }
}

void ModSettings::save_all_game_setting() {
    for(const auto& mod : mods) {
        flush_game_setting(mod);
    }
}

void ModSettings::insert_all_game_setting() {
    for(const auto& mod : mods) {
        insert_game_setting(mod);
    }
}

//
//bool ModSettings::API_RegisterForSettingUpdate(std::string a_mod, std::function<void()> a_callback)
//{
//  logger::info("Received registration for {} update.", a_mod);
//  for (auto mod : mods) {
//      if (mod->name == a_mod) {
//          mod->callbacks.push_back(a_callback);
//          logger::info("Successfully added callback for {} update.", a_mod);
//          return true;
//      }
//  }
//  logger::error("{} mod not found.", a_mod);
//  return false;
//}
//
//
//
//namespace Example
//{
//  bool RegisterForSettingUpdate(std::string a_mod, std::function<void()> a_callback)
//  {
//      static auto dMenu = GetModuleHandle("dmenu");
//      using _RegisterForSettingUpdate = bool (*)(std::string, std::function<void()>);
//      static auto func = reinterpret_cast<_RegisterForSettingUpdate>(GetProcAddress(dMenu, "RegisterForSettingUpdate"));
//      if (func) {
//          return func(a_mod, a_callback);
//      }
//      return false;
//  }
//}
//
//extern "C" DLLEXPORT bool RegisterForSettingUpdate(std::string a_mod, std::function<void()> a_callback)
//{
//  return ModSettings::API_RegisterForSettingUpdate(a_mod, a_callback);
//}

const char* ModSettings::setting_keymap::keyid_to_str(uint32_t key_id) {
    switch(key_id) {
        case 0:
            return "Unmapped";
        case 1:
            return "Escape";
        case 2:
            return "1";
        case 3:
            return "2";
        case 4:
            return "3";
        case 5:
            return "4";
        case 6:
            return "5";
        case 7:
            return "6";
        case 8:
            return "7";
        case 9:
            return "8";
        case 10:
            return "9";
        case 11:
            return "0";
        case 12:
            return "Minus";
        case 13:
            return "Equals";
        case 14:
            return "Backspace";
        case 15:
            return "Tab";
        case 16:
            return "Q";
        case 17:
            return "W";
        case 18:
            return "E";
        case 19:
            return "R";
        case 20:
            return "T";
        case 21:
            return "Y";
        case 22:
            return "U";
        case 23:
            return "I";
        case 24:
            return "O";
        case 25:
            return "P";
        case 26:
            return "Left Bracket";
        case 27:
            return "Right Bracket";
        case 28:
            return "Enter";
        case 29:
            return "Left Control";
        case 30:
            return "A";
        case 31:
            return "S";
        case 32:
            return "D";
        case 33:
            return "F";
        case 34:
            return "G";
        case 35:
            return "H";
        case 36:
            return "J";
        case 37:
            return "K";
        case 38:
            return "L";
        case 39:
            return "Semicolon";
        case 40:
            return "Apostrophe";
        case 41:
            return "~ (Console)";
        case 42:
            return "Left Shift";
        case 43:
            return "Back Slash";
        case 44:
            return "Z";
        case 45:
            return "X";
        case 46:
            return "C";
        case 47:
            return "V";
        case 48:
            return "B";
        case 49:
            return "N";
        case 50:
            return "M";
        case 51:
            return "Comma";
        case 52:
            return "Period";
        case 53:
            return "Forward Slash";
        case 54:
            return "Right Shift";
        case 55:
            return "NUM*";
        case 56:
            return "Left Alt";
        case 57:
            return "Spacebar";
        case 58:
            return "Caps Lock";
        case 59:
            return "F1";
        case 60:
            return "F2";
        case 61:
            return "F3";
        case 62:
            return "F4";
        case 63:
            return "F5";
        case 64:
            return "F6";
        case 65:
            return "F7";
        case 66:
            return "F8";
        case 67:
            return "F9";
        case 68:
            return "F10";
        case 69:
            return "Num Lock";
        case 70:
            return "Scroll Lock";
        case 71:
            return "NUM7";
        case 72:
            return "NUM8";
        case 73:
            return "NUM9";
        case 74:
            return "NUM-";
        case 75:
            return "NUM4";
        case 76:
            return "NUM5";
        case 77:
            return "NUM6";
        case 78:
            return "NUM+";
        case 79:
            return "NUM1";
        case 80:
            return "NUM2";
        case 81:
            return "NUM3";
        case 82:
            return "NUM0";
        case 83:
            return "NUM.";
        case 87:
            return "F11";
        case 88:
            return "F12";
        case 156:
            return "NUM Enter";
        case 157:
            return "Right Control";
        case 181:
            return "NUM/";
        case 183:
            return "SysRq / PtrScr";
        case 184:
            return "Right Alt";
        case 197:
            return "Pause";
        case 199:
            return "Home";
        case 200:
            return "Up Arrow";
        case 201:
            return "PgUp";
        case 203:
            return "Left Arrow";
        case 205:
            return "Right Arrow";
        case 207:
            return "End";
        case 208:
            return "Down Arrow";
        case 209:
            return "PgDown";
        case 210:
            return "Insert";
        case 211:
            return "Delete";
        case 256:
            return "Left Mouse Button";
        case 257:
            return "Right Mouse Button";
        case 258:
            return "Middle/Wheel Mouse Button";
        case 259:
            return "Mouse Button 3";
        case 260:
            return "Mouse Button 4";
        case 261:
            return "Mouse Button 5";
        case 262:
            return "Mouse Button 6";
        case 263:
            return "Mouse Button 7";
        case 264:
            return "Mouse Wheel Up";
        case 265:
            return "Mouse Wheel Down";
        case 266:
            return "DPAD_UP";
        case 267:
            return "DPAD_DOWN";
        case 268:
            return "DPAD_LEFT";
        case 269:
            return "DPAD_RIGHT";
        case 270:
            return "START";
        case 271:
            return "BACK";
        case 272:
            return "LEFT_THUMB";
        case 273:
            return "RIGHT_THUMB";
        case 274:
            return "LEFT_SHOULDER";
        case 275:
            return "RIGHT_SHOULDER";
        case 276:
            return "A";
        case 277:
            return "B";
        case 278:
            return "X";
        case 279:
            return "Y";
        case 280:
            return "LT";
        case 281:
            return "RT";
        default:
            return "Unknown Key";
    }
}
