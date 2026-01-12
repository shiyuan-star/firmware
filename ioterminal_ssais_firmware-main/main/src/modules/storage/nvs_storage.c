/**
 * @file nvs_storage.c
 * @brief NVS驱动
 * @version 1.0
 * @date 2023-12-24
 *
 * @copyright Copyright (c) 2023  雅马哈发动机（厦门）信息系统有限公司
 *
 */
#include "nvs_storage.h"
#include "common.h"

static const char *TAG = "NVS";

/**
 * @brief 初始化NVS存储
 * @return nvs_handle_t  nvs操作句柄
 */
nvs_handle_t nvsInit(void)
{
    nvs_handle _nvsHandle;
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &_nvsHandle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to open NVS handle, error=0x%x: %s", (int)err, esp_err_to_name(err));
        nvs_close(_nvsHandle);
        return 0;
    }
    else
    {
        // ESP_LOGI(TAG, "Success to init NVS.");
        return _nvsHandle;
    }
}

/**
 * @brief  从NVS中读取存储配置
 * @param  nvsData  nvsData结构体指针
 * @return esp_err_t
 */
esp_err_t readConfigFromNvs(NvsData_t *nvsData)
{
    size_t requiredSize;
    nvs_handle nvsHandle;
    esp_err_t err;
    char *buf;
    int ret;

    nvsHandle = nvsInit();
    err = nvs_get_str(nvsHandle, NVS_KEY_CONFIG_DATA, NULL, &requiredSize);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to read config info from NVS, error=0x%x: %s", (int)err, esp_err_to_name(err));
        nvs_close(nvsHandle);
        return err;
    }
    buf = malloc(requiredSize);
    err = nvs_get_str(nvsHandle, NVS_KEY_CONFIG_DATA, buf, &requiredSize);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to read config from NVS, error=0x%x: %s", (int)err, esp_err_to_name(err));
        nvs_close(nvsHandle);
        return err;
    }
    // 获取到的字符串为空
    if (!strlen(buf))
    {
        free(buf);
        buf = NULL;
        ESP_LOGE(TAG, "Config data is empty.");
        nvs_close(nvsHandle);
        return ESP_FAIL;
    }
    ret = cjsonx_str2struct(buf, nvsData, NvsData_reflection);
    free(buf);
    buf = NULL;
    if (ret != ERR_CJSONX_NONE)
    {
        ESP_LOGE(TAG, "Failed to convert JSON to Config.");
        nvs_close(nvsHandle);
        return ESP_FAIL;
    }
    // dumpNvsData(TAG, *nvsData);
    if ((strlen(nvsData->checksum) == 0) || (strcmp(nvsData->checksum, INVALID_CHECKSUM) == 0))
    {
        ESP_LOGE(TAG, "Config data is invalid.");
        nvs_close(nvsHandle);
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Success to read config from NVS.");
    nvs_close(nvsHandle);
    return ESP_OK;
}

/**
 * @brief  保存配置到NVS
 * @param  nvsData  nvsData结构体指针
 * @return esp_err_t
 */
esp_err_t saveConfigToNvs(NvsData_t *nvsData)
{
    esp_err_t err;
    char buf[MAX_CONFIG_LEN] = {0};
    nvs_handle nvsHandle;
    int ret;

    nvsHandle = nvsInit();
    ret = cjsonx_struct2str_preallocated(buf, sizeof(buf), nvsData, NvsData_reflection);
    if (ret != ERR_CJSONX_NONE)
    {
        ESP_LOGE(TAG, "Failed to convert Config to JSON.");
        nvs_close(nvsHandle);
        return ESP_FAIL;
    }
    // dumpNvsData(TAG, *nvsData);
    ESP_LOGI(TAG, "Saving config: %s", buf);
    err = nvs_set_str(nvsHandle, NVS_KEY_CONFIG_DATA, buf);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to save config to NVS, error=0x%x: %s", (int)err, esp_err_to_name(err));
        nvs_close(nvsHandle);
        return err;
    }
    ESP_LOGI(TAG, "Success to save config to NVS.");
    nvs_close(nvsHandle);
    return ESP_OK;
}

/**
 * @brief 读取nvs持久化配置信息
 * @param  nvsHandle
 * @param  nvsData
 * @return esp_err_t
 */
