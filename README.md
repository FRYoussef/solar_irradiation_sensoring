# solar_irradiation_sensoring
Solar irradiation sensoring using ESP-IDF

## Software Requirements
- Tested under ESP-IDF 4.2
- Using menuconfig you have to activate the following options:
    - Partition Table  → Partition Table → Custom partition table CSV
    - Component config → Power Management → Support for Power Management → Enable DFS at startup
    - Component config → FreeRTOS → Tickless Idle Support
    - Serial flasher config → Flash size → 4MB

## WiFi configuration
Download your WiFi certificate into ´src´ folder. For example, for UCM eduroam wifi you can download it from [here](https://ssii.ucm.es/file/eduroam). Next, change its name to "wpa2_ca.pem".

For the sake of simplicity "wpa2_client.crt" and "wpa2_client.key" are provided for testing. However, you should use your own private key.

Via menuconfig, set the following parameters (example of eduroam WiFi):
- SSID: eduroam
- Validate server: active
- EAP method: TTLS
- Phase2 method for TTLS: PAP
- EAP ID: anonymous@ucm.es
- EAP USERNAME: youruser@ucm.es
- EAP PASSWORD: (your UCM password)