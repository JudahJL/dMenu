#include "imgui.h"
// #include "imgui_internal.h"
// #include <imgui_impl_dx11.h>
// #include <imgui_impl_win32.h>

#include "Utils.h"
#include "menus/Settings.h"

constexpr auto SETTINGS_FILE_PATH{ R"(Data\SKSE\Plugins\dmenu\dmenu.ini)" };

namespace ini {
    void update() {
        constexpr auto section{ "UI" };
        settingsLoader loader(SETTINGS_FILE_PATH);
        loader.update(Settings::relative_window_size_h, section, "relative_window_size_h", nullptr);
        loader.update(Settings::relative_window_size_v, section, "relative_window_size_v", nullptr);
        loader.update(Settings::windowPos_x, section, "windowPos_x", nullptr);
        loader.update(Settings::windowPos_y, section, "windowPos_y", nullptr);
        loader.update(Settings::lockWindowSize, section, "lockWindowSize", nullptr);
        loader.update(Settings::lockWindowPos, section, "lockWindowPos", nullptr);
        loader.update(Settings::key_toggle_dmenu, section, "key_toggle_dmenu", nullptr);
        loader.update(Settings::fontScale, section, "fontScale", nullptr);
        loader.update(Settings::log_level, "LogLevel", "Level", ";Sets the level of detail in the log: trace, debug, info, warning, error, critical, off");
    }

    void load() {
        constexpr auto section{ "UI" };
        settingsLoader loader(SETTINGS_FILE_PATH);
        loader.load(Settings::relative_window_size_h, section, "relative_window_size_h");
        loader.load(Settings::relative_window_size_v, section, "relative_window_size_v");
        loader.load(Settings::windowPos_x, section, "windowPos_x");
        loader.load(Settings::windowPos_y, section, "windowPos_y");
        loader.load(Settings::lockWindowSize, section, "lockWindowSize");
        loader.load(Settings::lockWindowPos, section, "lockWindowPos");
        loader.load(Settings::key_toggle_dmenu, section, "key_toggle_dmenu");
        loader.load(Settings::fontScale, section, "fontScale");
        loader.load(Settings::log_level, "LogLevel", "Level");
    }

    inline void init() {
        load();
    }
}  // namespace ini

namespace UI {
    void show() {
        const ImVec2 parentSize = ImGui::GetMainViewport()->Size;

        const float new_relative_window_size_h = ImGui::GetWindowWidth() / parentSize.x;
        const float new_relative_window_size_v = ImGui::GetWindowHeight() / parentSize.y;

        if(Settings::relative_window_size_h != new_relative_window_size_h) {
            Settings::relative_window_size_h = new_relative_window_size_h;
        }

        if(Settings::relative_window_size_v != new_relative_window_size_v) {
            Settings::relative_window_size_v = new_relative_window_size_v;
        }

        const auto windowPos = ImGui::GetWindowPos();
        // get windowpos
        if(Settings::windowPos_x != windowPos.x) {
            Settings::windowPos_x = windowPos.x;
        }

        if(Settings::windowPos_y != windowPos.y) {
            Settings::windowPos_y = windowPos.y;
        }

        // Display the relative sizes in real-time
        ImGui::Text("Width: %.2f%%", Settings::relative_window_size_h * 100.0F);
        ImGui::SameLine();
        ImGui::Text("Height: %.2f%%", Settings::relative_window_size_v * 100.0F);

        //ImGui::SameLine(ImGui::GetWindowWidth() - 100);
        ImGui::Checkbox("Lock Size", &Settings::lockWindowSize);
        //ImGui::PopStyleVar();

        // Display the relative positions in real-time
        ImGui::Text("X pos: %f", Settings::windowPos_x);
        ImGui::SameLine();
        ImGui::Text("Y pos: %f", Settings::windowPos_y);

        //ImGui::SameLine(ImGui::GetWindowWidth() - 100);
        ImGui::Checkbox("Lock Pos", &Settings::lockWindowPos);
        //ImGui::PopStyleVar();

        if(ImGui::SliderFloat("Font Size", &Settings::fontScale, 0.5F, 2.0F)) {
            ImGui::GetIO().FontGlobalScale = Settings::fontScale;
        }

        ImGui::InputInt("Toggle Key", reinterpret_cast<int*>(&Settings::key_toggle_dmenu));
        static constexpr std::array log_levels{
            "trace",
            "debug",
            "info",
            "warning",
            "error",
            "critical",
            "off"
        };
        static int current_log_level{ spdlog::level::from_str(Settings::log_level) };
        if(ImGui::Combo("Log Level", &current_log_level, log_levels.data(), static_cast<int>(log_levels.size()))) {
            spdlog::set_level(spdlog::level::from_str(log_levels[current_log_level]));
            spdlog::flush_on(spdlog::level::from_str(log_levels[current_log_level]));
            Settings::log_level = log_levels[current_log_level];
        }
    }
}  // namespace UI

void Settings::show() {
    // Use consistent padding and alignment
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 10));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5, 5));

    // Indent the contents of the window
    ImGui::Indent();
    ImGui::Spacing();

    // Display size controls
    ImGui::Text("UI");
    UI::show();
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Unindent the contents of the window
    ImGui::Unindent();

    // Position the "Save" button at the bottom-right corner of the window
    ImGui::SameLine(ImGui::GetWindowWidth() - 100);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);

    // Set the button background and text colors
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

    if(ImGui::Button("Save")) {
        ini::update();
    }

    // Restore the previous style colors
    ImGui::PopStyleColor(4);

    // Use consistent padding and alignment
    ImGui::PopStyleVar(2);
}

void Settings::init() {
    ini::init();  // load all settings first

    logger::info("Settings initialized.");
}
