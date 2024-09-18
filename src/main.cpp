#include "Hooks.h"
// #include "InputListener.h"
#include "Renderer.h"
#include "menus/ModSettings.h"
#include "menus/Settings.h"

// ReSharper disable once CppParameterMayBeConstPtrOrRef
void message_handler(SKSE::MessagingInterface::Message* a_msg) {
    switch(a_msg->type) {
        case SKSE::MessagingInterface::kDataLoaded:
            Hooks::Install();
            ModSettings::save_all_game_setting();           // in case some .esp overwrite the game setting // fixme
            ModSettings::send_all_settings_update_event();  // notify all mods to update their settings
            break;
        default:
            break;
    }
}

inline void on_skse_init() {
    Renderer::Install();
    Settings::init();
    ModSettings::init();  // init mod settings before everyone else
}

namespace {
    void initialize_log(const SKSE::PluginDeclaration* plugin = SKSE::PluginDeclaration::GetSingleton()) {
        auto  log{ std::make_shared<spdlog::logger>(plugin->GetName().data()) };
        auto& sink{ log->sinks() };
        if(REX::W32::IsDebuggerPresent()) [[unlikely]] {
            sink.emplace_back(std::make_shared<spdlog::sinks::msvc_sink_mt>());
        } else [[likely]] {
            auto path{ logger::log_directory() };
            if(!path) {
                util::report_and_fail("Failed to find standard logging directory");
            }
            *path /= fmt::format("{}.log", plugin->GetName());
            sink.emplace_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true));
        }
        const auto level{ spdlog::level::from_str(Settings::log_level) };
        log->set_level(level);
        log->flush_on(level);
        log->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] [%t] [%s:%#] %v");
        spdlog::set_default_logger(std::move(log));
    }
}  // namespace

SKSEPluginLoad(const SKSE::LoadInterface* a_skse) {
    // while(IsDebuggerPresent() == 0) {
    //     using namespace std::chrono_literals;
    //     constexpr auto wait_time{ 5s };
    //     std::this_thread::sleep_for(wait_time);
    // }
    const auto plugin{ SKSE::PluginDeclaration::GetSingleton() };

    initialize_log(plugin);

    logger::info("{} v{}", plugin->GetName(), plugin->GetVersion().string("."));

    SKSE::Init(a_skse);

    if(const auto messaging{ SKSE::GetMessagingInterface() }; !messaging->RegisterListener("SKSE", message_handler)) [[unlikely]] {
        util::report_and_fail("failed to register messaging interface");
    }

    on_skse_init();

    return true;
}
