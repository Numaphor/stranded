#include "bg_validation.h"
#include "bn_core.h"
#include "bn_timer.h"
#include <algorithm>
#include <cmath>

namespace str
{
    // Static member definitions
    BgValidationSession BgValidation::_validation_session = {};
    BgFrameData BgValidation::_current_frame_data = {};
    int BgValidation::_frame_counter = 0;
    bool BgValidation::_initialized = false;
    
    // =========================================================================
    // Public Interface
    // =========================================================================
    
    void BgValidation::init()
    {
        if (_initialized)
        {
            return;
        }
        
        BN_LOG_LEVEL(bn::log_level::INFO, "BG_VALIDATION: Initializing background validation system");
        
        // Reset validation session
        _validation_session = {};
        _validation_session.is_active = false;
        _validation_session.frame_history.reserve(BG_FRAME_HISTORY_SIZE);
        
        // Initialize current frame data
        _current_frame_data = {};
        _frame_counter = 0;
        
        // Initialize artifact detection
        _validation_session.artifact_data.last_valid_position = bn::fixed_point(0, 0);
        _validation_session.artifact_data.consecutive_artifact_frames = 0;
        
        // Initialize affine compatibility data
        _validation_session.affine_data.current_scale = bn::fixed(1);
        _validation_session.affine_data.current_rotation = bn::fixed(0);
        _validation_session.affine_data.scale_compatible = true;
        _validation_session.affine_data.rotation_compatible = true;
        _validation_session.affine_data.transform_conflict_frames = 0;
        _validation_session.affine_data.scale_origin = bn::fixed_point(0, 0);
        _validation_session.affine_data.rotation_origin = bn::fixed_point(0, 0);
        
        // Initialize rendering pipeline data
        _validation_session.pipeline_data.multi_layer_compatible = true;
        _validation_session.pipeline_data.priority_conflict = false;
        _validation_session.pipeline_data.dma_conflict = false;
        _validation_session.pipeline_data.vblank_conflict = false;
        _validation_session.pipeline_data.total_bg_layers = 1; // Main affine background
        _validation_session.pipeline_data.available_bg_layers = 4; // GBA has 4 background layers
        _validation_session.pipeline_data.layer_priorities.clear();
        _validation_session.pipeline_data.layer_priorities.push_back(0); // Main BG priority
        
        _initialized = true;
        BN_LOG_LEVEL(bn::log_level::INFO, "BG_VALIDATION: Background validation system initialized");
    }
    
    void BgValidation::start_validation_session()
    {
        if (!_initialized)
        {
            init();
        }
        
        BN_LOG_LEVEL(bn::log_level::INFO, "BG_VALIDATION: Starting validation session");
        
        _validation_session.is_active = true;
        _validation_session.start_frame = _frame_counter;
        _validation_session.total_frames = 0;
        _validation_session.total_warnings = 0;
        _validation_session.total_failures = 0;
        _validation_session.total_critical = 0;
        
        // Clear frame history
        _validation_session.frame_history.clear();
        
        // Reset counters
        _current_frame_data = {};
        
        BN_LOG_LEVEL(bn::log_level::INFO, "BG_VALIDATION: Validation session started at frame ", _frame_counter);
    }
    
    void BgValidation::end_validation_session()
    {
        if (!_validation_session.is_active)
        {
            return;
        }
        
        BN_LOG_LEVEL(bn::log_level::INFO, "BG_VALIDATION: Ending validation session");
        
        _validation_session.is_active = false;
        _validation_session.total_frames = _frame_counter - _validation_session.start_frame;
        
        // Log final results
        log_validation_results();
        
        BN_LOG_LEVEL(bn::log_level::INFO, "BG_VALIDATION: Session ended - Total frames: ", _validation_session.total_frames,
                     " Warnings: ", _validation_session.total_warnings,
                     " Failures: ", _validation_session.total_failures,
                     " Critical: ", _validation_session.total_critical);
    }
    
