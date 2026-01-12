/**
 * @file ota.c
 * @brief 设备OTA
 * @version 1.0
 *
 * @copyright Copyright (c) 2024  雅马哈发动机（厦门）信息系统有限公司
 *
 */
#include "ota.h"
#include "mqtt_cmd_type.h"

static const char *TAG = "OTA";
SemaphoreHandle_t g_startOtaTaskSemphHandle; // 开始OTA更新句柄
/**
 * @brief  Ota Event handler
 * @param  arg
 * @param  event_base
 * @param  event_id
 * @param  event_data
 */
void otaEventHandler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == ESP_HTTPS_OTA_EVENT)
    {
        switch (event_id)
        {
        case ESP_HTTPS_OTA_START:
            ESP_LOGI(TAG, "OTA started");
            break;
        case ESP_HTTPS_OTA_CONNECTED:
            ESP_LOGI(TAG, "Connected to server");
            break;
        case ESP_HTTPS_OTA_GET_IMG_DESC:
            ESP_LOGI(TAG, "Reading Image Description");
            break;
        case ESP_HTTPS_OTA_VERIFY_CHIP_ID:
            ESP_LOGI(TAG, "Verifying chip id of new image: %d", *(esp_chip_id_t *)event_data);
            break;
        case ESP_HTTPS_OTA_DECRYPT_CB:
            ESP_LOGI(TAG, "Callback to decrypt function");
            break;
        case ESP_HTTPS_OTA_WRITE_FLASH:
            ESP_LOGD(TAG, "Writing to flash: %d written", *(int *)event_data);
            break;
        case ESP_HTTPS_OTA_UPDATE_BOOT_PARTITION:
            ESP_LOGI(TAG, "Boot partition updated. Next Partition: %d", *(esp_partition_subtype_t *)event_data);
            break;
        case ESP_HTTPS_OTA_FINISH:
            ESP_LOGI(TAG, "OTA finish");
            break;
        case ESP_HTTPS_OTA_ABORT:
            ESP_LOGI(TAG, "OTA abort");
            break;
        }
    }
}

/**
 * @brief  固件版本信息对比
 * @param  new_app_info
 * @return esp_err_t
 */
static esp_err_t validate_image_header(esp_app_desc_t *new_app_info)
{
    if (new_app_info == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_app_desc_t running_app_info;
    if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK)
    {
        ESP_LOGI(TAG, "Running firmware version: %s", running_app_info.version);
    }

    // #ifndef CONFIG_EXAMPLE_SKIP_VERSION_CHECK
    //     if (memcmp(new_app_info->app_elf_sha256, running_app_info.version, sizeof(new_app_info->version)) == 0)
    //     {
    //         ESP_LOGW(TAG, "Current running version is the same as a new. We will not continue the update.");
    //         return ESP_FAIL;
    //     }
    // #endif

    // #ifdef CONFIG_BOOTLOADER_APP_ANTI_ROLLBACK
    //     /**
    //      * Secure version check from firmware image header prevents subsequent download and flash write of
    //      * entire firmware image. However this is optional because it is also taken care in API
    //      * esp_https_ota_finish at the end of OTA update procedure.
    //      */
    //     const uint32_t hw_sec_version = esp_efuse_read_secure_version();
    //     if (new_app_info->secure_version < hw_sec_version)
    //     {
    //         ESP_LOGW(TAG, "New firmware security version is less than eFuse programmed, %" PRIu32 " < %" PRIu32, new_app_info->secure_version, hw_sec_version);
    //         return ESP_FAIL;
    //     }
    // #endif

    return ESP_OK;
}

/**
 * @brief  添加自定义请求头
 * @param  http_client
 * @return esp_err_t
 */
static esp_err_t _http_client_init_cb(esp_http_client_handle_t http_client)
{
    esp_err_t err = ESP_OK;
    /* Uncomment to add custom headers to HTTP request */
    // err = esp_http_client_set_header(http_client, "Custom-Header", "Value");
    return err;
}

/**
 * @brief OTA Task
 * @param  pvParameter
 */
