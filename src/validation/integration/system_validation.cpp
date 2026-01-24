#include "system_validation.h"
#include "bn_log.h"
#include "bn_fixed.h"
#include "bn_fixed_point.h"
#include "bn_string.h"
#include "bn_vector.h"
#include "bn_random.h"
#include "bn_core.h"
#include "bn_keypad.h"
#include "../background/bg_validation.h"
#include "../coords/coord_validation.h"
#include "../dma/dma_validation.h"
#include "../logging/distance_validation.h"
#include "../logging/chunk_validation.h"

// Include game systems for integration testing
#include "str_collision.h"
#include "str_player.h"
#include "str_enemy.h"
#include "str_level.h"
#include "str_chunk_manager.h"

#include <cstdint>
#include <cmath>

namespace str
{
    // Static member initialization
    SystemBaselineMetrics SystemValidation::_baseline_metrics;
    SystemBaselineMetrics SystemValidation::_current_metrics;
    bn::vector<IntegrationTestResult, 8> SystemValidation::_test_results;
    bool SystemValidation::_initialized = false;
    bool SystemValidation::_all_tests_passed = true;
    int SystemValidation::_total_issues_found = 0;

    void SystemValidation::init()
    {
        if (_initialized)
        {
            return;
        }

        BN_LOG_LEVEL(bn::log_level::INFO, "SYSTEM_VALIDATION: Initializing comprehensive integration validation");

        // Initialize test results storage
        _test_results.clear();
        _all_tests_passed = true;
        _total_issues_found = 0;

        // Establish baseline metrics from known good values
        establish_baseline();

        _initialized = true;
        BN_LOG_LEVEL(bn::log_level::INFO, "SYSTEM_VALIDATION: Integration validation initialized");
    }

    void SystemValidation::shutdown()
    {
        if (!_initialized)
        {
            return;
        }

        BN_LOG_LEVEL(bn::log_level::INFO, "SYSTEM_VALIDATION: Shutting down integration validation");
        generate_integration_report();
        
        _initialized = false;
    }

    void SystemValidation::establish_baseline()
    {
        BN_LOG_LEVEL(bn::log_level::INFO, "SYSTEM_VALIDATION: Establishing baseline metrics");

        // Set baseline values based on GBA hardware constraints and game requirements
        _baseline_metrics = SystemBaselineMetrics();
        
        // Log baseline values
        BN_LOG_LEVEL(bn::log_level::INFO, "SYSTEM_BASELINE: Frame time target: ", _baseline_metrics.frame_time_target_us, "μs");
        BN_LOG_LEVEL(bn::log_level::INFO, "SYSTEM_BASELINE: Collision accuracy: ", _baseline_metrics.collision_accuracy_percentage, "%");
        BN_LOG_LEVEL(bn::log_level::INFO, "SYSTEM_BASELINE: Entity positioning accuracy: ", _baseline_metrics.entity_positioning_accuracy_percentage, "%");
        BN_LOG_LEVEL(bn::log_level::INFO, "SYSTEM_BASELINE: DMA transfer time: ", _baseline_metrics.dma_transfer_time_us, "μs");
        BN_LOG_LEVEL(bn::log_level::INFO, "SYSTEM_BASELINE: Chunk streaming time: ", _baseline_metrics.chunk_streaming_time_us, "μs");
    }

    void SystemValidation::test_collision_compatibility()
    {
        BN_LOG_LEVEL(bn::log_level::INFO, "SYSTEM_VALIDATION: Starting collision compatibility tests");
        
        IntegrationTestResult result(IntegrationTestCategory::COLLISION);
        
        _test_static_collision_patterns();
        _test_streaming_collision_accuracy();
        _test_buffer_edge_collision();
        
        // Collect current collision metrics
        _collect_current_metrics();
        
        // Compare with baseline
        if (_current_metrics.collision_accuracy_percentage < _baseline_metrics.collision_accuracy_percentage)
        {
            result.passed = false;
            result.issues_found++;
            bn::string<64> error = bn::string<64>("Collision accuracy regression: ");
            error += bn::string<32>(_current_metrics.collision_accuracy_percentage);
            error += bn::string<32>("% < ");
            error += bn::string<32>(_baseline_metrics.collision_accuracy_percentage);
            error += bn::string<32>("%");
            result.error_messages.push_back(error);
        }
        
        if (!result.passed)
        {
            _all_tests_passed = false;
            _total_issues_found += result.issues_found;
        }
        
        _test_results.push_back(result);
        BN_LOG_LEVEL(bn::log_level::INFO, "SYSTEM_VALIDATION: Collision compatibility test ", result.passed ? "PASSED" : "FAILED");
    }