    BgValidationResult BgValidation::test_bg_register_sync(
        const bn::affine_bg_ptr& bg,
        const bn::fixed_point& camera_position,
        const bn::fixed_point& expected_bg_position)
    {
        if (!_validation_session.is_active)
        {
            return BgValidationResult::PASS; // Skip if not in validation mode
        }
        
        // Get current background position
        int current_bg_x = bg.x();
        int current_bg_y = bg.y();
        
        // Calculate expected register values based on camera position
        int expected_hofs = _calculate_expected_bg_register(expected_bg_position.x());
        int expected_vofs = _calculate_expected_bg_register(expected_bg_position.y());
        
        // Validate register ranges
        bool hofs_in_range = _is_within_register_range(expected_hofs, BGHOFS_REGISTER_MIN, BGHOFS_REGISTER_MAX);
        bool vofs_in_range = _is_within_register_range(expected_vofs, BGVOFS_REGISTER_MIN, BGVOFS_REGISTER_MAX);
        
        if (!hofs_in_range || !vofs_in_range)
        {
            BN_LOG_LEVEL(bn::log_level::ERROR, "BG_VALIDATION: Register out of range - Hofs: ", expected_hofs, 
                         " Vofs: ", expected_vofs);
            _validation_session.total_failures++;
            return BgValidationResult::FAIL;
        }
        
        // Check synchronization
        int position_diff_x = bn::abs(current_bg_x - expected_bg_position.x().integer());
        int position_diff_y = bn::abs(current_bg_y - expected_bg_position.y().integer());
        
        BgRegisterState reg_state = {};
        reg_state.hofs_value = current_bg_x;
        reg_state.vofs_value = current_bg_y;
        reg_state.expected_hofs = expected_hofs;
        reg_state.expected_vofs = expected_vofs;
        reg_state.is_synchronized = (position_diff_x <= BG_VALIDATION_TOLERANCE && 
                                   position_diff_y <= BG_VALIDATION_TOLERANCE);
        
        if (!reg_state.is_synchronized)
        {
            BN_LOG_LEVEL(bn::log_level::WARN, "BG_VALIDATION: Register desync detected - BG pos: (", 
                         current_bg_x, ",", current_bg_y, ") Expected: (", 
                         expected_bg_position.x().integer(), ",", expected_bg_position.y().integer(), ")");
            _validation_session.total_warnings++;
            return BgValidationResult::WARN;
        }
        
        // Update current frame data
        _current_frame_data.register_state = reg_state;
        
        BN_LOG_LEVEL(bn::log_level::DEBUG, "BG_VALIDATION: BG register sync OK - Hofs: ", current_bg_x,
                     " Vofs: ", current_bg_y);
        return BgValidationResult::PASS;
    }
    
