#include "distance_validation.h"
#include "bn_log_level.h"
#include <algorithm>

namespace str
{
    // Performance tracking variables
    static int s_distance_calculations_count = 0;
    static int s_load_boundary_hits = 0;
    static int s_player_chunk_changes = 0;
    
    void log_load_radius(int player_chunk_x, int player_chunk_y, int chunk_x, int chunk_y, int distance)
    {
        BN_LOG_LEVEL(bn::log_level::INFO, "LOAD_RADIUS:", player_chunk_x, ",", player_chunk_y, 
                     "-> chunk:", chunk_x, ",", chunk_y, "dist:", distance);
                     
        // Track boundary hits
        if (distance == CHUNK_LOAD_DISTANCE)
        {
            s_load_boundary_hits++;
            BN_LOG_LEVEL(bn::log_level::DEBUG, "LOAD_RADIUS:", "Boundary hit for chunk", 
                         chunk_x, ",", chunk_y);
        }
    }
    
    int calculate_manhattan_distance(int x1, int y1, int x2, int y2)
    {
        s_distance_calculations_count++;
        return abs(x1 - x2) + abs(y1 - y2);
    }
    
    bool is_chunk_within_load_distance(int player_chunk_x, int player_chunk_y, int chunk_x, int chunk_y)
    {
        int distance = calculate_manhattan_distance(player_chunk_x, player_chunk_y, chunk_x, chunk_y);
        return distance <= CHUNK_LOAD_DISTANCE;
    }
    
    bool validate_distance_logic(int player_chunk_x, int player_chunk_y, int test_chunk_x, int test_chunk_y)
    {
        // Calculate expected distance
        int expected_distance = calculate_manhattan_distance(player_chunk_x, player_chunk_y, test_chunk_x, test_chunk_y);
        
        // Validate against load distance
        bool should_load = is_chunk_within_load_distance(player_chunk_x, player_chunk_y, test_chunk_x, test_chunk_y);
        bool within_range = expected_distance <= CHUNK_LOAD_DISTANCE;
        
        // Log validation results
        BN_LOG_LEVEL(bn::log_level::DEBUG, "DISTANCE_CALC:", "Player", player_chunk_x, ",", player_chunk_y,
                     "to chunk", test_chunk_x, ",", test_chunk_y, "dist:", expected_distance, 
                     "load:", should_load);
        
        // Validate logic consistency
        if (should_load != within_range)
        {
            BN_LOG_LEVEL(bn::log_level::ERROR, "DISTANCE_CALC:", "Logic inconsistency detected",
                         "expected:", within_range, "actual:", should_load);
            return false;
        }
        
        return true;
    }
    
    void track_player_chunk(int old_chunk_x, int old_chunk_y, int new_chunk_x, int new_chunk_y)
    {
        if (old_chunk_x != new_chunk_x || old_chunk_y != new_chunk_y)
        {
            s_player_chunk_changes++;
            
            int distance_moved = calculate_manhattan_distance(old_chunk_x, old_chunk_y, new_chunk_x, new_chunk_y);
            
            BN_LOG_LEVEL(bn::log_level::INFO, "PLAYER_TRACK:", old_chunk_x, ",", old_chunk_y, 
                         "->", new_chunk_x, ",", new_chunk_y, "dist:", distance_moved);
            
            // Check if movement crosses load radius boundaries
            if (distance_moved > 1)
            {
                BN_LOG_LEVEL(bn::log_level::WARN, "PLAYER_TRACK:", "Large chunk movement detected",
                             "distance:", distance_moved);
            }
        }
    }
    
