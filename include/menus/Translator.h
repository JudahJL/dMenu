#pragma once

class Translator
{
public:
    static void ReLoadTranslations();

    // translate a string to the current language. Returns a null-pointer if the matching translation is not found.
    static const char* Translate(const std::string& id);

private:
    static inline std::unordered_map<std::string, std::pair<bool, std::string>> _translations;  // id -> {has_translation, translated text}
};

struct Translatable {
    Translatable() = default;
    Translatable(std::string def, std::string key):
        def(std::move(def)), key(std::move(key)){};
    explicit Translatable(std::string def):
        def(std::move(def)){};
    std::string               def{};        // default string
    std::string               key{};        // key to look up in the translation file
    [[nodiscard]] const char* get() const;  // get the translated string, or the default if no translation is found note the ptr points to the original data.
    [[nodiscard]] bool        empty() const;
};