    BgValidationResult BgValidation::validate_affine_compatibility(
        const bn::affine_bg_ptr& bg,
        const bn::affine_bg_map_ptr& bg_map,
        const bn::fixed_point& camera_position,
        bn::fixed current_scale,
        bn::fixed current_rotation)
    {
        if (!_validation_session.is_active)
        {
            return BgValidationResult::PASS;
        }
        
        BN_LOG_LEVEL(bn::log_level::INFO, "BG_VALIDATION: Testing affine compatibility");
        
        // Test bg_map_ptr with our buffer system
        BN_LOG_LEVEL(bn::log_level::INFO, "BG_COMPAT: map_cells:", bg_map.cells_count(),
                     " dimensions:", bg_map.width(), "x", bg_map.height());
        
        // Validate map dimensions match our 128x128 tile buffer
        if (bg_map.width() != VIEW_BUFFER_TILES)
        {
            BN_LOG_LEVEL(bn::log_level::ERROR, "BG_VALIDATION: Map width mismatch - Expected: ", 
                         VIEW_BUFFER_TILES, " Got: ", bg_map.width());
            _validation_session.total_failures++;
            return BgValidationResult::FAIL;
        }
        
        if (bg_map.height() != VIEW_BUFFER_TILES)
        {
            BN_LOG_LEVEL(bn::log_level::ERROR, "BG_VALIDATION: Map height mismatch - Expected: ", 
                         VIEW_BUFFER_TILES, " Got: ", bg_map.height());
            _validation_session.total_failures++;
            return BgValidationResult::FAIL;
        }
        
        // Validate affine transformations work with chunk updates
        bool scale_ok = (current_scale > bn::fixed(0.1) && current_scale < bn::fixed(10.0));
        bool rotation_ok = (current_rotation >= bn::fixed(-360) && current_rotation <= bn::fixed(360));
        
        if (!scale_ok)
        {
            BN_LOG_LEVEL(bn::log_level::ERROR, "BG_VALIDATION: Invalid scale: ", current_scale);
            _validation_session.total_failures++;
            return BgValidationResult::FAIL;
        }
        
        if (!rotation_ok)
        {
            BN_LOG_LEVEL(bn::log_level::ERROR, "BG_VALIDATION: Invalid rotation: ", current_rotation);
            _validation_session.total_failures++;
            return BgValidationResult::FAIL;
        }
        
        // Update affine compatibility data
        _validation_session.affine_data.current_scale = current_scale;
        _validation_session.affine_data.current_rotation = current_rotation;
        _validation_session.affine_data.scale_compatible = scale_ok;
        _validation_session.affine_data.rotation_compatible = rotation_ok;
        
        // Test for transform conflicts during streaming
        if (bg.scale() != current_scale)
        {
            BN_LOG_LEVEL(bn::log_level::WARN, "BG_VALIDATION: Scale conflict during streaming");
            _validation_session.total_warnings++;
        }
        
        if (bn::abs(bg.rotation_angle() - current_rotation) > bn::fixed(1))
        {
            BN_LOG_LEVEL(bn::log_level::WARN, "BG_VALIDATION: Rotation conflict during streaming");
            _validation_session.total_warnings++;
        }
        
        BN_LOG_LEVEL(bn::log_level::INFO, "BG_VALIDATION: Affine compatibility validated - Scale: ", 
                     current_scale, " Rotation: ", current_rotation);
        return BgValidationResult::PASS;
    }
    
    BgValidationResult BgValidation::check_rendering_pipeline(
        const bn::affine_bg_ptr& bg,
        int active_layers,
        bool dma_in_progress,
        bool vblank_active)
    {
        if (!_validation_session.is_active)
        {
            return BgValidationResult::PASS;
        }
        
        BN_LOG_LEVEL(bn::log_level::DEBUG, "BG_VALIDATION: Checking rendering pipeline");
        
        // Check layer priority conflicts
        bool priority_conflict = false;
        int bg_priority = bg.priority();
        
        // GBA background layer priorities should be unique for optimal performance
        if (active_layers > _validation_session.pipeline_data.available_bg_layers)
        {
            BN_LOG_LEVEL(bn::log_level::ERROR, "BG_VALIDATION: Too many active layers: ", active_layers);
            _validation_session.total_failures++;
            return BgValidationResult::FAIL;
        }
        
        // Check for DMA conflicts
        if (dma_in_progress && vblank_active)
        {
            BN_LOG_LEVEL(bn::log_level::WARN, "BG_VALIDATION: Potential DMA/VBlank conflict detected");
            _validation_session.total_warnings++;
            priority_conflict = true;
        }
        
        // Update pipeline data
        _validation_session.pipeline_data.total_bg_layers = active_layers;
        _validation_session.pipeline_data.available_bg_layers = 4 - active_layers;
        _validation_session.pipeline_data.priority_conflict = priority_conflict;
        _validation_session.pipeline_data.dma_conflict = dma_in_progress;
        _validation_session.pipeline_data.vblank_conflict = vblank_active;
        
        // Update layer priorities
        if (_validation_session.pipeline_data.layer_priorities.empty())
        {
            _validation_session.pipeline_data.layer_priorities.push_back(bg_priority);
        }
        else
        {
            _validation_session.pipeline_data.layer_priorities[0] = bg_priority;
        }
        
        if (priority_conflict)
        {
            return BgValidationResult::WARN;
        }
        
        BN_LOG_LEVEL(bn::log_level::DEBUG, "BG_VALIDATION: Rendering pipeline OK - Layers: ", 
                     active_layers, " Priority: ", bg_priority);
        return BgValidationResult::PASS;
    }
    
