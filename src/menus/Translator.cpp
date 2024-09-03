#include "menus/Translator.h"

void Translator::ReLoadTranslations() {
    _translations.clear();
}

const char* Translator::Translate(const std::string& id) {
    if(const auto it = _translations.find(id); it != _translations.end()) {
        return it->second.first ? it->second.second.c_str() : nullptr;
    }
    std::string res;
    SKSE::Translation::Translate(id, res);
    const bool hasTranslation = !res.empty();
    _translations[id]   = { hasTranslation, res };
    return hasTranslation ? _translations[id].second.c_str() : nullptr;
}

const char* Translatable::get() const {
    const char* ret = Translator::Translate(key);
    return ret ? ret : def.c_str();
}

bool Translatable::empty() const {
    return def.empty() && key.empty();
}
