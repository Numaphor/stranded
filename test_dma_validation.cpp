#include "src/validation/dma/dma_validation.h"
#include "bn_log.h"

// Simple test runner for DMA validation
int main() {
    bn::log::set_enabled(true);
    
    // Run all DMA validation tests
    str::measure_dma_performance();
    str::validate_vblank_timing();
    str::test_batch_efficiency();
    str::run_dma_stress_tests();
    str::validate_hardware_compliance();
    str::run_comprehensive_batch_tests();
    str::test_batch_transfer_scenarios();
    
    return 0;
}