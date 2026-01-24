#ifndef STR_SYSTEM_VALIDATION_H
#define STR_SYSTEM_VALIDATION_H

#include "bn_fixed_point.h"
#include "bn_vector.h"
#include "bn_optional.h"
#include "str_constants.h"
#include "str_chunk_manager.h"

namespace str
{
    // Integration test categories
    enum class IntegrationTestCategory : uint8_t
    {
        COLLISION = 0,
        ENTITIES = 1,
        PERFORMANCE = 2,
        CAMERA = 3,
        ALL = 255
    };

    // Baseline performance metrics for regression testing
    struct SystemBaselineMetrics
    {
        // Collision system metrics
        int collision_checks_per_frame = 100;
        int collision_accuracy_percentage = 100;
        
        // Entity system metrics
        int entity_update_time_us = 500;
        int entity_positioning_accuracy_percentage = 100;
        
        // Performance metrics
        int frame_time_target_us = 16667; // 60 FPS = 16.667ms
        int dma_transfer_time_us = 3000;
        int chunk_streaming_time_us = 2000;
        
        // Camera system metrics
        int camera_update_time_us = 200;
        int camera_smoothing_accuracy_percentage = 95;
    };

    // System integration test result
    struct IntegrationTestResult
    {
        IntegrationTestCategory category;
        bool passed;
        int issues_found;
        bn::vector<bn::string<64>, 16> error_messages;
        
        IntegrationTestResult(IntegrationTestCategory cat) : category(cat), passed(true), issues_found(0) {}
    };

    // System validation state
    class SystemValidation
    {
    public:
        static void init();
        static void shutdown();

        // Main integration testing functions
        static void test_collision_compatibility();
        static void validate_entity_positioning();
        static void measure_system_performance();
        
        // Camera system integration testing
        static void validate_camera_integration();
        
        // Comprehensive validation suite
        static void run_all_integration_tests();
        static void run_category_tests(IntegrationTestCategory category);
        
        // Baseline management
        static void establish_baseline();
        static void compare_with_baseline();
        static SystemBaselineMetrics get_current_metrics() { return _current_metrics; }
        
        // Regression detection
        static bool has_performance_regression();
        static bool has_functional_regression();
        
        // Reporting
        static void generate_integration_report();
        static bool all_tests_passed() { return _all_tests_passed; }
        static int total_issues_found() { return _total_issues_found; }

    private:
        static SystemBaselineMetrics _baseline_metrics;
        static SystemBaselineMetrics _current_metrics;
        static bn::vector<IntegrationTestResult, 8> _test_results;
        static bool _initialized;
        static bool _all_tests_passed;
        static int _total_issues_found;
        
        // Helper functions for collision testing
        static void _test_static_collision_patterns();
        static void _test_streaming_collision_accuracy();
        static void _test_buffer_edge_collision();
        static bool _check_collision_at_position(const bn::fixed_point& world_pos);
        
        // Helper functions for entity testing
        static void _test_entity_coordinate_accuracy();
        static void _test_entity_chunk_boundary_crossing();
        static void _test_entity_collision_with_streaming();
        static bool _validate_entity_world_position(const bn::fixed_point& entity_pos);
        
        // Helper functions for performance testing
        static void _measure_frame_time();
        static void _measure_collision_performance();
        static void _measure_entity_performance();
        static void _measure_streaming_performance();
        static void _measure_camera_performance();
        
        // Helper functions for camera testing
        static void _test_camera_smooth_following();
        static void _test_camera_lookahead_with_streaming();
        static void _test_camera_deadzone_behavior();
        
        // Metrics collection
        static void _collect_current_metrics();
        static void _update_current_frame_metrics();
        
        // Reporting helpers
        static bn::string<128> _format_test_result(const IntegrationTestResult& result);
        static void _log_performance_comparison();
    };
}

#endif // STR_SYSTEM_VALIDATION_H