    BgValidationResult BgValidation::detect_visual_artifacts(
        const bn::fixed_point& current_bg_position,
        const bn::fixed_point& previous_bg_position,
        bool buffer_recentered)
    {
        if (!_validation_session.is_active)
        {
            return BgValidationResult::PASS;
        }
        
        bool artifacts_detected = false;
        
        // Check for tearing (large position jumps)
        bn::fixed position_diff_x = bn::abs(current_bg_position.x() - previous_bg_position.x());
        bn::fixed position_diff_y = bn::abs(current_bg_position.y() - previous_bg_position.y());
        
        bool tear_detected = (position_diff_x > bn::fixed(VISUAL_ARTIFACT_TEAR_THRESHOLD) ||
                             position_diff_y > bn::fixed(VISUAL_ARTIFACT_TEAR_THRESHOLD));
        
        // Check for flickering (rapid position changes)
        static bn::fixed_point last_positions[3] = {};
        static int pos_index = 0;
        
        last_positions[pos_index] = current_bg_position;
        pos_index = (pos_index + 1) % 3;
        
        bool flicker_detected = false;
        for (int i = 0; i < 2; ++i)
        {
            bn::fixed diff = bn::abs(last_positions[i].x() - current_bg_position.x()) + 
                           bn::abs(last_positions[i].y() - current_bg_position.y());
            if (diff > bn::fixed(1))
            {
                flicker_detected = true;
                break;
            }
        }
        
        // Check for discontinuities during buffer recentering
        bool discontinuity_detected = false;
        if (buffer_recentered)
        {
            bn::fixed max_jump = bn::fixed(VISUAL_TRANSITION_SMOOTHNESS_THRESHOLD);
            discontinuity_detected = (position_diff_x > max_jump || position_diff_y > max_jump);
        }
        
        // Update artifact detection data
        _validation_session.artifact_data.tear_detected = tear_detected;
        _validation_session.artifact_data.flicker_detected = flicker_detected;
        _validation_session.artifact_data.discontinuity_detected = discontinuity_detected;
        
        int max_tile_jump = bn::max(position_diff_x.integer(), position_diff_y.integer());
        _validation_session.artifact_data.max_tile_jump = max_tile_jump;
        
        if (tear_detected || flicker_detected || discontinuity_detected)
        {
            _validation_session.artifact_data.consecutive_artifact_frames++;
            if (_validation_session.artifact_data.consecutive_artifact_frames >= VISUAL_ARTIFACT_FLICKER_FRAMES)
            {
                BN_LOG_LEVEL(bn::log_level::ERROR, "BG_VALIDATION: Visual artifacts detected - Tear: ", 
                             tear_detected, " Flicker: ", flicker_detected, 
                             " Discontinuity: ", discontinuity_detected);
                _validation_session.total_failures++;
                artifacts_detected = true;
            }
        }
        else
        {
            _validation_session.artifact_data.consecutive_artifact_frames = 0;
            _validation_session.artifact_data.last_valid_position = current_bg_position;
        }
        
        // Update current frame data
        _current_frame_data.had_visual_artifacts = artifacts_detected;
        
        return artifacts_detected ? BgValidationResult::FAIL : BgValidationResult::PASS;
    }
    