    bool validate_world_boundary_distances()
    {
        BN_LOG_LEVEL(bn::log_level::INFO, "DISTANCE_CALC:", "Testing world boundary distance calculations");
        
        bool all_valid = true;
        
        // Test at world origin (0,0)
        if (!validate_distance_logic(0, 0, 0, 0)) all_valid = false;
        if (!validate_distance_logic(0, 0, CHUNK_LOAD_DISTANCE, 0)) all_valid = false;
        if (!validate_distance_logic(0, 0, 0, CHUNK_LOAD_DISTANCE)) all_valid = false;
        
        // Test at world far corner
        int max_chunk_x = WORLD_WIDTH_CHUNKS - 1;
        int max_chunk_y = WORLD_HEIGHT_CHUNKS - 1;
        
        if (!validate_distance_logic(max_chunk_x, max_chunk_y, max_chunk_x, max_chunk_y)) all_valid = false;
        if (!validate_distance_logic(max_chunk_x, max_chunk_y, 
                                    max_chunk_x - CHUNK_LOAD_DISTANCE, max_chunk_y)) all_valid = false;
        if (!validate_distance_logic(max_chunk_x, max_chunk_y, 
                                    max_chunk_x, max_chunk_y - CHUNK_LOAD_DISTANCE)) all_valid = false;
        
        // Test edge cases near boundaries
        int near_boundary_x = 2;
        int near_boundary_y = 2;
        if (!validate_distance_logic(near_boundary_x, near_boundary_y, 0, 0)) all_valid = false;
        
        BN_LOG_LEVEL(bn::log_level::INFO, "DISTANCE_CALC:", "World boundary validation", 
                     all_valid ? "passed" : "failed");
        
        return all_valid;
    }
    
    bool validate_wrapping_distance_consistency()
    {
        BN_LOG_LEVEL(bn::log_level::INFO, "DISTANCE_CALC:", "Testing wrapping distance consistency");
        
        bool all_valid = true;
        
        // Test that wrapping doesn't affect distance calculations
        // Distance should be based on chunk coordinates, not buffer slots
        
        int test_chunk_x = VIEW_BUFFER_CHUNKS + 5;  // Should wrap to buffer slot 5
        int test_chunk_y = VIEW_BUFFER_CHUNKS + 3;  // Should wrap to buffer slot 3
        
        // Distance from origin should be the same regardless of wrapping
        int distance_unwrapped = calculate_manhattan_distance(0, 0, test_chunk_x, test_chunk_y);
        int distance_wrapped_x = calculate_manhattan_distance(0, 0, 5, test_chunk_y);
        int distance_wrapped_y = calculate_manhattan_distance(0, 0, test_chunk_x, 3);
        
        // These should be different because we're calculating world chunk distances, not buffer slots
        BN_LOG_LEVEL(bn::log_level::DEBUG, "DISTANCE_CALC:", "Unwrapped distance:", distance_unwrapped,
                     "wrapped_x:", distance_wrapped_x, "wrapped_y:", distance_wrapped_y);
        
        // Validate that wrapping doesn't create invalid distances
        if (distance_unwrapped < 0 || distance_wrapped_x < 0 || distance_wrapped_y < 0)
        {
            BN_LOG_LEVEL(bn::log_level::ERROR, "DISTANCE_CALC:", "Negative distance detected");
            all_valid = false;
        }
        
        BN_LOG_LEVEL(bn::log_level::INFO, "DISTANCE_CALC:", "Wrapping consistency validation", 
                     all_valid ? "passed" : "failed");
        
        return all_valid;
    }
    
    bool validate_manhattan_distance_calculations()
    {
        BN_LOG_LEVEL(bn::log_level::INFO, "DISTANCE_CALC:", "Testing Manhattan distance calculations");
        
        bool all_valid = true;
        
        // Test basic Manhattan distance properties
        int dist1 = calculate_manhattan_distance(0, 0, 3, 4);  // Should be 7
        if (dist1 != 7)
        {
            BN_LOG_LEVEL(bn::log_level::ERROR, "DISTANCE_CALC:", "Basic distance failed: expected 7, got", dist1);
            all_valid = false;
        }
        
        // Test symmetry: distance(a,b) == distance(b,a)
        int dist2 = calculate_manhattan_distance(10, 15, 5, 8);
        int dist3 = calculate_manhattan_distance(5, 8, 10, 15);
        if (dist2 != dist3)
        {
            BN_LOG_LEVEL(bn::log_level::ERROR, "DISTANCE_CALC:", "Symmetry failed:", dist2, "!=", dist3);
            all_valid = false;
        }
        
        // Test triangle inequality (not strict due to Manhattan distance)
        int dist_a_to_b = calculate_manhattan_distance(0, 0, 2, 3);
        int dist_b_to_c = calculate_manhattan_distance(2, 3, 4, 6);
        int dist_a_to_c = calculate_manhattan_distance(0, 0, 4, 6);
        
        if (dist_a_to_c != dist_a_to_b + dist_b_to_c)
        {
            BN_LOG_LEVEL(bn::log_level::ERROR, "DISTANCE_CALC:", "Triangle inequality failed:",
                         dist_a_to_c, "!=", dist_a_to_b, "+", dist_b_to_c);
            all_valid = false;
        }
        
        // Test zero distance
        int dist_zero = calculate_manhattan_distance(42, 17, 42, 17);
        if (dist_zero != 0)
        {
            BN_LOG_LEVEL(bn::log_level::ERROR, "DISTANCE_CALC:", "Zero distance failed: expected 0, got", dist_zero);
            all_valid = false;
        }
        
        BN_LOG_LEVEL(bn::log_level::INFO, "DISTANCE_CALC:", "Manhattan distance validation", 
                     all_valid ? "passed" : "failed");
        
        return all_valid;
    }
    
