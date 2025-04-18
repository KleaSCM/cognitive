#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <chrono>
#include <map>
#include <regex>
#include <iostream>
#include <sstream>
#include <deque>
#include <set>
#include <nlohmann/json.hpp>
#include "../database/database.hpp"
#include "memory_types.hpp"

namespace shandris {
namespace cognitive {

// Forward declarations
class PersonaManager;

struct MemoryEvent {
    std::string id;
    std::string content;
    std::string context;
    double importance;
    double emotional_weight;
    std::map<std::string, double> trait_influences;
    std::set<std::string> tags;
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point updated_at;
    
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(MemoryEvent, 
        id, content, context, importance, emotional_weight, 
        trait_influences, tags, created_at, updated_at)
};

struct EmotionalState {
    std::string id;
    double happiness;
    double sadness;
    double anger;
    double fear;
    double surprise;
    double disgust;
    double trust;
    double anticipation;
    std::chrono::system_clock::time_point timestamp;
    
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(EmotionalState,
        id, happiness, sadness, anger, fear, surprise,
        disgust, trust, anticipation, timestamp)
};

struct SapphicTraits {
    std::string id;
    double seductiveness;
    double intellectuality;
    double protectiveness;
    double clinginess;
    double independence;
    double playfulness;
    double sassiness;
    double emotional_depth;
    double confidence;
    double sensitivity;
    double lesbian_identity;
    double feminine_attraction;
    double sapphic_energy;
    
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(SapphicTraits,
        id, seductiveness, intellectuality, protectiveness,
        clinginess, independence, playfulness, sassiness,
        emotional_depth, confidence, sensitivity, lesbian_identity,
        feminine_attraction, sapphic_energy)
};

struct TraitBaseline {
    double current_value;
    double target_value;
    double adjustment_rate;
    double stability;
    std::chrono::system_clock::time_point last_adjustment;
    std::vector<std::string> supporting_memories;
    std::vector<std::string> conflicting_memories;
};

struct TraitEvolutionMetrics {
    double short_term_change;
    double long_term_trend;
    double volatility;
    double confidence;
    std::vector<double> historical_values;
    std::chrono::system_clock::time_point last_update;
};

struct MemoryContext {
    std::vector<MemoryEvent> short_term_memories;
    std::vector<MemoryEvent> long_term_memories;
    std::vector<MemoryConnection> memory_connections;
    std::map<std::string, double> memory_weights;
    std::chrono::system_clock::time_point last_memory_update;
    std::vector<SelfReflection> growth_insights;
    std::map<std::string, double> trait_growth_rates;
    std::map<std::string, double> pattern_stabilities;
};

class MemoryManager {
public:
    // Configuration constants
    static constexpr double STRONG_CONNECTION_THRESHOLD = 0.7;
    static constexpr double MIN_CONNECTION_THRESHOLD = 0.3;
    static constexpr double EMOTIONAL_INFLUENCE_FACTOR = 0.5;
    static constexpr size_t MAX_CACHE_SIZE = 1000;
    static constexpr double EMOTIONAL_WEIGHT_CLAMP_MIN = -1.0;
    static constexpr double EMOTIONAL_WEIGHT_CLAMP_MAX = 1.0;

    MemoryManager();
    ~MemoryManager();

    bool Initialize();
    bool IsInitialized() const { return is_initialized_; }
    
    // Memory operations
    bool SaveMemory(const MemoryEvent& memory);
    bool LoadMemory(const std::string& id, MemoryEvent& memory);
    bool UpdateMemory(const MemoryEvent& memory);
    bool DeleteMemory(const std::string& id);
    
    // Emotional state operations
    bool SaveEmotionalState(const EmotionalState& state);
    bool LoadEmotionalState(const std::string& id, EmotionalState& state);
    bool UpdateEmotionalState(const EmotionalState& state);
    bool DeleteEmotionalState(const std::string& id);
    
    // Trait operations
    bool SaveTraits(const std::map<std::string, TraitBaseline>& traits);
    bool LoadTraits(std::map<std::string, TraitBaseline>& traits);
    bool UpdateTraits(const std::map<std::string, TraitBaseline>& traits);
    bool DeleteTraits(const std::string& id);

