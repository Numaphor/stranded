#ifndef STR_BG_VALIDATION_H
#define STR_BG_VALIDATION_H

#include <cstdint>
#include "bn_fixed_point.h"
#include "bn_affine_bg_ptr.h"
#include "bn_affine_bg_map_ptr.h"
#include "bn_affine_bg_map_cell.h"
#include "bn_vector.h"
#include "bn_optional.h"
#include "bn_log.h"
#include "bn_assert.h"
#include "str_constants.h"

namespace str
{
    // Background validation constants
    constexpr int BG_VALIDATION_TOLERANCE = 1;           // 1-tile tolerance for position checks
    constexpr int BG_FRAME_HISTORY_SIZE = 60;            // 1 second of frame data at 60fps
    constexpr int BG_MAX_FRAME_TIME_US = 16667;           // 60 FPS target (16.667ms per frame)
    constexpr int BG_WARNING_THRESHOLD_US = 15000;       // Warning threshold for frame time
    constexpr int BG_CRITICAL_THRESHOLD_US = 20000;      // Critical threshold for frame time
    
    // Background register validation constants
    constexpr int BGHOFS_REGISTER_MIN = 0;
    constexpr int BGHOFS_REGISTER_MAX = 511;             // 9-bit offset register
    constexpr int BGVOFS_REGISTER_MIN = 0;
    constexpr int BGVOFS_REGISTER_MAX = 511;             // 9-bit offset register
    
    // Visual artifact detection thresholds
    constexpr int VISUAL_ARTIFACT_TEAR_THRESHOLD = 2;    // Pixels difference for tear detection
    constexpr int VISUAL_ARTIFACT_FLICKER_FRAMES = 3;     // Consecutive frames for flicker detection
    constexpr int VISUAL_TRANSITION_SMOOTHNESS_THRESHOLD = 4; // Maximum tile jump for smoothness
    
    // Stress testing configurations
    constexpr int STRESS_TEST_MOVEMENT_SPEED = 8;        // Tiles per frame for rapid movement
    constexpr int STRESS_TEST_ROTATION_SPEED = 5;        // Degrees per frame for rotation
    constexpr int STRESS_TEST_SCALE_SPEED = 2;           // Scale units per frame
    constexpr int STRESS_TEST_DURATION_FRAMES = 300;     // 5 seconds of stress testing
    
    // Performance benchmarking constants
    constexpr int PERF_BASELINE_SAMPLE_FRAMES = 60;      // 1 second baseline
    constexpr int PERF_REGRESSION_THRESHOLD = 5;        // 5% performance regression threshold
    constexpr int PERF_SAMPLE_INTERVAL = 300;            // Sample every 5 seconds
    
    // Background validation result types
    enum class BgValidationResult : uint8_t
    {
        PASS,
        WARN,
        FAIL,
        CRITICAL
    };
    
    // Background register synchronization state
    struct BgRegisterState
    {
        int hofs_value;                                  // Current BGHOFS register value
        int vofs_value;                                  // Current BGVOFS register value
        int expected_hofs;                               // Expected BGHOFS based on camera
        int expected_vofs;                               // Expected BGVOFS based on camera
        bool is_synchronized;                             // Register sync status
        int desync_frames;                               // Consecutive frames of desync
    };
    
    // Frame performance data
    struct BgFrameData
    {
        int frame_number;
        int frame_time_us;                               // Frame execution time in microseconds
        int chunks_processed;                            // Number of chunks processed this frame
        int tiles_transferred;                           // Number of tiles transferred to VRAM
        bool buffer_recentered;                          // Buffer was recentered this frame
        bool had_visual_artifacts;                       // Visual artifacts detected
        BgRegisterState register_state;                  // Background register state
    };
    
    // Visual artifact detection data
    struct VisualArtifactData
    {
        bool tear_detected;                             // Screen tearing detected
        bool flicker_detected;                          // Flickering detected
        bool discontinuity_detected;                   // Visual discontinuity detected
        int max_tile_jump;                               // Maximum tile position jump
        int consecutive_artifact_frames;                 // Consecutive frames with artifacts
        bn::fixed_point last_valid_position;             // Last known good position
    };
    