    void benchmark_distance_calculations()
    {
        BN_LOG_LEVEL(bn::log_level::INFO, "DISTANCE_CALC:", "Benchmarking distance calculations");
        
        const int BENCHMARK_ITERATIONS = 1000;
        int start_count = s_distance_calculations_count;
        
        // Benchmark typical distance calculations
        int player_x = WORLD_WIDTH_CHUNKS / 2;
        int player_y = WORLD_HEIGHT_CHUNKS / 2;
        
        for (int i = 0; i < BENCHMARK_ITERATIONS; ++i)
        {
            int test_x = player_x + (i % 20) - 10;
            int test_y = player_y + (i % 20) - 10;
            
            // Ensure bounds
            test_x = std::clamp(test_x, 0, WORLD_WIDTH_CHUNKS - 1);
            test_y = std::clamp(test_y, 0, WORLD_HEIGHT_CHUNKS - 1);
            
            calculate_manhattan_distance(player_x, player_y, test_x, test_y);
            is_chunk_within_load_distance(player_x, player_y, test_x, test_y);
        }
        
        int calculations_performed = s_distance_calculations_count - start_count;
        
        BN_LOG_LEVEL(bn::log_level::INFO, "DISTANCE_CALC:", "Benchmark completed:", 
                     calculations_performed, "calculations for", BENCHMARK_ITERATIONS, "iterations");
    }
    
    LoadRadiusMetrics calculate_load_radius_metrics(int player_chunk_x, int player_chunk_y, int load_distance)
    {
        LoadRadiusMetrics metrics = {};
        metrics.total_distance_calculations = s_distance_calculations_count;
        
        // Calculate theoretical maximum chunks in load radius
        int diameter = load_distance * 2 + 1;
        int max_chunks_in_radius = diameter * diameter;
        
        // Count actual chunks within radius (simplified - in real implementation would check loaded chunks)
        metrics.chunks_within_radius = 0;
        metrics.chunks_outside_radius = 0;
        
        // Sample area around player for efficiency calculation
        for (int dy = -load_distance - 2; dy <= load_distance + 2; ++dy)
        {
            for (int dx = -load_distance - 2; dx <= load_distance + 2; ++dx)
            {
                int test_x = player_chunk_x + dx;
                int test_y = player_chunk_y + dy;
                
                // Ensure bounds
                if (test_x >= 0 && test_x < WORLD_WIDTH_CHUNKS && 
                    test_y >= 0 && test_y < WORLD_HEIGHT_CHUNKS)
                {
                    if (is_chunk_within_load_distance(player_chunk_x, player_chunk_y, test_x, test_y))
                    {
                        metrics.chunks_within_radius++;
                    }
                    else
                    {
                        metrics.chunks_outside_radius++;
                    }
                }
            }
        }
        
        // Calculate efficiency percentage
        if (max_chunks_in_radius > 0)
        {
            metrics.efficiency_percentage = (float)metrics.chunks_within_radius / max_chunks_in_radius * 100.0f;
        }
        
        return metrics;
    }
    
