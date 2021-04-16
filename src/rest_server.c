/* HTTP Restful API Server

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include "rest_server.h"
#include "sensors_readings.h"

static const char *TAG = "server_http";
extern struct si7021_reading getTempyHum();
extern struct sgp30_reading get_co2();
extern void modificaSampleFreqCO2(int result);
extern void modificaSendFreqCO2(int result);
extern void modificaNSamplesCO2(int result);
extern void modificaSampleFreqTemp(int result);
extern void modificaSendFreqTemp(int result);
extern void modificaNSamplesTemp(int result);
extern void modificaScanFreq(int scan_freq);
extern void modificaTimeScan(int time_scan);
extern int getAforo();
extern void modificaDistanciaMaxima(int dist);
extern void updateDeepSleepTimer();
extern void modificaSleepHour(int sleep);
extern void modificaWakeupHour(int wakeup);

static int extract_int_cbor(uint8_t * buf){
    CborParser parser;
    CborValue value;
    int result = -1;
    if (cbor_parser_init(buf, 2, 0, &parser, &value) != CborNoError){
        ESP_LOGE(TAG, "No se inicializar el parser para cbor");
    }
    if (!cbor_value_is_integer(&value) ||
            cbor_value_get_int(&value, &result) != CborNoError){
        ESP_LOGE(TAG, "No se puede decodificar un entero");
    }
    return result;
}


static esp_err_t co2_sample_freq_post_handler(httpd_req_t *req)
{
    int total_len = req->content_len;
    int cur_len = 0;
    char *buf = ((rest_server_context_t *)(req->user_ctx))->scratch;
    int received = 0;
    if (total_len >= SCRATCH_BUFSIZE) {
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
        return ESP_FAIL;
    }
    while (cur_len < total_len) {
        received = httpd_req_recv(req, buf + cur_len, total_len);
        if (received <= 0) {
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
            return ESP_FAIL;
        }
        cur_len += received;
    }
    buf[total_len] = '\0';

    int result = extract_int_cbor((uint8_t *) buf);
    if (result != -1){
        modificaSampleFreqCO2(result);
    }
    httpd_resp_sendstr(req, "Post control value successfully");

    return ESP_OK;
}

static esp_err_t co2_send_freq_post_handler(httpd_req_t *req)
{
    int total_len = req->content_len;
    int cur_len = 0;
    char *buf = ((rest_server_context_t *)(req->user_ctx))->scratch;
    int received = 0;
    if (total_len >= SCRATCH_BUFSIZE) {
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
        return ESP_FAIL;
    }
    while (cur_len < total_len) {
        received = httpd_req_recv(req, buf + cur_len, total_len);
        if (received <= 0) {
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
            return ESP_FAIL;
        }
        cur_len += received;
    }
    buf[total_len] = '\0';

    int result = extract_int_cbor((uint8_t *) buf);
    if (result != -1){
        modificaSendFreqCO2(result);
    }

    httpd_resp_sendstr(req, "Post control value successfully");

    return ESP_OK;
}

static esp_err_t co2_n_samples_post_handler(httpd_req_t *req)
{
    int total_len = req->content_len;
    int cur_len = 0;
    char *buf = ((rest_server_context_t *)(req->user_ctx))->scratch;
    int received = 0;
    if (total_len >= SCRATCH_BUFSIZE) {
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
        return ESP_FAIL;
    }
    while (cur_len < total_len) {
        received = httpd_req_recv(req, buf + cur_len, total_len);
        if (received <= 0) {
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
            return ESP_FAIL;
        }
        cur_len += received;
    }
    buf[total_len] = '\0';

    int result = extract_int_cbor((uint8_t *) buf);
    if (result != -1){
        modificaNSamplesCO2(result);
    }

    httpd_resp_sendstr(req, "Post control value successfully");

    return ESP_OK;
}

static esp_err_t temp_sample_freq_post_handler(httpd_req_t *req)
{
    int total_len = req->content_len;
    int cur_len = 0;
    char *buf = ((rest_server_context_t *)(req->user_ctx))->scratch;
    int received = 0;
    if (total_len >= SCRATCH_BUFSIZE) {
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
        return ESP_FAIL;
    }
    while (cur_len < total_len) {
        received = httpd_req_recv(req, buf + cur_len, total_len);
        if (received <= 0) {
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
            return ESP_FAIL;
        }
        cur_len += received;
    }
    buf[total_len] = '\0';

    int result = extract_int_cbor((uint8_t *) buf);
    if (result != -1){
        modificaSampleFreqTemp(result);
    }
    httpd_resp_sendstr(req, "Post control value successfully");

    return ESP_OK;
}

static esp_err_t temp_send_freq_post_handler(httpd_req_t *req)
{
    int total_len = req->content_len;
    int cur_len = 0;
    char *buf = ((rest_server_context_t *)(req->user_ctx))->scratch;
    int received = 0;
    if (total_len >= SCRATCH_BUFSIZE) {
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
        return ESP_FAIL;
    }
    while (cur_len < total_len) {
        received = httpd_req_recv(req, buf + cur_len, total_len);
        if (received <= 0) {
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
            return ESP_FAIL;
        }
        cur_len += received;
    }
    buf[total_len] = '\0';

    int result = extract_int_cbor((uint8_t *) buf);
    if (result != -1){
        modificaSendFreqTemp(result);
    }

    httpd_resp_sendstr(req, "Post control value successfully");

    return ESP_OK;
}

static esp_err_t temp_n_samples_post_handler(httpd_req_t *req)
{
    int total_len = req->content_len;
    int cur_len = 0;
    char *buf = ((rest_server_context_t *)(req->user_ctx))->scratch;
    int received = 0;
    if (total_len >= SCRATCH_BUFSIZE) {
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
        return ESP_FAIL;
    }
    while (cur_len < total_len) {
        received = httpd_req_recv(req, buf + cur_len, total_len);
        if (received <= 0) {
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
            return ESP_FAIL;
        }
        cur_len += received;
    }
    buf[total_len] = '\0';

    int result = extract_int_cbor((uint8_t *) buf);
    if (result != -1){
        modificaNSamplesTemp(result);
    }

    httpd_resp_sendstr(req, "Post control value successfully");

    return ESP_OK;
}

static esp_err_t scan_freq_post_handler(httpd_req_t *req)
{
    int total_len = req->content_len;
    int cur_len = 0;
    char *buf = ((rest_server_context_t *)(req->user_ctx))->scratch;
    int received = 0;
    if (total_len >= SCRATCH_BUFSIZE) {
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
        return ESP_FAIL;
    }
    while (cur_len < total_len) {
        received = httpd_req_recv(req, buf + cur_len, total_len);
        if (received <= 0) {
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
            return ESP_FAIL;
        }
        cur_len += received;
    }
    buf[total_len] = '\0';

    int result = extract_int_cbor((uint8_t *) buf);
    if (result != -1){
        modificaScanFreq(result);
    }

    httpd_resp_sendstr(req, "Post control value successfully");

    return ESP_OK;
}

static esp_err_t time_scan_post_handler(httpd_req_t *req)
{
    int total_len = req->content_len;
    int cur_len = 0;
    char *buf = ((rest_server_context_t *)(req->user_ctx))->scratch;
    int received = 0;
    if (total_len >= SCRATCH_BUFSIZE) {
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
        return ESP_FAIL;
    }
    while (cur_len < total_len) {
        received = httpd_req_recv(req, buf + cur_len, total_len);
        if (received <= 0) {
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
            return ESP_FAIL;
        }
        cur_len += received;
    }
    buf[total_len] = '\0';

    int result = extract_int_cbor((uint8_t *) buf);
    if (result != -1){
        modificaTimeScan(result);
    }

    httpd_resp_sendstr(req, "Post control value successfully");

    return ESP_OK;
}

static esp_err_t dist_max_post_handler(httpd_req_t *req)
{
    int total_len = req->content_len;
    int cur_len = 0;
    char *buf = ((rest_server_context_t *)(req->user_ctx))->scratch;
    int received = 0;
    if (total_len >= SCRATCH_BUFSIZE) {
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
        return ESP_FAIL;
    }
    while (cur_len < total_len) {
        received = httpd_req_recv(req, buf + cur_len, total_len);
        if (received <= 0) {
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
            return ESP_FAIL;
        }
        cur_len += received;
    }
    buf[total_len] = '\0';

    int result = extract_int_cbor((uint8_t *) buf);
    if (result != -1){
        modificaDistanciaMaxima(result);
    }

    httpd_resp_sendstr(req, "Post control value successfully");

    return ESP_OK;
}

static esp_err_t sleep_post_handler(httpd_req_t *req)
{
    int total_len = req->content_len;
    int cur_len = 0;
    char *buf = ((rest_server_context_t *)(req->user_ctx))->scratch;
    int received = 0;
    if (total_len >= SCRATCH_BUFSIZE) {
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
        return ESP_FAIL;
    }
    while (cur_len < total_len) {
        received = httpd_req_recv(req, buf + cur_len, total_len);
        if (received <= 0) {
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
            return ESP_FAIL;
        }
        cur_len += received;
    }
    buf[total_len] = '\0';

    int result = extract_int_cbor((uint8_t *) buf);
    if (result != -1){
        modificaSleepHour(result);
    }

    httpd_resp_sendstr(req, "Post control value successfully");

    return ESP_OK;
}

static esp_err_t wakeup_post_handler(httpd_req_t *req)
{
    int total_len = req->content_len;
    int cur_len = 0;
    char *buf = ((rest_server_context_t *)(req->user_ctx))->scratch;
    int received = 0;
    if (total_len >= SCRATCH_BUFSIZE) {
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
        return ESP_FAIL;
    }
    while (cur_len < total_len) {
        received = httpd_req_recv(req, buf + cur_len, total_len);
        if (received <= 0) {
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
            return ESP_FAIL;
        }
        cur_len += received;
    }
    buf[total_len] = '\0';

    int result = extract_int_cbor((uint8_t *) buf);
    if (result != -1){
        modificaWakeupHour(result);
    }

    httpd_resp_sendstr(req, "Post control value successfully");

    return ESP_OK;
}

static esp_err_t deep_sleep_update_post_handler(httpd_req_t *req)
{
    updateDeepSleepTimer();
    httpd_resp_sendstr(req, "Post control value successfully");

    return ESP_OK;
}

static esp_err_t cbor_temp_hum_get_handler(httpd_req_t *req)
{   
    httpd_resp_set_type(req, "application/cbor");
    CborEncoder root_encoder;
    uint8_t buf[500];

    // Codificador CBOR.
    cbor_encoder_init(&root_encoder, buf, sizeof(buf), 0);

    //Leemos la temperatura y humedad
    struct si7021_reading temp_hum = getTempyHum();

    //Codificamos CBOR
    CborEncoder map_encoder;
    cbor_encoder_create_map(&root_encoder, &map_encoder, 2); // {
    	//1.
    cbor_encode_text_stringz(&map_encoder, "temperatura");
    cbor_encode_float(&map_encoder, temp_hum.temperature);
    	//2.
    cbor_encode_text_stringz(&map_encoder, "humedad");
    cbor_encode_float(&map_encoder, temp_hum.humidity);
    cbor_encoder_close_container(&root_encoder, &map_encoder); // }

    // Enviamos respuesta, consultando previamente el tamaño del buffer codificado.
    httpd_resp_send(req, (char*)buf, cbor_encoder_get_buffer_size( &root_encoder, buf));
    ESP_LOGI(TAG, "Buffer tam: %d content: %s\n", cbor_encoder_get_buffer_size( &root_encoder, buf), buf);

    return ESP_OK;
}

static esp_err_t cbor_co2_get_handler(httpd_req_t *req)
{   
    httpd_resp_set_type(req, "application/cbor");
    CborEncoder root_encoder;
    uint8_t buf[500];

    // Codificador CBOR.
    cbor_encoder_init(&root_encoder, buf, sizeof(buf), 0);

    //Leemos la temperatura y humedad
    struct sgp30_reading temp_hum = get_co2();

    //Codificamos CBOR
    CborEncoder map_encoder;
    cbor_encoder_create_map(&root_encoder, &map_encoder, 2); // {
    	//1.
    cbor_encode_text_stringz(&map_encoder, "TVOC");
    cbor_encode_float(&map_encoder, temp_hum.TVOC);
    	//2.
    cbor_encode_text_stringz(&map_encoder, "CO2");
    cbor_encode_float(&map_encoder, temp_hum.CO2);
    cbor_encoder_close_container(&root_encoder, &map_encoder); // }

    // Enviamos respuesta, consultando previamente el tamaño del buffer codificado.
    httpd_resp_send(req, (char*)buf, cbor_encoder_get_buffer_size( &root_encoder, buf));
    ESP_LOGI(TAG, "Buffer tam: %d content: %s\n", cbor_encoder_get_buffer_size( &root_encoder, buf), buf);

    return ESP_OK;
}

static esp_err_t cbor_aforo_get_handler(httpd_req_t *req)
{   
    httpd_resp_set_type(req, "application/cbor");
    CborEncoder root_encoder;
    uint8_t buf[500];

    // Codificador CBOR.
    cbor_encoder_init(&root_encoder, buf, sizeof(buf), 0);

    //Leemos el aforo
    int aforo =  getAforo();

    //Codificamos CBOR
    CborEncoder map_encoder;
    cbor_encoder_create_map(&root_encoder, &map_encoder, 1); // {
    	//1.
    cbor_encode_text_stringz(&map_encoder, "Aforo");
    cbor_encode_float(&map_encoder, aforo);
    cbor_encoder_close_container(&root_encoder, &map_encoder); // }

    // Enviamos respuesta, consultando previamente el tamaño del buffer codificado.
    httpd_resp_send(req, (char*)buf, cbor_encoder_get_buffer_size( &root_encoder, buf));
    ESP_LOGI(TAG, "Buffer tam: %d content: %s\n", cbor_encoder_get_buffer_size( &root_encoder, buf), buf);

    return ESP_OK;
}

esp_err_t start_rest_server(const char *base_path)
{
    REST_CHECK(base_path, "wrong base path", err);
    rest_server_context_t *rest_context = calloc(1, sizeof(rest_server_context_t));
    REST_CHECK(rest_context, "No memory for rest context", err);
    strlcpy(rest_context->base_path, base_path, sizeof(rest_context->base_path));

    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard;
    config.max_uri_handlers = 18;

    ESP_LOGI(TAG, "Starting HTTP Server");
    REST_CHECK(httpd_start(&server, &config) == ESP_OK, "Start server failed", err_start);

    /* POST SAMPLE_FREQ CO2 */
    httpd_uri_t co2_sample_freq_post_uri = {
        .uri = "/api/proyecto/co2/sample_freq",
        .method = HTTP_POST,
        .handler = co2_sample_freq_post_handler,
        .user_ctx = rest_context
    };
    httpd_register_uri_handler(server, &co2_sample_freq_post_uri);

    /* POST SEND_FREQ CO2 */
    httpd_uri_t co2_send_freq_post_uri = {
        .uri = "/api/proyecto/co2/send_freq",
        .method = HTTP_POST,
        .handler = co2_send_freq_post_handler,
        .user_ctx = rest_context
    };
    httpd_register_uri_handler(server, &co2_send_freq_post_uri);

    /* POST N_SAMPLES CO2 */
    httpd_uri_t co2_n_samples_post_uri = {
        .uri = "/api/proyecto/co2/n_samples",
        .method = HTTP_POST,
        .handler = co2_n_samples_post_handler,
        .user_ctx = rest_context
    };
    httpd_register_uri_handler(server, &co2_n_samples_post_uri);

    /* POST SAMPLE_FREQ TEMP*/
    httpd_uri_t temp_sample_freq_post_uri = {
        .uri = "/api/proyecto/temp/sample_freq",
        .method = HTTP_POST,
        .handler = temp_sample_freq_post_handler,
        .user_ctx = rest_context
    };
    httpd_register_uri_handler(server, &temp_sample_freq_post_uri);

    /* POST SEND_FREQ TEMP */
    httpd_uri_t temp_send_freq_post_uri = {
        .uri = "/api/proyecto/temp/send_freq",
        .method = HTTP_POST,
        .handler = temp_send_freq_post_handler,
        .user_ctx = rest_context
    };
    httpd_register_uri_handler(server, &temp_send_freq_post_uri);

    /* POST N_SAMPLES CO2 */
    httpd_uri_t temp_n_samples_post_uri = {
        .uri = "/api/proyecto/temp/n_samples",
        .method = HTTP_POST,
        .handler = temp_n_samples_post_handler,
        .user_ctx = rest_context
    };
    httpd_register_uri_handler(server, &temp_n_samples_post_uri);

    /* POST SCAN_FREQ */
    httpd_uri_t scan_freq_post_uri = {
        .uri = "/api/proyecto/aforo/scan_freq",
        .method = HTTP_POST,
        .handler = scan_freq_post_handler,
        .user_ctx = rest_context
    };
    httpd_register_uri_handler(server, &scan_freq_post_uri);

    /* POST TIME_SCAN */
    httpd_uri_t time_scan_post_uri = {
        .uri = "/api/proyecto/aforo/time_scan",
        .method = HTTP_POST,
        .handler = time_scan_post_handler,
        .user_ctx = rest_context
    };
    httpd_register_uri_handler(server, &time_scan_post_uri);

    /* POST DISTANCIA_MAXIMA */
    httpd_uri_t dist_max_post_uri = {
        .uri = "/api/proyecto/aforo/dist_max",
        .method = HTTP_POST,
        .handler = dist_max_post_handler,
        .user_ctx = rest_context
    };
    httpd_register_uri_handler(server, &dist_max_post_uri);

    /* POST SLEEP */
    httpd_uri_t sleep_post_uri = {
        .uri = "/api/proyecto/deep_sleep/sleep",
        .method = HTTP_POST,
        .handler = sleep_post_handler,
        .user_ctx = rest_context
    };
    httpd_register_uri_handler(server, &sleep_post_uri);

    /* POST WAKEUP */
    httpd_uri_t wakeup_post_uri = {
        .uri = "/api/proyecto/deep_sleep/wakeup",
        .method = HTTP_POST,
        .handler = wakeup_post_handler,
        .user_ctx = rest_context
    };
    httpd_register_uri_handler(server, &wakeup_post_uri);

    /* POST UPDATE */
    httpd_uri_t deep_sleep_update_post_uri = {
        .uri = "/api/proyecto/deep_sleep/update",
        .method = HTTP_POST,
        .handler = deep_sleep_update_post_handler,
        .user_ctx = rest_context
    };
    httpd_register_uri_handler(server, &deep_sleep_update_post_uri);

    /* URI handler for temp and hum */
    httpd_uri_t cbor_temp_hum_get_uri = {
        .uri = "/api/proyecto/temphum",
        .method = HTTP_GET,
        .handler = cbor_temp_hum_get_handler,
        .user_ctx = rest_context
    };
    httpd_register_uri_handler(server, &cbor_temp_hum_get_uri);

    /* URI handler for CO2 */
    httpd_uri_t cbor_co2_get_uri = {
        .uri = "/api/proyecto/co2",
        .method = HTTP_GET,
        .handler = cbor_co2_get_handler,
        .user_ctx = rest_context
    };
    httpd_register_uri_handler(server, &cbor_co2_get_uri);

    /* URI handler for CO2 */
    httpd_uri_t cbor_aforo_get_uri = {
        .uri = "/api/proyecto/aforo",
        .method = HTTP_GET,
        .handler = cbor_aforo_get_handler,
        .user_ctx = rest_context
    };
    httpd_register_uri_handler(server, &cbor_aforo_get_uri);




    return ESP_OK;
err_start:
    free(rest_context);
err:
    return ESP_FAIL;
}