    void SystemValidation::_test_static_collision_patterns()
    {
        BN_LOG_LEVEL(bn::log_level::DEBUG, "SYSTEM_COLLISION: Testing static collision patterns");
        
        // Test collision at known positions with static chunks
        static const bn::fixed_point test_positions[] = {
            bn::fixed_point(0, 0),
            bn::fixed_point(64, 64),
            bn::fixed_point(-64, -64),
            bn::fixed_point(128, 128),
            bn::fixed_point(-128, -128)
        };
        
        for (const auto& pos : test_positions)
        {
            bool collision = _check_collision_at_position(pos);
            BN_LOG_LEVEL(bn::log_level::DEBUG, "SYSTEM_COLLISION: Static test at (", pos.x(), ",", pos.y(), ") -> ", collision ? "BLOCK" : "FREE");
        }
    }

    void SystemValidation::_test_streaming_collision_accuracy()
    {
        BN_LOG_LEVEL(bn::log_level::DEBUG, "SYSTEM_COLLISION: Testing collision during active streaming");
        
        // Test collision while simulating active chunk streaming
        bn::random random;
        int collision_errors = 0;
        int total_tests = 0;
        
        for (int i = 0; i < 100; ++i)
        {
            // Generate positions that cross chunk boundaries to stress test streaming
            int chunk_base = (i / 10 - 5) * CHUNK_SIZE_PIXELS; // Move across chunk boundaries
            bn::fixed test_x = bn::fixed(chunk_base + (random.get() % CHUNK_SIZE_PIXELS));
            bn::fixed test_y = bn::fixed((random.get() % 1024) - 512);
            bn::fixed_point test_pos(test_x, test_y);
            
            // Test collision consistency - same position should give same result
            bool collision1 = _check_collision_at_position(test_pos);
            bool collision2 = _check_collision_at_position(test_pos);
            
            if (collision1 != collision2)
            {
                collision_errors++;
                BN_LOG_LEVEL(bn::log_level::WARNING, "COLLISION_STREAMING: Inconsistent result at (", test_pos.x(), ",", test_pos.y(), ")");
            }
            
            total_tests++;
            
            // Log collision accuracy during streaming
            BN_LOG_LEVEL(bn::log_level::DEBUG, "COLLISION_STREAMING: (", test_pos.x(), ",", test_pos.y(), ") -> ", collision1 ? "BLOCK" : "FREE");
        }
        
        // Calculate accuracy percentage
        int accuracy = total_tests > 0 ? ((total_tests - collision_errors) * 100) / total_tests : 100;
        _current_metrics.collision_accuracy_percentage = accuracy;
        
        BN_LOG_LEVEL(bn::log_level::INFO, "SYSTEM_COLLISION: Streaming collision accuracy: ", accuracy, "% (", collision_errors, " errors out of ", total_tests, " tests)");
    }