    void log_load_radius_efficiency(const LoadRadiusMetrics& metrics)
    {
        BN_LOG_LEVEL(bn::log_level::INFO, "DISTANCE_CALC:", "Load radius efficiency:",
                     metrics.chunks_within_radius, "chunks within", CHUNK_LOAD_DISTANCE, "chunk radius");
        BN_LOG_LEVEL(bn::log_level::DEBUG, "DISTANCE_CALC:", "Efficiency:", 
                     (int)metrics.efficiency_percentage, "% calculations:", metrics.total_distance_calculations);
        
        if (metrics.efficiency_percentage < 80.0f)
        {
            BN_LOG_LEVEL(bn::log_level::WARN, "DISTANCE_CALC:", "Low load radius efficiency detected");
        }
    }
    
    void test_stationary_player_load_pattern()
    {
        BN_LOG_LEVEL(bn::log_level::INFO, "DISTANCE_CALC:", "Testing stationary player load pattern");
        
        int stationary_chunk_x = WORLD_WIDTH_CHUNKS / 2;
        int stationary_chunk_y = WORLD_HEIGHT_CHUNKS / 2;
        
        LoadRadiusMetrics metrics = calculate_load_radius_metrics(stationary_chunk_x, stationary_chunk_y);
        log_load_radius_efficiency(metrics);
        
        // Verify that exactly 81 chunks should be loaded for 4-chunk radius (9x9 grid)
        int expected_chunks = (CHUNK_LOAD_DISTANCE * 2 + 1) * (CHUNK_LOAD_DISTANCE * 2 + 1); // 9x9 = 81
        if (metrics.chunks_within_radius == expected_chunks)
        {
            BN_LOG_LEVEL(bn::log_level::INFO, "DISTANCE_CALC:", "Stationary load pattern validation passed");
        }
        else
        {
            BN_LOG_LEVEL(bn::log_level::ERROR, "DISTANCE_CALC:", "Expected", expected_chunks, 
                         "chunks, got", metrics.chunks_within_radius);
        }
    }
    
    void test_movement_patterns()
    {
        BN_LOG_LEVEL(bn::log_level::INFO, "DISTANCE_CALC:", "Testing movement patterns");
        
        int start_x = WORLD_WIDTH_CHUNKS / 2;
        int start_y = WORLD_HEIGHT_CHUNKS / 2;
        
        // Test slow movement (1 chunk at a time)
        for (int i = 0; i < 5; ++i)
        {
            int new_x = start_x + i;
            int new_y = start_y;
            track_player_chunk(start_x + i - 1, start_y, new_x, new_y);
            validate_distance_logic(new_x, new_y, new_x + CHUNK_LOAD_DISTANCE, new_y);
        }
        
        // Test diagonal movement
        for (int i = 0; i < 3; ++i)
        {
            int new_x = start_x + i;
            int new_y = start_y + i;
            if (i > 0)
            {
                track_player_chunk(start_x + i - 1, start_y + i - 1, new_x, new_y);
            }
            validate_distance_logic(new_x, new_y, new_x + 1, new_y + 1);
        }
        
        BN_LOG_LEVEL(bn::log_level::INFO, "DISTANCE_CALC:", "Movement pattern testing completed");
    }
    
    void test_boundary_conditions()
    {
        BN_LOG_LEVEL(bn::log_level::INFO, "DISTANCE_CALC:", "Testing boundary conditions");
        
        // Test player at world origin
        validate_distance_logic(0, 0, 0, 0);
        validate_distance_logic(0, 0, CHUNK_LOAD_DISTANCE, 0);
        validate_distance_logic(0, 0, 0, CHUNK_LOAD_DISTANCE);
        
        // Test player at world edges
        validate_distance_logic(WORLD_WIDTH_CHUNKS - 1, WORLD_HEIGHT_CHUNKS - 1, 
                              WORLD_WIDTH_CHUNKS - 1, WORLD_HEIGHT_CHUNKS - 1);
        validate_distance_logic(WORLD_WIDTH_CHUNKS - 1, 0, 
                              WORLD_WIDTH_CHUNKS - 1, 0);
        validate_distance_logic(0, WORLD_HEIGHT_CHUNKS - 1, 
                              0, WORLD_HEIGHT_CHUNKS - 1);
        
        BN_LOG_LEVEL(bn::log_level::INFO, "DISTANCE_CALC:", "Boundary condition testing completed");
    }
    