esp_err_t readNvsDataConfig(NvsData_t *nvsData)
{
    esp_err_t err;
    size_t requiredSize;
    nvs_handle nvsHandle;
    bool checksumNotFind = false;
    bool versionNotFind = false;
    char checksum[MAX_HASH_BUF_LEN] = {0};
    char version[MAX_VERSION_BUF_LEN] = {0};

    nvsHandle = nvsInit();
    ESP_ERROR_CHECK(nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvsHandle));
    // Checking Checksum
    err = nvs_get_str(nvsHandle, NVS_KEY_ELF_SHA256_VAULE, checksum, &requiredSize);
    if (err == ESP_ERR_NVS_NOT_FOUND) // 未曾存储过配置 第一次出厂烧录
    {
        ESP_LOGW(TAG, "NVS App checksum does not exist.");
        checksumNotFind = true;
    }
    else if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to read App Checksum from NVS, error=0x%x: %s", (int)err, esp_err_to_name(err));
        nvs_close(nvsHandle);
        return ESP_FAIL;
    }

    // Checking appVersion
    err = nvs_get_str(nvsHandle, NVS_KEY_APP_VERSION, version, &requiredSize);
    if (err == ESP_ERR_NVS_NOT_FOUND) // 未曾存储过配置 第一次出厂烧录
    {
        ESP_LOGW(TAG, "NVS App version does not exist.");
        versionNotFind = true;
    }
    else if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to read App version from NVS, error=0x%x: %s", (int)err, esp_err_to_name(err));
        nvs_close(nvsHandle);
        return ESP_FAIL;
    }

    if (checksumNotFind || versionNotFind) // 第一次出厂烧录，使用默认配置
    {
        ESP_LOGW(TAG, "NVS first factory burn, using default configuration");
        esp_app_get_elf_sha256(g_defaultNvsData.checksum, sizeof(g_defaultNvsData.checksum));
        ESP_ERROR_CHECK(nvs_set_str(nvsHandle, NVS_KEY_APP_VERSION, g_defaultNvsData.version));       // 覆写现在的version
        ESP_ERROR_CHECK(nvs_set_str(nvsHandle, NVS_KEY_ELF_SHA256_VAULE, g_defaultNvsData.checksum)); // 覆写现在的checksum
        // 读取默认配置 并写入nvs中持久化存储
        *nvsData = g_defaultNvsData;
        ESP_ERROR_CHECK(saveConfigToNvs(nvsData));
        nvs_close(nvsHandle);
        return ESP_OK;
    }
    else // 非第一次烧录，以及存在配置数据
    {
        ESP_LOGI(TAG, "Read config data from NVS.");
        err = readConfigFromNvs(nvsData);
        if (err != ESP_OK)
        {
            nvs_close(nvsHandle);
            return ESP_FAIL;
        }
        esp_app_get_elf_sha256(g_defaultNvsData.checksum, sizeof(g_defaultNvsData.checksum));
        if (strcmp(g_defaultNvsData.checksum, nvsData->checksum) != 0 || strcmp(g_defaultNvsData.version, nvsData->version) != 0) // NVS FLASH被外部覆写过
        {
            ESP_LOGW(TAG, "NVS App checksum: %s, Now App checksum: %s", nvsData->checksum, g_defaultNvsData.checksum);
            ESP_LOGW(TAG, "NVS App version: %s, Now App version: %s", nvsData->version, g_defaultNvsData.version);
            ESP_LOGW(TAG, "The firmware has been updated.");
            ESP_ERROR_CHECK(nvs_set_str(nvsHandle, NVS_KEY_APP_VERSION, g_defaultNvsData.version));       // 覆写现在的version
            ESP_ERROR_CHECK(nvs_set_str(nvsHandle, NVS_KEY_ELF_SHA256_VAULE, g_defaultNvsData.checksum)); // 覆写现在的checksum
            if (NETWORK_NVS_DATA_OVERWRITE)
            {
                ESP_LOGW(TAG, "Rewrite network configuration.");
                nvsData->networkConfigData = g_defaultNvsData.networkConfigData;
            }
            if (DEVICE_NVS_DATA_OVERWRITE)
            {
                ESP_LOGW(TAG, "Rewrite Device configuration.");
                nvsData->DeviceConfigData = g_defaultNvsData.DeviceConfigData;
            }
            if (PROJECT_NVS_DATA_OVERWRITE)
            {
                ESP_LOGW(TAG, "Rewrite project configuration.");
                nvsData->projectConfigData = g_defaultNvsData.projectConfigData;
            }
            strcpy(nvsData->checksum, g_defaultNvsData.checksum);
            strcpy(nvsData->version, g_defaultNvsData.version);
            ESP_LOGW(TAG, "NVS data update successful.");
            ESP_ERROR_CHECK(saveConfigToNvs(nvsData));
            nvs_close(nvsHandle);
            return ESP_OK;
        }
        else
        {
            ESP_LOGI(TAG, "Read successful.");
            nvs_close(nvsHandle);
            return ESP_OK;
        }
    }
    nvs_close(nvsHandle);
    return ESP_FAIL;
}
