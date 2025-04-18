#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

struct Trait {
    std::string name;
    float intensity;
};

class Persona {
public:
    Persona(const std::string& name = "");
    void AddTrait(const std::string& name, float intensity);
    void UpdateTrait(const std::string& name, float delta);
    float GetTraitIntensity(const std::string& name) const;
    std::string GetName() const;

    std::string ToJSON() const;
    void FromJSON(const std::string& jsonStr);

private:
    std::string name;
    std::vector<Trait> traits;
};
