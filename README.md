# solar_irradiation_sensoring
Solar irradiation sensoring using ESP-IDF

## Software Requirements
- Tested under ESP-IDF 4.2
- Using menuconfig you have to activate the following options:
    - Partition Table  → Partition Table → Custom partition table CSV
    - Component config → Power Management → Support for Power Management → Enable DFS at startup
    - Component config → FreeRTOS → Tickless Idle Support