#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <chrono>
#include "persona.hpp"
#include "memory.hpp"
#include "../database/database.hpp"

namespace shandris::cognitive {

class PersonaSystem {
public:
    PersonaSystem();
    ~PersonaSystem();

    void InitializeDefaultPersonas();

    bool CanTransition(const std::string& targetPersonaID) const;
    bool SwitchPersona(const std::string& targetPersonaID, const std::string& reason);
    std::shared_ptr<Persona> GetActivePersona() const;

    void UpdateContext(const std::shared_ptr<PersonaContext>& update);
    std::shared_ptr<PersonaUpdate> ProcessInteraction(
        const std::shared_ptr<Interaction>& interaction,
        const std::shared_ptr<PersonaContext>& context);

    void UpdateTrait(const std::string& traitName, double influence, const std::string& evidence);
    void UpdateTraitEvidence(const std::string& traitName, const std::string& evidence);
    void EvolvePersonality(const std::shared_ptr<Interaction>& interaction);
    void ApplyTraitDecay();
    void AddTraitCorrelation(const std::string& trait1, const std::string& trait2, double correlation);
    void AddDerivedTrait(const std::string& baseTrait, const std::string& newTrait, double influence);
    std::vector<std::string> GetEvolvingTraits() const;
    double GetTraitStrength(const std::string& traitName) const;

    InteractionResponse CalculateResponseStyle(const std::shared_ptr<Interaction>& interaction) const;
    PersonaStyleRule GetResponseStyle(const std::shared_ptr<PersonaContext>& context) const;

private:
    void CheckTraitConsistency();
    void PropagateTraitInfluence(const std::string& traitName, double influence);
    void UpdateAttachmentLevel(const std::chrono::system_clock::time_point& now);
    void CalculateEmotionalInfluence(const std::shared_ptr<Interaction>& interaction);
    void ApplyTimeBasedEffects();
    void AdjustResponseBiases(const std::shared_ptr<Interaction>& interaction);
    void UpdateEmotionalState(const std::shared_ptr<Interaction>& interaction);

    std::unordered_map<std::string, std::shared_ptr<Persona>> personas_;
    std::shared_ptr<Persona> activePersona_;
    std::shared_ptr<PersonaContext> context_;
    std::vector<PersonaEvent> history_;

    std::unique_ptr<TransitionManager> transitions_;
    std::unique_ptr<TraitManager> traits_;
    database::Database& db_;
};

} // namespace shandris::cognitive