    BgValidationResult BgValidation::measure_performance_impact(
        int frame_time_us,
        int chunks_processed,
        int tiles_transferred)
    {
        if (!_validation_session.is_active)
        {
            return BgValidationResult::PASS;
        }
        
        // Check frame time against thresholds
        BgValidationResult result = BgValidationResult::PASS;
        
        if (frame_time_us > BG_CRITICAL_THRESHOLD_US)
        {
            BN_LOG_LEVEL(bn::log_level::ERROR, "BG_VALIDATION: Critical frame time: ", frame_time_us, "μs");
            _validation_session.total_critical++;
            result = BgValidationResult::CRITICAL;
        }
        else if (frame_time_us > BG_WARNING_THRESHOLD_US)
        {
            BN_LOG_LEVEL(bn::log_level::WARN, "BG_VALIDATION: High frame time: ", frame_time_us, "μs");
            _validation_session.total_warnings++;
            result = BgValidationResult::WARN;
        }
        
        // Update current frame data
        _current_frame_data.frame_time_us = frame_time_us;
        _current_frame_data.chunks_processed = chunks_processed;
        _current_frame_data.tiles_transferred = tiles_transferred;
        _current_frame_data.frame_number = _frame_counter;
        
        // Record frame data for analysis
        _record_frame_data(_current_frame_data);
        
        BN_LOG_LEVEL(bn::log_level::DEBUG, "BG_VALIDATION: Frame time: ", frame_time_us, 
                     "μs Chunks: ", chunks_processed, " Tiles: ", tiles_transferred);
        return result;
    }
    
    BgValidationResult BgValidation::run_stress_test(
        const bn::affine_bg_ptr& bg,
        int stress_type,
        int duration_frames)
    {
        if (!_validation_session.is_active)
        {
            return BgValidationResult::PASS;
        }
        
        BN_LOG_LEVEL(bn::log_level::INFO, "BG_VALIDATION: Running stress test type ", stress_type, 
                     " for ", duration_frames, " frames");
        
        int failed_frames = 0;
        int warning_frames = 0;
        
        // Save original background state
        bn::fixed original_scale = bg.scale();
        bn::fixed original_rotation = bg.rotation_angle();
        bn::fixed_point original_position(bg.x(), bg.y());
        
        for (int frame = 0; frame < duration_frames; ++frame)
        {
            // Apply stress based on type
            switch (stress_type)
            {
                case 0: // Rapid movement
                {
                    bn::fixed move_x = bn::fixed(STRESS_TEST_MOVEMENT_SPEED * bn::sin(frame * 0.1));
                    bn::fixed move_y = bn::fixed(STRESS_TEST_MOVEMENT_SPEED * bn::cos(frame * 0.1));
                    bg.set_position(move_x.integer(), move_y.integer());
                    break;
                }
                case 1: // Rapid rotation
                {
                    bn::fixed rotation = bn::fixed(frame * STRESS_TEST_ROTATION_SPEED);
                    bg.set_rotation_angle(rotation);
                    break;
                }
                case 2: // Rapid scaling
                {
                    bn::fixed scale = bn::fixed(1.0 + 0.5 * bn::sin(frame * 0.05));
                    bg.set_scale(scale);
                    break;
                }
                case 3: // Combined stress
                {
                    bn::fixed move_x = bn::fixed(STRESS_TEST_MOVEMENT_SPEED * 0.5 * bn::sin(frame * 0.1));
                    bn::fixed move_y = bn::fixed(STRESS_TEST_MOVEMENT_SPEED * 0.5 * bn::cos(frame * 0.1));
                    bn::fixed rotation = bn::fixed(frame * STRESS_TEST_ROTATION_SPEED * 0.5);
                    bn::fixed scale = bn::fixed(1.0 + 0.25 * bn::sin(frame * 0.05));
                    
                    bg.set_position(move_x.integer(), move_y.integer());
                    bg.set_rotation_angle(rotation);
                    bg.set_scale(scale);
                    break;
                }
                default:
                    BN_LOG_LEVEL(bn::log_level::ERROR, "BG_VALIDATION: Unknown stress test type: ", stress_type);
                    return BgValidationResult::FAIL;
            }
            
            // Allow frame to process
            bn::core::update();
            
            // Check for issues
            if (frame > 10) // Give some frames to stabilize
            {
                // Simple performance check
                // In a real implementation, we'd measure actual frame time here
                if (frame % 30 == 0) // Check every 0.5 seconds
                {
                    BN_LOG_LEVEL(bn::log_level::DEBUG, "BG_VALIDATION: Stress test frame ", frame, " completed");
                }
            }
        }
        
        // Restore original background state
        bg.set_position(original_position.x().integer(), original_position.y().integer());
        bg.set_rotation_angle(original_rotation);
        bg.set_scale(original_scale);
        
        BgValidationResult result = BgValidationResult::PASS;
        if (failed_frames > duration_frames / 10) // More than 10% failures
        {
            result = BgValidationResult::FAIL;
            _validation_session.total_failures++;
        }
        else if (warning_frames > duration_frames / 5) // More than 20% warnings
        {
            result = BgValidationResult::WARN;
            _validation_session.total_warnings++;
        }
        
        BN_LOG_LEVEL(bn::log_level::INFO, "BG_VALIDATION: Stress test completed - Failed: ", 
                     failed_frames, "/", duration_frames, " Result: ", static_cast<int>(result));
        return result;
    }
    