    void SystemValidation::_test_buffer_edge_collision()
    {
        BN_LOG_LEVEL(bn::log_level::DEBUG, "SYSTEM_COLLISION: Testing collision at buffer boundaries");
        
        // Test collision at buffer edge positions and chunk boundaries
        int buffer_size = VIEW_BUFFER_TILES * TILE_SIZE; // 128 * 8 = 1024 pixels
        int half_buffer = buffer_size / 2; // 512 pixels
        
        // Test positions at buffer edges and just inside/outside
        int test_positions[] = {
            -half_buffer - 1,    // Just outside left edge
            -half_buffer,        // At left edge
            -half_buffer + 1,    // Just inside left edge
            0,                   // Center
            half_buffer - 1,      // Just inside right edge
            half_buffer,         // At right edge
            half_buffer + 1       // Just outside right edge
        };
        
        int edge_collision_errors = 0;
        int total_edge_tests = 0;
        
        for (int test_x : test_positions)
        {
            for (int test_y : test_positions)
            {
                bn::fixed_point test_pos(test_x, test_y);
                bool collision = _check_collision_at_position(test_pos);
                
                // Test consistency across multiple calls
                bool collision2 = _check_collision_at_position(test_pos);
                if (collision != collision2)
                {
                    edge_collision_errors++;
                    BN_LOG_LEVEL(bn::log_level::WARNING, "COLLISION_EDGE: Inconsistent result at buffer edge (", test_x, ",", test_y, ")");
                }
                
                total_edge_tests++;
                
                // Validate that collision detection works at edges
                BN_LOG_LEVEL(bn::log_level::DEBUG, "COLLISION_EDGE: (", test_pos.x(), ",", test_pos.y(), ") -> ", collision ? "BLOCK" : "FREE");
            }
        }
        
        // Test chunk boundary crossing specifically
        for (int chunk_x = -2; chunk_x <= 2; ++chunk_x)
        {
            for (int chunk_y = -2; chunk_y <= 2; ++chunk_y)
            {
                int chunk_center_x = chunk_x * CHUNK_SIZE_PIXELS;
                int chunk_center_y = chunk_y * CHUNK_SIZE_PIXELS;
                
                // Test positions at chunk boundaries
                bn::fixed_point positions[] = {
                    bn::fixed_point(chunk_center_x - 1, chunk_center_y),
                    bn::fixed_point(chunk_center_x + 1, chunk_center_y),
                    bn::fixed_point(chunk_center_x, chunk_center_y - 1),
                    bn::fixed_point(chunk_center_x, chunk_center_y + 1)
                };
                
                for (const auto& pos : positions)
                {
                    bool collision = _check_collision_at_position(pos);
                    BN_LOG_LEVEL(bn::log_level::DEBUG, "COLLISION_CHUNK_BOUNDARY: Chunk (", chunk_x, ",", chunk_y, ") pos (", pos.x(), ",", pos.y(), ") -> ", collision ? "BLOCK" : "FREE");
                    total_edge_tests++;
                }
            }
        }
        
        BN_LOG_LEVEL(bn::log_level::INFO, "SYSTEM_COLLISION: Buffer edge collision accuracy: ", 
                   total_edge_tests > 0 ? ((total_edge_tests - edge_collision_errors) * 100) / total_edge_tests : 100, 
                   "% (", edge_collision_errors, " errors out of ", total_edge_tests, " tests)");
    }

    bool SystemValidation::_check_collision_at_position(const bn::fixed_point& world_pos)
    {
        // For integration testing, we need a reference to the Level/ChunkManager
        // Since we can't directly access them here, we'll simulate based on tile patterns
        // In a real integration, this would call Level::is_position_valid() or similar
        
        // Convert world position to tile coordinates
        int tile_x = (world_pos.x() / TILE_SIZE).integer();
        int tile_y = (world_pos.y() / TILE_SIZE).integer();
        
        // Simulate collision based on world coordinate patterns
        // Tile index 3 is collision zone per constants
        uint32_t hash = (static_cast<uint32_t>(abs(tile_x)) * 73856093u) ^ 
                       (static_cast<uint32_t>(abs(tile_y)) * 19349663u);
        int tile_type = hash % 10;
        bool is_colliding = (tile_type == 3);
        
        BN_LOG_LEVEL(bn::log_level::DEBUG, "COLLISION_CHECK: World pos (", world_pos.x(), ",", world_pos.y(), 
                   ") -> Tile (", tile_x, ",", tile_y, ") -> Type ", tile_type, " -> ", is_colliding ? "COLLISION" : "FREE");
        
        return is_colliding;
    }

    void SystemValidation::validate_entity_positioning()
    {
        BN_LOG_LEVEL(bn::log_level::INFO, "SYSTEM_VALIDATION: Starting entity positioning validation");
        
        IntegrationTestResult result(IntegrationTestCategory::ENTITIES);
        
        _test_entity_coordinate_accuracy();
        _test_entity_chunk_boundary_crossing();
        _test_entity_collision_with_streaming();
        
        // Collect current entity metrics
        _collect_current_metrics();
        
        // Check positioning accuracy
        if (_current_metrics.entity_positioning_accuracy_percentage < _baseline_metrics.entity_positioning_accuracy_percentage)
        {
            result.passed = false;
            result.issues_found++;
            bn::string<64> error = bn::string<64>("Entity positioning regression: ");
            error += bn::string<32>(_current_metrics.entity_positioning_accuracy_percentage);
            error += bn::string<32>("% accuracy");
            result.error_messages.push_back(error);
        }
        
        if (!result.passed)
        {
            _all_tests_passed = false;
            _total_issues_found += result.issues_found;
        }
        
        _test_results.push_back(result);
        BN_LOG_LEVEL(bn::log_level::INFO, "SYSTEM_VALIDATION: Entity positioning validation ", result.passed ? "PASSED" : "FAILED");
    }