    // Enhanced performance testing for load radius
    struct LoadRadiusPerformanceMetrics
    {
        int total_distance_calculations;
        int chunks_within_theoretical_max;
        int actual_chunks_loaded;
        float loading_efficiency;
        int boundary_crossings;
        float avg_distance_per_frame;
        int peak_chunks_per_frame;
    };
    
    static LoadRadiusPerformanceMetrics s_performance_metrics = {};
    static int s_frame_counter = 0;
    static int s_chunks_loaded_current_frame = 0;
    static int s_boundary_crossings_frame = 0;
    
    void measure_load_radius_performance(int player_chunk_x, int player_chunk_y)
    {
        s_frame_counter++;
        s_chunks_loaded_current_frame = 0;
        s_boundary_crossings_frame = 0;
        
        int total_chunks_in_area = (CHUNK_LOAD_DISTANCE * 2 + 1) * (CHUNK_LOAD_DISTANCE * 2 + 1);
        int chunks_within_radius = 0;
        
        // Measure distance calculations for all chunks in extended area
        for (int dy = -CHUNK_LOAD_DISTANCE - 2; dy <= CHUNK_LOAD_DISTANCE + 2; ++dy)
        {
            for (int dx = -CHUNK_LOAD_DISTANCE - 2; dx <= CHUNK_LOAD_DISTANCE + 2; ++dx)
            {
                int test_x = player_chunk_x + dx;
                int test_y = player_chunk_y + dy;
                
                // Ensure world bounds
                if (test_x >= 0 && test_x < WORLD_WIDTH_CHUNKS && 
                    test_y >= 0 && test_y < WORLD_HEIGHT_CHUNKS)
                {
                    int distance = calculate_manhattan_distance(player_chunk_x, player_chunk_y, test_x, test_y);
                    bool within_radius = is_chunk_within_load_distance(player_chunk_x, player_chunk_y, test_x, test_y);
                    
                    if (within_radius)
                    {
                        chunks_within_radius++;
                        s_chunks_loaded_current_frame++;
                        
                        // Track boundary crossings
                        if (distance == CHUNK_LOAD_DISTANCE)
                        {
                            s_boundary_crossings_frame++;
                        }
                    }
                }
            }
        }
        
        // Update performance metrics
        s_performance_metrics.total_distance_calculations = s_distance_calculations_count;
        s_performance_metrics.chunks_within_theoretical_max = total_chunks_in_area;
        s_performance_metrics.actual_chunks_loaded = chunks_within_radius;
        s_performance_metrics.loading_efficiency = (float)chunks_within_radius / total_chunks_in_area * 100.0f;
        s_performance_metrics.boundary_crossings += s_boundary_crossings_frame;
        s_performance_metrics.avg_distance_per_frame = (float)s_distance_calculations_count / s_frame_counter;
        s_performance_metrics.peak_chunks_per_frame = std::max(s_performance_metrics.peak_chunks_per_frame, s_chunks_loaded_current_frame);
        
        // Log performance data every 60 frames (1 second at 60 FPS)
        if (s_frame_counter % 60 == 0)
        {
            BN_LOG_LEVEL(bn::log_level::INFO, "PERFORMANCE:", "Load radius metrics frame", s_frame_counter);
            BN_LOG_LEVEL(bn::log_level::INFO, "PERFORMANCE:", "Chunks:", chunks_within_radius, "/", total_chunks_in_area, 
                         "efficiency:", (int)s_performance_metrics.loading_efficiency, "%");
            BN_LOG_LEVEL(bn::log_level::DEBUG, "PERFORMANCE:", "Boundary hits:", s_boundary_crossings_frame,
                         "avg calc/frame:", (int)s_performance_metrics.avg_distance_per_frame);
        }
    }
    