    // Memory clustering and analysis
    void ProcessMemoryClusters(const std::string& sessionID);
    void ProcessMemoryCluster(const std::string& sessionID, const std::vector<MemoryEvent>& cluster);
    void UpdateMemoryIndex(const std::string& sessionID, const MemoryEvent& memory);
    void UpdateMemoryCluster(const std::string& sessionID, const MemoryEvent& memory);
    void UpdateTraitBaseline(const std::string& traitName, double influence);
    void AnalyzeTraitTrends(const std::string& traitName);
    void ProcessTraitInteractions(const std::string& traitName);
    EnhancedConfidence CalculateEnhancedConfidence(const std::string& traitName);

    // Core processing functions
    void ProcessEmotionalResonance(const std::string& trigger, double intensity);
    void UpdateEmotionalPatterns();
    void ProcessSelfReflection();
    void ProcessLongTermReflection();
    void UpdateMemoryWeightsWithEmotion();
    void ProcessPatternRecognition();
    void UpdateMemoryAssociations();
    void UpdateEmotionalConnections();
    void UpdateTraitBaselines();
    void ProcessTraitEvolution();
    void UpdateGrowthInsights();
    void UpdateMemoryIndex();
    void UpdateCache();
    void SaveToDatabase();
    void LoadFromDatabase();

    // Memory analysis
    void AnalyzeMemoryPatterns();
    void ProcessMemoryClusters();
    void UpdateMemoryWeights();
    void PruneMemories();

    // Trait analysis
    void AnalyzeTraitTrends();
    void ProcessTraitInteractions();
    void UpdateTraitStability();

private:
    std::shared_ptr<PersonaManager> persona_manager_;
    std::shared_ptr<database::Database> db_;
    std::deque<MemoryEvent> short_term_cache_;
    std::deque<MemoryEvent> long_term_cache_;
    size_t max_cache_size_;
    
    // Memory storage and indexing
    std::map<std::string, MemoryEvent> memories_;
    MemoryIndex memory_index_;
    
    // Memory clustering
    std::map<std::string, std::vector<MemoryCluster>> memory_clusters_;
    std::map<std::string, ClusterEvolution> cluster_evolutions_;
    std::vector<ClusterRelationship> cluster_relationships_;
    
    // Trait tracking
    std::map<std::string, TraitBaseline> trait_baselines_;
    std::map<std::string, TraitEvolutionMetrics> trait_evolution_metrics_;
    std::map<std::string, TraitTrendAnalysis> trait_trend_analyses_;
    std::map<std::string, std::vector<TraitInteraction>> trait_interactions_;
    
    // Memory context
    MemoryContext context_;

    // Helper methods
    MemoryEvent* GetMemory(const std::string& id);
    void UpdateClusterMetrics(MemoryCluster& cluster);
    double CalculateTraitDivergence(const std::map<std::string, double>& trait_frequencies);
    double CalculateTemporalDivergence(const MemoryCluster& cluster);
    double CalculateEmotionalDivergence(const MemoryCluster& cluster);
    std::vector<MemoryCluster> SplitCluster(const MemoryCluster& cluster, const ClusterDivergence& divergence);
    std::string FindDominantTrait(const MemoryEvent& memory, const std::vector<std::string>& diverging_traits);
    std::string GenerateUUID();
    double CalculateClusterStability(const MemoryCluster& cluster);
    double CalculateTraitSimilarity(const MemoryEvent& memory, const MemoryCluster& cluster);
    double CalculateTagOverlap(const MemoryEvent& memory, const MemoryCluster& cluster);
    MemoryCluster MergeClusters(const MemoryCluster& cluster1, const MemoryCluster& cluster2, const ClusterSimilarity& similarity);
    double AnalyzeSentiment(const std::string& content);
    double CalculateEngagement(const std::vector<std::string>& responses);
    double AssessSatisfaction(const std::vector<std::string>& responses);
    double EvaluateConsistency(const std::string& content);
    std::vector<std::string> ExtractKeyPhrases(const std::string& content);
    std::vector<MemoryEvent> GetRecentMemories(const std::chrono::hours& duration);
    void UpdateTraitInfluence(const MemoryEvent& memory, const std::string& trait, double weight);
    void UpdateMemory(const MemoryEvent& memory);
    std::string GetCurrentMemoryID();
    void UpdateTraitStability(const std::string& traitName);
    void UpdateEvolutionMetrics(const std::string& traitName, double newValue);
    double CalculateTraitConfidence(const std::string& traitName);
    std::vector<MemoryEvent> GetMemoriesByTrait(const std::string& trait);
    void RemoveMemory(const std::string& id);

    bool is_initialized_ = false;
};

} // namespace cognitive
} // namespace shandris
