#include "imgui.h"
// #include "imgui_internal.h"
// #include <imgui_impl_dx11.h>
// #include <imgui_impl_win32.h>
#include "menus/AIM.h"

// #include "dMenu.h"

#include "Utils.h"

#include <nlohmann/json.hpp>

// I swear I will RE this
inline void sendConsoleCommand(const std::string& a_command) {
    const auto scriptFactory{ RE::IFormFactory::GetConcreteFormFactoryByType<RE::Script>() };
    if(const auto script = scriptFactory ? scriptFactory->Create() : nullptr; script) {
        const auto selectedRef{ RE::Console::GetSelectedRef() };
        script->SetCommand(a_command);
        script->CompileAndRun(selectedRef.get());
        delete script;
    }
}

static bool init_ = false;

static std::map<std::string, bool> bgs_types = {
    {     "Weapon", false },
    {      "Armor", false },
    {       "Ammo", false },
    {       "Book", false },
    { "Ingredient", false },
    {        "Key", false },
    {       "Misc", false },
    {        "NPC", false }
};

static std::vector<std::pair<RE::TESFile*, bool>> _mods;
static int                                        _selectedModIndex = -1;

static std::map<std::string, RE::TESForm*> _items;  // items to show on AIM
static RE::TESForm*                        _selectedItem;

static bool _cached = false;

static ImGuiTextFilter _modFilter;
static ImGuiTextFilter _itemFilter;

void AIM::init() {
    if(init_) {
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

    for(auto mod : mods) {
        _mods.emplace_back(mod, false);
    }

    _selectedModIndex = 0;

    init_ = true;
    logger::info("AIM initialized");
}

template<class T>
void cacheItems(RE::TESDataHandler* a_data) {
    RE::TESFile* selectedMod = _mods[_selectedModIndex].first;
    for(auto form : a_data->GetFormArray<T>()) {
        if(selectedMod->IsFormInMod(form->GetFormID()) && form->GetFullNameLength() != 0) {
            std::string name = form->GetFullName();
            if(form->IsArmor()) {
                if(form->template As<RE::TESObjectARMO>()->IsHeavyArmor()) {
                    name += " (Heavy)";
                } else {
                    name += " (Light)";
                }
            }
            _items.insert({ name, form });
        }
    }
}

/* Present all filtered items to the user under the "Items" section*/
void cache() {
    _items.clear();
    const auto data{ RE::TESDataHandler::GetSingleton() };
    if(!data || _selectedModIndex == -1) {
        return;
    }
    constexpr std::array cacheFunctions = {
        cacheItems<RE::TESObjectWEAP>,
        cacheItems<RE::TESObjectARMO>,
        cacheItems<RE::TESAmmo>,
        cacheItems<RE::TESObjectBOOK>,
        cacheItems<RE::IngredientItem>,
        cacheItems<RE::TESKey>,
        cacheItems<RE::TESObjectMISC>,
        cacheItems<RE::TESNPC>
    };
    int i{};
    for(const auto& [_, value] : bgs_types) {
        if(value) {
            cacheFunctions[i](data);
        }
        i++;
    }
    // filter out unplayable weapons in 2nd pass
    for(auto it = _items.begin(); it != _items.end();) {
        const auto* form = it->second;
        switch(form->GetFormType()) {
            case RE::FormType::Weapon:
                if(form->As<RE::TESObjectWEAP>()->weaponData.flags.any(RE::TESObjectWEAP::Data::Flag::kNonPlayable)) {
                    it = _items.erase(it);
                    continue;
                }
                break;
            default:
                break;
        }
        ++it;
    }
}

void AIM::show() {
    // Use consistent padding and alignment
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 10));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5, 5));

    // Render the mod filter box
    _modFilter.Draw("Mod Name");

    // Render the mod dropdown menu
    if(ImGui::BeginCombo("Mods", _mods[_selectedModIndex].first->GetFilename().data())) {
        for(int i = 0; i < _mods.size(); i++) {
            if(_modFilter.PassFilter(_mods[i].first->GetFilename().data())) {
                const bool isSelected = (_mods[_selectedModIndex].first == _mods[i].first);
                if(ImGui::Selectable(_mods[i].first->GetFilename().data(), isSelected)) {
                    _selectedModIndex = i;
                    _cached           = false;
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
        auto& [key, value] = *i;
        if(ImGui::Checkbox(key.c_str(), &value)) {
            _cached = false;
        }
        if(i != std::prev(bgs_types.end())) {
            ImGui::SameLine();
        }
    }
    ImGui::PopStyleVar();

    ImGui::Spacing();
    // Render the list of items
    _itemFilter.Draw("Item Name");

    // Use a child window to limit the size of the item list
    ImGui::BeginChild("Items", ImVec2(0, ImGui::GetContentRegionAvail().y * 0.7F), true);

    for(auto& [key, value] : _items) {
        // Filter
        if(_itemFilter.PassFilter(key.data())) {
            ImGui::Selectable(key.data());

            if(ImGui::IsItemClicked()) {
                _selectedItem = value;
            }

            if(ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::Text(std::format("{:x}", value->GetFormID()).c_str());
                ImGui::EndTooltip();
            }
            // show item texture
        }
    }
    ImGui::EndChild();

    // Show selected item info and spawn button
    if(_selectedItem != nullptr) {
        ImGui::Text("Selected Item: ");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "%s", _selectedItem->GetName());
        static char buf[16] = "1";
        if(ImGui::Button("Spawn", ImVec2(ImGui::GetContentRegionAvail().x * 0.2F, 0.0F))) {
            if(RE::PlayerCharacter::GetSingleton() != nullptr) {
                // Spawn item
                const std::string cmd{ std::format("player.{} {:x} {}", _selectedItem->GetFormType() == RE::FormType::NPC ? "placeatme" : "additem", _selectedItem->GetFormID(), buf) };
                sendConsoleCommand(cmd);
            }
        }
        ImGui::SameLine();
        ImGui::InputText("Amount", buf, 16, ImGuiInputTextFlags_CharsDecimal);
    }
    //todo: make this work
    //if (ImGui::Button("Inspect", ImVec2(ImGui::GetContentRegionAvail().x * 0.2, 0))) {
    //  QUIHelper::inspect();
    //}
    //

    if(!_cached) {
        cache();
    }

    // Use consistent padding and alignment
    ImGui::PopStyleVar(2);
}
