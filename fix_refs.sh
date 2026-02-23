#!/bin/bash

# Fix all struct field references after refactoring

# CurrentSensor fields (isense -> current_sensor)
find Core/Src -name "*.c" -type f -exec sed -i 's/->isense\./->current_sensor./g' {} +
find Core/Src -name "*.c" -type f -exec sed -i 's/\.isense\./\.current_sensor./g' {} +

# Timing fields
find Core/Src -name "*.c" -type f -exec sed -i 's/->iter_start_us/->timing.iter_start_us/g' {} +
find Core/Src -name "*.c" -type f -exec sed -i 's/->iter_end_us/->timing.iter_end_us/g' {} +
find Core/Src -name "*.c" -type f -exec sed -i 's/->m_led_blink_ts/->timing.m_led_blink_ts/g' {} +
find Core/Src -name "*.c" -type f -exec sed -i 's/->wakeup_ts/->timing.wakeup_ts/g' {} +
find Core/Src -name "*.c" -type f -exec sed -i 's/->last_rx_heartbeat/->timing.last_rx_heartbeat/g' {} +
find Core/Src -name "*.c" -type f -exec sed -i 's/->last_rx_telembeat/->timing.last_rx_telembeat/g' {} +
find Core/Src -name "*.c" -type f -exec sed -i 's/->pl_last_ok_ts/->timing.pl_last_ok_ts/g' {} +
find Core/Src -name "*.c" -type f -exec sed -i 's/->pl_pulse_t/->timing.pl_pulse_t/g' {} +
find Core/Src -name "*.c" -type f -exec sed -i 's/->overtemp_last_ok_ts/->timing.overtemp_last_ok_ts/g' {} +

# Flags fields
find Core/Src -name "*.c" -type f -exec sed -i 's/->active/->flags.active/g' {} +
find Core/Src -name "*.c" -type f -exec sed -i 's/->need_to_reset_qstats/->flags.need_to_reset_qstats/g' {} +
find Core/Src -name "*.c" -type f -exec sed -i 's/->telem_enable/->flags.telem_enable/g' {} +
find Core/Src -name "*.c" -type f -exec sed -i 's/->need_to_save_faults/->flags.need_to_save_faults/g' {} +
find Core/Src -name "*.c" -type f -exec sed -i 's/->req_fault_clear/->flags.req_fault_clear/g' {} +
find Core/Src -name "*.c" -type f -exec sed -i 's/->need_to_sync_settings/->flags.need_to_sync_settings/g' {} +
find Core/Src -name "*.c" -type f -exec sed -i 's/->m_led_on/->flags.m_led_on/g' {} +
find Core/Src -name "*.c" -type f -exec sed -i 's/->has_balanced/->flags.has_balanced/g' {} +
find Core/Src -name "*.c" -type f -exec sed -i 's/->shutdown_requested/->flags.shutdown_requested/g' {} +
find Core/Src -name "*.c" -type f -exec sed -i 's/->shutdown_stack_requested/->flags.shutdown_stack_requested/g' {} +

# Profiling fields (times -> profiling.times)
find Core/Src -name "*.c" -type f -exec sed -i 's/->times\./->profiling.times./g' {} +
find Core/Src -name "*.c" -type f -exec sed -i 's/\.times\./\.profiling.times./g' {} +

echo "All references updated!"
