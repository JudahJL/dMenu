#pragma once

// #include "menus/Translator.h"

class Settings
{
public:
    static inline spdlog::level::level_enum log_level{ spdlog::level::trace };
    static inline float                     relative_window_size_h{ 1.0F };
    static inline float                     relative_window_size_v{ 1.0F };
    static inline float                     windowPos_x{ 0.00F };
    static inline float                     windowPos_y{ 0.0F };
    static inline bool                      lockWindowPos{ false };
    static inline bool                      lockWindowSize{ false };
    static inline float                     fontScale{ 1.0F };
    static inline uint32_t                  key_toggle_dmenu{ 199 };

    static void show();

    static void init();
};
