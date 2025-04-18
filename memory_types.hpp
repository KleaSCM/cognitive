#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>
#include <chrono>
#include <nlohmann/json.hpp>

namespace shandris {
namespace cognitive {

struct MemoryConnection {
    std::string source_memory;
    std::string target_memory;
    double strength;
    std::string connection_type;
    std::vector<std::string> shared_traits;
    
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(MemoryConnection,
        source_memory, target_memory, strength, connection_type, shared_traits)
};

struct SelfReflection {
    std::string type;
    std::string content;
    double confidence;
    std::chrono::system_clock::time_point timestamp;
    
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(SelfReflection,
        type, content, confidence, timestamp)
};

struct EnhancedConfidence {
    double base_confidence;
    double pattern_consistency;
    double cross_validation;
    double temporal_stability;
    double emotional_alignment;
    double trait_correlation;
    double overall_confidence;
    
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(EnhancedConfidence,
        base_confidence, pattern_consistency, cross_validation,
        temporal_stability, emotional_alignment, trait_correlation,
        overall_confidence)
};

struct MemoryIndex {
    std::map<std::string, std::vector<std::string>> by_tag;
    std::map<std::string, std::vector<std::string>> by_trait;
    std::map<std::string, std::vector<std::string>> by_time_bucket;
};

struct MemoryCluster {
    std::vector<std::string> memory_ids;
    std::map<std::string, double> trait_frequencies;
    std::set<std::string> common_tags;
    double emotional_theme;
    double stability;
    std::chrono::system_clock::time_point last_accessed;
    
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(MemoryCluster,
        memory_ids, trait_frequencies, common_tags,
        emotional_theme, stability, last_accessed)
};

struct ClusterEvolution {
    std::vector<std::string> trait_changes;
    std::vector<double> stability_metrics;
    std::vector<std::string> growth_patterns;
    std::chrono::system_clock::time_point last_evolution;
    
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ClusterEvolution,
        trait_changes, stability_metrics, growth_patterns, last_evolution)
};

struct ClusterRelationship {
    std::string source_cluster;
    std::string target_cluster;
    double current_strength;
    double historical_strength;
    std::vector<std::string> shared_traits;
    std::chrono::system_clock::time_point last_interaction;
    
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ClusterRelationship,
        source_cluster, target_cluster, current_strength,
        historical_strength, shared_traits, last_interaction)
};

struct TraitTrendAnalysis {
    double short_term_slope;
    double long_term_slope;
    double acceleration;
    double volatility;
    double seasonality;
    double cyclicality;
    std::vector<double> moving_averages;
    std::vector<double> seasonal_components;
    std::chrono::system_clock::time_point last_analysis;
    
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(TraitTrendAnalysis,
        short_term_slope, long_term_slope, acceleration,
        volatility, seasonality, cyclicality, moving_averages,
        seasonal_components, last_analysis)
};

struct TraitInteraction {
    std::string source_trait;
    std::string target_trait;
    double influence_strength;
    double temporal_correlation;
    double emotional_correlation;
    std::vector<std::string> shared_memories;
    std::vector<std::string> shared_triggers;
    std::chrono::system_clock::time_point last_interaction;
    
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(TraitInteraction,
        source_trait, target_trait, influence_strength,
        temporal_correlation, emotional_correlation,
        shared_memories, shared_triggers, last_interaction)
};

struct ClusterDivergence {
    double trait_divergence;
    double temporal_divergence;
    double emotional_divergence;
    std::vector<std::string> diverging_traits;
    std::chrono::system_clock::time_point divergence_point;
    
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ClusterDivergence,
        trait_divergence, temporal_divergence, emotional_divergence,
        diverging_traits, divergence_point)
};

struct ClusterSimilarity {
    double trait_similarity;
    double tag_overlap;
    double emotional_alignment;
    double temporal_proximity;
    double overall_similarity;
    
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ClusterSimilarity,
        trait_similarity, tag_overlap, emotional_alignment,
        temporal_proximity, overall_similarity)
};

}} // namespace shandris::cognitive 