#include "Renderer.h"

#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>

#include <imgui_internal.h>
#include <dxgi.h>

#include "dMenu.h"

#include "Utils.h"

// #include "menus/Settings.h"

// #include "menus/Translator.h"
// stole this from MaxSu's detection meter

namespace stl {
    using namespace SKSE::stl;

    template<class T>
    void write_thunk_call() {
        auto&                                 trampoline{ SKSE::GetTrampoline() };
        const REL::Relocation<std::uintptr_t> hook{ T::id, T::offset };
        T::func = trampoline.write_call<5>(hook.address(), T::thunk);
    }
}  // namespace stl

LRESULT Renderer::WndProcHook::thunk(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    auto& io{ ImGui::GetIO() };
    if(uMsg == WM_KILLFOCUS) {
        io.ClearInputCharacters();
        io.ClearInputKeys();
    }

    return func(hWnd, uMsg, wParam, lParam);
}

void SetupImGuiStyle() {
    auto& style{ ImGui::GetStyle() };
    auto& colors{ style.Colors };

    // Theme from https://github.com/ArranzCNL/ImprovedCameraSE-NG

    //style.WindowTitleAlign = ImVec2(0.5, 0.5);
    //style.FramePadding = ImVec2(4, 4);

    // Rounded slider grabber
    style.GrabRounding = 12.0F;

    // Window
    colors[ImGuiCol_WindowBg]          = ImVec4{ 0.118f, 0.118f, 0.118f, 0.784f };
    colors[ImGuiCol_ResizeGrip]        = ImVec4{ 0.2f, 0.2f, 0.2f, 0.5f };
    colors[ImGuiCol_ResizeGripHovered] = ImVec4{ 0.3f, 0.3f, 0.3f, 0.75f };
    colors[ImGuiCol_ResizeGripActive]  = ImVec4{ 0.15f, 0.15f, 0.15f, 1.0f };

    // Header
    colors[ImGuiCol_Header]        = ImVec4{ 0.2f, 0.2f, 0.2f, 1.0f };
    colors[ImGuiCol_HeaderHovered] = ImVec4{ 0.3f, 0.3f, 0.3f, 1.0f };
    colors[ImGuiCol_HeaderActive]  = ImVec4{ 0.15f, 0.15f, 0.15f, 1.0f };

    // Title
    colors[ImGuiCol_TitleBg]          = ImVec4{ 0.15f, 0.15f, 0.15f, 1.0f };
    colors[ImGuiCol_TitleBgActive]    = ImVec4{ 0.15f, 0.15f, 0.15f, 1.0f };
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4{ 0.15f, 0.15f, 0.15f, 1.0f };

    // Frame Background
    colors[ImGuiCol_FrameBg]        = ImVec4{ 0.2f, 0.2f, 0.2f, 1.0f };
    colors[ImGuiCol_FrameBgHovered] = ImVec4{ 0.3f, 0.3f, 0.3f, 1.0f };
    colors[ImGuiCol_FrameBgActive]  = ImVec4{ 0.15f, 0.15f, 0.15f, 1.0f };

    // Button
    colors[ImGuiCol_Button]        = ImVec4{ 0.2f, 0.2f, 0.2f, 1.0f };
    colors[ImGuiCol_ButtonHovered] = ImVec4{ 0.3f, 0.3f, 0.3f, 1.0f };
    colors[ImGuiCol_ButtonActive]  = ImVec4{ 0.15f, 0.15f, 0.15f, 1.0f };

    // Tab
    colors[ImGuiCol_Tab]                = ImVec4{ 0.15f, 0.15f, 0.15f, 1.0f };
    colors[ImGuiCol_TabHovered]         = ImVec4{ 0.38f, 0.38f, 0.38f, 1.0f };
    colors[ImGuiCol_TabActive]          = ImVec4{ 0.28f, 0.28f, 0.28f, 1.0f };
    colors[ImGuiCol_TabUnfocused]       = ImVec4{ 0.15f, 0.15f, 0.15f, 1.0f };
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4{ 0.2f, 0.2f, 0.2f, 1.0f };
}