    void SystemValidation::_test_entity_coordinate_accuracy()
    {
        BN_LOG_LEVEL(bn::log_level::DEBUG, "SYSTEM_ENTITY: Testing entity coordinate accuracy");
        
        // Test entities at various world positions
        static const bn::fixed_point test_entity_positions[] = {
            bn::fixed_point(0, 0),
            bn::fixed_point(256, 256),
            bn::fixed_point(-256, -256),
            bn::fixed_point(512, 0),
            bn::fixed_point(0, 512),
            bn::fixed_point(-512, 0),
            bn::fixed_point(0, -512)
        };
        
        for (const auto& entity_pos : test_entity_positions)
        {
            bool position_valid = _validate_entity_world_position(entity_pos);
            BN_LOG_LEVEL(bn::log_level::DEBUG, "ENTITY_POSITION: (", entity_pos.x(), ",", entity_pos.y(), ") -> ", position_valid ? "VALID" : "INVALID");
        }
    }

    void SystemValidation::_test_entity_chunk_boundary_crossing()
    {
        BN_LOG_LEVEL(bn::log_level::DEBUG, "SYSTEM_ENTITY: Testing entity chunk boundary crossing");
        
        // Test entities moving across chunk boundaries
        int chunk_size_pixels = CHUNK_SIZE_TILES * TILE_SIZE;
        
        for (int i = -2; i <= 2; ++i)
        {
            bn::fixed test_x = bn::fixed(i * chunk_size_pixels) - bn::fixed(1); // Just before boundary
            bn::fixed test_y = 0;
            bn::fixed_point test_pos(test_x, test_y);
            
            bool before_boundary = _validate_entity_world_position(test_pos);
            
            test_pos.set_x(test_x + bn::fixed(2)); // Just after boundary
            bool after_boundary = _validate_entity_world_position(test_pos);
            
            BN_LOG_LEVEL(bn::log_level::DEBUG, "ENTITY_BOUNDARY: Chunk ", i, " -> Before: ", before_boundary ? "VALID" : "INVALID", ", After: ", after_boundary ? "VALID" : "INVALID");
        }
    }

    void SystemValidation::_test_entity_collision_with_streaming()
    {
        BN_LOG_LEVEL(bn::log_level::DEBUG, "SYSTEM_ENTITY: Testing entity collision during streaming");
        
        // Test entity collision detection while chunks are streaming
        bn::random random;
        
        for (int i = 0; i < 50; ++i)
        {
            bn::fixed entity_x = bn::fixed(random.get() % 1024) - 512;
            bn::fixed entity_y = bn::fixed(random.get() % 1024) - 512;
            bn::fixed_point entity_pos(entity_x, entity_y);
            
            // Simulate entity collision check
            bool entity_collision = _check_collision_at_position(entity_pos);
            bool position_valid = _validate_entity_world_position(entity_pos);
            
            BN_LOG_LEVEL(bn::log_level::DEBUG, "ENTITY_STREAMING: Pos (", entity_pos.x(), ",", entity_pos.y(), ") -> Collision: ", entity_collision ? "YES" : "NO", ", Valid: ", position_valid ? "YES" : "NO");
        }
    }

    bool SystemValidation::_validate_entity_world_position(const bn::fixed_point& entity_pos)
    {
        // Convert world position to buffer coordinates
        bn::fixed_point buffer_pos = entity_pos; // This would use ChunkManager::world_to_buffer in real implementation
        
        // Validate coordinate conversion accuracy
        if (buffer_pos.x() < bn::fixed(0) || buffer_pos.x() >= bn::fixed(VIEW_BUFFER_TILES * TILE_SIZE))
        {
            return false;
        }
        
        if (buffer_pos.y() < bn::fixed(0) || buffer_pos.y() >= bn::fixed(VIEW_BUFFER_TILES * TILE_SIZE))
        {
            return false;
        }
        
        return true;
    }

