set(SOURCES main.c)
list(APPEND SOURCES "adc_reader.c")
list(APPEND SOURCES "app_prov_handlers.c")
list(APPEND SOURCES "app_prov.c")
list(APPEND SOURCES "mqtt.c")
list(APPEND SOURCES "provisioning.c")
list(APPEND SOURCES "redireccionLogs.c")
list(APPEND SOURCES "sincTime.c")
list(APPEND SOURCES "wpa2_enterprise.c")

idf_component_register(SRCS ${SOURCES} 
                    INCLUDE_DIRS "." 
                    EMBED_TXTFILES wpa2_ca.pem wpa2_client.crt wpa2_client.key)