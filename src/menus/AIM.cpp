#include <imgui.h>
// #include "imgui_internal.h"
// #include <imgui_impl_dx11.h>
// #include <imgui_impl_win32.h>
#include "menus/AIM.h"

// #include "dMenu.h"

#include "Utils.h"

#include <nlohmann/json.hpp>

// I swear I will RE this
inline void sendConsoleCommand(const std::string_view a_command) {
    const auto scriptFactory{ RE::IFormFactory::GetConcreteFormFactoryByType<RE::Script>() };
    if(const auto script{ scriptFactory ? scriptFactory->Create() : nullptr }; script) {
        const auto selectedRef{ RE::Console::GetSelectedRef() };
        script->SetCommand(a_command);
        script->CompileAndRun(selectedRef.get());
        delete script;
    }
}

static bool init_{ false };

static std::vector<std::pair<const RE::TESFile*, bool>> g_mods;
static size_t                                           g_selectedModIndex{};

static std::map<std::string, RE::TESForm*> g_items;  // items to show on AIM
static RE::TESForm*                        g_selectedItem;

static bool g_cached{ false };

static ImGuiTextFilter g_modFilter;
static ImGuiTextFilter g_itemFilter;

template<class T>
void cacheItems(RE::TESDataHandler* a_data) {
    const RE::TESFile* selectedMod{ g_mods[g_selectedModIndex].first };
    for(const auto form : a_data->GetFormArray<T>()) {
        if(selectedMod->IsFormInMod(form->GetFormID()) && form->GetFullNameLength() != 0) {
            const std::string name{ std::format("{}{}", form->GetFullName(), form->IsArmor() ? (form->template As<RE::TESObjectARMO>()->IsHeavyArmor() ? " (Heavy)" : " (Light)") : "") };
            // if(form->IsArmor()) {
            //     if(form->template As<RE::TESObjectARMO>()->IsHeavyArmor()) {
            //         name.append(" (Heavy)"s, 8);
            //     } else {
            //         name.append(" (Light)"s, 8);
            //     }
            // }
            g_items.insert({ name, form });
        }
    }
}

static std::array<std::tuple<const std::string, bool, void(*)(RE::TESDataHandler*)>, 8> bgs_types{{
    { "Weapon", false, cacheItems<RE::TESObjectWEAP> },
    { "Armor", false, cacheItems<RE::TESObjectARMO> },
    { "Ammo", false, cacheItems<RE::TESAmmo> },
    { "Book", false, cacheItems<RE::TESObjectBOOK> },
    { "Ingredient", false, cacheItems<RE::IngredientItem> },
    { "Key", false, cacheItems<RE::TESKey> },
    { "Misc", false, cacheItems<RE::TESObjectMISC> },
    { "NPC", false, cacheItems<RE::TESNPC> }
}};

void AIM::init() {
    if(init_) [[unlikely]] {
        return;
    }
    std::unordered_set<RE::TESFile*> mods;

    // only show mods with valid items
    Utils::loadUsefulPlugins<RE::TESObjectWEAP>(mods);
    Utils::loadUsefulPlugins<RE::TESObjectARMO>(mods);
    Utils::loadUsefulPlugins<RE::TESAmmo>(mods);
    Utils::loadUsefulPlugins<RE::TESObjectBOOK>(mods);
    Utils::loadUsefulPlugins<RE::IngredientItem>(mods);
    Utils::loadUsefulPlugins<RE::TESKey>(mods);
    Utils::loadUsefulPlugins<RE::TESObjectMISC>(mods);
    Utils::loadUsefulPlugins<RE::TESNPC>(mods);

    for(const auto* mod : mods) {
        g_mods.emplace_back(mod, false);
    }

    init_ = true;
    logger::info("AIM initialized");
}

