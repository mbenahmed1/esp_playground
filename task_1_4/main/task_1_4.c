/* 
    1.3 Senden eines Audio per TCP-Socket
*/

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "es8388.h"
#include "sdkconfig.h"
#include "audio_element.h"
#include "audio_pipeline.h"
#include "audio_common.h"
#include "board.h"
#include "fatfs_stream.h"
#include "i2s_stream.h"
#include "tcp_client_stream.h"
#include "wav_encoder.h"
#include "esp_peripherals.h"
#include "periph_button.h"
#include "periph_sdcard.h"
#include "periph_wifi.h"
#include "filter_resample.h"
#include "input_key_service.h"

#if __has_include("esp_idf_version.h")
#include "esp_idf_version.h"
#else
#define ESP_IDF_VERSION_VAL(major, minor, patch) 1
#endif

#if (ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 1, 0))
#include "esp_netif.h"
#else
#include "tcpip_adapter.h"
#endif

static const char *TAG = "TASK_1_4";

#define DEMO_EXIT_BIT (BIT0)

static audio_pipeline_handle_t rec_pipeline;
static audio_pipeline_handle_t tcp_pipeline;
static EventGroupHandle_t EXIT_FLAG;
static bool rec = false;

esp_err_t tcp_stream_event_handle(tcp_stream_event_msg_t *msg, tcp_stream_status_t state, void *event_ctx)
{
    return ESP_OK;
}

static esp_err_t input_key_service_cb(periph_service_handle_t handle, periph_service_event_t *evt, void *ctx)
{
    audio_element_handle_t fatfs_stream_writer = (audio_element_handle_t)ctx;
    if (evt->type == INPUT_KEY_SERVICE_ACTION_CLICK)
    {
        switch ((int)evt->data)
        {
        case INPUT_KEY_USER_ID_REC:
            if (rec)
            {
                ESP_LOGI(TAG, "[ * ] [Rec] Stop audio rec_pipeline ...");
                audio_pipeline_stop(rec_pipeline);
                audio_pipeline_wait_for_stop(rec_pipeline);
                audio_pipeline_terminate(rec_pipeline);

                xEventGroupSetBits(EXIT_FLAG, DEMO_EXIT_BIT);
            }
            else
            {
                ESP_LOGI(TAG, "[ * ] [Rec] Start audio rec_pipeline ...");
                audio_element_set_uri(fatfs_stream_writer, "/sdcard/rec.wav");
                audio_pipeline_run(rec_pipeline);
                rec = true;
            }
        }
    }
    return ESP_OK;
}

