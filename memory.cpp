#include "memory.hpp"
#include "database.hpp"
#include <algorithm>
#include <cmath>
#include <random>
#include <sstream>
#include <iomanip>
#include <format>

namespace shandris {
namespace cognitive {

MemoryManager::MemoryManager() {
    db_ = std::make_shared<database::Database>();
    maxCacheSize_ = 1000;
}

MemoryManager::~MemoryManager() {
    // Cleanup if needed
}

bool MemoryManager::Initialize() {
    if (!db_->Initialize()) {
        return false;
    }
    is_initialized_ = true;
    return true;
}

bool MemoryManager::SaveMemory(const MemoryEvent& memory) {
    if (!is_initialized_) return false;
    
    std::string query = "INSERT INTO memories (id, content, context, importance, emotional_weight, trait_influences, tags, created_at, updated_at) "
                       "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)";
    
    std::vector<std::string> params = {
        memory.id,
        memory.content,
        memory.context,
        std::to_string(memory.importance),
        std::to_string(memory.emotional_weight),
        nlohmann::json(memory.trait_influences).dump(),
        nlohmann::json(memory.tags).dump(),
        std::to_string(std::chrono::system_clock::to_time_t(memory.created_at)),
        std::to_string(std::chrono::system_clock::to_time_t(memory.updated_at))
    };
    
    if (!db_->ExecuteQueryWithParams(query, params)) {
        return false;
    }
    
    // Update cache
    memories_[memory.id] = memory;
    UpdateMemoryIndex("default", memory);
    UpdateMemoryCluster("default", memory);
    
    return true;
}

bool MemoryManager::LoadMemory(const std::string& id, MemoryEvent& memory) {
    if (!is_initialized_) return false;
    
    // Check cache first
    auto it = memories_.find(id);
    if (it != memories_.end()) {
        memory = it->second;
        return true;
    }
    
    std::string query = "SELECT * FROM memories WHERE id = ?";
    auto result = db_->ExecuteQueryWithResultAndParams(query, {id});
    
    if (result.empty()) {
        return false;
    }
    
    memory.id = result[0]["id"];
    memory.content = result[0]["content"];
    memory.context = result[0]["context"];
    memory.importance = std::stod(result[0]["importance"]);
    memory.emotional_weight = std::stod(result[0]["emotional_weight"]);
    memory.trait_influences = nlohmann::json::parse(result[0]["trait_influences"]).get<std::map<std::string, double>>();
    memory.tags = nlohmann::json::parse(result[0]["tags"]).get<std::set<std::string>>();
    memory.created_at = std::chrono::system_clock::from_time_t(std::stoll(result[0]["created_at"]));
    memory.updated_at = std::chrono::system_clock::from_time_t(std::stoll(result[0]["updated_at"]));
    
    // Update cache
    memories_[id] = memory;
    
    return true;
}

bool MemoryManager::UpdateMemory(const MemoryEvent& memory) {
    if (!is_initialized_) return false;
    
    std::string query = "UPDATE memories SET content = ?, context = ?, importance = ?, emotional_weight = ?, "
                       "trait_influences = ?, tags = ?, updated_at = ? WHERE id = ?";
    
    std::vector<std::string> params = {
        memory.content,
        memory.context,
        std::to_string(memory.importance),
        std::to_string(memory.emotional_weight),
        nlohmann::json(memory.trait_influences).dump(),
        nlohmann::json(memory.tags).dump(),
        std::to_string(std::chrono::system_clock::to_time_t(memory.updated_at)),
        memory.id
    };
    
    if (!db_->ExecuteQueryWithParams(query, params)) {
        return false;
    }
    
    // Update cache
    memories_[memory.id] = memory;
    UpdateMemoryIndex("default", memory);
    UpdateMemoryCluster("default", memory);
    
    return true;
}

bool MemoryManager::DeleteMemory(const std::string& id) {
    if (!is_initialized_) return false;
    
    std::string query = "DELETE FROM memories WHERE id = ?";
    if (!db_->ExecuteQueryWithParams(query, {id})) {
        return false;
    }
    
    // Update cache
    memories_.erase(id);
    RemoveMemory(id);
    
    return true;
}

bool MemoryManager::SaveEmotionalState(const EmotionalState& state) {
    try {
        // Convert state to JSON
        nlohmann::json j = state;
        
        // Prepare SQL statement
        const char* sql = R"(
            INSERT INTO emotional_states (
                id, happiness, sadness, anger, fear, surprise,
                disgust, trust, anticipation, timestamp
            ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        )";
        
        // Execute SQL with parameters
        if (!db_->ExecuteSQL(sql, {
            state.id,
            std::to_string(state.happiness),
            std::to_string(state.sadness),
            std::to_string(state.anger),
            std::to_string(state.fear),
            std::to_string(state.surprise),
            std::to_string(state.disgust),
            std::to_string(state.trust),
            std::to_string(state.anticipation),
            std::to_string(std::chrono::system_clock::to_time_t(state.timestamp))
        })) {
            return false;
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error saving emotional state: " << e.what() << std::endl;
        return false;
    }
}

bool MemoryManager::LoadEmotionalState(const std::string& stateId, EmotionalState& state) {
    try {
        // Prepare SQL statement
        const char* sql = "SELECT * FROM emotional_states WHERE id = ?";
        
        // Execute SQL with parameters
        auto result = db_->ExecuteQuery(sql, {stateId});
        if (result.empty()) {
            return false;
        }
        
        // Parse result
        state.id = result[0]["id"];
        state.happiness = std::stod(result[0]["happiness"]);
        state.sadness = std::stod(result[0]["sadness"]);
        state.anger = std::stod(result[0]["anger"]);
        state.fear = std::stod(result[0]["fear"]);
        state.surprise = std::stod(result[0]["surprise"]);
        state.disgust = std::stod(result[0]["disgust"]);
        state.trust = std::stod(result[0]["trust"]);
        state.anticipation = std::stod(result[0]["anticipation"]);
        state.timestamp = std::chrono::system_clock::from_time_t(std::stoll(result[0]["timestamp"]));
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error loading emotional state: " << e.what() << std::endl;
        return false;
    }
}

bool MemoryManager::UpdateEmotionalState(const EmotionalState& state) {
    try {
        // Prepare SQL statement
        const char* sql = R"(
            UPDATE emotional_states SET
                happiness = ?,
                sadness = ?,
                anger = ?,
                fear = ?,
                surprise = ?,
                disgust = ?,
                trust = ?,
                anticipation = ?,
                timestamp = ?
            WHERE id = ?
        )";
        
        // Execute SQL with parameters
        if (!db_->ExecuteSQL(sql, {
            std::to_string(state.happiness),
            std::to_string(state.sadness),
            std::to_string(state.anger),
            std::to_string(state.fear),
            std::to_string(state.surprise),
            std::to_string(state.disgust),
            std::to_string(state.trust),
            std::to_string(state.anticipation),
            std::to_string(std::chrono::system_clock::to_time_t(state.timestamp)),
            state.id
        })) {
            return false;
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error updating emotional state: " << e.what() << std::endl;
        return false;
    }
}

bool MemoryManager::DeleteEmotionalState(const std::string& stateId) {
    try {
        // Prepare SQL statement
        const char* sql = "DELETE FROM emotional_states WHERE id = ?";
        
        // Execute SQL with parameters
        if (!db_->ExecuteSQL(sql, {stateId})) {
            return false;
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error deleting emotional state: " << e.what() << std::endl;
        return false;
    }
}

bool MemoryManager::SaveTraits(const SapphicTraits& traits) {
    try {
        // Convert traits to JSON
        nlohmann::json j = traits;
        
        // Prepare SQL statement
        const char* sql = R"(
            INSERT INTO sapphic_traits (
                id, seductiveness, intellectuality, protectiveness,
                clinginess, independence, playfulness, sassiness,
                emotional_depth, confidence, sensitivity, lesbian_identity,
                feminine_attraction, sapphic_energy
            ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        )";
        
        // Execute SQL with parameters
        if (!db_->ExecuteSQL(sql, {
            traits.id,
            std::to_string(traits.seductiveness),
            std::to_string(traits.intellectuality),
            std::to_string(traits.protectiveness),
            std::to_string(traits.clinginess),
            std::to_string(traits.independence),
            std::to_string(traits.playfulness),
            std::to_string(traits.sassiness),
            std::to_string(traits.emotional_depth),
            std::to_string(traits.confidence),
            std::to_string(traits.sensitivity),
            std::to_string(traits.lesbian_identity),
            std::to_string(traits.feminine_attraction),
            std::to_string(traits.sapphic_energy)
        })) {
            return false;
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error saving traits: " << e.what() << std::endl;
        return false;
    }
}

bool MemoryManager::LoadTraits(const std::string& traitsId, SapphicTraits& traits) {
    try {
        // Prepare SQL statement
        const char* sql = "SELECT * FROM sapphic_traits WHERE id = ?";
        
        // Execute SQL with parameters
        auto result = db_->ExecuteQuery(sql, {traitsId});
        if (result.empty()) {
            return false;
        }
        
        // Parse result
        traits.id = result[0]["id"];
        traits.seductiveness = std::stod(result[0]["seductiveness"]);
        traits.intellectuality = std::stod(result[0]["intellectuality"]);
        traits.protectiveness = std::stod(result[0]["protectiveness"]);
        traits.clinginess = std::stod(result[0]["clinginess"]);
        traits.independence = std::stod(result[0]["independence"]);
        traits.playfulness = std::stod(result[0]["playfulness"]);
        traits.sassiness = std::stod(result[0]["sassiness"]);
        traits.emotional_depth = std::stod(result[0]["emotional_depth"]);
        traits.confidence = std::stod(result[0]["confidence"]);
        traits.sensitivity = std::stod(result[0]["sensitivity"]);
        traits.lesbian_identity = std::stod(result[0]["lesbian_identity"]);
        traits.feminine_attraction = std::stod(result[0]["feminine_attraction"]);
        traits.sapphic_energy = std::stod(result[0]["sapphic_energy"]);
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error loading traits: " << e.what() << std::endl;
        return false;
    }
}

bool MemoryManager::UpdateTraits(const SapphicTraits& traits) {
    try {
        // Prepare SQL statement
        const char* sql = R"(
            UPDATE sapphic_traits SET
                seductiveness = ?,
                intellectuality = ?,
                protectiveness = ?,
                clinginess = ?,
                independence = ?,
                playfulness = ?,
                sassiness = ?,
                emotional_depth = ?,
                confidence = ?,
                sensitivity = ?,
                lesbian_identity = ?,
                feminine_attraction = ?,
                sapphic_energy = ?
            WHERE id = ?
        )";
        
        // Execute SQL with parameters
        if (!db_->ExecuteSQL(sql, {
            std::to_string(traits.seductiveness),
            std::to_string(traits.intellectuality),
            std::to_string(traits.protectiveness),
            std::to_string(traits.clinginess),
            std::to_string(traits.independence),
            std::to_string(traits.playfulness),
            std::to_string(traits.sassiness),
            std::to_string(traits.emotional_depth),
            std::to_string(traits.confidence),
            std::to_string(traits.sensitivity),
            std::to_string(traits.lesbian_identity),
            std::to_string(traits.feminine_attraction),
            std::to_string(traits.sapphic_energy),
            traits.id
        })) {
            return false;
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error updating traits: " << e.what() << std::endl;
        return false;
    }
}

bool MemoryManager::DeleteTraits(const std::string& traitsId) {
    try {
        // Prepare SQL statement
        const char* sql = "DELETE FROM sapphic_traits WHERE id = ?";
        
        // Execute SQL with parameters
        if (!db_->ExecuteSQL(sql, {traitsId})) {
            return false;
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error deleting traits: " << e.what() << std::endl;
        return false;
    }
}

std::map<std::string, std::string> RecallTraits(const std::string& sessionID) {
    std::map<std::string, std::string> traits;

    // TODO: Implement database retrieval
    // std::string traitsStr = Database::GetInstance().RecallTraits(sessionID);
    // nlohmann::json traitsJson = nlohmann::json::parse(traitsStr);
    // traits = traitsJson.get<std::map<std::string, std::string>>();

    return traits;
}

std::string extractName(const std::string& prompt) {
    std::string lowerPrompt = prompt;
    std::transform(lowerPrompt.begin(), lowerPrompt.end(), lowerPrompt.begin(), ::tolower);
    
    size_t idx = lowerPrompt.find("my name is");
    if (idx == std::string::npos) {
        return "";
    }
    
    std::string namePart = prompt.substr(idx + 10);
    namePart = std::regex_replace(namePart, std::regex(R"(^\s+)"), "");
    
    size_t spacePos = namePart.find(' ');
    if (spacePos != std::string::npos) {
        namePart = namePart.substr(0, spacePos);
    }
    
    if (!namePart.empty()) {
        namePart[0] = std::toupper(namePart[0]);
    }
    
    return namePart;
}

std::string extractMood(const std::string& prompt) {
    std::string lowerPrompt = prompt;
    std::transform(lowerPrompt.begin(), lowerPrompt.end(), lowerPrompt.begin(), ::tolower);

    std::vector<std::string> moods = {
        "happy", "sad", "angry", "tired", "excited",
        "grumpy", "anxious", "stressed", "curious", "bored"
    };

    for (const auto& mood : moods) {
        if (lowerPrompt.find("i'm feeling " + mood) != std::string::npos ||
            lowerPrompt.find("i feel " + mood) != std::string::npos ||
            lowerPrompt.find("i am " + mood) != std::string::npos) {
            return mood;
        }
    }
    return "";
}

bool detectMoodClear(const std::string& prompt) {
    std::string lowerPrompt = prompt;
    std::transform(lowerPrompt.begin(), lowerPrompt.end(), lowerPrompt.begin(), ::tolower);
    
    return lowerPrompt.find("forget my mood") != std::string::npos ||
           lowerPrompt.find("reset my mood") != std::string::npos ||
           lowerPrompt.find("ignore how i feel") != std::string::npos ||
           lowerPrompt.find("never mind my feelings") != std::string::npos ||
           lowerPrompt.find("i'm over it") != std::string::npos ||
           lowerPrompt.find("it doesn't matter how i feel") != std::string::npos ||
           lowerPrompt.find("change the subject") != std::string::npos ||
           lowerPrompt.find("move on from that") != std::string::npos ||
           lowerPrompt.find("stop talking about my mood") != std::string::npos;
}

std::string FindUserByName(const std::string& name) {
    // TODO: Implement database query
    // return Database::GetInstance().FindUserByName(name);
    return "";
}

bool HasExistingProfile(const std::string& sessionID) {
    // TODO: Implement database query
    // return Database::GetInstance().HasExistingProfile(sessionID);
    return false;
}

std::vector<std::string> GetMoodHistory(const std::string& sessionID) {
    // TODO: Implement database query
    // return Database::GetInstance().GetMoodHistory(sessionID);
    return {};
}

void SaveMoodHistory(const std::string& sessionID, const std::vector<std::string>& moods) {
    // TODO: Implement database save
    // Database::GetInstance().SaveMoodHistory(sessionID, moods);
}

void ConsolidateMemories(const std::string& sessionID) {
    std::cout << "🧠 Consolidating memories for session: " << sessionID << std::endl;

    // TODO: Implement database query and update
    // auto memories = Database::GetInstance().GetMemoriesForConsolidation(sessionID);
    // for (const auto& mem : memories) {
    //     float strength = calculateMemoryStrength(mem.Importance, mem.RecallCount);
    //     Database::GetInstance().UpdateMemoryStrength(mem.MemoryID, strength);
    //     
    //     if (mem.Importance > 0.8f) {
    //         createRelatedMemories(sessionID, mem.Content, mem.Tags);
    //     }
    // }
}

float calculateMemoryStrength(float importance, int recallCount) {
    float strength = importance;
    if (recallCount > 0) {
        strength += recallCount * 0.1f;
    }
    return std::min(1.0f, strength);
}

void createRelatedMemories(const std::string& sessionID, 
                          const std::string& content, 
                          const std::vector<std::string>& tags) {
    std::vector<std::string> concepts = extractKeyConcepts(content);
    
    for (const auto& concept : concepts) {
        std::string relatedContent = "Related to: " + content;
        
        // TODO: Implement database save
        // Database::GetInstance().SaveMemory(sessionID, concept, relatedContent);
    }
}

std::vector<std::string> MemoryManager::extractKeyConcepts(const std::string& content) {
    std::vector<std::string> concepts;
    std::stringstream ss(content);
    std::string word;
    
    while (ss >> word) {
        if (!word.empty() && std::isupper(word[0])) {
            concepts.push_back(word);
        }
        
        if (word.starts_with("the") || word.starts_with("a")) {
            if (word.length() > 3) {
                concepts.push_back(word.substr(3));
            }
        }
    }
    
    return concepts;
}

void ProcessEmotionalResonance(const std::string& sessionID, const std::string& trigger, double intensity) {
    auto& memoryContext = GetMemoryContext(sessionID);
    auto now = std::chrono::system_clock::now();

    // Create new resonance
    EmotionalResonance resonance;
    resonance.Intensity = intensity;
    resonance.Duration = 1.0; // Base duration in hours
    resonance.Trigger = trigger;
    resonance.StartTime = now;
    resonance.PeakTime = now + std::chrono::hours(1);

    // Find associated memories
    auto relevantMemories = RecallRelevantMemories(trigger);
    for (const auto& memory : relevantMemories) {
        if (memory.EmotionalWeight > 0.5) {
            resonance.AssociatedMemories.push_back(memory.Content);
        }
    }

    memoryContext.ActiveResonances.push_back(resonance);

    // Update emotional patterns
    UpdateEmotionalPatterns(sessionID);
}

void UpdateEmotionalPatterns(const std::string& sessionID) {
    auto& memoryContext = GetMemoryContext(sessionID);
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

void CreateMemoryConnections(const std::string& sessionID) {
    auto& memoryContext = GetMemoryContext(sessionID);
    
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

void MemoryManager::ProcessMemoryClusters(const std::string& sessionID) {
    auto& context = GetMemoryContext(sessionID);
    for (const auto& cluster : context.clusters) {
        ProcessMemoryCluster(sessionID, cluster);
    }
}

void MemoryManager::ProcessMemoryCluster(const std::string& sessionID, const std::vector<MemoryEvent>& cluster) {
    for (const auto& memory : cluster) {
        ProcessMemoryEvent(sessionID, memory);
    }
}

void MemoryManager::UpdateMemoryIndex(const std::string& sessionID, const MemoryEvent& memory) {
    auto& context = GetMemoryContext(sessionID);
    context.memory_index[memory.id] = memory;
}

void MemoryManager::UpdateMemoryCluster(const std::string& sessionID, const MemoryEvent& memory) {
    auto& context = GetMemoryContext(sessionID);
    bool added = false;
    for (auto& cluster : context.clusters) {
        if (cluster.empty()) continue;
        if (std::abs(cluster[0].emotional_weight - memory.emotional_weight) < 0.1) {
            cluster.push_back(memory);
            added = true;
            break;
        }
    }
    if (!added) {
        context.clusters.push_back({memory});
    }
}

void MemoryManager::UpdateTraitBaseline(const std::string& traitName, double influence) {
    if (!is_initialized_) {
        throw std::runtime_error("MemoryManager not initialized");
    }
    
    auto& context = GetMemoryContext("default");
    if (!context) {
        throw std::runtime_error("Failed to get memory context");
    }
    
    // Update trait baseline with new influence
    auto& baseline = trait_baselines_[traitName];
    baseline.CurrentValue += influence;
    baseline.LastAdjustment = std::chrono::system_clock::now();
    
    // Update stability based on recent changes
    UpdateTraitStability(traitName);
}

void MemoryManager::UpdateTraitStability(const std::string& traitName) {
    auto& baseline = traitBaselines_[traitName];
    auto& metrics = traitEvolutionMetrics_[traitName];
    
    // Calculate volatility from historical values
    if (metrics.HistoricalValues.size() >= 2) {
        double sum = 0.0;
        for (double value : metrics.HistoricalValues) {
            sum += value;
        }
        double mean = sum / metrics.HistoricalValues.size();
        
        double variance = 0.0;
        for (double value : metrics.HistoricalValues) {
            variance += std::pow(value - mean, 2);
        }
        variance /= metrics.HistoricalValues.size();
        
        metrics.Volatility = std::sqrt(variance);
    }
    
    // Update stability based on volatility and confidence
    baseline.Stability = std::exp(-metrics.Volatility) * metrics.Confidence;
}

void MemoryManager::UpdateEvolutionMetrics(const std::string& traitName, double newValue) {
    auto& metrics = traitEvolutionMetrics_[traitName];
    
    // Update historical values
    metrics.HistoricalValues.push_back(newValue);
    if (metrics.HistoricalValues.size() > 100) {
        metrics.HistoricalValues.erase(metrics.HistoricalValues.begin());
    }
    
    // Calculate short-term change
    if (metrics.HistoricalValues.size() >= 2) {
        metrics.ShortTermChange = newValue - metrics.HistoricalValues[metrics.HistoricalValues.size() - 2];
    }
    
    // Calculate long-term trend
    if (metrics.HistoricalValues.size() >= 10) {
        double sum = 0.0;
        for (size_t i = metrics.HistoricalValues.size() - 10; i < metrics.HistoricalValues.size(); ++i) {
            sum += metrics.HistoricalValues[i];
        }
        double recentMean = sum / 10;
        
        sum = 0.0;
        for (size_t i = 0; i < 10; ++i) {
            sum += metrics.HistoricalValues[i];
        }
        double oldMean = sum / 10;
        
        metrics.LongTermTrend = recentMean - oldMean;
    }
    
    // Update confidence based on consistency
    metrics.Confidence = CalculateTraitConfidence(traitName);
    metrics.LastUpdate = std::chrono::system_clock::now();
}

double MemoryManager::CalculateTraitConfidence(const std::string& traitName) {
    const auto& baseline = traitBaselines_[traitName];
    const auto& metrics = traitEvolutionMetrics_[traitName];
    
    // Calculate confidence based on multiple factors
    double consistencyScore = 1.0 - metrics.Volatility;
    double memorySupportScore = static_cast<double>(baseline.SupportingMemories.size()) /
                              (baseline.SupportingMemories.size() + baseline.ConflictingMemories.size() + 1);
    double trendConfidence = std::exp(-std::abs(metrics.LongTermTrend));
    
    // Combine factors with weights
    return consistencyScore * 0.4 +
           memorySupportScore * 0.3 +
           trendConfidence * 0.3;
}

void MemoryManager::AnalyzeTraitTrends(const std::string& traitName) {
    auto& metrics = traitEvolutionMetrics_[traitName];
    auto& trendAnalysis = traitTrendAnalyses_[traitName];
    
    // Calculate moving averages for different time windows
    const size_t shortWindow = 5;
    const size_t longWindow = 20;
    const size_t seasonalWindow = 24; // Assuming daily seasonality
    
    std::vector<double> shortMA, longMA, seasonalMA;
    for (size_t i = 0; i < metrics.HistoricalValues.size(); ++i) {
        if (i >= shortWindow - 1) {
            double sum = 0.0;
            for (size_t j = 0; j < shortWindow; ++j) {
                sum += metrics.HistoricalValues[i - j];
            }
            shortMA.push_back(sum / shortWindow);
        }
        
        if (i >= longWindow - 1) {
            double sum = 0.0;
            for (size_t j = 0; j < longWindow; ++j) {
                sum += metrics.HistoricalValues[i - j];
            }
            longMA.push_back(sum / longWindow);
        }
        
        if (i >= seasonalWindow - 1) {
            double sum = 0.0;
            for (size_t j = 0; j < seasonalWindow; ++j) {
                sum += metrics.HistoricalValues[i - j];
            }
            seasonalMA.push_back(sum / seasonalWindow);
        }
    }
    
    // Calculate slopes and acceleration
    if (shortMA.size() >= 2) {
        trendAnalysis.ShortTermSlope = (shortMA.back() - shortMA[shortMA.size() - 2]) / shortWindow;
    }
    if (longMA.size() >= 2) {
        trendAnalysis.LongTermSlope = (longMA.back() - longMA[longMA.size() - 2]) / longWindow;
    }
    if (shortMA.size() >= 3) {
        trendAnalysis.Acceleration = (shortMA.back() - 2 * shortMA[shortMA.size() - 2] + 
                                    shortMA[shortMA.size() - 3]) / (shortWindow * shortWindow);
    }
    
    // Calculate volatility
    double sum = 0.0;
    for (double value : metrics.HistoricalValues) {
        sum += value;
    }
    double mean = sum / metrics.HistoricalValues.size();
    
    double variance = 0.0;
    for (double value : metrics.HistoricalValues) {
        variance += std::pow(value - mean, 2);
    }
    variance /= metrics.HistoricalValues.size();
    trendAnalysis.Volatility = std::sqrt(variance);
    
    // Calculate seasonality
    if (seasonalMA.size() >= 2) {
        double seasonalSum = 0.0;
        for (size_t i = 0; i < seasonalMA.size() - 1; ++i) {
            seasonalSum += std::abs(seasonalMA[i + 1] - seasonalMA[i]);
        }
        trendAnalysis.Seasonality = seasonalSum / (seasonalMA.size() - 1);
    }
    
    // Calculate cyclicality
    if (metrics.HistoricalValues.size() >= 4) {
        std::vector<double> differences;
        for (size_t i = 1; i < metrics.HistoricalValues.size(); ++i) {
            differences.push_back(metrics.HistoricalValues[i] - metrics.HistoricalValues[i - 1]);
        }
        
        double cycleSum = 0.0;
        for (size_t i = 1; i < differences.size(); ++i) {
            cycleSum += std::abs(differences[i] - differences[i - 1]);
        }
        trendAnalysis.Cyclicality = cycleSum / (differences.size() - 1);
    }
    
    trendAnalysis.MovingAverages = shortMA;
    trendAnalysis.SeasonalComponents = seasonalMA;
    trendAnalysis.LastAnalysis = std::chrono::system_clock::now();
}

void MemoryManager::ProcessTraitInteractions(const std::string& traitName) {
    auto& interactions = traitInteractions_[traitName];
    auto& memories = GetMemoriesByTrait(traitName);
    
    // Find related traits
    std::set<std::string> relatedTraits;
    for (const auto& memory : memories) {
        for (const auto& [trait, _] : memory.TraitInfluences) {
            if (trait != traitName) {
                relatedTraits.insert(trait);
            }
        }
    }
    
    // Analyze interactions with each related trait
    for (const auto& relatedTrait : relatedTraits) {
        TraitInteraction interaction;
        interaction.SourceTrait = traitName;
        interaction.TargetTrait = relatedTrait;
        
        // Calculate influence strength
        double totalInfluence = 0.0;
        int influenceCount = 0;
        for (const auto& memory : memories) {
            if (memory.TraitInfluences.find(relatedTrait) != memory.TraitInfluences.end()) {
                totalInfluence += memory.TraitInfluences.at(relatedTrait);
                influenceCount++;
            }
        }
        interaction.InfluenceStrength = influenceCount > 0 ? totalInfluence / influenceCount : 0.0;
        
        // Calculate temporal correlation
        auto& sourceMetrics = traitEvolutionMetrics_[traitName];
        auto& targetMetrics = traitEvolutionMetrics_[relatedTrait];
        
        if (sourceMetrics.HistoricalValues.size() == targetMetrics.HistoricalValues.size()) {
            double sumX = 0.0, sumY = 0.0, sumXY = 0.0;
            double sumX2 = 0.0, sumY2 = 0.0;
            
            for (size_t i = 0; i < sourceMetrics.HistoricalValues.size(); ++i) {
                double x = sourceMetrics.HistoricalValues[i];
                double y = targetMetrics.HistoricalValues[i];
                sumX += x;
                sumY += y;
                sumXY += x * y;
                sumX2 += x * x;
                sumY2 += y * y;
            }
            
            double n = sourceMetrics.HistoricalValues.size();
            double numerator = n * sumXY - sumX * sumY;
            double denominator = std::sqrt((n * sumX2 - sumX * sumX) * (n * sumY2 - sumY * sumY));
            interaction.TemporalCorrelation = denominator != 0 ? numerator / denominator : 0.0;
        }
        
        // Calculate emotional correlation
        double emotionalSum = 0.0;
        int emotionalCount = 0;
        for (const auto& memory : memories) {
            if (memory.EmotionalWeight > 0.0) {
                emotionalSum += memory.EmotionalWeight;
                emotionalCount++;
            }
        }
        interaction.EmotionalCorrelation = emotionalCount > 0 ? emotionalSum / emotionalCount : 0.0;
        
        // Find shared memories and triggers
        for (const auto& memory : memories) {
            if (memory.TraitInfluences.find(relatedTrait) != memory.TraitInfluences.end()) {
                interaction.SharedMemories.push_back(memory.ID);
                for (const auto& trigger : memory.Triggers) {
                    interaction.SharedTriggers.push_back(trigger);
                }
            }
        }
        
        interaction.LastInteraction = std::chrono::system_clock::now();
        interactions[relatedTrait] = interaction;
    }
}

void MemoryManager::PruneMemoriesBasedOnTraits() {
    auto now = std::chrono::system_clock::now();
    std::vector<std::string> memoriesToPrune;
    
    for (const auto& [memoryID, memory] : memories_) {
        MemoryPruningMetrics metrics;
        metrics.LastEvaluation = now;
        
        // Calculate relevance score based on trait evolution
        double totalRelevance = 0.0;
        int traitCount = 0;
        for (const auto& [trait, influence] : memory.TraitInfluences) {
            if (traitEvolutionMetrics_.find(trait) != traitEvolutionMetrics_.end()) {
                const auto& metrics = traitEvolutionMetrics_[trait];
                double timeSinceLast = std::chrono::duration_cast<std::chrono::hours>(
                    now - metrics.LastUpdate).count();
                double decayFactor = std::exp(-0.1 * timeSinceLast);
                totalRelevance += std::abs(influence) * decayFactor;
                traitCount++;
                metrics.AffectedTraits.push_back(trait);
            }
        }
        metrics.RelevanceScore = traitCount > 0 ? totalRelevance / traitCount : 0.0;
        
        // Calculate emotional impact
        metrics.EmotionalImpact = memory.EmotionalWeight * 
            std::exp(-0.05 * std::chrono::duration_cast<std::chrono::hours>(
                now - memory.Timestamp).count());
        
        // Calculate trait contribution
        double totalContribution = 0.0;
        for (const auto& trait : metrics.AffectedTraits) {
            if (traitTrendAnalyses_.find(trait) != traitTrendAnalyses_.end()) {
                const auto& trend = traitTrendAnalyses_[trait];
                totalContribution += std::abs(trend.ShortTermSlope) * 
                    (1.0 - trend.Volatility);
            }
        }
        metrics.TraitContribution = metrics.AffectedTraits.size() > 0 ? 
            totalContribution / metrics.AffectedTraits.size() : 0.0;
        
        // Calculate temporal decay
        auto timeSinceMemory = std::chrono::duration_cast<std::chrono::hours>(
            now - memory.Timestamp).count();
        metrics.TemporalDecay = std::exp(-0.1 * timeSinceMemory);
        
        // Calculate overall score
        metrics.OverallScore = 
            metrics.RelevanceScore * 0.3 +
            metrics.EmotionalImpact * 0.2 +
            metrics.TraitContribution * 0.3 +
            metrics.TemporalDecay * 0.2;
        
        // Mark memory for pruning if score is below threshold
        if (metrics.OverallScore < 0.2) {
            memoriesToPrune.push_back(memoryID);
        }
    }
    
    // Remove pruned memories
    for (const auto& memoryID : memoriesToPrune) {
        RemoveMemory(memoryID);
    }
}

EnhancedConfidence MemoryManager::CalculateEnhancedConfidence(const std::string& traitName) {
    EnhancedConfidence confidence;
    
    // Get base confidence from existing metrics
    confidence.BaseConfidence = CalculateTraitConfidence(traitName);
    
    // Calculate pattern consistency
    if (traitTrendAnalyses_.find(traitName) != traitTrendAnalyses_.end()) {
        const auto& trend = traitTrendAnalyses_[traitName];
        confidence.PatternConsistency = 1.0 - (trend.Volatility * 0.5 + 
            std::abs(trend.ShortTermSlope - trend.LongTermSlope) * 0.5);
    }
    
    // Calculate cross-validation
    double totalCorrelation = 0.0;
    int correlationCount = 0;
    for (const auto& [relatedTrait, interactions] : traitInteractions_) {
        for (const auto& interaction : interactions) {
            if (interaction.TargetTrait == traitName) {
                totalCorrelation += interaction.TemporalCorrelation;
                correlationCount++;
            }
        }
    }
    confidence.CrossValidation = correlationCount > 0 ? totalCorrelation / correlationCount : 0.0;
    
    // Calculate temporal stability
    if (traitEvolutionMetrics_.find(traitName) != traitEvolutionMetrics_.end()) {
        const auto& metrics = traitEvolutionMetrics_[traitName];
        confidence.TemporalStability = 1.0 - metrics.Volatility;
    }
    
    // Calculate emotional alignment
    double totalEmotionalCorrelation = 0.0;
    int emotionalCount = 0;
    for (const auto& [relatedTrait, interactions] : traitInteractions_) {
        for (const auto& interaction : interactions) {
            if (interaction.TargetTrait == traitName) {
                totalEmotionalCorrelation += interaction.EmotionalCorrelation;
                emotionalCount++;
            }
        }
    }
    confidence.EmotionalAlignment = emotionalCount > 0 ? totalEmotionalCorrelation / emotionalCount : 0.0;
    
    // Calculate trait correlation
    confidence.TraitCorrelation = confidence.CrossValidation * 0.5 + confidence.EmotionalAlignment * 0.5;
    
    // Calculate overall confidence
    confidence.OverallConfidence = 
        confidence.BaseConfidence * 0.2 +
        confidence.PatternConsistency * 0.2 +
        confidence.CrossValidation * 0.2 +
        confidence.TemporalStability * 0.2 +
        confidence.EmotionalAlignment * 0.1 +
        confidence.TraitCorrelation * 0.1;
    
    return confidence;
}

void MemoryManager::ProcessEmotionalResonance(const std::string& trigger, double intensity) {
    if (!is_initialized_) {
        throw std::runtime_error("MemoryManager not initialized");
    }
    
    auto& context = GetMemoryContext("default");
    if (!context) {
        throw std::runtime_error("Failed to get memory context");
    }
    
    // Process emotional resonance
    EmotionalResonance resonance;
    resonance.Intensity = intensity;
    resonance.Trigger = trigger;
    resonance.StartTime = std::chrono::system_clock::now();
    resonance.PeakTime = resonance.StartTime;
    
    // Update emotional patterns
    UpdateEmotionalPatterns();
    
    // Update memory weights based on emotional impact
    UpdateMemoryWeightsWithEmotion();
}

void MemoryManager::UpdateEmotionalPatterns() {
    if (!is_initialized_) return;
    
    auto& context = GetMemoryContext("default");
    for (auto& pattern : context.Evolution.GrowthPatterns) {
        double newIntensity = 0.0;
        for (const auto& memory : memories_) {
            if (std::find(pattern.TriggeringEvents.begin(), pattern.TriggeringEvents.end(), 
                         memory.second.id) != pattern.TriggeringEvents.end()) {
                newIntensity += memory.second.emotional_weight;
            }
        }
        pattern.Strength = newIntensity / pattern.TriggeringEvents.size();
    }
}

void MemoryManager::ProcessSelfReflection() {
    if (!is_initialized_) return;
    
    auto& context = GetMemoryContext("default");
    SelfReflection reflection;
    reflection.Type = "SelfReflection";
    reflection.Timestamp = std::chrono::system_clock::now();
    
    // Analyze recent memories
    std::vector<MemoryEvent> recentMemories = GetRecentMemories(std::chrono::hours(24));
    if (recentMemories.empty()) return;
    
    // Calculate average emotional weight
    double totalWeight = 0.0;
    for (const auto& memory : recentMemories) {
        totalWeight += memory.emotional_weight;
    }
    double avgWeight = totalWeight / recentMemories.size();
    
    reflection.Content = "Recent emotional state: " + std::to_string(avgWeight);
    reflection.Confidence = 0.8; // Placeholder
    
    context.GrowthInsights.push_back(reflection);
}

void MemoryManager::ProcessLongTermReflection() {
    if (!is_initialized_) return;
    
    auto& context = GetMemoryContext("default");
    SelfReflection reflection;
    reflection.Type = "LongTermReflection";
    reflection.Timestamp = std::chrono::system_clock::now();
    
    // Analyze trait evolution
    std::stringstream ss;
    ss << "Trait Evolution Analysis:\n";
    for (const auto& trait : trait_baselines_) {
        ss << trait.first << ": " << trait.second.CurrentValue << " (Target: " 
           << trait.second.TargetValue << ")\n";
    }
    
    reflection.Content = ss.str();
    reflection.Confidence = 0.9; // Placeholder
    
    context.GrowthInsights.push_back(reflection);
}

void MemoryManager::UpdateMemoryWeightsWithEmotion() {
    if (!is_initialized_) return;
    
    for (auto& memory : memories_) {
        double emotionalBoost = memory.second.emotional_weight * 0.5; // Placeholder factor
        memory.second.importance += emotionalBoost;
        UpdateMemory(memory.second);
    }
}

void MemoryManager::ProcessPatternRecognition() {
    if (!is_initialized_) {
        throw std::runtime_error("MemoryManager not initialized");
    }
    
    auto& context = GetMemoryContext("default");
    if (!context) {
        throw std::runtime_error("Failed to get memory context");
    }
    
    // Analyze memory clusters for patterns
    for (const auto& cluster : context.MemoryConnections) {
        if (cluster.Strength > STRONG_CONNECTION_THRESHOLD) {
            EmotionalPattern pattern;
            pattern.PatternType = "StrongConnection";
            pattern.BaseIntensity = cluster.Strength;
            pattern.CurrentIntensity = cluster.Strength;
            pattern.LastTriggered = std::chrono::system_clock::now();
            pattern.Triggers = {cluster.SourceMemory, cluster.TargetMemory};
            
            context.CorePatterns.push_back(pattern);
        }
    }
}

void MemoryManager::UpdateMemoryAssociations() {
    if (!is_initialized_) {
        throw std::runtime_error("MemoryManager not initialized");
    }
    
    auto& context = GetMemoryContext("default");
    if (!context) {
        throw std::runtime_error("Failed to get memory context");
    }
    
    // Clear existing connections
    context.MemoryConnections.clear();
    
    // Create new connections based on shared traits and tags
    for (const auto& mem1 : memories_) {
        for (const auto& mem2 : memories_) {
            if (mem1.first >= mem2.first) continue; // Avoid duplicates
            
            MemoryConnection connection;
            connection.SourceMemory = mem1.first;
            connection.TargetMemory = mem2.first;
            
            // Calculate connection strength based on shared traits
            double traitStrength = 0.0;
            for (const auto& trait : mem1.second.trait_influences) {
                if (mem2.second.trait_influences.find(trait.first) != mem2.second.trait_influences.end()) {
                    traitStrength += std::min(trait.second, mem2.second.trait_influences[trait.first]);
                }
            }
            
            // Calculate connection strength based on shared tags
            double tagStrength = 0.0;
            for (const auto& tag : mem1.second.tags) {
                if (mem2.second.tags.find(tag) != mem2.second.tags.end()) {
                    tagStrength += 1.0;
                }
            }
            
            connection.Strength = (traitStrength + tagStrength) / 2.0;
            if (connection.Strength > 0.3) { // Minimum threshold
                context.MemoryConnections.push_back(connection);
            }
        }
    }
}

void MemoryManager::UpdateEmotionalConnections() {
    if (!is_initialized_) {
        throw std::runtime_error("MemoryManager not initialized");
    }
    
    auto& context = GetMemoryContext("default");
    if (!context) {
        throw std::runtime_error("Failed to get memory context");
    }
    
    // Update emotional weights based on connections
    for (auto& connection : context.MemoryConnections) {
        auto& mem1 = memories_[connection.SourceMemory];
        auto& mem2 = memories_[connection.TargetMemory];
        
        // Emotional influence between connected memories
        double influence = connection.Strength * EMOTIONAL_INFLUENCE_FACTOR;
        mem1.emotional_weight += mem2.emotional_weight * influence;
        mem2.emotional_weight += mem1.emotional_weight * influence;
        
        // Normalize emotional weights
        mem1.emotional_weight = std::clamp(mem1.emotional_weight, 
                                         EMOTIONAL_WEIGHT_CLAMP_MIN, 
                                         EMOTIONAL_WEIGHT_CLAMP_MAX);
        mem2.emotional_weight = std::clamp(mem2.emotional_weight, 
                                         EMOTIONAL_WEIGHT_CLAMP_MIN, 
                                         EMOTIONAL_WEIGHT_CLAMP_MAX);
        
        UpdateMemory(mem1);
        UpdateMemory(mem2);
    }
}

void MemoryManager::UpdateTraitBaselines() {
    if (!is_initialized_) return;
    
    for (auto& trait : trait_baselines_) {
        double totalInfluence = 0.0;
        int count = 0;
        
        // Calculate average influence from memories
        for (const auto& memory : memories_) {
            if (memory.second.trait_influences.find(trait.first) != memory.second.trait_influences.end()) {
                totalInfluence += memory.second.trait_influences[trait.first];
                count++;
            }
        }
        
        if (count > 0) {
            trait.second.CurrentValue = totalInfluence / count;
        }
    }
}

void MemoryManager::ProcessTraitEvolution() {
    if (!is_initialized_) return;
    
    auto& context = GetMemoryContext("default");
    
    // Analyze trait changes over time
    for (const auto& trait : trait_baselines_) {
        TraitEvolution evolution;
        evolution.TraitName = trait.first;
        evolution.CurrentValue = trait.second.CurrentValue;
        evolution.TargetValue = trait.second.TargetValue;
        evolution.ChangeRate = (evolution.CurrentValue - evolution.TargetValue) / 
                             std::chrono::duration_cast<std::chrono::hours>(
                                 std::chrono::system_clock::now() - trait.second.LastUpdate).count();
        
        context.Evolution.TraitChanges.push_back(evolution);
    }
}

void MemoryManager::UpdateGrowthInsights() {
    if (!is_initialized_) return;
    
    auto& context = GetMemoryContext("default");
    
    // Clear old insights
    context.GrowthInsights.clear();
    
    // Process self-reflection
    ProcessSelfReflection();
    
    // Process long-term reflection
    ProcessLongTermReflection();
    
    // Sort insights by confidence
    std::sort(context.GrowthInsights.begin(), context.GrowthInsights.end(),
              [](const SelfReflection& a, const SelfReflection& b) {
                  return a.Confidence > b.Confidence;
              });
}

void MemoryManager::UpdateMemoryIndex() {
    if (!is_initialized_) return;
    
    // Clear existing index
    memory_index_.clear();
    
    // Rebuild index
    for (const auto& memory : memories_) {
        // Index by content keywords
        std::stringstream ss(memory.second.content);
        std::string word;
        while (ss >> word) {
            if (word.length() > 3) { // Only index words longer than 3 characters
                memory_index_[word].push_back(memory.first);
            }
        }
        
        // Index by tags
        for (const auto& tag : memory.second.tags) {
            memory_index_[tag].push_back(memory.first);
        }
    }
}

void MemoryManager::UpdateCache() {
    if (!is_initialized_) return;
    
    // Remove least recently used memories if cache is full
    while (memory_cache_.size() > maxCacheSize_) {
        auto oldest = memory_cache_.begin();
        for (auto it = memory_cache_.begin(); it != memory_cache_.end(); ++it) {
            if (it->second.last_accessed < oldest->second.last_accessed) {
                oldest = it;
            }
        }
        memory_cache_.erase(oldest);
    }
}

void MemoryManager::SaveToDatabase() {
    if (!is_initialized_ || !db_) {
        throw std::runtime_error("MemoryManager not properly initialized");
    }
    
    try {
        // Begin transaction
        if (!db_->BeginTransaction()) {
            throw std::runtime_error("Failed to begin database transaction");
        }
        
        // Save memories
        for (const auto& memory : memories_) {
            if (!db_->SaveMemory(memory.second)) {
                throw std::runtime_error("Failed to save memory: " + memory.second.id);
            }
        }
        
        // Save emotional states
        for (const auto& state : emotional_states_) {
            if (!db_->SaveEmotionalState(state.second)) {
                throw std::runtime_error("Failed to save emotional state: " + state.second.id);
            }
        }
        
        // Commit transaction
        if (!db_->CommitTransaction()) {
            throw std::runtime_error("Failed to commit database transaction");
        }
    } catch (const std::exception& e) {
        db_->RollbackTransaction();
        throw std::runtime_error("Failed to save to database: " + std::string(e.what()));
    }
}

void MemoryManager::LoadFromDatabase() {
    if (!is_initialized_ || !db_) return;
    
    try {
        // Load memories
        std::string sql = "SELECT id, content, importance, emotional_weight, timestamp FROM memories";
        auto results = db_->Query(sql);
        
        while (results->Next()) {
            MemoryEvent memory;
            memory.id = results->GetString(0);
            memory.content = results->GetString(1);
            memory.importance = results->GetDouble(2);
            memory.emotional_weight = results->GetDouble(3);
            memory.timestamp = std::chrono::system_clock::from_time_t(results->GetInt64(4));
            
            // Load tags
            sql = "SELECT tag FROM memory_tags WHERE memory_id = ?";
            auto tagResults = db_->Query(sql, {memory.id});
            while (tagResults->Next()) {
                memory.tags.insert(tagResults->GetString(0));
            }
            
            // Load trait influences
            sql = "SELECT trait_name, influence FROM memory_traits WHERE memory_id = ?";
            auto traitResults = db_->Query(sql, {memory.id});
            while (traitResults->Next()) {
                memory.trait_influences[traitResults->GetString(0)] = traitResults->GetDouble(1);
            }
            
            memories_[memory.id] = memory;
        }
        
        // Load emotional states
        sql = "SELECT id, happiness, sadness, timestamp FROM emotional_states";
        results = db_->Query(sql);
        while (results->Next()) {
            EmotionalState state;
            state.id = results->GetString(0);
            state.happiness = results->GetDouble(1);
            state.sadness = results->GetDouble(2);
            state.timestamp = std::chrono::system_clock::from_time_t(results->GetInt64(3));
            
            emotional_states_[state.id] = state;
        }
        
        // Update memory index
        UpdateMemoryIndex();
        
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to load from database: " + std::string(e.what()));
    }
}

// ... existing code ...
} // namespace cognitive 