/* Present all filtered items to the user under the "Items" section*/
void cache() {
    g_items.clear();
    auto* data{ RE::TESDataHandler::GetSingleton() };
    if(!data || g_selectedModIndex == std::numeric_limits<std::size_t>::max()) {
        return;
    }

    uint8_t i{};
    for(const auto& [_,value, function] : bgs_types) {
        if(value) {
            function(data);
        }
        i++;
    }
    // filter out unplayable weapons in 2nd pass
    for(auto it{ g_items.begin() }; it != g_items.end(); ++it) {
        const auto* const form{ it->second };
        switch(form->GetFormType()) {
            case RE::FormType::Weapon:
                if(form->As<RE::TESObjectWEAP>()->weaponData.flags.any(RE::TESObjectWEAP::Data::Flag::kNonPlayable)) {
                    it = g_items.erase(it);
                }
                break;
            default:
                break;
        }
    }
}

void AIM::show() {
    // Use consistent padding and alignment
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 10));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5, 5));

    // Render the mod filter box
    g_modFilter.Draw("Mod Name");

    // Render the mod dropdown menu
    if(ImGui::BeginCombo("Mods", g_mods[g_selectedModIndex].first->GetFilename().data())) {
        for(size_t i{}; i < g_mods.size(); i++) {
            if(g_modFilter.PassFilter(g_mods[i].first->GetFilename().data())) {
                const bool isSelected{ (g_mods[g_selectedModIndex].first == g_mods[i].first) };
                if(ImGui::Selectable(g_mods[i].first->GetFilename().data(), isSelected)) {
                    g_selectedModIndex = i;
                    g_cached           = false;
                }
                if(isSelected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
        }
        ImGui::EndCombo();
    }

    // Item type filtering
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Group related controls together
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 0));
    for(auto i{ bgs_types.begin() }; i != bgs_types.end(); ++i) {
        auto& [key, value, _]{ *i };
        if(ImGui::Checkbox(key.c_str(), &value)) {
            g_cached = false;
        }
        if(i != std::prev(bgs_types.end())) {
            ImGui::SameLine();
        }
    }
    ImGui::PopStyleVar();

    ImGui::Spacing();
    // Render the list of items
    g_itemFilter.Draw("Item Name");

    // Use a child window to limit the size of the item list
    ImGui::BeginChild("Items", ImVec2(0, ImGui::GetContentRegionAvail().y * 0.7F), true);

    for(auto& [key, value] : g_items) {
        // Filter
        if(g_itemFilter.PassFilter(key.data())) {
            ImGui::Selectable(key.data());

            if(ImGui::IsItemClicked()) {
                g_selectedItem = value;
            }

            if(ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::Text(std::format("{:X}", value->GetFormID()).c_str());
                ImGui::EndTooltip();
            }
            // show item texture
        }
    }
    ImGui::EndChild();

    // Show selected item info and spawn button
    if(g_selectedItem != nullptr) [[likely]] {
        ImGui::Text("Selected Item: ");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0F, 0.5F, 0.0F, 1.0F), "%s", g_selectedItem->GetName());
        static std::array<char, 16> buf{ "1" };
        if(ImGui::Button("Spawn", ImVec2(ImGui::GetContentRegionAvail().x * 0.2F, 0.0F))) {
            if(RE::PlayerCharacter::GetSingleton() != nullptr) [[likely]] {
                // Spawn item
                const std::string cmd{ std::format("player.{} {:x} {}", g_selectedItem->GetFormType() == RE::FormType::NPC ? "placeatme" : "additem", g_selectedItem->GetFormID(), buf.data()) };
                sendConsoleCommand(cmd);
            }
        }
        ImGui::SameLine();
        ImGui::InputText("Amount", buf.data(), buf.size(), ImGuiInputTextFlags_CharsDecimal);
    }
    //todo: make this work
    //if (ImGui::Button("Inspect", ImVec2(ImGui::GetContentRegionAvail().x * 0.2, 0))) {
    //  QUIHelper::inspect();
    //}

    if(!g_cached) {
        cache();
    }

    // Use consistent padding and alignment
    ImGui::PopStyleVar(2);
}