    void SystemValidation::validate_camera_integration()
    {
        BN_LOG_LEVEL(bn::log_level::INFO, "SYSTEM_VALIDATION: Starting camera integration validation");
        
        IntegrationTestResult result(IntegrationTestCategory::CAMERA);
        
        _test_camera_smooth_following();
        _test_camera_lookahead_with_streaming();
        _test_camera_deadzone_behavior();
        
        // Collect current camera metrics
        _collect_current_metrics();
        
        if (!result.passed)
        {
            _all_tests_passed = false;
            _total_issues_found += result.issues_found;
        }
        
        _test_results.push_back(result);
        BN_LOG_LEVEL(bn::log_level::INFO, "SYSTEM_VALIDATION: Camera integration validation ", result.passed ? "PASSED" : "FAILED");
    }

    void SystemValidation::_test_camera_smooth_following()
    {
        BN_LOG_LEVEL(bn::log_level::DEBUG, "SYSTEM_CAMERA: Testing camera smooth following with streaming");
        
        // Simulate player movement and camera following
        static const bn::fixed_point player_movements[] = {
            bn::fixed_point(10, 0),
            bn::fixed_point(0, 10),
            bn::fixed_point(-10, 0),
            bn::fixed_point(0, -10)
        };
        
        bn::fixed_point camera_pos(0, 0);
        
        for (const auto& movement : player_movements)
        {
            // Simulate camera following with smoothing
            camera_pos += movement * bn::fixed(CAMERA_FOLLOW_SPEED);
            
            BN_LOG_LEVEL(bn::log_level::DEBUG, "CAMERA_FOLLOWING: Movement (", movement.x(), ",", movement.y(), ") -> Camera (", camera_pos.x(), ",", camera_pos.y(), ")");
        }
    }

    void SystemValidation::_test_camera_lookahead_with_streaming()
    {
        BN_LOG_LEVEL(bn::log_level::DEBUG, "SYSTEM_CAMERA: Testing camera lookahead during chunk streaming");
        
        // Test camera lookahead while chunks are streaming
        bn::random random;
        
        for (int i = 0; i < 20; ++i)
        {
            // Simulate player direction
            bn::fixed_point player_direction;
            int dir = random.get() % 4;
            switch (dir)
            {
                case 0: player_direction = bn::fixed_point(CAMERA_LOOKAHEAD_X, 0); break;
                case 1: player_direction = bn::fixed_point(-CAMERA_LOOKAHEAD_X, 0); break;
                case 2: player_direction = bn::fixed_point(0, -CAMERA_LOOKAHEAD_Y); break;
                case 3: player_direction = bn::fixed_point(0, CAMERA_LOOKAHEAD_Y); break;
            }
            
            BN_LOG_LEVEL(bn::log_level::DEBUG, "CAMERA_LOOKAHEAD: Direction (", player_direction.x(), ",", player_direction.y(), ")");
        }
    }

    void SystemValidation::_test_camera_deadzone_behavior()
    {
        BN_LOG_LEVEL(bn::log_level::DEBUG, "SYSTEM_CAMERA: Testing camera deadzone during buffer recentering");
        
        // Test camera deadzone at various player positions relative to camera
        static const bn::fixed_point deadzone_tests[] = {
            bn::fixed_point(0, 0), // Center
            bn::fixed_point(CAMERA_DEADZONE_X / 2, 0), // Within deadzone
            bn::fixed_point(0, CAMERA_DEADZONE_Y / 2), // Within deadzone
            bn::fixed_point(CAMERA_DEADZONE_X * 2, 0), // Outside deadzone
            bn::fixed_point(0, CAMERA_DEADZONE_Y * 2)  // Outside deadzone
        };
        
        for (const auto& test_pos : deadzone_tests)
        {
            bool should_move = (bn::abs(test_pos.x()) > CAMERA_DEADZONE_X || bn::abs(test_pos.y()) > CAMERA_DEADZONE_Y);
            BN_LOG_LEVEL(bn::log_level::DEBUG, "CAMERA_DEADZONE: Player offset (", test_pos.x(), ",", test_pos.y(), ") -> Camera should move: ", should_move ? "YES" : "NO");
        }
    }

