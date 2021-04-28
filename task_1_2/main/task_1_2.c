/* 
    1.2 Aufnahme eines Audios und Speichern auf der SD-Card
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
// #include "audio_event_iface.h"
#include "audio_common.h"
#include "board.h"
#include "fatfs_stream.h"
#include "i2s_stream.h"
#include "wav_encoder.h"
#include "esp_peripherals.h"
#include "periph_button.h"
#include "periph_sdcard.h"
#include "filter_resample.h"
#include "input_key_service.h"

static const char *TAG = "TASK_1_2";

#define DEMO_EXIT_BIT (BIT0)

static audio_pipeline_handle_t pipeline;
static EventGroupHandle_t EXIT_FLAG;
static bool rec = false;

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
                ESP_LOGI(TAG, "[ * ] [Rec] Stop audio pipeline ...");
                audio_pipeline_stop(pipeline);
                audio_pipeline_wait_for_stop(pipeline);
                audio_pipeline_terminate(pipeline);
                xEventGroupSetBits(EXIT_FLAG, DEMO_EXIT_BIT);
            }
            else
            {
                ESP_LOGI(TAG, "[ * ] [Rec] Start audio pipeline ...");
                audio_element_set_uri(fatfs_stream_writer, "/sdcard/rec.wav");
                audio_pipeline_run(pipeline);
                rec = true;
            }
        }
    }
    return ESP_OK;
}

void app_main(void)
{
    audio_element_handle_t fatfs_stream_writer, i2s_stream_reader, wav_encoder;

    esp_log_level_set("*", ESP_LOG_WARN);
    esp_log_level_set(TAG, ESP_LOG_INFO);

    EXIT_FLAG = xEventGroupCreate();

    ESP_LOGI(TAG, "[ 1 ] Mount sdcard");
    // Initialize peripherals management
    esp_periph_config_t periph_cfg = DEFAULT_ESP_PERIPH_SET_CONFIG();
    esp_periph_set_handle_t set = esp_periph_set_init(&periph_cfg);

    // Initialize SD Card peripheral
    audio_board_sdcard_init(set, SD_MODE_1_LINE);

    ESP_LOGI(TAG, "[ 2 ] Start codec chip");
    audio_board_handle_t board_handle = audio_board_init();
    audio_hal_ctrl_codec(board_handle->audio_hal, AUDIO_HAL_CODEC_MODE_ENCODE, AUDIO_HAL_CTRL_START);

    ESP_LOGI(TAG, "[3.0] Create audio pipeline for recording");
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    pipeline = audio_pipeline_init(&pipeline_cfg);
    mem_assert(pipeline);

    ESP_LOGI(TAG, "[3.1] Create fatfs stream to write data to sdcard");
    fatfs_stream_cfg_t fatfs_cfg = FATFS_STREAM_CFG_DEFAULT();
    fatfs_cfg.type = AUDIO_STREAM_WRITER;
    fatfs_stream_writer = fatfs_stream_init(&fatfs_cfg);

    ESP_LOGI(TAG, "[3.2] Create i2s stream to read audio data from codec chip");
    i2s_stream_cfg_t i2s_cfg = I2S_STREAM_CFG_DEFAULT();
    i2s_cfg.type = AUDIO_STREAM_READER;
#if defined CONFIG_ESP_LYRAT_MINI_V1_1_BOARD
    i2s_cfg.i2s_port = 1;
#endif
    i2s_stream_reader = i2s_stream_init(&i2s_cfg);

    ESP_LOGI(TAG, "[3.3] Create wav encoder to encode wav format");
    wav_encoder_cfg_t wav_cfg = DEFAULT_WAV_ENCODER_CONFIG();
    wav_encoder = wav_encoder_init(&wav_cfg);

    ESP_LOGI(TAG, "[3.4] Register all elements to audio pipeline");
    audio_pipeline_register(pipeline, i2s_stream_reader, "i2s");
    /**
     * Wav encoder actually passes data without doing anything, which makes the pipeline structure easy to understand.
     * Because WAV is raw data and audio information is stored in the header,
     * I2S Stream will write the WAV header after ending the record with enough the information
     */
    audio_pipeline_register(pipeline, wav_encoder, "wav");
    audio_pipeline_register(pipeline, fatfs_stream_writer, "file");

    ESP_LOGI(TAG, "[3.5] Link it together [codec_chip]-->i2s_stream-->wav_encoder-->fatfs_stream-->[sdcard]");
    const char *link_tag[3] = {"i2s", "wav", "file"};
    audio_pipeline_link(pipeline, &link_tag[0], 3);

    ESP_LOGI(TAG, "[ 4 ] Initialize buttons and assign callback function");

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
    i2s_stream_set_clk(i2s_stream_reader, 44100, 16, 2);

    ESP_LOGI(TAG, "[ 5 ] Press [Rec] button to record, Press [Mode] to exit");

    // wait for input
    xEventGroupWaitBits(EXIT_FLAG, DEMO_EXIT_BIT, true, false, portMAX_DELAY);

    ESP_LOGI(TAG, "[ -1 ] End");

    audio_pipeline_unregister(pipeline, wav_encoder);
    audio_pipeline_unregister(pipeline, i2s_stream_reader);
    audio_pipeline_unregister(pipeline, fatfs_stream_writer);

    /* Release all resources */
    esp_periph_set_destroy(set);
}