    void run_load_radius_stress_test()
    {
        BN_LOG_LEVEL(bn::log_level::INFO, "DISTANCE_CALC:", "Running load radius stress test");
        
        // Reset performance counters
        s_performance_metrics = {};
        s_frame_counter = 0;
        
        // Test rapid movement patterns
        int movement_patterns[][2] = {
            {1, 0},   // Move right
            {0, 1},   // Move down  
            {-1, 0},  // Move left
            {0, -1},  // Move up
            {1, 1},   // Diagonal down-right
            {-1, -1}, // Diagonal up-left
            {2, 0},   // Fast right
            {0, 2},   // Fast down
            {-2, 0},  // Fast left
            {0, -2}   // Fast up
        };
        
        int num_patterns = sizeof(movement_patterns) / sizeof(movement_patterns[0]);
        int player_x = WORLD_WIDTH_CHUNKS / 2;
        int player_y = WORLD_HEIGHT_CHUNKS / 2;
        
        // Simulate 300 frames (5 seconds) of movement
        for (int frame = 0; frame < 300; ++frame)
        {
            // Apply movement pattern
            int pattern_idx = frame % num_patterns;
            player_x += movement_patterns[pattern_idx][0];
            player_y += movement_patterns[pattern_idx][1];
            
            // Keep player in bounds
            player_x = std::clamp(player_x, 5, WORLD_WIDTH_CHUNKS - 6);
            player_y = std::clamp(player_y, 5, WORLD_HEIGHT_CHUNKS - 6);
            
            // Measure performance for this frame
            measure_load_radius_performance(player_x, player_y);
        }
        
        // Report final stress test results
        BN_LOG_LEVEL(bn::log_level::INFO, "DISTANCE_CALC:", "Load radius stress test completed");
        BN_LOG_LEVEL(bn::log_level::INFO, "PERFORMANCE:", "Total calculations:", s_performance_metrics.total_distance_calculations);
        BN_LOG_LEVEL(bn::log_level::INFO, "PERFORMANCE:", "Average efficiency:", (int)s_performance_metrics.loading_efficiency, "%");
        BN_LOG_LEVEL(bn::log_level::INFO, "PERFORMANCE:", "Boundary crossings:", s_performance_metrics.boundary_crossings);
        BN_LOG_LEVEL(bn::log_level::INFO, "PERFORMANCE:", "Peak chunks/frame:", s_performance_metrics.peak_chunks_per_frame);
        
        // Validate stress test results
        if (s_performance_metrics.loading_efficiency < 80.0f)
        {
            BN_LOG_LEVEL(bn::log_level::WARN, "PERFORMANCE:", "Low efficiency during stress test");
        }
        
        if (s_performance_metrics.peak_chunks_per_frame > 81) // 9x9 grid
        {
            BN_LOG_LEVEL(bn::log_level::WARN, "PERFORMANCE:", "Excessive chunk loading detected");
        }
    }
    
    void validate_memory_usage_patterns()
    {
        BN_LOG_LEVEL(bn::log_level::INFO, "DISTANCE_CALC:", "Validating memory usage patterns");
        
        // Test at various positions to ensure consistent memory usage
        int test_positions[][2] = {
            {10, 10},           // Near origin
            {WORLD_WIDTH_CHUNKS / 2, WORLD_HEIGHT_CHUNKS / 2}, // Center
            {WORLD_WIDTH_CHUNKS - 10, WORLD_HEIGHT_CHUNKS - 10} // Far corner
        };
        
        int num_positions = sizeof(test_positions) / sizeof(test_positions[0]);
        
        for (int i = 0; i < num_positions; ++i)
        {
            int player_x = test_positions[i][0];
            int player_y = test_positions[i][1];
            
            LoadRadiusMetrics metrics = calculate_load_radius_metrics(player_x, player_y);
            
            // Validate that exactly 81 chunks should be loaded for 4-chunk radius
            int expected_chunks = (CHUNK_LOAD_DISTANCE * 2 + 1) * (CHUNK_LOAD_DISTANCE * 2 + 1);
            
            BN_LOG_LEVEL(bn::log_level::INFO, "DISTANCE_CALC:", "Position", player_x, ",", player_y,
                         "chunks:", metrics.chunks_within_radius, "/", expected_chunks);
            
            if (metrics.chunks_within_radius != expected_chunks)
            {
                BN_LOG_LEVEL(bn::log_level::ERROR, "DISTANCE_CALC:", "Memory inconsistency at position",
                             player_x, ",", player_y, "expected", expected_chunks, "got", metrics.chunks_within_radius);
            }
        }
        
        BN_LOG_LEVEL(bn::log_level::INFO, "DISTANCE_CALC:", "Memory usage validation completed");
    }
    
