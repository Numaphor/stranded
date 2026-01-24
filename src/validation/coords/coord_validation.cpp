#include "coord_validation.h"
#include "str_chunk_manager.h"
#include "bn_log.h"
#include "bn_log_level.h"
#include "bn_fixed.h"
#include <cmath>

namespace str
{
    // External reference to chunk manager for testing
    extern ChunkManager* g_chunk_manager;
    
    void test_coordinate_wrapping()
    {
        COORD_CONV_INFO("Testing coordinate wrapping at buffer boundaries");
        
        // Test at maximum world coordinates
        bn::fixed_point max_world(WORLD_WIDTH_PIXELS - 1, WORLD_HEIGHT_PIXELS - 1);
        if (g_chunk_manager) {
            bn::fixed_point buffer_pos = g_chunk_manager->world_to_buffer(max_world);
            
            COORD_CONV_INFO("MAX_COORD:", max_world.x(), ",", max_world.y(), 
                         "-> BUFFER:", buffer_pos.x(), ",", buffer_pos.y());
            
            // Buffer coordinates should be within 0-511 range
            int buffer_tile_x = (buffer_pos.x() + VIEW_BUFFER_TILES * TILE_SIZE / 2).integer() / TILE_SIZE;
            int buffer_tile_y = (buffer_pos.y() + VIEW_BUFFER_TILES * TILE_SIZE / 2).integer() / TILE_SIZE;
            
            if (buffer_tile_x >= 0 && buffer_tile_x < VIEW_BUFFER_TILES &&
                buffer_tile_y >= 0 && buffer_tile_y < VIEW_BUFFER_TILES) {
                COORD_CONV_INFO("WRAP_SUCCESS: Max coordinates wrapped correctly to buffer slot", 
                             buffer_tile_x, ",", buffer_tile_y);
            } else {
                COORD_CONV_ERROR("WRAP_FAIL: Max coordinates wrapped out of bounds:",
                              buffer_tile_x, ",", buffer_tile_y);
            }
        }
        
        // Test wrapping at buffer edges: (511,511) -> should wrap correctly
        for (int boundary : WRAP_TEST_BOUNDARIES) {
            bn::fixed_point test_world(boundary * TILE_SIZE, boundary * TILE_SIZE);
            
            if (g_chunk_manager) {
                bn::fixed_point buffer_pos = g_chunk_manager->world_to_buffer(test_world);
                int buffer_tile_x = (buffer_pos.x() + VIEW_BUFFER_TILES * TILE_SIZE / 2).integer() / TILE_SIZE;
                int buffer_tile_y = (buffer_pos.y() + VIEW_BUFFER_TILES * TILE_SIZE / 2).integer() / TILE_SIZE;
                
                COORD_CONV_DEBUG("WRAP_TEST: World", test_world.x(), ",", test_world.y(),
                              "-> Buffer", buffer_tile_x, ",", buffer_tile_y);
                
                // Validate wrapping stays within bounds
                if (!validate_buffer_bounds(buffer_tile_x, buffer_tile_y)) {
                    COORD_CONV_ERROR("WRAP_ERROR: Coordinates out of bounds after wrapping:",
                                  buffer_tile_x, ",", buffer_tile_y);
                }
            }
        }
    }
    