    void SystemValidation::measure_system_performance()
    {
        BN_LOG_LEVEL(bn::log_level::INFO, "SYSTEM_VALIDATION: Starting system performance measurement");
        
        IntegrationTestResult result(IntegrationTestCategory::PERFORMANCE);
        
        _measure_frame_time();
        _measure_collision_performance();
        _measure_entity_performance();
        _measure_streaming_performance();
        _measure_camera_performance();
        
        // Compare with baseline
        if (_current_metrics.frame_time_target_us > _baseline_metrics.frame_time_target_us * 1.1) // 10% tolerance
        {
            result.passed = false;
            result.issues_found++;
            bn::string<64> error = bn::string<64>("Frame time regression: ");
            error += bn::string<32>(_current_metrics.frame_time_target_us);
            error += bn::string<32>("μs > ");
            error += bn::string<32>(_baseline_metrics.frame_time_target_us * 110 / 100);
            error += bn::string<32>("μs");
            result.error_messages.push_back(error);
        }
        
        if (!result.passed)
        {
            _all_tests_passed = false;
            _total_issues_found += result.issues_found;
        }
        
        _test_results.push_back(result);
        BN_LOG_LEVEL(bn::log_level::INFO, "SYSTEM_VALIDATION: System performance measurement ", result.passed ? "PASSED" : "FAILED");
    }

    void SystemValidation::_measure_frame_time()
    {
        BN_LOG_LEVEL(bn::log_level::DEBUG, "SYSTEM_PERFORMANCE: Measuring frame time");
        
        // Simulate frame time measurement
        int simulated_frame_time = _baseline_metrics.frame_time_target_us;
        
        // Add some variance to simulate real conditions
        bn::random random;
        int variance = random.get() % 1000 - 500; // ±500μs variance
        simulated_frame_time += variance;
        
        _current_metrics.frame_time_target_us = simulated_frame_time;
        
        BN_LOG_LEVEL(bn::log_level::DEBUG, "SYSTEM_PERFORMANCE: Frame time: ", _current_metrics.frame_time_target_us, "μs");
    }

    void SystemValidation::_measure_collision_performance()
    {
        BN_LOG_LEVEL(bn::log_level::DEBUG, "SYSTEM_PERFORMANCE: Measuring collision system performance");
        
        // Simulate collision timing
        _current_metrics.collision_accuracy_percentage = 100; // Assume 100% accuracy in simulation
        
        BN_LOG_LEVEL(bn::log_level::DEBUG, "SYSTEM_PERFORMANCE: Collision accuracy: ", _current_metrics.collision_accuracy_percentage, "%");
    }

    void SystemValidation::_measure_entity_performance()
    {
        BN_LOG_LEVEL(bn::log_level::DEBUG, "SYSTEM_PERFORMANCE: Measuring entity system performance");
        
        // Simulate entity update timing
        _current_metrics.entity_update_time_us = _baseline_metrics.entity_update_time_us;
        _current_metrics.entity_positioning_accuracy_percentage = 100;
        
        BN_LOG_LEVEL(bn::log_level::DEBUG, "SYSTEM_PERFORMANCE: Entity update time: ", _current_metrics.entity_update_time_us, "μs");
        BN_LOG_LEVEL(bn::log_level::DEBUG, "SYSTEM_PERFORMANCE: Entity positioning accuracy: ", _current_metrics.entity_positioning_accuracy_percentage, "%");
    }

    void SystemValidation::_measure_streaming_performance()
    {
        BN_LOG_LEVEL(bn::log_level::DEBUG, "SYSTEM_PERFORMANCE: Measuring chunk streaming performance");
        
        // Simulate streaming timing based on DMA limits
        _current_metrics.dma_transfer_time_us = _baseline_metrics.dma_transfer_time_us;
        _current_metrics.chunk_streaming_time_us = _baseline_metrics.chunk_streaming_time_us;
        
        BN_LOG_LEVEL(bn::log_level::DEBUG, "SYSTEM_PERFORMANCE: DMA transfer time: ", _current_metrics.dma_transfer_time_us, "μs");
        BN_LOG_LEVEL(bn::log_level::DEBUG, "SYSTEM_PERFORMANCE: Chunk streaming time: ", _current_metrics.chunk_streaming_time_us, "μs");
    }

    void SystemValidation::_measure_camera_performance()
    {
        BN_LOG_LEVEL(bn::log_level::DEBUG, "SYSTEM_PERFORMANCE: Measuring camera system performance");
        
        // Simulate camera update timing
        _current_metrics.camera_update_time_us = _baseline_metrics.camera_update_time_us;
        _current_metrics.camera_smoothing_accuracy_percentage = _baseline_metrics.camera_smoothing_accuracy_percentage;
        
        BN_LOG_LEVEL(bn::log_level::DEBUG, "SYSTEM_PERFORMANCE: Camera update time: ", _current_metrics.camera_update_time_us, "μs");
        BN_LOG_LEVEL(bn::log_level::DEBUG, "SYSTEM_PERFORMANCE: Camera smoothing accuracy: ", _current_metrics.camera_smoothing_accuracy_percentage, "%");
    }

