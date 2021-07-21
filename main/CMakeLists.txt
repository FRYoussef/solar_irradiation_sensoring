set(SOURCES main.c)
list(APPEND SOURCES "adc_reader.c")
list(APPEND SOURCES "app_prov.c")
list(APPEND SOURCES "app_prov_handlers.c")
list(APPEND SOURCES "fsm.c")
list(APPEND SOURCES "mqtt.c")
list(APPEND SOURCES "redireccionLogs.c")
list(APPEND SOURCES "si7021.c")


idf_component_register(SRCS ${SOURCES}
	            INCLUDE_DIRS "."
                EMBED_TXTFILES wpa2_ca.pem wpa2_client.crt wpa2_client.key)