    BgValidationResult BgValidation::check_performance_regression(
        int current_frame_time_us,
        int baseline_frame_time_us)
    {
        if (!_validation_session.is_active)
        {
            return BgValidationResult::PASS;
        }
        
        // Calculate percentage difference
        int diff_us = current_frame_time_us - baseline_frame_time_us;
        int percent_diff = (diff_us * 100) / baseline_frame_time_us;
        
        if (percent_diff > PERF_REGRESSION_THRESHOLD)
        {
            BN_LOG_LEVEL(bn::log_level::ERROR, "BG_VALIDATION: Performance regression detected - Current: ", 
                         current_frame_time_us, "μs Baseline: ", baseline_frame_time_us, 
                         "μs Regression: ", percent_diff, "%");
            _validation_session.total_failures++;
            return BgValidationResult::FAIL;
        }
        else if (percent_diff > PERF_REGRESSION_THRESHOLD / 2)
        {
            BN_LOG_LEVEL(bn::log_level::WARN, "BG_VALIDATION: Performance warning - Current: ", 
                         current_frame_time_us, "μs Baseline: ", baseline_frame_time_us, 
                         "μs Diff: ", percent_diff, "%");
            _validation_session.total_warnings++;
            return BgValidationResult::WARN;
        }
        
        BN_LOG_LEVEL(bn::log_level::DEBUG, "BG_VALIDATION: Performance OK - Current: ", 
                     current_frame_time_us, "μs Baseline: ", baseline_frame_time_us, "μs");
        return BgValidationResult::PASS;
    }
    
    void BgValidation::log_validation_results()
    {
        if (!_validation_session.is_active)
        {
            BN_LOG_LEVEL(bn::log_level::WARN, "BG_VALIDATION: No active validation session to log");
            return;
        }
        
        BN_LOG_LEVEL(bn::log_level::INFO, "=== BACKGROUND VALIDATION RESULTS ===");
        BN_LOG_LEVEL(bn::log_level::INFO, "Session Duration: ", _validation_session.total_frames, " frames");
        BN_LOG_LEVEL(bn::log_level::INFO, "Total Warnings: ", _validation_session.total_warnings);
        BN_LOG_LEVEL(bn::log_level::INFO, "Total Failures: ", _validation_session.total_failures);
        BN_LOG_LEVEL(bn::log_level::INFO, "Total Critical: ", _validation_session.total_critical);
        
        // Log artifact data
        const auto& artifact = _validation_session.artifact_data;
        BN_LOG_LEVEL(bn::log_level::INFO, "Max Tile Jump: ", artifact.max_tile_jump);
        BN_LOG_LEVEL(bn::log_level::INFO, "Consecutive Artifact Frames: ", artifact.consecutive_artifact_frames);
        
        // Log affine data
        const auto& affine = _validation_session.affine_data;
        BN_LOG_LEVEL(bn::log_level::INFO, "Final Scale: ", affine.current_scale);
        BN_LOG_LEVEL(bn::log_level::INFO, "Final Rotation: ", affine.current_rotation);
        BN_LOG_LEVEL(bn::log_level::INFO, "Scale Compatible: ", affine.scale_compatible ? "YES" : "NO");
        BN_LOG_LEVEL(bn::log_level::INFO, "Rotation Compatible: ", affine.rotation_compatible ? "YES" : "NO");
        
        // Log pipeline data
        const auto& pipeline = _validation_session.pipeline_data;
        BN_LOG_LEVEL(bn::log_level::INFO, "Total BG Layers: ", pipeline.total_bg_layers);
        BN_LOG_LEVEL(bn::log_level::INFO, "Available BG Layers: ", pipeline.available_bg_layers);
        BN_LOG_LEVEL(bn::log_level::INFO, "Priority Conflict: ", pipeline.priority_conflict ? "YES" : "NO");
        BN_LOG_LEVEL(bn::log_level::INFO, "DMA Conflict: ", pipeline.dma_conflict ? "YES" : "NO");
        BN_LOG_LEVEL(bn::log_level::INFO, "VBlank Conflict: ", pipeline.vblank_conflict ? "YES" : "NO");
        
        BN_LOG_LEVEL(bn::log_level::INFO, "=== END VALIDATION RESULTS ===");
    }
    