    void validate_edge_cases()
    {
        COORD_CONV_INFO("Testing coordinate conversion edge cases");
        
        // Test all four world corners
        const bn::fixed_point world_corners[] = {
            bn::fixed_point(0, 0),                                    // Bottom-left
            bn::fixed_point(WORLD_WIDTH_PIXELS - 1, 0),              // Bottom-right  
            bn::fixed_point(0, WORLD_HEIGHT_PIXELS - 1),              // Top-left
            bn::fixed_point(WORLD_WIDTH_PIXELS - 1, WORLD_HEIGHT_PIXELS - 1) // Top-right
        };
        
        const char* corner_names[] = {"BL", "BR", "TL", "TR"};
        
        for (int i = 0; i < 4; ++i) {
            if (g_chunk_manager) {
                bn::fixed_point buffer_pos = g_chunk_manager->world_to_buffer(world_corners[i]);
                bn::fixed_point back_to_world = g_chunk_manager->buffer_to_world(buffer_pos);
                
                COORD_CONV_INFO("CORNER_", corner_names[i], ":",
                             world_corners[i].x(), ",", world_corners[i].y(),
                             "->", buffer_pos.x(), ",", buffer_pos.y(),
                             "->", back_to_world.x(), ",", back_to_world.y());
                
                // Test bidirectional conversion consistency
                if (!test_bidirectional_conversion(world_corners[i])) {
                    COORD_CONV_ERROR("CORNER_ERROR: Bidirectional conversion failed for corner", corner_names[i]);
                }
            }
        }
        
        // Test buffer boundary transitions: wrapping at 511->0 coordinates
        COORD_CONV_INFO("Testing buffer boundary transitions");
        for (int boundary : {510, 511, 512, 513}) {
            bn::fixed_point test_pos(boundary * TILE_SIZE, boundary * TILE_SIZE);
            
            if (g_chunk_manager) {
                bn::fixed_point buffer_pos = g_chunk_manager->world_to_buffer(test_pos);
                int buffer_tile = (buffer_pos.x() + VIEW_BUFFER_TILES * TILE_SIZE / 2).integer() / TILE_SIZE;
                
                COORD_CONV_DEBUG("BOUNDARY_TEST:", boundary, "-> buffer tile", buffer_tile);
                
                // Should wrap to 0-511 range
                if (buffer_tile < 0 || buffer_tile >= VIEW_BUFFER_TILES) {
                    COORD_CONV_ERROR("BOUNDARY_FAIL: Tile", buffer_tile, "out of range [0,", VIEW_BUFFER_TILES-1, "]");
                }
            }
        }
        
        // Test negative coordinate handling (though world coordinates are positive)
        COORD_CONV_INFO("Testing negative coordinate safety");
        bn::fixed_point negative_pos(-100, -100);
        
        if (g_chunk_manager) {
            bn::fixed_point buffer_pos = g_chunk_manager->world_to_buffer(negative_pos);
            COORD_CONV_DEBUG("NEGATIVE_TEST: -100,-100 ->", buffer_pos.x(), ",", buffer_pos.y());
            
            // Should handle gracefully without crashing
            int buffer_tile_x = (buffer_pos.x() + VIEW_BUFFER_TILES * TILE_SIZE / 2).integer() / TILE_SIZE;
            int buffer_tile_y = (buffer_pos.y() + VIEW_BUFFER_TILES * TILE_SIZE / 2).integer() / TILE_SIZE;
            
            if (!validate_buffer_bounds(buffer_tile_x, buffer_tile_y)) {
                COORD_CONV_WARN("NEGATIVE_WARN: Negative coordinates resulted in out-of-bounds buffer position");
            }
        }
    }
    
    void track_origin_consistency()
    {
        COORD_CONV_INFO("Tracking buffer origin consistency");
        
        if (!g_chunk_manager) {
            COORD_CONV_ERROR("ORIGIN_ERROR: No chunk manager available for origin tracking");
            return;
        }
        
        int origin_x = g_chunk_manager->buffer_origin_x();
        int origin_y = g_chunk_manager->buffer_origin_y();
        
        COORD_CONV_INFO("CURRENT_ORIGIN:", origin_x, ",", origin_y);
        
        // Validate origin is within world bounds
        if (!validate_world_bounds(origin_x, origin_y)) {
            COORD_CONV_ERROR("ORIGIN_ERROR: Buffer origin out of world bounds:", origin_x, ",", origin_y);
        }
        
        // Test that origin changes correctly during movement simulation
        static int last_origin_x = origin_x;
        static int last_origin_y = origin_y;
        
        if (origin_x != last_origin_x || origin_y != last_origin_y) {
            COORD_CONV_INFO("ORIGIN_CHANGED: From", last_origin_x, ",", last_origin_y,
                         "to", origin_x, ",", origin_y);
            
            // Validate that origin changes are reasonable (not too large jumps)
            int delta_x = abs(origin_x - last_origin_x);
            int delta_y = abs(origin_y - last_origin_y);
            
            if (delta_x > CHUNK_SIZE_TILES || delta_y > CHUNK_SIZE_TILES) {
                COORD_CONV_WARN("ORIGIN_LARGE_JUMP: Delta", delta_x, ",", delta_y, "exceeds chunk size");
            }
            
            last_origin_x = origin_x;
            last_origin_y = origin_y;
        }
    }
    