    // Affine transformation compatibility data
    struct AffineCompatibilityData
    {
        bn::fixed current_scale;                         // Current background scale
        bn::fixed current_rotation;                      // Current background rotation
        bool scale_compatible;                          // Scale works with chunk streaming
        bool rotation_compatible;                       // Rotation works with chunk streaming
        int transform_conflict_frames;                  // Frames with transform conflicts
        bn::fixed_point scale_origin;                   // Scale transformation origin
        bn::fixed_point rotation_origin;                // Rotation transformation origin
    };
    
    // Rendering pipeline compatibility data
    struct RenderingPipelineData
    {
        bool multi_layer_compatible;                    // Multi-layer rendering works
        bool priority_conflict;                         // Background priority conflicts
        bool dma_conflict;                              // DMA transfer conflicts
        bool vblank_conflict;                           // VBlank timing conflicts
        int total_bg_layers;                            // Total background layers in use
        int available_bg_layers;                        // Available background layers
        bn::vector<int, 4> layer_priorities;            // Background layer priorities
    };
    
    // Background validation session
    struct BgValidationSession
    {
        bool is_active;                                 // Validation session active
        int start_frame;                                // Session start frame
        int total_frames;                               // Total frames validated
        bn::vector<BgFrameData, BG_FRAME_HISTORY_SIZE> frame_history; // Frame history
        VisualArtifactData artifact_data;                // Visual artifact tracking
        AffineCompatibilityData affine_data;             // Affine compatibility tracking
        RenderingPipelineData pipeline_data;             // Rendering pipeline data
        int total_warnings;                             // Total warning count
        int total_failures;                             // Total failure count
        int total_critical;                             // Total critical issues
    };
    
    // Background validation interface
    class BgValidation
    {
    public:
        // Initialize background validation
        static void init();
        
        // Start a validation session
        static void start_validation_session();
        
        // End validation session and generate report
        static void end_validation_session();
        
        // Validate BG register synchronization
        static BgValidationResult test_bg_register_sync(
            const bn::affine_bg_ptr& bg,
            const bn::fixed_point& camera_position,
            const bn::fixed_point& expected_bg_position
        );
        
        // Validate affine background compatibility
        static BgValidationResult validate_affine_compatibility(
            const bn::affine_bg_ptr& bg,
            const bn::affine_bg_map_ptr& bg_map,
            const bn::fixed_point& camera_position,
            bn::fixed current_scale,
            bn::fixed current_rotation
        );
        
        // Check rendering pipeline compatibility
        static BgValidationResult check_rendering_pipeline(
            const bn::affine_bg_ptr& bg,
            int active_layers,
            bool dma_in_progress,
            bool vblank_active
        );
        
        // Detect visual artifacts during chunk streaming
        static BgValidationResult detect_visual_artifacts(
            const bn::fixed_point& current_bg_position,
            const bn::fixed_point& previous_bg_position,
            bool buffer_recentered
        );
        
        // Measure performance impact of background integration
        static BgValidationResult measure_performance_impact(
            int frame_time_us,
            int chunks_processed,
            int tiles_transferred
        );
        
        // Run stress testing scenarios
        static BgValidationResult run_stress_test(
            const bn::affine_bg_ptr& bg,
            int stress_type,  // 0=movement, 1=rotation, 2=scale, 3=combined
            int duration_frames
        );
        
        // Check for performance regression
        static BgValidationResult check_performance_regression(
            int current_frame_time_us,
            int baseline_frame_time_us
        );
        
        // Get current validation session data
        static const BgValidationSession& get_session_data() { return _validation_session; }
        
        // Check if validation session is active
        static bool is_validation_active() { return _validation_session.is_active; }
        
        // Log validation results (using Butano logging system)
        static void log_validation_results();
        
        // Reset validation state
        static void reset();
        
    private:
        static BgValidationSession _validation_session;
        static BgFrameData _current_frame_data;
        static int _frame_counter;
        static bool _initialized;
        
        // Internal helper methods
        static int _calculate_expected_bg_register(const bn::fixed& position);
        static bool _is_within_register_range(int value, int min_val, int max_val);
        static void _record_frame_data(const BgFrameData& data);
        static BgValidationResult _analyze_frame_history();
        static void _update_artifact_detection(
            const bn::fixed_point& current_pos,
            const bn::fixed_point& previous_pos,
            bool buffer_recentered
        );
    };
}

#endif // STR_BG_VALIDATION_H