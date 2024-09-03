#pragma once

class Renderer
{
    struct WndProcHook {
        static LRESULT        thunk(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
        static inline WNDPROC func;
    };

    struct D3DInitHook {
        static void                                    thunk();
        static inline REL::Relocation<decltype(thunk)> func;

        static constexpr auto id     = REL::RelocationID(75'595, 77'226);
        static constexpr auto offset = REL::VariantOffset(0x9, 0x2'75, 0x00);  // VR unknown

        static inline std::atomic<bool> initialized = false;
    };

    struct DXGIPresentHook {
        static void                                    thunk(std::uint32_t a_p1);
        static inline REL::Relocation<decltype(thunk)> func;

        static constexpr auto id     = REL::RelocationID(75'461, 77'246);
        static constexpr auto offset = REL::Offset(0x9);
    };

    static void draw();  //Rendering Meters.
    static void MessageCallback(SKSE::MessagingInterface::Message* msg);

    static inline bool                           ShowMeters = false;
    static inline REX::W32::ID3D11Device*        device     = nullptr;
    static inline REX::W32::ID3D11DeviceContext* context    = nullptr;

    static inline bool enable = false;

public:
                Renderer() = delete;
    static bool Install();

    static void flip();

    static bool IsEnabled() { return enable; }

    static float GetResolutionScaleWidth();   // { return ImGui::GetIO().DisplaySize.x / 1920.f; }
    static float GetResolutionScaleHeight();  //{ return ImGui::GetIO().DisplaySize.y / 1080.f; }
};
