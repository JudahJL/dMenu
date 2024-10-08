#pragma once

class InputListener
{
public:
    static InputListener* GetSingleton() {
        static InputListener listener;
        return std::addressof(listener);
    }

    void ProcessEvent(RE::InputEvent** a_event);
};