void Renderer::D3DInitHook::thunk() {
    func();

    const auto renderer{ RE::BSGraphics::Renderer::GetSingleton() };
    if(!renderer) [[unlikely]] {
        SKSE::log::error("couldn't find renderer");
        return;
    }

    const auto swapChain{ renderer->GetRuntimeData().renderWindows[0].swapChain };
    if(!swapChain) [[unlikely]] {
        SKSE::log::error("couldn't find swapChain");
        return;
    }

    REX::W32::DXGI_SWAP_CHAIN_DESC desc{};
    if(FAILED(swapChain->GetDesc(std::addressof(desc)))) [[unlikely]] {
        SKSE::log::error("IDXGISwapChain::GetDesc failed.");
        return;
    }
    const auto id_3d_11device{ reinterpret_cast<ID3D11Device*>(renderer->GetRuntimeData().forwarder) };
    const auto id_3d_11device_context{ reinterpret_cast<ID3D11DeviceContext*>(renderer->GetRuntimeData().context) };

    SKSE::log::info("Initializing ImGui...");

    ImGui::CreateContext();
    if(!ImGui_ImplWin32_Init(desc.outputWindow)) [[unlikely]] {
        SKSE::log::error("ImGui initialization failed (Win32)");
        return;
    }

    if(!ImGui_ImplDX11_Init(id_3d_11device, id_3d_11device_context)) [[unlikely]] {
        SKSE::log::error("ImGui initialization failed (DX11)");
        return;
    }

    logger::info("ImGui initialized!");

    initialized.store(true);

    WndProcHook::func = reinterpret_cast<WNDPROC>(
        REX::W32::SetWindowLongPtrA(desc.outputWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WndProcHook::thunk)));
    if(!WndProcHook::func) [[unlikely]] {
        SKSE::log::error("SetWindowLongPtrA failed!");
    }

    // initialize font selection here
    logger::info("Building font atlas...");
    std::filesystem::path fontPath;
    bool                  foundCustomFont{ false };
    const ImWchar*        glyphRanges{ nullptr };
    constexpr auto        FONT_SETTING_PATH{ R"(Data\SKSE\Plugins\dMenu\fonts\FontConfig.ini)" };
    CSimpleIniA           ini;
    ini.LoadFile(FONT_SETTING_PATH);
    if(!ini.IsEmpty()) [[likely]] {
        if(const char* language = ini.GetValue("config", "font", nullptr); language) {
            // check if folder exists
            if(const std::string fontDir{ std::format(R"(Data\SKSE\Plugins\dMenu\fonts\{})", language) }; std::filesystem::exists(fontDir) && std::filesystem::is_directory(fontDir)) {
                for(const auto& entry : std::filesystem::directory_iterator(fontDir)) {
                    const auto& entryPath{ entry.path() };
                    if(entryPath.extension() == ".ttf" || entryPath.extension() == ".ttc") {
                        fontPath        = entryPath;
                        foundCustomFont = true;
                        break;
                    }
                }
            }
            if(foundCustomFont) {
                logger::info("Loading font: {}", fontPath.string().c_str());
                if(language == "Chinese"sv) {
                    logger::info("Glyph range set to Chinese");
                    glyphRanges = ImGui::GetIO().Fonts->GetGlyphRangesChineseFull();
                } else if(language == "Korean"sv) {
                    logger::info("Glyph range set to Korean");
                    glyphRanges = ImGui::GetIO().Fonts->GetGlyphRangesKorean();
                } else if(language == "Japanese"sv) {
                    logger::info("Glyph range set to Japanese");
                    glyphRanges = ImGui::GetIO().Fonts->GetGlyphRangesJapanese();
                } else if(language == "Thai"sv) {
                    logger::info("Glyph range set to Thai");
                    glyphRanges = ImGui::GetIO().Fonts->GetGlyphRangesThai();
                } else if(language == "Vietnamese"sv) {
                    logger::info("Glyph range set to Vietnamese");
                    glyphRanges = ImGui::GetIO().Fonts->GetGlyphRangesVietnamese();
                } else if(language == "Cyrillic"sv) {
                    glyphRanges = ImGui::GetIO().Fonts->GetGlyphRangesCyrillic();
                    logger::info("Glyph range set to Cyrillic");
                }
            } else [[unlikely]] {
                logger::info("No font found for language: {}", language);
            }
        }
    }
#define ENABLE_FREETYPE 0
#if ENABLE_FREETYPE
    ImFontAtlas* atlas      = ImGui::GetIO().Fonts;
    atlas->FontBuilderIO    = ImGuiFreeType::GetBuilderForFreeType();
    atlas->FontBuilderFlags = ImGuiFreeTypeBuilderFlags_LightHinting;
#else
#endif
    if(foundCustomFont) {
        ImGui::GetIO().Fonts->AddFontFromFileTTF(fontPath.string().c_str(), 32.0f, nullptr, glyphRanges);
    }
    SetupImGuiStyle();
}

void Renderer::DXGIPresentHook::thunk(std::uint32_t a_p1) {
    func(a_p1);

    if(!D3DInitHook::initialized.load()) [[unlikely]] {
        return;
    }

    // prologue
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // do stuff
    Renderer::draw();

    // epilogue
    ImGui::EndFrame();
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

struct ImageSet {
    std::int32_t              my_image_width{ 0 };
    std::int32_t              my_image_height{ 0 };
    ID3D11ShaderResourceView* my_texture{ nullptr };
};

// ReSharper disable once CppParameterMayBeConstPtrOrRef
void Renderer::MessageCallback(SKSE::MessagingInterface::Message* msg)  //CallBack & LoadTextureFromFile should called after resource loaded.
{
    if(msg->type == SKSE::MessagingInterface::kDataLoaded && D3DInitHook::initialized) {
        // Read Texture only after game engine finished load all it renderer resource.
        auto& io{ ImGui::GetIO() };
        io.MouseDrawCursor = true;
        io.WantSetMousePos = true;
    }
}

bool Renderer::Install() {
    const auto g_message{ SKSE::GetMessagingInterface() };
    if(!g_message) [[unlikely]] {
        logger::error("Messaging Interface Not Found!");
        return false;
    }

    g_message->RegisterListener(MessageCallback);

    SKSE::AllocTrampoline(14 * 2);

    stl::write_thunk_call<D3DInitHook>();
    stl::write_thunk_call<DXGIPresentHook>();

    return true;
}

void Renderer::flip() {
    enable                         = !enable;
    ImGui::GetIO().MouseDrawCursor = enable;
}

float Renderer::GetResolutionScaleWidth() {
    return ImGui::GetIO().DisplaySize.x / 1920.0F;
}

float Renderer::GetResolutionScaleHeight() {
    return ImGui::GetIO().DisplaySize.y / 1080.0F;
}

void Renderer::draw() {
    //static constexpr ImGuiWindowFlags windowFlag = ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration;

    // resize window
    //ImGui::SetNextWindowPos(ImVec2(0, 0));
    //ImGui::SetNextWindowSize(ImVec2(screenSizeX, screenSizeY));

    // Add UI elements here
    //ImGui::Text("sizeX: %f, sizeYL %f", screenSizeX, screenSizeY);

    if(enable) {
        if(!DMenu::initialized) [[unlikely]] {
            const ImVec2 screenSize{ ImGui::GetMainViewport()->Size };
            DMenu::init(screenSize.x, screenSize.y);
        }

        DMenu::draw();
    }
}