    void validate_coordinate_transformations()
    {
        COORD_CONV_INFO("Validating coordinate transformation consistency");
        
        // Test bidirectional conversion consistency
        const bn::fixed_point test_positions[] = {
            bn::fixed_point(100, 200),        // Normal position
            bn::fixed_point(4000, 3000),     // Large position
            bn::fixed_point(8191, 8191),     // Maximum position
            bn::fixed_point(0, 0),           // Origin
            bn::fixed_point(1024, 1024)      // Chunk-aligned position
        };
        
        for (const auto& world_pos : test_positions) {
            if (test_bidirectional_conversion(world_pos)) {
                COORD_CONV_DEBUG("TRANSFORM_OK:", world_pos.x(), ",", world_pos.y());
            } else {
                COORD_CONV_ERROR("TRANSFORM_FAIL:", world_pos.x(), ",", world_pos.y());
            }
        }
    }
    
    void test_buffer_boundaries()
    {
        COORD_CONV_INFO("Testing buffer boundary conditions");
        
        // Test positions that should map to buffer edges
        for (int buffer_x = 0; buffer_x < VIEW_BUFFER_TILES; buffer_x += 32) {
            for (int buffer_y = 0; buffer_y < VIEW_BUFFER_TILES; buffer_y += 32) {
                bn::fixed_point buffer_center(0, 0);
                
                if (g_chunk_manager) {
                    // Convert buffer coordinates to world and back
                    bn::fixed_point world_pos = g_chunk_manager->buffer_to_world(buffer_center);
                    bn::fixed_point back_to_buffer = g_chunk_manager->world_to_buffer(world_pos);
                    
                    // Should maintain consistency
                    if (std::abs(back_to_buffer.x().data() - buffer_center.x().data()) > 100 ||
                        std::abs(back_to_buffer.y().data() - buffer_center.y().data()) > 100) {
                        COORD_CONV_WARN("BUFFER_CONSISTENCY: Large discrepancy at buffer position",
                                      buffer_x, ",", buffer_y);
                    }
                }
            }
        }
        
        COORD_CONV_INFO("Buffer boundary testing completed");
    }
    
    void test_world_boundaries()
    {
        COORD_CONV_INFO("Testing world boundary conditions");
        
        // Test positions at world edges
        const bn::fixed_point boundary_positions[] = {
            bn::fixed_point(0, 0),
            bn::fixed_point(WORLD_WIDTH_PIXELS - 1, 0),
            bn::fixed_point(0, WORLD_HEIGHT_PIXELS - 1),
            bn::fixed_point(WORLD_WIDTH_PIXELS - 1, WORLD_HEIGHT_PIXELS - 1),
            bn::fixed_point(WORLD_WIDTH_PIXELS / 2, WORLD_HEIGHT_PIXELS / 2)
        };
        
        for (const auto& pos : boundary_positions) {
            if (!validate_world_bounds(pos.x().integer(), pos.y().integer())) {
                COORD_CONV_ERROR("WORLD_BOUNDARY: Position out of bounds:", pos.x(), ",", pos.y());
            } else if (g_chunk_manager) {
                bn::fixed_point buffer_pos = g_chunk_manager->world_to_buffer(pos);
                if (!validate_buffer_bounds(
                    (buffer_pos.x() + VIEW_BUFFER_TILES * TILE_SIZE / 2).integer() / TILE_SIZE,
                    (buffer_pos.y() + VIEW_BUFFER_TILES * TILE_SIZE / 2).integer() / TILE_SIZE)) {
                    COORD_CONV_ERROR("WORLD_TO_BUFFER: Valid world pos mapped to invalid buffer pos");
                }
            }
        }
        
        COORD_CONV_INFO("World boundary testing completed");
    }
    