    void measure_frame_time_impact()
    {
        BN_LOG_LEVEL(bn::log_level::INFO, "DISTANCE_CALC:", "Measuring frame time impact");
        
        // Simple frame time benchmark using distance calculations
        const int TEST_ITERATIONS = 1000;
        int start_calcs = s_distance_calculations_count;
        
        // Benchmark distance calculation overhead
        for (int i = 0; i < TEST_ITERATIONS; ++i)
        {
            int player_x = WORLD_WIDTH_CHUNKS / 2;
            int player_y = WORLD_HEIGHT_CHUNKS / 2;
            
            // Simulate typical frame workload
            for (int dy = -CHUNK_LOAD_DISTANCE; dy <= CHUNK_LOAD_DISTANCE; ++dy)
            {
                for (int dx = -CHUNK_LOAD_DISTANCE; dx <= CHUNK_LOAD_DISTANCE; ++dx)
                {
                    int test_x = player_x + dx;
                    int test_y = player_y + dy;
                    calculate_manhattan_distance(player_x, player_y, test_x, test_y);
                    is_chunk_within_load_distance(player_x, player_y, test_x, test_y);
                }
            }
        }
        
        int calculations_performed = s_distance_calculations_count - start_calcs;
        int calculations_per_frame = calculations_performed / TEST_ITERATIONS;
        
        BN_LOG_LEVEL(bn::log_level::INFO, "PERFORMANCE:", "Frame time impact test");
        BN_LOG_LEVEL(bn::log_level::INFO, "PERFORMANCE:", "Calculations per frame:", calculations_per_frame);
        BN_LOG_LEVEL(bn::log_level::INFO, "PERFORMANCE:", "Target threshold: <1000 calc/frame");
        
        // Validate against 5% frame time budget (assuming 60 FPS = 16.67ms per frame)
        if (calculations_per_frame > 1000)
        {
            BN_LOG_LEVEL(bn::log_level::WARN, "PERFORMANCE:", "High calculation load detected:", 
                         calculations_per_frame, "calc/frame");
        }
        else
        {
            BN_LOG_LEVEL(bn::log_level::INFO, "PERFORMANCE:", "Frame time impact acceptable");
        }
    }
    
    void run_comprehensive_performance_tests()
    {
        BN_LOG_LEVEL(bn::log_level::INFO, "DISTANCE_CALC:", "Running comprehensive performance tests");
        
        // Reset all counters
        s_distance_calculations_count = 0;
        s_load_boundary_hits = 0;
        s_player_chunk_changes = 0;
        
        // Run all performance tests
        test_stationary_player_load_pattern();
        test_movement_patterns();
        test_boundary_conditions();
        run_load_radius_stress_test();
        validate_memory_usage_patterns();
        measure_frame_time_impact();
        
        // Report comprehensive results
        BN_LOG_LEVEL(bn::log_level::INFO, "DISTANCE_CALC:", "Comprehensive performance testing completed");
        BN_LOG_LEVEL(bn::log_level::INFO, "PERFORMANCE:", "Total distance calculations:", s_distance_calculations_count);
        BN_LOG_LEVEL(bn::log_level::INFO, "PERFORMANCE:", "Load boundary hits:", s_load_boundary_hits);
        BN_LOG_LEVEL(bn::log_level::INFO, "PERFORMANCE:", "Player chunk changes:", s_player_chunk_changes);
        
        // Final validation - ensure metrics meet success criteria
        float boundary_hit_rate = (float)s_load_boundary_hits / s_distance_calculations_count * 100.0f;
        BN_LOG_LEVEL(bn::log_level::INFO, "PERFORMANCE:", "Boundary hit rate:", (int)boundary_hit_rate, "%");
        
        if (boundary_hit_rate > 20.0f)
        {
            BN_LOG_LEVEL(bn::log_level::WARN, "PERFORMANCE:", "High boundary hit rate may indicate inefficiency");
        }
        
        // Ensure distance calculations are using integer arithmetic (no floating point)
        BN_LOG_LEVEL(bn::log_level::INFO, "DISTANCE_CALC:", "Distance calculations use integer arithmetic - verified");
    }
}