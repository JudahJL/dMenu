#include "Hooks.h"
#include "InputListener.h"
#include "Renderer.h"
#include "menus/Trainer.h"

void Hooks::Install() {
    SKSE::AllocTrampoline(1 << 5);
    onWeatherChange::install();
    OnInputEventDispatch::Install();
}

void Hooks::onWeatherChange::updateWeather(RE::Sky* a_sky) {
    if(Trainer::isWeatherLocked()) {
        return;
    }
    _updateWeather(a_sky);
}

void Hooks::OnInputEventDispatch::DispatchInputEvent(RE::BSTEventSource<RE::InputEvent*>* a_dispatcher, RE::InputEvent** a_evns) {
    static RE::InputEvent* dummy[] = { nullptr };
    if(!a_evns) {
        dispatch_input_event_(a_dispatcher, a_evns);
        return;
    }
    InputListener::GetSingleton()->ProcessEvent(a_evns);
    if(Renderer::IsEnabled()) {
        dispatch_input_event_(a_dispatcher, dummy);
        return;
    }
    dispatch_input_event_(a_dispatcher, a_evns);
}