    void validate_precision_consistency()
    {
        COORD_CONV_INFO("Validating coordinate precision consistency");
        
        // Test bn::fixed vs integer calculations
        bn::fixed_pos test_world = bn::fixed_point(128.5, 256.75);
        
        if (g_chunk_manager) {
            bn::fixed_point buffer_pos = g_chunk_manager->world_to_buffer(test_world);
            bn::fixed_point back_to_world = g_chunk_manager->buffer_to_world(buffer_pos);
            
            COORD_CONV_INFO("PRECISION_TEST: World (", test_world.x(), ",", test_world.y(), 
                         ") -> Buffer (", buffer_pos.x(), ",", buffer_pos.y(), 
                         ") -> World (", back_to_world.x(), ",", back_to_world.y(), ")");
            
            // Check for precision loss
            bn::fixed world_diff_x = back_to_world.x() - test_world.x();
            bn::fixed world_diff_y = back_to_world.y() - test_world.y();
            
            if (std::abs(world_diff_x.data()) > 10 || std::abs(world_diff_y.data()) > 10) {
                COORD_CONV_WARN("PRECISION_LOSS: Significant precision difference detected");
            }
        }
        
        COORD_CONV_INFO("Precision validation completed");
    }
    
    void stress_test_coordinate_calculations()
    {
        COORD_CONV_INFO("Running coordinate calculation stress tests");
        
        // Rapid movement across buffer boundaries simulation
        int stress_errors = 0;
        int stress_tests = 100;
        
        for (int i = 0; i < stress_tests; ++i) {
            // Generate pseudo-random test position
            int test_x = (i * 73) % WORLD_WIDTH_PIXELS;  // Pseudo-random but deterministic
            int test_y = (i * 37) % WORLD_HEIGHT_PIXELS;
            
            bn::fixed_point test_pos(test_x, test_y);
            
            if (g_chunk_manager) {
                bn::fixed_point buffer_pos = g_chunk_manager->world_to_buffer(test_pos);
                bn::fixed_point back_to_world = g_chunk_manager->buffer_to_world(buffer_pos);
                
                // Validate consistency
                if (!test_bidirectional_conversion(test_pos)) {
                    stress_errors++;
                }
            }
        }
        
        COORD_CONV_INFO("STRESS_RESULTS:", stress_tests - stress_errors, "/", stress_tests, "passed");
        
        if (stress_errors > 0) {
            COORD_CONV_WARN("STRESS_ERRORS:", stress_errors, "coordinate conversion failures detected");
        }
        
        // Continuous operation at world edges
        COORD_CONV_INFO("Testing continuous operation at world edges");
        for (int edge_test = 0; edge_test < 10; ++edge_test) {
            bn::fixed_point edge_pos(WORLD_WIDTH_PIXELS - 1 - edge_test, WORLD_HEIGHT_PIXELS - 1 - edge_test);
            
            if (g_chunk_manager && test_bidirectional_conversion(edge_pos)) {
                COORD_CONV_DEBUG("EDGE_CONTINUOUS: Position", edge_pos.x(), ",", edge_pos.y(), "stable");
            }
        }
        
        COORD_CONV_INFO("Stress testing completed");
    }
    
    bool validate_buffer_bounds(int buffer_x, int buffer_y)
    {
        return buffer_x >= 0 && buffer_x < VIEW_BUFFER_TILES &&
               buffer_y >= 0 && buffer_y < VIEW_BUFFER_TILES;
    }
    
    bool validate_world_bounds(int world_x, int world_y)
    {
        return world_x >= MIN_WORLD_COORD && world_x <= MAX_WORLD_COORD &&
               world_y >= MIN_WORLD_COORD && world_y <= MAX_WORLD_COORD;
    }
    
    bool test_bidirectional_conversion(const bn::fixed_point& world_pos)
    {
        if (!g_chunk_manager) {
            return false;
        }
        
        bn::fixed_point buffer_pos = g_chunk_manager->world_to_buffer(world_pos);
        bn::fixed_point back_to_world = g_chunk_manager->buffer_to_world(buffer_pos);
        
        // Allow small tolerance for bn::fixed precision
        bn::fixed tolerance = bn::fixed(1.0);
        bn::fixed diff_x = back_to_world.x() - world_pos.x();
        bn::fixed diff_y = back_to_world.y() - world_pos.y();
        
        return std::abs(diff_x.data()) <= tolerance.data() && 
               std::abs(diff_y.data()) <= tolerance.data();
    }
    
    // Global chunk manager reference for testing
    ChunkManager* g_chunk_manager = nullptr;
}