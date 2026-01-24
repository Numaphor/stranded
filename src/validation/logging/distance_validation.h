#ifndef STR_DISTANCE_VALIDATION_H
#define STR_DISTANCE_VALIDATION_H

#include "bn_fixed_point.h"
#include "str_constants.h"
#include "bn_log.h"

namespace str
{
    // Distance validation constants
    constexpr int CHUNK_LOAD_DISTANCE = 4;  // 4-chunk radius for loading validation
    
    // Distance calculation logging categories
    enum class DistanceLogCategory
    {
        DISTANCE_CALC,     // Distance calculation validation
        LOAD_RADIUS,       // Load radius boundary tracking
        PLAYER_TRACK       // Player chunk position tracking
    };

    // Distance calculation validation functions
    void log_load_radius(int player_chunk_x, int player_chunk_y, int chunk_x, int chunk_y, int distance);
    bool validate_distance_logic(int player_chunk_x, int player_chunk_y, int test_chunk_x, int test_chunk_y);
    void track_player_chunk(int old_chunk_x, int old_chunk_y, int new_chunk_x, int new_chunk_y);
    
    // Edge case validation functions
    bool validate_world_boundary_distances();
    bool validate_wrapping_distance_consistency();
    bool validate_manhattan_distance_calculations();
    
    // Performance testing for distance calculations
    void benchmark_distance_calculations();
    int calculate_manhattan_distance(int x1, int y1, int x2, int y2);
    bool is_chunk_within_load_distance(int player_chunk_x, int player_chunk_y, int chunk_x, int chunk_y);
    
    // Load radius efficiency tracking
    struct LoadRadiusMetrics
    {
        int chunks_within_radius;
        int chunks_outside_radius;
        float efficiency_percentage;
        int total_distance_calculations;
    };
    
    LoadRadiusMetrics calculate_load_radius_metrics(int player_chunk_x, int player_chunk_y, int load_distance = CHUNK_LOAD_DISTANCE);
    void log_load_radius_efficiency(const LoadRadiusMetrics& metrics);
    
    // Stress testing for distance-based loading
    void test_stationary_player_load_pattern();
    void test_movement_patterns();
    void test_boundary_conditions();
}

#endif // STR_DISTANCE_VALIDATION_H