void otaTask(void *pvParameter)
{
    for (;;)
    {
        xSemaphoreTake(g_startOtaTaskSemphHandle, portMAX_DELAY);
        ESP_ERROR_CHECK(esp_event_handler_register(ESP_HTTPS_OTA_EVENT, ESP_EVENT_ANY_ID, &otaEventHandler, NULL));
        ESP_LOGI(TAG, "\n\n -------Start OTA------- \n\n");
        ESP_LOGI(TAG, "Download Firmware form: %s", g_nvsData.networkConfigData.otaConfigData.esp32OtaServer1);
        char _mqttMsg[MAX_URL_BUF_LEN + 25] = "Download Firmware form: ";
        strcat(_mqttMsg, g_nvsData.networkConfigData.otaConfigData.esp32OtaServer1);
        mqttDefaultTopicPubStrMsg(MQTT_CONTROL_TYPE_SYSTEM_OTA, NOTIFY_OTA_STATE, "state", _mqttMsg);
        esp_err_t ota_finish_err = ESP_OK;
        esp_http_client_config_t config = {
            .url = g_nvsData.networkConfigData.otaConfigData.esp32OtaServer1,
            .timeout_ms = OTA_SERVER_RECV_TIMEOUT_MS,
            .crt_bundle_attach = esp_crt_bundle_attach,
            .keep_alive_enable = true,
        };
        esp_https_ota_config_t ota_config = {
            .http_config = &config,
            .http_client_init_cb = _http_client_init_cb, // Register a callback to be invoked after esp_http_client is initialized
        };

        esp_https_ota_handle_t https_ota_handle = NULL;
        esp_err_t err = esp_https_ota_begin(&ota_config, &https_ota_handle);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "ESP HTTPS OTA Begin failed");
            mqttDefaultTopicPubStrMsg(MQTT_CONTROL_TYPE_SYSTEM_OTA, NOTIFY_OTA_STATE, "state", "ESP HTTPS OTA Begin failed");
            goto http_end;
        }

        esp_app_desc_t app_desc;
        err = esp_https_ota_get_img_desc(https_ota_handle, &app_desc);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "esp_https_ota_read_img_desc failed");
            mqttDefaultTopicPubStrMsg(MQTT_CONTROL_TYPE_SYSTEM_OTA, NOTIFY_OTA_STATE, "state", "esp_https_ota_read_img_desc failed");
            goto ota_end;
        }
        err = validate_image_header(&app_desc);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "image header verification failed");
            mqttDefaultTopicPubStrMsg(MQTT_CONTROL_TYPE_SYSTEM_OTA, NOTIFY_OTA_STATE, "state", "image header verification failed");
            goto ota_end;
        }

        while (1)
        {
            err = esp_https_ota_perform(https_ota_handle);
            if (err != ESP_ERR_HTTPS_OTA_IN_PROGRESS)
            {
                break;
            }
            // esp_https_ota_perform returns after every read operation which gives user the ability to
            // monitor the status of OTA upgrade by calling esp_https_ota_get_image_len_read, which gives length of image
            // data read so far.
            ESP_LOGD(TAG, "Image bytes read: %d", esp_https_ota_get_image_len_read(https_ota_handle));
        }

        if (esp_https_ota_is_complete_data_received(https_ota_handle) != true)
        {
            // the OTA image was not completely received and user can customise the response to this situation.
            ESP_LOGE(TAG, "Complete data was not received.");
        }
        else
        {
            ota_finish_err = esp_https_ota_finish(https_ota_handle);
            if ((err == ESP_OK) && (ota_finish_err == ESP_OK))
            {
                ESP_LOGI(TAG, "ESP_HTTPS_OTA upgrade successful. Rebooting ...");
                mqttDefaultTopicPubStrMsg(MQTT_CONTROL_TYPE_SYSTEM_OTA, NOTIFY_OTA_STATE, "state", "SUCCEED System Restart");
                vTaskDelay(2000 / portTICK_PERIOD_MS);
                esp_restart();
            }
            else
            {
                if (ota_finish_err == ESP_ERR_OTA_VALIDATE_FAILED)
                {
                    ESP_LOGE(TAG, "Image validation failed, image is corrupted");
                    mqttDefaultTopicPubStrMsg(MQTT_CONTROL_TYPE_SYSTEM_OTA, NOTIFY_OTA_STATE, "state", "Image validation failed, image is corrupted");
                }
                ESP_LOGE(TAG, "ESP_HTTPS_OTA upgrade failed 0x%x", ota_finish_err);
                goto ota_end;
            }
        }
    ota_end:
        esp_https_ota_abort(https_ota_handle);
        ESP_LOGE(TAG, "ESP_HTTPS_OTA upgrade failed");
        mqttDefaultTopicPubStrMsg(MQTT_CONTROL_TYPE_SYSTEM_OTA, NOTIFY_OTA_STATE, "state", "FAILED");
    http_end:
        mqttDefaultTopicPubStrMsg(MQTT_CONTROL_TYPE_SYSTEM_OTA, NOTIFY_OTA_STATE, "state", "FAILED");
    }
}
