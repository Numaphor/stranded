#ifndef STR_COORD_VALIDATION_H
#define STR_COORD_VALIDATION_H

#include "bn_fixed_point.h"
#include "bn_log_level.h"

namespace str
{
    // Buffer size constants for validation
    constexpr int VIEW_BUFFER_TILES = 128;
    constexpr int VIEW_BUFFER_CHUNKS = 16;
    constexpr int TILE_SIZE = 8;
    
    // World boundary constants
    constexpr int WORLD_WIDTH_PIXELS = 8192;
    constexpr int WORLD_HEIGHT_PIXELS = 8192;
    constexpr int MAX_WORLD_COORD = 8191;
    constexpr int MIN_WORLD_COORD = 0;
    
    // Wrap test boundaries
    constexpr int WRAP_TEST_BOUNDARIES[] = {
        0,      // Minimum coordinate
        511,    // Maximum buffer tile coordinate
        512,    // Just beyond buffer boundary
        8191    // Maximum world coordinate
    };
    
    // Coordinate validation function declarations
    void test_coordinate_wrapping();
    void validate_edge_cases();
    void track_origin_consistency();
    void validate_coordinate_transformations();
    void test_buffer_boundaries();
    void test_world_boundaries();
    void validate_precision_consistency();
    void stress_test_coordinate_calculations();
    
    // Helper functions for coordinate testing
    bool validate_buffer_bounds(int buffer_x, int buffer_y);
    bool validate_world_bounds(int world_x, int world_y);
    bool test_bidirectional_conversion(const bn::fixed_point& world_pos);
    
    // Validation logging macros
    #define COORD_CONV_LOG(level, ...) BN_LOG_LEVEL(bn::log_level::level, "COORD_CONV:", __VA_ARGS__)
    #define COORD_CONV_DEBUG(...) COORD_CONV_LOG(DEBUG, __VA_ARGS__)
    #define COORD_CONV_INFO(...) COORD_CONV_LOG(INFO, __VA_ARGS__)
    #define COORD_CONV_WARN(...) COORD_CONV_LOG(WARN, __VA_ARGS__)
    #define COORD_CONV_ERROR(...) COORD_CONV_LOG(ERROR, __VA_ARGS__)
}

#endif // STR_COORD_VALIDATION_H