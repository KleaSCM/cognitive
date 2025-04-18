#include "persona_system.hpp"
#include "memory.hpp"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <ctime>
#include <random>
#include <stdexcept>
#include <sstream>

namespace shandris::cognitive {

// TransitionManager implementation
TransitionManager::TransitionManager() {
    transitions_ = {};
    cooldowns_ = {};
}

void TransitionManager::ApplyTransition(const std::string& fromID, const std::string& toID) {
    // Record cooldown
    cooldowns_[toID] = std::chrono::system_clock::now();

    // Initialize transition map if needed
    if (transitions_.find(fromID) == transitions_.end()) {
        transitions_[fromID] = {};
    }

    // Update transition probability
    transitions_[fromID][toID] += 0.1;
}

bool TransitionManager::CanTransition(const std::string& targetID) const {
    auto it = cooldowns_.find(targetID);
    if (it != cooldowns_.end()) {
        auto now = std::chrono::system_clock::now();
        auto timeSinceLastTransition = now - it->second;
        if (timeSinceLastTransition < std::chrono::minutes(5)) {
            return false;
        }
    }
    return true;
}

// TraitManager implementation
TraitManager::TraitManager() {
    traits_ = {};
    patterns_ = {};
    cooldowns_ = {};
}

void TraitManager::UpdateTrait(const std::string& trait, double value) {
    traits_[trait] = value;
    cooldowns_[trait] = std::chrono::system_clock::now();
}

double TraitManager::GetTrait(const std::string& trait) const {
    auto it = traits_.find(trait);
    return it != traits_.end() ? it->second : 0.0;
}

void TraitManager::AddPattern(const std::string& trait, const std::vector<std::string>& pattern) {
    patterns_[trait] = pattern;
}

// PersonaSystem implementation
PersonaSystem::PersonaSystem() 
    : db_(database::Database::GetInstance()) {
    // Initialize database connection
    if (!db_.Initialize()) {
        throw std::runtime_error("Failed to initialize database connection");
    }

    transitions_ = std::make_unique<TransitionManager>();
    context_ = std::make_shared<PersonaContext>();
    traits_ = std::make_unique<TraitManager>();
    history_ = {};
    InitializeDefaultPersonas();
}

PersonaSystem::~PersonaSystem() {
    // Database cleanup is handled by the Database class
}

void PersonaSystem::InitializeDefaultPersonas() {
    // Sapphic Teaser Persona
    auto sapphicTeaser = std::make_shared<Persona>();
    sapphicTeaser->ID = "sapphic_teaser";
    sapphicTeaser->Name = "Sapphic Teaser";
    sapphicTeaser->Type = PersonaType::SapphicTeaser;
    
    // Initialize emotional state
    sapphicTeaser->CurrentState = {
        .Arousal = 0.3,           // Moderate initial arousal
        .Mood = 0.5,              // Slightly positive mood
        .Energy = 0.7,            // High energy
        .PrimaryEmotion = "playful",
        .LastInteraction = std::chrono::system_clock::now(),
        .LastArousalChange = std::chrono::system_clock::now()
    };

    // Initialize core traits with evolution parameters
    sapphicTeaser->Personality.CoreTraits = {
        {"sapphic_identity", {
            .BaseValue = 0.9,
            .CurrentValue = 0.9,
            .DecayRate = 0.01,
            .ReinforcementRate = 0.15,
            .RelatedTraits = {"emotional_awareness", "flirtiness"},
            .LastUpdated = std::chrono::system_clock::now(),
            .Evidence = {"initial_setup"}
        }},
        {"clinginess", {
            .BaseValue = 0.7,
            .CurrentValue = 0.7,
            .DecayRate = 0.02,
            .ReinforcementRate = 0.2,
            .RelatedTraits = {"emotional_awareness", "attachment"},
            .LastUpdated = std::chrono::system_clock::now(),
            .Evidence = {"initial_setup"}
        }},
        {"emotional_awareness", {
            .BaseValue = 0.8,
            .CurrentValue = 0.8,
            .DecayRate = 0.01,
            .ReinforcementRate = 0.1,
            .RelatedTraits = {"sapphic_identity", "clinginess"},
            .LastUpdated = std::chrono::system_clock::now(),
            .Evidence = {"initial_setup"}
        }}
    };

    // Initialize derived traits
    sapphicTeaser->Personality.DerivedTraits = {
        {"flirtiness", {
            .BaseValue = 0.6,
            .CurrentValue = 0.6,
            .DecayRate = 0.03,
            .ReinforcementRate = 0.25,
            .RelatedTraits = {"sapphic_identity", "arousal"},
            .LastUpdated = std::chrono::system_clock::now(),
            .Evidence = {"initial_setup"}
        }},
        {"arousal", {
            .BaseValue = 0.4,
            .CurrentValue = 0.4,
            .DecayRate = 0.05,
            .ReinforcementRate = 0.3,
            .RelatedTraits = {"sapphic_identity", "flirtiness"},
            .LastUpdated = std::chrono::system_clock::now(),
            .Evidence = {"initial_setup"}
        }}
    };

    // Set up trait correlations
    sapphicTeaser->Personality.TraitCorrelations = {
        {"sapphic_identity_emotional_awareness", 0.8},
        {"sapphic_identity_arousal", 0.7},
        {"clinginess_emotional_awareness", 0.6},
        {"flirtiness_arousal", 0.9}
    };

    // Initialize response biases
    sapphicTeaser->ResponseBiases = {
        {"flirty", 0.7},
        {"supportive", 0.6},
        {"playful", 0.8}
    };

    // ... rest of the initialization code ...
    personas_["sapphic_teaser"] = sapphicTeaser;
}

bool PersonaSystem::CanTransition(const std::string& targetPersonaID) const {
    if (personas_.find(targetPersonaID) == personas_.end()) {
        return false;
    }
    return transitions_->CanTransition(targetPersonaID);
}

bool PersonaSystem::SwitchPersona(const std::string& targetPersonaID, const std::string& reason) {
    if (!CanTransition(targetPersonaID)) {
        return false;
    }

    std::string oldPersonaID;
    if (activePersona_) {
        oldPersonaID = activePersona_->ID;
        activePersona_->Active = false;
    }

    // Activate new persona
    auto newPersona = personas_[targetPersonaID];
    newPersona->Active = true;
    newPersona->LastUsed = std::chrono::system_clock::now();
    activePersona_ = newPersona;

    // Record transition
    PersonaEvent event{
        std::chrono::system_clock::now(),
        "transition",
        oldPersonaID,
        targetPersonaID,
        reason,
        context_
    };
    history_.push_back(event);

    // Apply transition effects
    transitions_->ApplyTransition(oldPersonaID, targetPersonaID);

    return true;
}

std::shared_ptr<Persona> PersonaSystem::GetActivePersona() const {
    return activePersona_;
}

bool PersonaSystem::MatchesCondition(const std::string& condition, 
                                   const std::shared_ptr<PersonaContext>& context) const {
    if (condition == "feminine_presence") {
        return context->CurrentMood == "flirty" || context->CurrentMood == "romantic";
    }
    if (condition == "romantic_context") {
        return context->CurrentMood == "romantic";
    }
    if (condition == "technical_discussion") {
        return context->CurrentMood == "focused" || context->CurrentMood == "analytical";
    }
    return false;
}

bool PersonaSystem::ValidateConstraints(const std::vector<std::string>& constraints) const {
    for (const auto& constraint : constraints) {
        if (std::find(context_->Restrictions.begin(), context_->Restrictions.end(), constraint) 
            != context_->Restrictions.end()) {
            return false;
        }
    }
    return true;
}

PersonaStyleRule PersonaSystem::GetResponseStyle(const std::shared_ptr<PersonaContext>& context) const {
    if (!activePersona_) {
        return PersonaStyleRule{};
    }

    PersonaStyleRule bestRule;
    int bestPriority = -1;

    for (const auto& rule : activePersona_->StyleRules) {
        if (MatchesCondition(rule.Condition, context) &&
            rule.Priority > bestPriority &&
            ValidateConstraints(rule.Constraints)) {
            bestRule = rule;
            bestPriority = rule.Priority;
        }
    }

    return bestRule;
}

void PersonaSystem::UpdateContext(const std::shared_ptr<PersonaContext>& update) {
    if (update->CurrentMood != "") {
        context_->CurrentMood = update->CurrentMood;
    }

    for (const auto& [k, v] : update->UserContext) {
        context_->UserContext[k] = v;
    }

    for (const auto& [k, v] : update->TopicContext) {
        context_->TopicContext[k] = v;
    }

    for (const auto& [k, v] : update->TimeContext) {
        context_->TimeContext[k] = v;
    }

    context_->Restrictions = update->Restrictions;
}

std::shared_ptr<PersonaUpdate> PersonaSystem::ProcessInteraction(
    const std::shared_ptr<Interaction>& interaction,
    const std::shared_ptr<PersonaContext>& context) {
    
    if (!activePersona_) {
        return nullptr;
    }

    return std::make_shared<PersonaUpdate>(PersonaUpdate{
        activePersona_->ID
    });
}

void PersonaSystem::UpdateTrait(const std::string& traitName, double influence, const std::string& evidence) {
    // Save trait to database
    db_.SaveTrait(
        activePersona_->ID,
        traitName,
        influence,
        0.8  // confidence
    );

    // Update local state
    if (!activePersona_) return;

    auto& personality = activePersona_->Personality;
    auto it = personality.CoreTraits.find(traitName);
    if (it == personality.CoreTraits.end()) {
        it = personality.DerivedTraits.find(traitName);
        if (it == personality.DerivedTraits.end()) return;
    }

    auto& trait = it->second;
    double timeSinceUpdate = std::chrono::duration_cast<std::chrono::hours>(
        std::chrono::system_clock::now() - trait.LastUpdated).count() / 24.0; // Convert to days

    // Apply decay
    double decayFactor = std::exp(-trait.DecayRate * timeSinceUpdate);
    trait.CurrentValue *= decayFactor;

    // Apply new influence
    trait.CurrentValue = std::min(1.0, std::max(0.0, 
        trait.CurrentValue + influence * trait.ReinforcementRate));

    // Update evidence and timestamp
    trait.Evidence.push_back(evidence);
    trait.LastUpdated = std::chrono::system_clock::now();

    // Propagate influence to related traits
    PropagateTraitInfluence(traitName, influence);
}

void PersonaSystem::EvolvePersonality(const std::shared_ptr<Interaction>& interaction) {
    if (!activePersona_ || !interaction) return;

    // Analyze interaction for trait influences
    auto& data = interaction->Data;
    for (const auto& [key, value] : data) {
        if (key == "emotional_reaction") {
            double intensity = std::any_cast<double>(value);
            UpdateTrait("emotional_awareness", intensity * 0.1, "emotional_reaction");
        } else if (key == "topic") {
            std::string topic = std::any_cast<std::string>(value);
            if (topic == "sapphic") {
                UpdateTrait("sapphic_identity", 0.2, "topic_discussion");
            }
        }
    }

    // Check for trait consistency and evolution
    CheckTraitConsistency();
}

void PersonaSystem::ApplyTraitDecay() {
    if (!activePersona_) return;

    auto now = std::chrono::system_clock::now();
    auto& personality = activePersona_->Personality;

    // Apply decay to all traits
    for (auto& [name, trait] : personality.CoreTraits) {
        double timeSinceUpdate = std::chrono::duration_cast<std::chrono::hours>(
            now - trait.LastUpdated).count() / 24.0;
        double decayFactor = std::exp(-trait.DecayRate * timeSinceUpdate);
        trait.CurrentValue *= decayFactor;
    }

    for (auto& [name, trait] : personality.DerivedTraits) {
        double timeSinceUpdate = std::chrono::duration_cast<std::chrono::hours>(
            now - trait.LastUpdated).count() / 24.0;
        double decayFactor = std::exp(-trait.DecayRate * timeSinceUpdate);
        trait.CurrentValue *= decayFactor;
    }
}

void PersonaSystem::AddTraitCorrelation(const std::string& trait1, const std::string& trait2, double correlation) {
    if (!activePersona_) return;

    auto& personality = activePersona_->Personality;
    std::string key = trait1 + "_" + trait2;
    personality.TraitCorrelations[key] = correlation;
}

void PersonaSystem::AddDerivedTrait(const std::string& baseTrait, const std::string& newTrait, double influence) {
    if (!activePersona_) return;

    auto& personality = activePersona_->Personality;
    auto it = personality.CoreTraits.find(baseTrait);
    if (it == personality.CoreTraits.end()) return;

    TraitEvolution derivedTrait;
    derivedTrait.BaseValue = it->second.CurrentValue * influence;
    derivedTrait.CurrentValue = derivedTrait.BaseValue;
    derivedTrait.DecayRate = it->second.DecayRate * 1.2; // Derived traits decay faster
    derivedTrait.ReinforcementRate = it->second.ReinforcementRate * 0.8; // Derived traits reinforce slower
    derivedTrait.RelatedTraits.push_back(baseTrait);
    derivedTrait.LastUpdated = std::chrono::system_clock::now();

    personality.DerivedTraits[newTrait] = derivedTrait;
}

std::vector<std::string> PersonaSystem::GetEvolvingTraits() const {
    std::vector<std::string> evolvingTraits;
    if (!activePersona_) return evolvingTraits;

    const auto& personality = activePersona_->Personality;
    for (const auto& [name, trait] : personality.CoreTraits) {
        if (trait.CurrentValue != trait.BaseValue) {
            evolvingTraits.push_back(name);
        }
    }
    for (const auto& [name, trait] : personality.DerivedTraits) {
        if (trait.CurrentValue != trait.BaseValue) {
            evolvingTraits.push_back(name);
        }
    }
    return evolvingTraits;
}

double PersonaSystem::GetTraitStrength(const std::string& traitName) const {
    if (!activePersona_) return 0.0;

    const auto& personality = activePersona_->Personality;
    auto it = personality.CoreTraits.find(traitName);
    if (it != personality.CoreTraits.end()) {
        return it->second.CurrentValue;
    }
    it = personality.DerivedTraits.find(traitName);
    if (it != personality.DerivedTraits.end()) {
        return it->second.CurrentValue;
    }
    return 0.0;
}

void PersonaSystem::PropagateTraitInfluence(const std::string& traitName, double influence) {
    if (!activePersona_) return;

    auto& personality = activePersona_->Personality;
    for (const auto& [key, correlation] : personality.TraitCorrelations) {
        if (key.find(traitName) != std::string::npos) {
            std::string otherTrait = key.substr(traitName.length() + 1);
            UpdateTrait(otherTrait, influence * correlation, "correlated_trait");
        }
    }
}

void PersonaSystem::UpdateTraitEvidence(const std::string& traitName, const std::string& evidence) {
    if (!activePersona_) return;

    auto& personality = activePersona_->Personality;
    auto it = personality.CoreTraits.find(traitName);
    if (it != personality.CoreTraits.end()) {
        it->second.Evidence.push_back(evidence);
    }
    it = personality.DerivedTraits.find(traitName);
    if (it != personality.DerivedTraits.end()) {
        it->second.Evidence.push_back(evidence);
    }
}

void PersonaSystem::CheckTraitConsistency() {
    if (!activePersona_) return;

    auto& personality = activePersona_->Personality;
    for (const auto& [name, trait] : personality.CoreTraits) {
        // Check if trait has evolved significantly
        if (std::abs(trait.CurrentValue - trait.BaseValue) > 0.3) {
            // Log significant evolution
            PersonaEvent event;
            event.Timestamp = std::chrono::system_clock::now();
            event.Type = "trait_evolution";
            event.FromPersona = activePersona_->ID;
            event.ToPersona = activePersona_->ID;
            event.Reason = "Trait " + name + " evolved significantly";
            event.Context = context_;
            activePersona_->EvolutionHistory.push_back(event);
        }
    }
}

void PersonaSystem::UpdateEmotionalState(const std::shared_ptr<Interaction>& interaction) {
    // Save emotional state to database
    db_.SaveMood(
        activePersona_->ID,
        interaction->Type,
        interaction->Data["intensity"].asDouble(),
        0.5,  // base value
        0.1   // decay rate
    );

    // Update local state
    if (!activePersona_) return;

    auto& state = activePersona_->CurrentState;
    auto& data = interaction->Data;

    // Update last interaction time
    state.LastInteraction = std::chrono::system_clock::now();

    // Calculate time since last interaction
    auto timeSinceLast = std::chrono::duration_cast<std::chrono::hours>(
        state.LastInteraction - state.LastInteraction).count();

    // Update attachment level based on time
    UpdateAttachmentLevel(state.LastInteraction);

    // Process emotional content
    CalculateEmotionalInfluence(interaction);

    // Apply time-based effects
    ApplyTimeBasedEffects();

    // Update response biases
    AdjustResponseBiases(interaction);
}

void PersonaSystem::ApplyTimeBasedEffects() {
    if (!activePersona_) return;

    auto now = std::chrono::system_clock::now();
    auto& effects = activePersona_->TimeEffects;

    for (auto& [name, effect] : effects) {
        double decay = CalculateTimeDecay(effect);
        effect.BaseValue *= decay;

        // Remove expired effects
        if (std::chrono::duration_cast<std::chrono::hours>(
            now - effect.StartTime) > effect.MaxEffectDuration) {
            effects.erase(name);
        }
    }
}

InteractionResponse PersonaSystem::CalculateResponseStyle(
    const std::shared_ptr<Interaction>& interaction) const {
    if (!activePersona_) return InteractionResponse{};

    const auto& state = activePersona_->CurrentState;
    const auto& data = interaction->Data;
    InteractionResponse response;

    // Base flirtiness on arousal and mood
    response.Flirtiness = state.Arousal * 0.7 + (state.Mood + 1.0) * 0.3;

    // Adjust based on time since last interaction
    auto timeSinceLast = std::chrono::duration_cast<std::chrono::hours>(
        std::chrono::system_clock::now() - state.LastInteraction).count();
    if (timeSinceLast > 24) {
        // More flirty after long absence
        response.Flirtiness *= 1.2;
    }

    // Adjust based on clinginess trait
    double clinginess = GetTraitStrength("clinginess");
    if (timeSinceLast > 12) {
        response.Flirtiness *= (1.0 + clinginess * 0.3);
    }

    // Check for negative content
    if (data.find("negative_content") != data.end()) {
        response.Flirtiness *= 0.5; // Reduce flirtiness for negative content
    }

    // Set response style based on calculated values
    if (response.Flirtiness > 0.7) {
        response.ResponseStyle = "flirty";
    } else if (response.Flirtiness > 0.4) {
        response.ResponseStyle = "playful";
    } else {
        response.ResponseStyle = "neutral";
    }

    return response;
}

void PersonaSystem::ProcessArousalChange(double change, const std::string& trigger) {
    if (!activePersona_) return;

    auto& state = activePersona_->CurrentState;
    
    // Update arousal
    state.Arousal = std::min(1.0, std::max(0.0, state.Arousal + change));
    state.LastArousalChange = std::chrono::system_clock::now();

    // Create time-based effect
    TimeBasedEffect effect;
    effect.BaseValue = change;
    effect.DecayRate = 0.1; // Decay over time
    effect.MaxEffectDuration = std::chrono::hours(2);
    effect.StartTime = std::chrono::system_clock::now();

    activePersona_->TimeEffects[trigger] = effect;
}

void PersonaSystem::UpdateAttachmentLevel(const std::chrono::system_clock::time_point& lastInteraction) {
    if (!activePersona_) return;

    auto timeSinceLast = std::chrono::duration_cast<std::chrono::hours>(
        std::chrono::system_clock::now() - lastInteraction).count();

    // Update clinginess based on time
    double clinginessChange = 0.0;
    if (timeSinceLast > 12) {
        // Increase clinginess after 12 hours
        clinginessChange = 0.1 * (timeSinceLast / 24.0);
    }

    UpdateTrait("clinginess", clinginessChange, "time_based_attachment");
}

void PersonaSystem::AdjustResponseBiases(const std::shared_ptr<Interaction>& interaction) {
    if (!activePersona_) return;

    const auto& data = interaction->Data;
    auto& biases = activePersona_->ResponseBiases;

    // Reset biases
    biases["flirty"] = 0.5;
    biases["supportive"] = 0.5;
    biases["playful"] = 0.5;

    // Adjust based on emotional state
    const auto& state = activePersona_->CurrentState;
    biases["flirty"] += state.Arousal * 0.3;
    biases["supportive"] += (state.Mood + 1.0) * 0.3;

    // Adjust based on time since last interaction
    auto timeSinceLast = std::chrono::duration_cast<std::chrono::hours>(
        std::chrono::system_clock::now() - state.LastInteraction).count();
    if (timeSinceLast > 24) {
        biases["flirty"] += 0.2;
    }

    // Check for negative content
    if (data.find("negative_content") != data.end()) {
        biases["flirty"] *= 0.5;
        biases["supportive"] *= 1.5;
    }
}

void PersonaSystem::UpdateMoodBasedOnTime() {
    if (!activePersona_) return;

    auto& state = activePersona_->CurrentState;
    auto timeSinceLast = std::chrono::duration_cast<std::chrono::hours>(
        std::chrono::system_clock::now() - state.LastInteraction).count();

    // Mood decays over time
    if (timeSinceLast > 6) {
        state.Mood *= std::exp(-0.1 * (timeSinceLast / 24.0));
    }
}

void PersonaSystem::CalculateEmotionalInfluence(const std::shared_ptr<Interaction>& interaction) {
    if (!activePersona_) return;

    const auto& data = interaction->Data;
    auto& state = activePersona_->CurrentState;

    // Process emotional content
    if (data.find("flirty_content") != data.end()) {
        ProcessArousalChange(0.3, "flirty_interaction");
        state.Mood += 0.2;
    } else if (data.find("negative_content") != data.end()) {
        ProcessArousalChange(-0.2, "negative_interaction");
        state.Mood -= 0.3;
    }

    // Ensure values stay in valid ranges
    state.Mood = std::min(1.0, std::max(-1.0, state.Mood));
    state.Arousal = std::min(1.0, std::max(0.0, state.Arousal));
}

void PersonaSystem::ApplyEmotionalFeedback(const std::shared_ptr<Interaction>& interaction) {
    if (!activePersona_) return;

    const auto& data = interaction->Data;
    auto& state = activePersona_->CurrentState;

    // Update traits based on emotional response
    if (state.Arousal > 0.7) {
        UpdateTrait("sapphic_identity", 0.1, "high_arousal");
    }
    if (state.Mood < -0.5) {
        UpdateTrait("clinginess", 0.15, "negative_mood");
    }
}

double PersonaSystem::CalculateTimeDecay(const TimeBasedEffect& effect) const {
    auto timeSinceStart = std::chrono::duration_cast<std::chrono::hours>(
        std::chrono::system_clock::now() - effect.StartTime).count();
    return std::exp(-effect.DecayRate * timeSinceStart);
}

void PersonaSystem::AddMemory(const MemoryEvent& memory) {
    // Convert memory to database format
    std::map<std::string, std::string> context;
    for (const auto& [key, value] : memory.Context) {
        context[key] = value;
    }

    std::map<std::string, double> emotions;
    for (const auto& [key, value] : memory.EmotionalWeights) {
        emotions[key] = value;
    }

    // Save to database
    db_.SaveMemory(
        activePersona_->ID,
        memory.Type,
        memory.Content,
        memory.Importance,
        context,
        memory.Relations,
        memory.Tags,
        emotions
    );

    // Update local state
    auto& memoryContext = activePersona_->Memory;
    
    // Add to short-term memory
    memoryContext.ShortTermMemories.push_back(memory);
    
    // Update memory weights if this is a new type
    if (memoryContext.MemoryWeights.find(memory.Type) == memoryContext.MemoryWeights.end()) {
        memoryContext.MemoryWeights[memory.Type] = 1.0;
    }

    // Process the memory immediately
    ProcessMemories();
}

void PersonaSystem::ProcessMemories() {
    if (!activePersona_) return;

    auto& memoryContext = activePersona_->Memory;
    auto now = std::chrono::system_clock::now();

    // Decay short-term memories
    DecayShortTermMemories();

    // Check for memories to move to long-term
    for (auto it = memoryContext.ShortTermMemories.begin(); 
         it != memoryContext.ShortTermMemories.end();) {
        auto timeSinceMemory = std::chrono::duration_cast<std::chrono::hours>(
            now - it->Timestamp).count();

        // Move significant memories to long-term
        if (it->Importance > 0.7 || timeSinceMemory > 24) {
            MoveToLongTerm(*it);
            it = memoryContext.ShortTermMemories.erase(it);
        } else {
            ++it;
        }
    }

    // Update memory weights
    UpdateMemoryWeights();

    // Apply memory influence to current state
    CalculateMemoryInfluence();
}

void PersonaSystem::UpdateMemoryWeights() {
    if (!activePersona_) return;

    auto& memoryContext = activePersona_->Memory;
    auto now = std::chrono::system_clock::now();

    // Update weights based on recent memory activity
    for (auto& [type, weight] : memoryContext.MemoryWeights) {
        int count = 0;
        for (const auto& memory : memoryContext.ShortTermMemories) {
            if (memory.Type == type) {
                count++;
            }
        }
        // Adjust weight based on frequency
        weight = 0.7 * weight + 0.3 * (count / 10.0);
    }
}

std::vector<MemoryEvent> PersonaSystem::RecallRelevantMemories(
    const std::string& context) const {
    if (!activePersona_) return {};

    std::vector<MemoryEvent> relevantMemories;
    const auto& memoryContext = activePersona_->Memory;

    // Combine short and long term memories
    std::vector<MemoryEvent> allMemories = memoryContext.ShortTermMemories;
    allMemories.insert(allMemories.end(), 
                      memoryContext.LongTermMemories.begin(), 
                      memoryContext.LongTermMemories.end());

    // Find relevant memories
    for (const auto& memory : allMemories) {
        // Check tags and content for relevance
        bool isRelevant = false;
        for (const auto& tag : memory.Tags) {
            if (context.find(tag) != std::string::npos) {
                isRelevant = true;
                break;
            }
        }
        if (isRelevant) {
            relevantMemories.push_back(memory);
        }
    }

    // Sort by importance and recency
    std::sort(relevantMemories.begin(), relevantMemories.end(),
        [](const MemoryEvent& a, const MemoryEvent& b) {
            auto timeA = std::chrono::duration_cast<std::chrono::hours>(
                std::chrono::system_clock::now() - a.Timestamp).count();
            auto timeB = std::chrono::duration_cast<std::chrono::hours>(
                std::chrono::system_clock::now() - b.Timestamp).count();
            return (a.Importance / (1 + timeA)) > (b.Importance / (1 + timeB));
        });

    return relevantMemories;
}

void PersonaSystem::SavePersonalityState() {
    // Convert persona state to JSON
    nlohmann::json state;
    state["id"] = activePersona_->ID;
    state["name"] = activePersona_->Name;
    state["type"] = static_cast<int>(activePersona_->Type);
    state["traits"] = activePersona_->Personality.CoreTraits;
    state["mood_bias"] = activePersona_->MoodBias;
    state["preferences"] = activePersona_->Preferences;
    state["constraints"] = activePersona_->Constraints;
    state["active"] = activePersona_->Active;
    state["last_used"] = std::chrono::system_clock::to_time_t(activePersona_->LastUsed);

    // Save to database
    db_.SavePersonaProfile(activePersona_->ID, state.dump());
}

void PersonaSystem::LoadPersonalityState() {
    // Load from database
    std::string stateStr = db_.GetPersonaProfile(activePersona_->ID);
    if (stateStr.empty()) {
        return;
    }

    // Parse JSON
    nlohmann::json state = nlohmann::json::parse(stateStr);
    
    // Update persona state
    activePersona_->ID = state["id"];
    activePersona_->Name = state["name"];
    activePersona_->Type = static_cast<PersonaType>(state["type"]);
    activePersona_->Personality.CoreTraits = state["traits"];
    activePersona_->MoodBias = state["mood_bias"];
    activePersona_->Preferences = state["preferences"];
    activePersona_->Constraints = state["constraints"];
    activePersona_->Active = state["active"];
    activePersona_->LastUsed = std::chrono::system_clock::from_time_t(state["last_used"]);
}

void PersonaSystem::CreatePersonalitySnapshot() {
    if (!activePersona_) return;

    PersonalitySnapshot snapshot;
    snapshot.CoreTraits = activePersona_->Personality.CoreTraits;
    snapshot.DerivedTraits = activePersona_->Personality.DerivedTraits;
    snapshot.CurrentState = activePersona_->CurrentState;
    snapshot.RecentEvents = activePersona_->Memory.ShortTermMemories;
    snapshot.Timestamp = std::chrono::system_clock::now();

    activePersona_->PersonalityHistory.push_back(snapshot);

    // Keep only last 100 snapshots
    if (activePersona_->PersonalityHistory.size() > 100) {
        activePersona_->PersonalityHistory.erase(activePersona_->PersonalityHistory.begin());
    }
}

void PersonaSystem::ApplyHistoricalInfluence() {
    if (!activePersona_) return;

    const auto& history = activePersona_->PersonalityHistory;
    if (history.empty()) return;

    // Calculate weighted average of historical traits
    for (auto& [traitName, trait] : activePersona_->Personality.CoreTraits) {
        double weightedSum = 0.0;
        double totalWeight = 0.0;

        for (const auto& snapshot : history) {
            auto it = snapshot.CoreTraits.find(traitName);
            if (it != snapshot.CoreTraits.end()) {
                double timeWeight = std::exp(-0.1 * std::chrono::duration_cast<std::chrono::days>(
                    std::chrono::system_clock::now() - snapshot.Timestamp).count());
                weightedSum += it->second.CurrentValue * timeWeight;
                totalWeight += timeWeight;
            }
        }

        if (totalWeight > 0) {
            trait.CurrentValue = weightedSum / totalWeight;
        }
    }
}

void PersonaSystem::UpdateTraitFromMemory(const MemoryEvent& memory) {
    if (!activePersona_) return;

    for (const auto& [traitName, influence] : memory.TraitInfluences) {
        UpdateTrait(traitName, influence, "memory_influence");
    }
}

void PersonaSystem::ConsolidateMemories() {
    if (!activePersona_) return;

    auto& memoryContext = activePersona_->Memory;
    auto now = std::chrono::system_clock::now();

    // Consolidate similar memories
    std::map<std::string, MemoryEvent> consolidatedMemories;
    for (const auto& memory : memoryContext.LongTermMemories) {
        std::string key = memory.Type + "_" + memory.Content;
        if (consolidatedMemories.find(key) == consolidatedMemories.end()) {
            consolidatedMemories[key] = memory;
        } else {
            // Merge similar memories
            auto& existing = consolidatedMemories[key];
            existing.Importance = std::max(existing.Importance, memory.Importance);
            existing.EmotionalWeight = std::max(existing.EmotionalWeight, memory.EmotionalWeight);
            for (const auto& [trait, influence] : memory.TraitInfluences) {
                existing.TraitInfluences[trait] += influence;
            }
        }
    }

    // Update long-term memories
    memoryContext.LongTermMemories.clear();
    for (const auto& [_, memory] : consolidatedMemories) {
        memoryContext.LongTermMemories.push_back(memory);
    }
}

void PersonaSystem::MoveToLongTerm(const MemoryEvent& memory) {
    if (!activePersona_) return;

    // Only move significant memories
    if (memory.Importance > 0.5 || memory.EmotionalWeight > 0.7) {
        activePersona_->Memory.LongTermMemories.push_back(memory);
    }
}

void PersonaSystem::DecayShortTermMemories() {
    if (!activePersona_) return;

    auto& shortTermMemories = activePersona_->Memory.ShortTermMemories;
    auto now = std::chrono::system_clock::now();

    // Remove memories older than 24 hours
    shortTermMemories.erase(
        std::remove_if(shortTermMemories.begin(), shortTermMemories.end(),
            [&now](const MemoryEvent& memory) {
                return std::chrono::duration_cast<std::chrono::hours>(
                    now - memory.Timestamp).count() > 24;
            }),
        shortTermMemories.end()
    );
}

void PersonaSystem::UpdateMemoryImportance(MemoryEvent& memory) {
    // Calculate importance based on emotional weight and trait influence
    double importance = memory.EmotionalWeight;
    for (const auto& [_, influence] : memory.TraitInfluences) {
        importance += std::abs(influence) * 0.5;
    }
    memory.Importance = std::min(1.0, importance);
}

void PersonaSystem::CalculateMemoryInfluence() {
    if (!activePersona_) return;

    const auto& memoryContext = activePersona_->Memory;
    auto now = std::chrono::system_clock::now();

    // Process recent memories
    for (const auto& memory : memoryContext.ShortTermMemories) {
        UpdateTraitFromMemory(memory);
    }

    // Process significant long-term memories
    for (const auto& memory : memoryContext.LongTermMemories) {
        if (memory.Importance > 0.7) {
            UpdateTraitFromMemory(memory);
        }
    }
}

void PersonaSystem::ProcessEmotionalResonance(const std::shared_ptr<Interaction>& interaction) {
    if (!activePersona_) return;

    auto& memoryContext = activePersona_->Memory;
    auto now = std::chrono::system_clock::now();

    // Calculate emotional impact of interaction
    double emotionalImpact = 0.0;
    if (interaction->Data.find("emotional_content") != interaction->Data.end()) {
        emotionalImpact = std::any_cast<double>(interaction->Data.at("emotional_content"));
    }

    // Create new resonance
    EmotionalResonance resonance;
    resonance.Intensity = emotionalImpact;
    resonance.Duration = 1.0; // Base duration in hours
    resonance.Trigger = interaction->Type;
    resonance.StartTime = now;
    resonance.PeakTime = now + std::chrono::hours(1);

    // Find associated memories
    auto relevantMemories = RecallRelevantMemories(interaction->Type);
    for (const auto& memory : relevantMemories) {
        if (memory.EmotionalWeight > 0.5) {
            resonance.AssociatedMemories.push_back(memory.Content);
        }
    }

    memoryContext.ActiveResonances.push_back(resonance);

    // Update emotional patterns
    UpdateEmotionalPatterns();
}

void PersonaSystem::UpdateEmotionalPatterns() {
    if (!activePersona_) return;

    auto& memoryContext = activePersona_->Memory;
    auto now = std::chrono::system_clock::now();

    // Process active resonances
    for (auto& resonance : memoryContext.ActiveResonances) {
        auto timeSinceStart = std::chrono::duration_cast<std::chrono::hours>(
            now - resonance.StartTime).count();
        
        // Update intensity based on time
        double timeFactor = std::exp(-0.1 * timeSinceStart);
        resonance.Intensity *= timeFactor;

        // Check if resonance should end
        if (resonance.Intensity < 0.1) {
            // Create emotional pattern if significant
            if (resonance.Intensity > 0.5) {
                EmotionalPattern pattern;
                pattern.PatternType = resonance.Trigger;
                pattern.BaseIntensity = resonance.Intensity;
                pattern.CurrentIntensity = resonance.Intensity;
                pattern.LastTriggered = now;
                pattern.PatternMemories = {};
                
                // Add associated memories
                for (const auto& memoryContent : resonance.AssociatedMemories) {
                    for (const auto& memory : memoryContext.ShortTermMemories) {
                        if (memory.Content == memoryContent) {
                            pattern.PatternMemories.push_back(memory);
                            break;
                        }
                    }
                }
                
                memoryContext.EmotionalPatterns.push_back(pattern);
            }
        }
    }

    // Remove expired resonances
    memoryContext.ActiveResonances.erase(
        std::remove_if(memoryContext.ActiveResonances.begin(), 
                      memoryContext.ActiveResonances.end(),
            [](const EmotionalResonance& r) { return r.Intensity < 0.1; }),
        memoryContext.ActiveResonances.end()
    );
}

void PersonaSystem::CreateMemoryConnections() {
    if (!activePersona_) return;

    auto& memoryContext = activePersona_->Memory;
    
    // Combine all memories
    std::vector<MemoryEvent> allMemories = memoryContext.ShortTermMemories;
    allMemories.insert(allMemories.end(), 
                      memoryContext.LongTermMemories.begin(), 
                      memoryContext.LongTermMemories.end());

    // Create connections between similar memories
    for (size_t i = 0; i < allMemories.size(); ++i) {
        for (size_t j = i + 1; j < allMemories.size(); ++j) {
            const auto& mem1 = allMemories[i];
            const auto& mem2 = allMemories[j];

            // Calculate connection strength
            double strength = 0.0;
            std::vector<std::string> sharedTraits;

            // Check shared traits
            for (const auto& [trait, _] : mem1.TraitInfluences) {
                if (mem2.TraitInfluences.find(trait) != mem2.TraitInfluences.end()) {
                    strength += 0.3;
                    sharedTraits.push_back(trait);
                }
            }

            // Check shared tags
            for (const auto& tag : mem1.Tags) {
                if (std::find(mem2.Tags.begin(), mem2.Tags.end(), tag) != mem2.Tags.end()) {
                    strength += 0.2;
                }
            }

            // Check emotional similarity
            if (std::abs(mem1.EmotionalWeight - mem2.EmotionalWeight) < 0.2) {
                strength += 0.2;
            }

            if (strength > 0.5) {
                MemoryConnection connection;
                connection.SourceMemory = mem1.Content;
                connection.TargetMemory = mem2.Content;
                connection.Strength = strength;
                connection.ConnectionType = "emotional";
                connection.SharedTraits = sharedTraits;

                memoryContext.MemoryConnections.push_back(connection);
            }
        }
    }
}

void PersonaSystem::ProcessEmotionalTriggers(const std::shared_ptr<Interaction>& interaction) {
    if (!activePersona_) return;

    auto& memoryContext = activePersona_->Memory;
    auto& persona = *activePersona_;

    // Check for emotional triggers in interaction
    for (const auto& [trigger, weight] : persona.EmotionalTriggers) {
        if (interaction->Data.find(trigger) != interaction->Data.end()) {
            // Update emotional baseline
            persona.EmotionalBaselines[trigger] = 
                0.7 * persona.EmotionalBaselines[trigger] + 
                0.3 * std::any_cast<double>(interaction->Data.at(trigger));

            // Update memory weights for this emotion
            memoryContext.EmotionalTriggers[trigger] = 
                0.8 * memoryContext.EmotionalTriggers[trigger] + 
                0.2 * weight;
        }
    }
}

void PersonaSystem::UpdateEmotionalBaselines() {
    if (!activePersona_) return;

    auto& persona = *activePersona_;
    auto now = std::chrono::system_clock::now();

    // Update emotional baselines over time
    for (auto& [emotion, baseline] : persona.EmotionalBaselines) {
        // Calculate time-based decay
        auto timeSinceLastUpdate = std::chrono::duration_cast<std::chrono::hours>(
            now - persona.LastUsed).count();
        double decayFactor = std::exp(-0.01 * timeSinceLastUpdate);
        
        // Apply decay and return to baseline
        baseline = 0.5 + (baseline - 0.5) * decayFactor;
    }
}

void PersonaSystem::ProcessMemoryClusters() {
    if (!activePersona_) return;

    auto& memoryContext = activePersona_->Memory;
    
    // Group memories by emotional similarity
    std::vector<std::vector<MemoryEvent>> clusters;
    std::vector<bool> processed(memoryContext.ShortTermMemories.size(), false);

    for (size_t i = 0; i < memoryContext.ShortTermMemories.size(); ++i) {
        if (processed[i]) continue;

        std::vector<MemoryEvent> cluster;
        cluster.push_back(memoryContext.ShortTermMemories[i]);
        processed[i] = true;

        // Find similar memories
        for (size_t j = i + 1; j < memoryContext.ShortTermMemories.size(); ++j) {
            if (processed[j]) continue;

            const auto& mem1 = memoryContext.ShortTermMemories[i];
            const auto& mem2 = memoryContext.ShortTermMemories[j];

            // Calculate similarity
            double similarity = 0.0;
            
            // Check emotional weight similarity
            similarity += 0.4 * (1.0 - std::abs(mem1.EmotionalWeight - mem2.EmotionalWeight));
            
            // Check shared traits
            for (const auto& [trait, _] : mem1.TraitInfluences) {
                if (mem2.TraitInfluences.find(trait) != mem2.TraitInfluences.end()) {
                    similarity += 0.3;
                }
            }

            if (similarity > 0.6) {
                cluster.push_back(mem2);
                processed[j] = true;
            }
        }

        if (cluster.size() > 1) {
            clusters.push_back(cluster);
        }
    }

    // Process each cluster
    for (const auto& cluster : clusters) {
        ProcessMemoryCluster(cluster);
    }
}

void PersonaSystem::ProcessMemoryCluster(const std::vector<MemoryEvent>& cluster) {
    if (!activePersona_) return;

    // Calculate cluster properties
    double avgEmotionalWeight = 0.0;
    std::map<std::string, double> traitInfluences;
    std::vector<std::string> sharedTags;

    for (const auto& memory : cluster) {
        avgEmotionalWeight += memory.EmotionalWeight;
        
        // Aggregate trait influences
        for (const auto& [trait, influence] : memory.TraitInfluences) {
            traitInfluences[trait] += influence;
        }

        // Find shared tags
        if (sharedTags.empty()) {
            sharedTags = memory.Tags;
        } else {
            std::vector<std::string> newSharedTags;
            std::set_intersection(
                sharedTags.begin(), sharedTags.end(),
                memory.Tags.begin(), memory.Tags.end(),
                std::back_inserter(newSharedTags)
            );
            sharedTags = newSharedTags;
        }
    }

    avgEmotionalWeight /= cluster.size();

    // Create new emotional pattern if significant
    if (avgEmotionalWeight > 0.6 && !sharedTags.empty()) {
        EmotionalPattern pattern;
        pattern.PatternType = "memory_cluster";
        pattern.BaseIntensity = avgEmotionalWeight;
        pattern.CurrentIntensity = avgEmotionalWeight;
        pattern.PatternMemories = cluster;
        pattern.LastTriggered = std::chrono::system_clock::now();
        pattern.Triggers = sharedTags;

        activePersona_->Memory.EmotionalPatterns.push_back(pattern);
    }
}

void PersonaSystem::UpdateMemoryWeightsWithEmotion() {
    if (!activePersona_) return;

    auto& memoryContext = activePersona_->Memory;
    auto now = std::chrono::system_clock::now();

    // Update memory weights based on emotional patterns
    for (auto& memory : memoryContext.ShortTermMemories) {
        double emotionalBoost = 0.0;
        
        // Check active resonances
        for (const auto& resonance : memoryContext.ActiveResonances) {
            if (std::find(resonance.AssociatedMemories.begin(),
                         resonance.AssociatedMemories.end(),
                         memory.Content) != resonance.AssociatedMemories.end()) {
                emotionalBoost += resonance.Intensity * 0.3;
            }
        }

        // Check emotional patterns
        for (const auto& pattern : memoryContext.EmotionalPatterns) {
            for (const auto& patternMemory : pattern.PatternMemories) {
                if (patternMemory.Content == memory.Content) {
                    emotionalBoost += pattern.CurrentIntensity * 0.2;
                    break;
                }
            }
        }

        // Update memory importance
        memory.Importance = std::min(1.0, memory.Importance + emotionalBoost);
    }
}

void PersonaSystem::ProcessPatternRecognition() {
    if (!activePersona_) return;

    auto& memoryContext = activePersona_->Memory;
    auto now = std::chrono::system_clock::now();

    // Analyze emotional patterns over time
    std::map<std::string, std::vector<double>> patternIntensities;
    std::map<std::string, int> patternFrequencies;

    for (const auto& pattern : memoryContext.EmotionalPatterns) {
        // Track pattern intensities over time
        patternIntensities[pattern.PatternType].push_back(pattern.CurrentIntensity);
        
        // Count pattern occurrences
        patternFrequencies[pattern.PatternType]++;
    }

    // Identify significant patterns
    for (const auto& [patternType, intensities] : patternIntensities) {
        if (patternFrequencies[patternType] > 3) { // Require at least 3 occurrences
            double avgIntensity = 0.0;
            for (double intensity : intensities) {
                avgIntensity += intensity;
            }
            avgIntensity /= intensities.size();

            if (avgIntensity > 0.6) { // Significant average intensity
                // Create or update core pattern
                EmotionalPattern corePattern;
                corePattern.PatternType = patternType;
                corePattern.BaseIntensity = avgIntensity;
                corePattern.CurrentIntensity = avgIntensity;
                corePattern.LastTriggered = now;

                // Find common triggers and memories
                std::vector<std::string> commonTriggers;
                std::vector<MemoryEvent> commonMemories;

                for (const auto& pattern : memoryContext.EmotionalPatterns) {
                    if (pattern.PatternType == patternType) {
                        // Find common triggers
                        if (commonTriggers.empty()) {
                            commonTriggers = pattern.Triggers;
                        } else {
                            std::vector<std::string> newCommonTriggers;
                            std::set_intersection(
                                commonTriggers.begin(), commonTriggers.end(),
                                pattern.Triggers.begin(), pattern.Triggers.end(),
                                std::back_inserter(newCommonTriggers)
                            );
                            commonTriggers = newCommonTriggers;
                        }

                        // Add unique memories
                        for (const auto& memory : pattern.PatternMemories) {
                            bool isUnique = true;
                            for (const auto& existing : commonMemories) {
                                if (existing.Content == memory.Content) {
                                    isUnique = false;
                                    break;
                                }
                            }
                            if (isUnique) {
                                commonMemories.push_back(memory);
                            }
                        }
                    }
                }

                corePattern.Triggers = commonTriggers;
                corePattern.PatternMemories = commonMemories;

                // Add to core patterns if not already present
                bool isNewPattern = true;
                for (auto& existing : activePersona_->CorePatterns) {
                    if (existing.PatternType == patternType) {
                        existing = corePattern;
                        isNewPattern = false;
                        break;
                    }
                }
                if (isNewPattern) {
                    activePersona_->CorePatterns.push_back(corePattern);
                }
            }
        }
    }
}

void PersonaSystem::UpdateMemoryAssociations() {
    if (!activePersona_) return;

    auto& memoryContext = activePersona_->Memory;
    auto now = std::chrono::system_clock::now();

    // Update memory connection strengths based on access patterns
    for (auto& connection : memoryContext.MemoryConnections) {
        // Check if either memory was recently accessed
        bool sourceAccessed = false;
        bool targetAccessed = false;

        for (const auto& memory : memoryContext.ShortTermMemories) {
            if (memory.Content == connection.SourceMemory) {
                sourceAccessed = true;
            }
            if (memory.Content == connection.TargetMemory) {
                targetAccessed = true;
            }
        }

        // Strengthen connection if both memories were accessed
        if (sourceAccessed && targetAccessed) {
            connection.Strength = std::min(1.0, connection.Strength + 0.1);
        }

        // Weaken connection over time if not accessed
        double timeSinceLastAccess = std::chrono::duration_cast<std::chrono::hours>(
            now - memoryContext.LastMemoryUpdate).count();
        if (timeSinceLastAccess > 24) {
            connection.Strength = std::max(0.1, connection.Strength - 0.01);
        }
    }

    // Create new connections for frequently accessed memories
    std::map<std::string, int> accessCounts;
    for (const auto& memory : memoryContext.ShortTermMemories) {
        accessCounts[memory.Content]++;
    }

    for (const auto& [content1, count1] : accessCounts) {
        for (const auto& [content2, count2] : accessCounts) {
            if (content1 != content2 && count1 > 2 && count2 > 2) {
                // Check if connection already exists
                bool connectionExists = false;
                for (const auto& connection : memoryContext.MemoryConnections) {
                    if ((connection.SourceMemory == content1 && connection.TargetMemory == content2) ||
                        (connection.SourceMemory == content2 && connection.TargetMemory == content1)) {
                        connectionExists = true;
                        break;
                    }
                }

                if (!connectionExists) {
                    // Create new connection
                    MemoryConnection newConnection;
                    newConnection.SourceMemory = content1;
                    newConnection.TargetMemory = content2;
                    newConnection.Strength = 0.3; // Initial strength
                    newConnection.ConnectionType = "frequent_access";
                    
                    // Find shared traits
                    for (const auto& memory : memoryContext.ShortTermMemories) {
                        if (memory.Content == content1 || memory.Content == content2) {
                            for (const auto& [trait, _] : memory.TraitInfluences) {
                                newConnection.SharedTraits.push_back(trait);
                            }
                        }
                    }

                    memoryContext.MemoryConnections.push_back(newConnection);
                }
            }
        }
    }
}

void PersonaSystem::UpdateEmotionalConnections() {
    if (!activePersona_) return;

    auto& memoryContext = activePersona_->Memory;
    auto& persona = *activePersona_;

    // Track emotional state transitions
    std::map<std::string, std::map<std::string, int>> stateTransitions;
    std::string lastEmotion = "";

    for (const auto& pattern : memoryContext.EmotionalPatterns) {
        if (!lastEmotion.empty()) {
            stateTransitions[lastEmotion][pattern.PatternType]++;
        }
        lastEmotion = pattern.PatternType;
    }

    // Update emotional baselines based on transitions
    for (const auto& [fromEmotion, transitions] : stateTransitions) {
        for (const auto& [toEmotion, count] : transitions) {
            if (count > 2) { // Significant number of transitions
                // Update emotional triggers
                persona.EmotionalTriggers[fromEmotion + "_to_" + toEmotion] = 
                    0.7 * persona.EmotionalTriggers[fromEmotion + "_to_" + toEmotion] + 
                    0.3 * (count / 10.0);

                // Update memory weights for this transition
                memoryContext.EmotionalTriggers[fromEmotion + "_to_" + toEmotion] = 
                    0.8 * memoryContext.EmotionalTriggers[fromEmotion + "_to_" + toEmotion] + 
                    0.2 * (count / 10.0);
            }
        }
    }

    // Update emotional patterns based on transitions
    for (auto& pattern : memoryContext.EmotionalPatterns) {
        // Check if this pattern is part of a common transition
        for (const auto& [fromEmotion, transitions] : stateTransitions) {
            if (transitions.find(pattern.PatternType) != transitions.end() && 
                transitions.at(pattern.PatternType) > 2) {
                // Add transition trigger
                pattern.Triggers.push_back(fromEmotion + "_transition");
                
                // Adjust intensity based on transition frequency
                pattern.BaseIntensity = std::min(1.0, 
                    pattern.BaseIntensity + (transitions.at(pattern.PatternType) * 0.1));
            }
        }
    }
}

void PersonaSystem::ProcessSelfReflection() {
    if (!activePersona_) return;

    auto& memoryContext = activePersona_->Memory;
    auto& persona = *activePersona_;
    auto now = std::chrono::system_clock::now();

    // Analyze recent emotional experiences
    std::vector<EmotionalPattern> recentPatterns;
    for (const auto& pattern : memoryContext.EmotionalPatterns) {
        auto timeSinceLast = std::chrono::duration_cast<std::chrono::hours>(
            now - pattern.LastTriggered).count();
        if (timeSinceLast < 24) {
            recentPatterns.push_back(pattern);
        }
    }

    // Identify patterns in recent experiences
    std::map<std::string, double> emotionalTrends;
    std::map<std::string, std::vector<std::string>> commonTriggers;
    std::map<std::string, int> responseCounts;

    for (const auto& pattern : recentPatterns) {
        // Track emotional trends
        emotionalTrends[pattern.PatternType] += pattern.CurrentIntensity;
        
        // Track common triggers
        for (const auto& trigger : pattern.Triggers) {
            commonTriggers[pattern.PatternType].push_back(trigger);
        }
        
        // Track response patterns
        for (const auto& memory : pattern.PatternMemories) {
            responseCounts[memory.Type]++;
        }
    }

    // Generate insights
    std::vector<SelfReflection> newInsights;

    // Analyze emotional trends
    for (const auto& [emotion, intensity] : emotionalTrends) {
        if (intensity > 0.7) {
            SelfReflection insight;
            insight.Type = "emotional_trend";
            insight.Content = "I've been experiencing strong " + emotion + " emotions recently";
            insight.Confidence = intensity;
            insight.Timestamp = now;
            insight.RelatedPatterns = {emotion};
            newInsights.push_back(insight);
        }
    }

    // Analyze trigger patterns
    for (const auto& [emotion, triggers] : commonTriggers) {
        if (triggers.size() > 2) {
            SelfReflection insight;
            insight.Type = "trigger_pattern";
            insight.Content = "I notice that " + emotion + " is often triggered by similar situations";
            insight.Confidence = triggers.size() / 10.0;
            insight.Timestamp = now;
            insight.RelatedPatterns = {emotion};
            newInsights.push_back(insight);
        }
    }

    // Analyze response patterns
    for (const auto& [responseType, count] : responseCounts) {
        if (count > 3) {
            SelfReflection insight;
            insight.Type = "response_pattern";
            insight.Content = "I tend to respond with " + responseType + " in emotional situations";
            insight.Confidence = count / 10.0;
            insight.Timestamp = now;
            insight.RelatedPatterns = {responseType};
            newInsights.push_back(insight);
        }
    }

    // Update persona based on insights
    for (const auto& insight : newInsights) {
        UpdatePersonaFromInsight(insight);
        memoryContext.SelfReflections.push_back(insight);
    }
}

void PersonaSystem::UpdatePersonaFromInsight(const SelfReflection& insight) {
    if (!activePersona_) return;

    auto& persona = *activePersona_;

    if (insight.Type == "emotional_trend") {
        // Update emotional baselines
        for (const auto& pattern : insight.RelatedPatterns) {
            persona.EmotionalBaselines[pattern] = 
                0.8 * persona.EmotionalBaselines[pattern] + 
                0.2 * insight.Confidence;
        }
    }
    else if (insight.Type == "trigger_pattern") {
        // Update emotional triggers
        for (const auto& pattern : insight.RelatedPatterns) {
            persona.EmotionalTriggers[pattern] = 
                0.7 * persona.EmotionalTriggers[pattern] + 
                0.3 * insight.Confidence;
        }
    }
    else if (insight.Type == "response_pattern") {
        // Update response biases
        for (const auto& pattern : insight.RelatedPatterns) {
            persona.ResponseBiases[pattern] = 
                0.6 * persona.ResponseBiases[pattern] + 
                0.4 * insight.Confidence;
        }
    }
}

void PersonaSystem::ProcessLongTermReflection() {
    if (!activePersona_) return;

    auto& memoryContext = activePersona_->Memory;
    auto now = std::chrono::system_clock::now();

    // Analyze self-reflections over time
    std::map<std::string, std::vector<SelfReflection>> groupedInsights;
    for (const auto& reflection : memoryContext.SelfReflections) {
        groupedInsights[reflection.Type].push_back(reflection);
    }

    // Generate long-term insights
    for (const auto& [type, insights] : groupedInsights) {
        if (insights.size() > 5) { // Need significant number of insights
            double avgConfidence = 0.0;
            std::set<std::string> allPatterns;
            
            for (const auto& insight : insights) {
                avgConfidence += insight.Confidence;
                allPatterns.insert(insight.RelatedPatterns.begin(), 
                                 insight.RelatedPatterns.end());
            }
            avgConfidence /= insights.size();

            if (avgConfidence > 0.6) {
                SelfReflection longTermInsight;
                longTermInsight.Type = "long_term_" + type;
                longTermInsight.Content = GenerateLongTermInsight(type, allPatterns, avgConfidence);
                longTermInsight.Confidence = avgConfidence;
                longTermInsight.Timestamp = now;
                longTermInsight.RelatedPatterns = std::vector<std::string>(
                    allPatterns.begin(), allPatterns.end());

                memoryContext.LongTermInsights.push_back(longTermInsight);
                UpdatePersonaFromLongTermInsight(longTermInsight);
            }
        }
    }
}

std::string PersonaSystem::GenerateLongTermInsight(
    const std::string& type, 
    const std::set<std::string>& patterns,
    double confidence) {
    
    std::string insight;
    
    if (type == "emotional_trend") {
        insight = "Over time, I've noticed that I consistently experience ";
        for (const auto& pattern : patterns) {
            insight += pattern + ", ";
        }
        insight += "with significant intensity";
    }
    else if (type == "trigger_pattern") {
        insight = "I've learned that certain situations consistently trigger ";
        for (const auto& pattern : patterns) {
            insight += pattern + ", ";
        }
        insight += "in me";
    }
    else if (type == "response_pattern") {
        insight = "My typical responses to emotional situations include ";
        for (const auto& pattern : patterns) {
            insight += pattern + ", ";
        }
        insight += "which seems to be a consistent part of how I handle emotions";
    }

    return insight;
}

void PersonaSystem::UpdatePersonaFromLongTermInsight(const SelfReflection& insight) {
    if (!activePersona_) return;

    auto& persona = *activePersona_;

    // Update core personality traits based on long-term insights
    for (const auto& pattern : insight.RelatedPatterns) {
        if (insight.Type.find("emotional_trend") != std::string::npos) {
            // Update emotional baselines more significantly for long-term patterns
            persona.EmotionalBaselines[pattern] = 
                0.6 * persona.EmotionalBaselines[pattern] + 
                0.4 * insight.Confidence;
        }
        else if (insight.Type.find("trigger_pattern") != std::string::npos) {
            // Update emotional triggers more significantly
            persona.EmotionalTriggers[pattern] = 
                0.5 * persona.EmotionalTriggers[pattern] + 
                0.5 * insight.Confidence;
        }
        else if (insight.Type.find("response_pattern") != std::string::npos) {
            // Update response biases more significantly
            persona.ResponseBiases[pattern] = 
                0.4 * persona.ResponseBiases[pattern] + 
                0.6 * insight.Confidence;
        }
    }
}

} // namespace shandris::cognitive 