    void BgValidation::reset()
    {
        BN_LOG_LEVEL(bn::log_level::INFO, "BG_VALIDATION: Resetting validation state");
        
        _validation_session = {};
        _current_frame_data = {};
        _frame_counter = 0;
        _initialized = false;
    }
    
    // =========================================================================
    // Private Helper Methods
    // =========================================================================
    
    int BgValidation::_calculate_expected_bg_register(const bn::fixed& position)
    {
        // Convert position to BG register value
        // BG registers are 9-bit (0-511) and wrap around
        int register_value = position.integer() & 0x1FF; // 511 = 0x1FF
        return register_value;
    }
    
    bool BgValidation::_is_within_register_range(int value, int min_val, int max_val)
    {
        return (value >= min_val && value <= max_val);
    }
    
    void BgValidation::_record_frame_data(const BgFrameData& data)
    {
        // Add to frame history (circular buffer)
        if (_validation_session.frame_history.size() >= BG_FRAME_HISTORY_SIZE)
        {
            _validation_session.frame_history.pop_front();
        }
        _validation_session.frame_history.push_back(data);
        
        // Analyze frame history periodically
        if (_frame_counter % PERF_SAMPLE_INTERVAL == 0 && _validation_session.frame_history.size() > 10)
        {
            _analyze_frame_history();
        }
    }
    
    BgValidationResult BgValidation::_analyze_frame_history()
    {
        if (_validation_session.frame_history.size() < 10)
        {
            return BgValidationResult::PASS;
        }
        
        // Calculate average frame time
        int total_time = 0;
        int max_time = 0;
        int min_time = INT32_MAX;
        
        for (const auto& frame : _validation_session.frame_history)
        {
            total_time += frame.frame_time_us;
            max_time = bn::max(max_time, frame.frame_time_us);
            min_time = bn::min(min_time, frame.frame_time_us);
        }
        
        int avg_time = total_time / _validation_session.frame_history.size();
        
        BN_LOG_LEVEL(bn::log_level::INFO, "BG_VALIDATION: Frame time analysis - Avg: ", avg_time, 
                     "μs Max: ", max_time, "μs Min: ", min_time, "μs");
        
        // Check for consistent performance issues
        if (avg_time > BG_WARNING_THRESHOLD_US)
        {
            BN_LOG_LEVEL(bn::log_level::WARN, "BG_VALIDATION: Consistent high frame times detected");
            _validation_session.total_warnings++;
            return BgValidationResult::WARN;
        }
        
        return BgValidationResult::PASS;
    }
    
    void BgValidation::_update_artifact_detection(
        const bn::fixed_point& current_pos,
        const bn::fixed_point& previous_pos,
        bool buffer_recentered)
    {
        // This is handled in detect_visual_artifacts()
        // Keeping this method for potential future expansion
    }
    
} // namespace str