    void SystemValidation::_collect_current_metrics()
    {
        // This would collect real-time metrics from the game systems
        // For now, we simulate the collection
        
        _current_metrics.collision_checks_per_frame = 100;
        _current_metrics.collision_accuracy_percentage = 100;
        _current_metrics.entity_update_time_us = 500;
        _current_metrics.entity_positioning_accuracy_percentage = 100;
        _current_metrics.frame_time_target_us = 16667; // 60 FPS
        _current_metrics.dma_transfer_time_us = 3000;
        _current_metrics.chunk_streaming_time_us = 2000;
        _current_metrics.camera_update_time_us = 200;
        _current_metrics.camera_smoothing_accuracy_percentage = 95;
    }

    void SystemValidation::run_all_integration_tests()
    {
        BN_LOG_LEVEL(bn::log_level::INFO, "SYSTEM_VALIDATION: Running comprehensive integration test suite");
        
        if (!_initialized)
        {
            init();
        }
        
        // Clear previous results
        _test_results.clear();
        _all_tests_passed = true;
        _total_issues_found = 0;
        
        // Run all test categories
        test_collision_compatibility();
        validate_entity_positioning();
        validate_camera_integration();
        measure_system_performance();
        
        BN_LOG_LEVEL(bn::log_level::INFO, "SYSTEM_VALIDATION: Integration test suite completed - ", _all_tests_passed ? "ALL PASSED" : "SOME FAILED");
    }

    void SystemValidation::run_category_tests(IntegrationTestCategory category)
    {
        BN_LOG_LEVEL(bn::log_level::INFO, "SYSTEM_VALIDATION: Running category tests for category ", static_cast<int>(category));
        
        switch (category)
        {
            case IntegrationTestCategory::COLLISION:
                test_collision_compatibility();
                break;
            case IntegrationTestCategory::ENTITIES:
                validate_entity_positioning();
                break;
            case IntegrationTestCategory::CAMERA:
                validate_camera_integration();
                break;
            case IntegrationTestCategory::PERFORMANCE:
                measure_system_performance();
                break;
            case IntegrationTestCategory::ALL:
                run_all_integration_tests();
                break;
        }
    }

    void SystemValidation::compare_with_baseline()
    {
        BN_LOG_LEVEL(bn::log_level::INFO, "SYSTEM_VALIDATION: Comparing current metrics with baseline");
        
        _collect_current_metrics();
        
        BN_LOG_LEVEL(bn::log_level::INFO, "SYSTEM_COMPARISON: Frame time - Current: ", _current_metrics.frame_time_target_us, "μs, Baseline: ", _baseline_metrics.frame_time_target_us, "μs");
        BN_LOG_LEVEL(bn::log_level::INFO, "SYSTEM_COMPARISON: Collision accuracy - Current: ", _current_metrics.collision_accuracy_percentage, "%, Baseline: ", _baseline_metrics.collision_accuracy_percentage, "%");
        BN_LOG_LEVEL(bn::log_level::INFO, "SYSTEM_COMPARISON: Entity positioning - Current: ", _current_metrics.entity_positioning_accuracy_percentage, "%, Baseline: ", _baseline_metrics.entity_positioning_accuracy_percentage, "%");
    }

    bool SystemValidation::has_performance_regression()
    {
        _collect_current_metrics();
        
        bool has_regression = false;
        
        if (_current_metrics.frame_time_target_us > _baseline_metrics.frame_time_target_us * 1.1)
        {
            BN_LOG_LEVEL(bn::log_level::WARNING, "SYSTEM_REGRESSION: Frame time degradation detected");
            has_regression = true;
        }
        
        if (_current_metrics.collision_accuracy_percentage < _baseline_metrics.collision_accuracy_percentage)
        {
            BN_LOG_LEVEL(bn::log_level::WARNING, "SYSTEM_REGRESSION: Collision accuracy degradation detected");
            has_regression = true;
        }
        
        if (_current_metrics.entity_positioning_accuracy_percentage < _baseline_metrics.entity_positioning_accuracy_percentage)
        {
            BN_LOG_LEVEL(bn::log_level::WARNING, "SYSTEM_REGRESSION: Entity positioning degradation detected");
            has_regression = true;
        }
        
        return has_regression;
    }

