#pragma once

namespace Hooks {
    class onWeatherChange
    {
    public:
        static void install() {
            auto&                                 trampoline = SKSE::GetTrampoline();
            const REL::Relocation<std::uintptr_t> caller_setWeather{ RELOCATION_ID(25'682, 26'229) };  //Up p   Sky__Update_1403B1670+29C   call    Sky__UpdateWeather_1403B1C80 Up p   sub_1403C8F20+3E6   call    sub_1403C9870
            _updateWeather = trampoline.write_call<5>(caller_setWeather.address() + RELOCATION_OFFSET(0x2'9c, 0x3'E6), updateWeather);
            logger::info("hook:onWeatherChange");
        }

    private:
        static void updateWeather(RE::Sky* a_sky);

        static inline REL::Relocation<decltype(updateWeather)> _updateWeather;
    };

    class OnInputEventDispatch
    {
    public:
        static void Install() {
            auto&                      trampoline = SKSE::GetTrampoline();
            REL::Relocation<uintptr_t> caller{ RELOCATION_ID(67'315, 68'617) };
            dispatch_input_event_ = trampoline.write_call<5>(caller.address() + RELOCATION_OFFSET(0x7B, 0x7B), DispatchInputEvent);
        }

    private:
        static void                                                 DispatchInputEvent(RE::BSTEventSource<RE::InputEvent*>* a_dispatcher, RE::InputEvent** a_evns);
        static inline REL::Relocation<decltype(DispatchInputEvent)> dispatch_input_event_;
    };

    void Install();
}  // namespace Hooks