void app_main(void)
{
    audio_element_handle_t fatfs_stream_reader, fatfs_stream_writer, i2s_stream_reader, wav_encoder, tcp_stream_writer;

    esp_log_level_set("*", ESP_LOG_WARN);
    esp_log_level_set(TAG, ESP_LOG_INFO);

    EXIT_FLAG = xEventGroupCreate();

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES)
    {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
#if (ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 1, 0))
    ESP_ERROR_CHECK(esp_netif_init());
#else
    tcpip_adapter_init();
#endif

    ESP_LOGI(TAG, "[ 1 ] Mount sdcard");
    // Initialize peripherals management
    esp_periph_config_t periph_cfg = DEFAULT_ESP_PERIPH_SET_CONFIG();
    esp_periph_set_handle_t set = esp_periph_set_init(&periph_cfg);

    // Initialize SD Card peripheral
    audio_board_sdcard_init(set, SD_MODE_1_LINE);

    // set = esp_periph_set_init(&periph_cfg);
    ESP_LOGI(TAG, "[ 2 ] Establish WiFi connection");
    periph_wifi_cfg_t wifi_cfg = {
        .ssid = CONFIG_WIFI_SSID,
        .password = CONFIG_WIFI_PASSWORD,
    };
    esp_periph_handle_t wifi_handle = periph_wifi_init(&wifi_cfg);

    // // Start wifi & button peripheral
    esp_periph_start(set, wifi_handle);
    periph_wifi_wait_for_connected(wifi_handle, portMAX_DELAY);

    ESP_LOGI(TAG, "[ 3 ] Start codec chip");
    audio_board_handle_t board_handle = audio_board_init();
    audio_hal_ctrl_codec(board_handle->audio_hal, AUDIO_HAL_CODEC_MODE_ENCODE, AUDIO_HAL_CTRL_START);

    ESP_LOGI(TAG, "[4.0] Create audio rec_pipeline for recording");
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    rec_pipeline = audio_pipeline_init(&pipeline_cfg);

    ESP_LOGI(TAG, "[4.1] Create audio tcp_pipeline for recording");
    tcp_pipeline = audio_pipeline_init(&pipeline_cfg);
    mem_assert(rec_pipeline);
    mem_assert(tcp_pipeline);

    ESP_LOGI(TAG, "[4.2] Create fatfs stream to write data to sdcard");
    fatfs_stream_cfg_t fatfs_cfg = FATFS_STREAM_CFG_DEFAULT();
    fatfs_cfg.type = AUDIO_STREAM_WRITER;
    fatfs_stream_writer = fatfs_stream_init(&fatfs_cfg);

    ESP_LOGI(TAG, "[4.3] Create fatfs stream to read data from sdcard");
    fatfs_cfg.type = AUDIO_STREAM_READER;
    fatfs_stream_reader = fatfs_stream_init(&fatfs_cfg);
    audio_element_set_uri(fatfs_stream_reader, "/sdcard/REC.WAV");

    ESP_LOGI(TAG, "[4.4] Create i2s stream to read audio data from codec chip");
    i2s_stream_cfg_t i2s_cfg = I2S_STREAM_CFG_DEFAULT();
    i2s_cfg.type = AUDIO_STREAM_READER;
#if defined CONFIG_ESP_LYRAT_MINI_V1_1_BOARD
    i2s_cfg.i2s_port = 1;
#endif
    i2s_stream_reader = i2s_stream_init(&i2s_cfg);

    ESP_LOGI(TAG, "[4.5] Create wav encoder to encode wav format");
    wav_encoder_cfg_t wav_cfg = DEFAULT_WAV_ENCODER_CONFIG();
    wav_encoder = wav_encoder_init(&wav_cfg);

    ESP_LOGI(TAG, "[4.6] Create tcp stream to post data to server");
    tcp_stream_cfg_t tcp_cfg = TCP_STREAM_CFG_DEFAULT();
    tcp_cfg.type = AUDIO_STREAM_WRITER;
    tcp_cfg.event_handler = tcp_stream_event_handle;
    tcp_cfg.port = CONFIG_SERVER_PORT;
    tcp_cfg.host = CONFIG_SERVER_HOST;
    tcp_cfg.timeout_ms = CONFIG_TCP_TIMEOUT;
    tcp_stream_writer = tcp_stream_init(&tcp_cfg);

    ESP_LOGI(TAG, "[4.7] Register all elements to audio rec_pipeline");
    audio_pipeline_register(rec_pipeline, i2s_stream_reader, "i2s");
    /**
     * Wav encoder actually passes data without doing anything, which makes the rec_pipeline structure easy to understand.
     * Because WAV is raw data and audio information is stored in the header,
     * I2S Stream will write the WAV header after ending the record with enough the information
     */
    audio_pipeline_register(rec_pipeline, wav_encoder, "wav");
    audio_pipeline_register(rec_pipeline, fatfs_stream_writer, "writer");

    ESP_LOGI(TAG, "[4.8] Register all elements to audio tcp_pipeline");
    audio_pipeline_register(tcp_pipeline, fatfs_stream_reader, "reader");
    audio_pipeline_register(tcp_pipeline, tcp_stream_writer, "tcp");

    ESP_LOGI(TAG, "[4.9] Link it together [sd-card]-->i2s_stream->tcp_stream-->[tcp_server]");
    const char *tcp_link_tag[2] = {"reader", "tcp"};
    audio_pipeline_link(tcp_pipeline, &tcp_link_tag[0], 2);

    ESP_LOGI(TAG, "[4.10] Link it together [codec_chip]-->i2s_stream-->wav_encoder-->fatfs_stream-->[sdcard]");
    const char *rec_link_tag[3] = {"i2s", "wav", "writer"};
    audio_pipeline_link(rec_pipeline, &rec_link_tag[0], 3);

    ESP_LOGI(TAG, "[4.11] Set up  event listener");
    audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
    audio_event_iface_handle_t evt = audio_event_iface_init(&evt_cfg);

    ESP_LOGI(TAG, "[4.12] Listening event from pipeline");
    audio_pipeline_set_listener(tcp_pipeline, evt);

    ESP_LOGI(TAG, "[ 5 ] Initialize buttons and assign callback function");

    // Initialize Button peripheral
    audio_board_key_init(set);
    input_key_service_info_t input_key_info[] = INPUT_KEY_DEFAULT_INFO();
    input_key_service_cfg_t input_cfg = INPUT_KEY_SERVICE_DEFAULT_CONFIG();
    input_cfg.handle = set;
    periph_service_handle_t input_ser = input_key_service_create(&input_cfg);
    input_key_service_add_key(input_ser, input_key_info, INPUT_KEY_NUM);
    periph_service_set_callback(input_ser, input_key_service_cb, (void *)fatfs_stream_writer);

    // setting mic gain
    es8388_write_reg(ES8388_ADCCONTROL1, 0x84);

    // set clock rate
    i2s_stream_set_clk(i2s_stream_reader, CONFIG_AUDIO_RATE, CONFIG_AUDIO_BITS, CONFIG_AUDIO_CHANNELS);

    ESP_LOGI(TAG, "[ 6 ] Press [Rec] button to record, Press [Mode] to exit");

    // wait for input
    xEventGroupWaitBits(EXIT_FLAG, DEMO_EXIT_BIT, true, false, portMAX_DELAY);

    ESP_LOGI(TAG, "[ 7 ] Start tcp pipeline");
    audio_event_iface_set_listener(esp_periph_set_get_event_iface(set), evt);

    audio_pipeline_run(tcp_pipeline);

    while (1)
    {
        audio_event_iface_msg_t msg;
        if (audio_event_iface_listen(evt, &msg, 1000 / portTICK_RATE_MS) != ESP_OK)
        {
            continue;
        }
        if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT && msg.source == (void *)fatfs_stream_reader && msg.cmd == AEL_MSG_CMD_REPORT_STATUS && (((int)msg.data == AEL_STATUS_STATE_STOPPED) || ((int)msg.data == AEL_STATUS_STATE_FINISHED)))
        {
            ESP_LOGW(TAG, "[ * ] Stop event received");
            break;
        }
    }

    // stop tcp pipeline
    audio_pipeline_stop(tcp_pipeline);
    audio_pipeline_wait_for_stop(tcp_pipeline);
    audio_pipeline_terminate(tcp_pipeline);

    // unregister streams
    audio_pipeline_unregister(tcp_pipeline, fatfs_stream_reader);
    audio_pipeline_unregister(tcp_pipeline, tcp_stream_writer);

    // same for rec pipeline
    audio_pipeline_unregister(rec_pipeline, wav_encoder);
    audio_pipeline_unregister(rec_pipeline, i2s_stream_reader);
    audio_pipeline_unregister(rec_pipeline, fatfs_stream_writer);

    /* Release all resources */
    esp_periph_set_destroy(set);

    ESP_LOGI(TAG, "[ -1 ] End");
}