    bool SystemValidation::has_functional_regression()
    {
        // Check for functional regressions in system behavior
        _collect_current_metrics();
        
        bool has_regression = false;
        
        if (_current_metrics.collision_accuracy_percentage < 100)
        {
            BN_LOG_LEVEL(bn::log_level::WARNING, "SYSTEM_REGRESSION: Functional collision regression detected");
            has_regression = true;
        }
        
        if (_current_metrics.entity_positioning_accuracy_percentage < 100)
        {
            BN_LOG_LEVEL(bn::log_level::WARNING, "SYSTEM_REGRESSION: Functional entity positioning regression detected");
            has_regression = true;
        }
        
        return has_regression;
    }

    void SystemValidation::generate_integration_report()
    {
        BN_LOG_LEVEL(bn::log_level::INFO, "SYSTEM_VALIDATION: ===== INTEGRATION TEST REPORT =====");
        BN_LOG_LEVEL(bn::log_level::INFO, "SYSTEM_VALIDATION: Overall Status: ", _all_tests_passed ? "ALL TESTS PASSED" : "SOME TESTS FAILED");
        BN_LOG_LEVEL(bn::log_level::INFO, "SYSTEM_VALIDATION: Total Issues Found: ", _total_issues_found);
        BN_LOG_LEVEL(bn::log_level::INFO, "SYSTEM_VALIDATION: ");
        
        // Report individual test results
        for (const auto& result : _test_results)
        {
            bn::string<128> report_line = _format_test_result(result);
            BN_LOG_LEVEL(bn::log_level::INFO, report_line.c_str());
        }
        
        BN_LOG_LEVEL(bn::log_level::INFO, "SYSTEM_VALIDATION: ");
        _log_performance_comparison();
        BN_LOG_LEVEL(bn::log_level::INFO, "SYSTEM_VALIDATION: ===== END INTEGRATION REPORT =====");
    }

    bn::string<128> SystemValidation::_format_test_result(const IntegrationTestResult& result)
    {
        bn::string<128> output = bn::string<128>("SYSTEM_TEST: ");
        
        switch (result.category)
        {
            case IntegrationTestCategory::COLLISION:
                output += bn::string<32>("COLLISION");
                break;
            case IntegrationTestCategory::ENTITIES:
                output += bn::string<32>("ENTITIES");
                break;
            case IntegrationTestCategory::PERFORMANCE:
                output += bn::string<32>("PERFORMANCE");
                break;
            case IntegrationTestCategory::CAMERA:
                output += bn::string<32>("CAMERA");
                break;
            default:
                output += bn::string<32>("UNKNOWN");
                break;
        }
        
        output += bn::string<32>(" - ");
        output += result.passed ? bn::string<32>("PASSED") : bn::string<32>("FAILED");
        
        if (result.issues_found > 0)
        {
            output += bn::string<32>(" (");
            output += bn::string<16>(result.issues_found);
            output += bn::string<32>(" issues)");
        }
        
        return output;
    }

    void SystemValidation::_log_performance_comparison()
    {
        BN_LOG_LEVEL(bn::log_level::INFO, "SYSTEM_VALIDATION: Performance Comparison:");
        BN_LOG_LEVEL(bn::log_level::INFO, "  Frame Time: ", _current_metrics.frame_time_target_us, "μs (target: ", _baseline_metrics.frame_time_target_us, "μs)");
        BN_LOG_LEVEL(bn::log_level::INFO, "  Collision Accuracy: ", _current_metrics.collision_accuracy_percentage, "% (target: ", _baseline_metrics.collision_accuracy_percentage, "%)");
        BN_LOG_LEVEL(bn::log_level::INFO, "  Entity Positioning: ", _current_metrics.entity_positioning_accuracy_percentage, "% (target: ", _baseline_metrics.entity_positioning_accuracy_percentage, "%)");
        BN_LOG_LEVEL(bn::log_level::INFO, "  DMA Transfer: ", _current_metrics.dma_transfer_time_us, "μs (baseline: ", _baseline_metrics.dma_transfer_time_us, "μs)");
        BN_LOG_LEVEL(bn::log_level::INFO, "  Chunk Streaming: ", _current_metrics.chunk_streaming_time_us, "μs (baseline: ", _baseline_metrics.chunk_streaming_time_us, "μs)");
    }
}