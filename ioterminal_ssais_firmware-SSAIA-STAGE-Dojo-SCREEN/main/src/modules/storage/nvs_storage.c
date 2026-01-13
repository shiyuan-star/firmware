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
    nvs_handle nvsHandle;
    bool checksumNotFind = false;
    bool versionNotFind = false;
    char checksum[MAX_HASH_BUF_LEN] = {0};
    char version[MAX_VERSION_BUF_LEN] = {0};

    nvsHandle = nvsInit();
    ESP_ERROR_CHECK(nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvsHandle));
    // Checking Checksum
    size_t checksumRequiredSize = MAX_HASH_BUF_LEN;
    err = nvs_get_str(nvsHandle, NVS_KEY_ELF_SHA256_VAULE, checksum, &checksumRequiredSize);
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
    size_t versionRequiredSize = MAX_VERSION_BUF_LEN;
    err = nvs_get_str(nvsHandle, NVS_KEY_APP_VERSION, version, &versionRequiredSize);
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

/**
 * @brief  从NVS中读取物料盒配置
 * @return char*  重要：必须使用heap_caps_free释放内存
 */
char *readBoxParamFromNvs()
{
    esp_err_t err;
    nvs_handle nvsHandle;
    // 从PSRAM动态分配内存
    char *storedBoxesStr = (char *)heap_caps_malloc(MAX_BOX_PARAM_LEN, MALLOC_CAP_SPIRAM);
    if (storedBoxesStr == NULL)
    {
        ESP_LOGE(TAG, "Failed to allocate PSRAM for storedBoxesStr");
        return NULL;
    }
    memset(storedBoxesStr, 0, MAX_BOX_PARAM_LEN);

    nvsHandle = nvsInit();
    size_t boxParamRequiredSize = MAX_BOX_PARAM_LEN;
    err = nvs_get_blob(nvsHandle, NVS_BOX_PARAM_DATA, storedBoxesStr, &boxParamRequiredSize);
    if (err == ESP_ERR_NVS_NOT_FOUND) // 未曾存储过库位配置 第一次出厂烧录
    {
        ESP_LOGW(TAG, "NVS box param does not exist.");
        nvs_close(nvsHandle);
        heap_caps_free(storedBoxesStr);
        return NULL;
    }
    else if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to read box param from NVS, error=0x%x: %s", (int)err, esp_err_to_name(err));
        nvs_close(nvsHandle);
        heap_caps_free(storedBoxesStr);
        return NULL;
    }

    nvs_close(nvsHandle);

    // 为返回的字符串分配新的PSRAM内存
    size_t strLen = strlen(storedBoxesStr) + 1; // +1 for null terminator
    char *returnStr = (char *)heap_caps_malloc(strLen, MALLOC_CAP_SPIRAM);
    if (returnStr == NULL)
    {
        ESP_LOGE(TAG, "Failed to allocate PSRAM for return string");
        heap_caps_free(storedBoxesStr);
        return NULL;
    }

    // 复制数据到新的内存空间
    strcpy(returnStr, storedBoxesStr);

    // 释放临时缓冲区
    heap_caps_free(storedBoxesStr);

    return returnStr; // 调用者负责释放这个内存
}

/**
 * @brief  通过屏幕修改库位配置并存储到NVS 库位数据来自g_drawBoxParam全局变量
 *          一次NVS读写只能操作一个库位
 * @return esp_err_t
 */
esp_err_t screenModifyBoxInfoSaveToNvs()
{

    esp_err_t err;

    /* 组装新增的库位JSON */
    cJSON *boxParamDataArray = NULL; // 新增库位数据数组
    boxParamDataArray = cJSON_CreateArray();
    cJSON_AddItemToArray(boxParamDataArray, cJSON_CreateNumber(g_drawBoxParam.minX));
    cJSON_AddItemToArray(boxParamDataArray, cJSON_CreateNumber(g_drawBoxParam.maxX));
    cJSON_AddItemToArray(boxParamDataArray, cJSON_CreateNumber(g_drawBoxParam.minY));
    cJSON_AddItemToArray(boxParamDataArray, cJSON_CreateNumber(g_drawBoxParam.maxY));
    cJSON_AddItemToArray(boxParamDataArray, cJSON_CreateNumber(g_drawBoxParam.beginLed));
    cJSON_AddItemToArray(boxParamDataArray, cJSON_CreateNumber(g_drawBoxParam.endLed));

    /* 读取现有的库位存储JSON 并融入新增的JSON数组*/

    char *storedBoxesStr = (char *)heap_caps_malloc(MAX_BOX_PARAM_LEN, MALLOC_CAP_SPIRAM); // NVS中已经存储的库位JSON字符串
    if (storedBoxesStr == NULL)
    {
        ESP_LOGE(TAG, "Failed to allocate PSRAM for storedBoxesStr");
        return ESP_ERR_NO_MEM;
    }
    memset(storedBoxesStr, 0, MAX_BOX_PARAM_LEN);

    nvs_handle nvsHandle;
    nvsHandle = nvsInit();
    size_t boxParamRequiredSize = MAX_BOX_PARAM_LEN;
    err = nvs_get_blob(nvsHandle, NVS_BOX_PARAM_DATA, storedBoxesStr, &boxParamRequiredSize);
    if (err == ESP_ERR_NVS_NOT_FOUND) // 未曾存储过库位配置 第一次出厂烧录
    {
        ESP_LOGW(TAG, "NVS box param does not exist.");
        cJSON *newBoxParamJson = NULL; // 新增的库位名称JSON
        newBoxParamJson = cJSON_CreateObject();
        setTextValueMultilingual(SCREEN_MESSAGE_DIALOG_PAGE, SCREEN_MESSAGE_DAILOG_INFO_TEXT, "存储首个库位配置", "", "");
        memset(storedBoxesStr, 0, MAX_BOX_PARAM_LEN);
        cJSON_AddItemToObject(newBoxParamJson, g_drawBoxParam.boxName, boxParamDataArray); // 组装成为 {"xxx":[32767,0,32767,0,1,2]}} 格式
        char *newBoxParamStr = cJSON_PrintUnformatted(newBoxParamJson);
        strcpy(storedBoxesStr, newBoxParamStr); // 直接存储新增的库位JSON
        cJSON_free(newBoxParamStr);
        cJSON_Delete(newBoxParamJson);
        newBoxParamJson = NULL;
    }
    else if (err == ESP_OK) // 存储过库位配置，增加当前库位
    {
        ESP_LOGW(TAG, "NVS box param exist.");
        cJSON *storedBoxParamJson = cJSON_Parse(storedBoxesStr);                     // 解析现有的库位JSON
        if (cJSON_GetObjectItem(storedBoxParamJson, g_drawBoxParam.boxName) != NULL) // 查看要新增的库位是否已经存在，不为空则存在
        {
            cJSON_ReplaceItemInObject(storedBoxParamJson, g_drawBoxParam.boxName, cJSON_Duplicate(boxParamDataArray, cJSON_True)); // 替换
            setTextValueMultilingual(SCREEN_MESSAGE_DIALOG_PAGE, SCREEN_MESSAGE_DAILOG_INFO_TEXT, "库位已存在，更改配置成功", "The box already exists, the configuration change was successful", "ボックスは既に存在し、構成変更に成功しました");
        }
        else
        {
            cJSON_AddItemToObject(storedBoxParamJson, g_drawBoxParam.boxName, cJSON_Duplicate(boxParamDataArray, cJSON_True));
            setTextValueMultilingual(SCREEN_MESSAGE_DIALOG_PAGE, SCREEN_MESSAGE_DAILOG_INFO_TEXT, "添加新库位配置成功", "Successfully added new box configuration", "新しいボックス構成の追加に成功しました");
        }
        memset(storedBoxesStr, 0, MAX_BOX_PARAM_LEN);
        char *storedBoxesStrTemp = cJSON_PrintUnformatted(storedBoxParamJson);
        strcpy(storedBoxesStr, storedBoxesStrTemp);
        cJSON_free(storedBoxesStrTemp);
        cJSON_Delete(boxParamDataArray);
        boxParamDataArray = NULL;
        cJSON_Delete(storedBoxParamJson);
        storedBoxParamJson = NULL;
    }
    else
    {
        ESP_LOGE(TAG, "Failed to read box param from NVS, error=0x%x: %s", (int)err, esp_err_to_name(err));
        nvs_close(nvsHandle);
        cJSON_Delete(boxParamDataArray);
        boxParamDataArray = NULL;
        heap_caps_free(storedBoxesStr);
        return ESP_FAIL;
    }

    size_t len = strlen(storedBoxesStr) + 1; // 包括结束符
    err = nvs_set_blob(nvsHandle, NVS_BOX_PARAM_DATA, storedBoxesStr, len);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to save config to NVS, error=0x%x: %s", (int)err, esp_err_to_name(err));
        nvs_close(nvsHandle);
        cJSON_Delete(boxParamDataArray);
        boxParamDataArray = NULL;
        heap_caps_free(storedBoxesStr);
        return err;
    }
    ESP_ERROR_CHECK(nvs_commit(nvsHandle));
    ESP_LOGI(TAG, "Success to save box param to NVS.");
    ESP_LOGI(TAG, "All BoxParam: %s", storedBoxesStr);
    nvs_close(nvsHandle);
    heap_caps_free(storedBoxesStr);
    return ESP_OK;
}

/**
 * @brief  通过MQTT修改库位配置并存储到NVS 库位数据来自MQTT命令的data数组
 *          一次NVS读写读写多个库位
 * @return esp_err_t
 */
esp_err_t mqttModifyBoxInfoSaveToNvs(cJSON *data)
{

    esp_err_t err;
    nvs_handle nvsHandle;
    nvsHandle = nvsInit();
    size_t boxParamRequiredSize = MAX_BOX_PARAM_LEN;
    bool isNvsBoxParamExist = pdTRUE; // 假设NVS已经存在库位的JSON数据

    /* 读取现有的库位存储JSON 并融入新增的JSON数组*/
    char *storedBoxesStr = (char *)heap_caps_malloc(MAX_BOX_PARAM_LEN, MALLOC_CAP_SPIRAM); // NVS中已经存储的库位JSON字符串
    if (storedBoxesStr == NULL)
    {
        ESP_LOGE(TAG, "Failed to allocate PSRAM for storedBoxesStr");
        return ESP_ERR_NO_MEM;
    }
    memset(storedBoxesStr, 0, MAX_BOX_PARAM_LEN);

    err = nvs_get_blob(nvsHandle, NVS_BOX_PARAM_DATA, storedBoxesStr, &boxParamRequiredSize);
    if (err == ESP_ERR_NVS_NOT_FOUND) // 未曾存储过库位配置 第一次出厂烧录
    {
        ESP_LOGW(TAG, "NVS box param does not exist.");
        isNvsBoxParamExist = pdFALSE;
    }

    cJSON *item = NULL;
    if (isNvsBoxParamExist) // 如果NVS已经存在库位存储，先解析当前JSON
    {
        cJSON *storedBoxParamJson = cJSON_Parse(storedBoxesStr); // 解析现有的库位JSON
        cJSON_ArrayForEach(item, data)
        {
            if (cJSON_GetObjectItem(storedBoxParamJson, item->string) != NULL) // 查看要新增的库位是否已经存在，不为空则存在
            {
                cJSON_ReplaceItemInObject(storedBoxParamJson, item->string, cJSON_Duplicate(item, cJSON_True)); // 替换
                ESP_LOGI(TAG, "Box %s already exists, replace it.", item->string);
            }
            else
            {
                cJSON_AddItemToObject(storedBoxParamJson, item->string, cJSON_Duplicate(item, cJSON_True));
                ESP_LOGI(TAG, "Box %s not exists, add it.", item->string);
            }
        }
        char *storedBoxesStrTemp = cJSON_PrintUnformatted(storedBoxParamJson);
        strcpy(storedBoxesStr, storedBoxesStrTemp);
        cJSON_free(storedBoxesStrTemp);
        cJSON_Delete(storedBoxParamJson);
        storedBoxParamJson = NULL;
    }
    else
    {
        cJSON *newBoxParamJson = cJSON_CreateObject();
        cJSON_ArrayForEach(item, data)
        {
            cJSON_AddItemToObject(newBoxParamJson, item->string, cJSON_Duplicate(item, cJSON_True));
            ESP_LOGI(TAG, "Box %s not exists, add it.", item->string);
        }
        char *storedBoxesStrTemp = cJSON_PrintUnformatted(newBoxParamJson);
        strcpy(storedBoxesStr, storedBoxesStrTemp);
        cJSON_free(storedBoxesStrTemp);
        cJSON_Delete(newBoxParamJson);
        newBoxParamJson = NULL;
    }
    size_t len = strlen(storedBoxesStr) + 1; // 包括结束符
    err = nvs_set_blob(nvsHandle, NVS_BOX_PARAM_DATA, storedBoxesStr, len);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to save config to NVS, error=0x%x: %s", (int)err, esp_err_to_name(err));
        nvs_close(nvsHandle);
        heap_caps_free(storedBoxesStr);
        return err;
    }
    ESP_ERROR_CHECK(nvs_commit(nvsHandle));
    ESP_LOGI(TAG, "Success to save box param to NVS.");
    ESP_LOGI(TAG, "All BoxParam: %s", storedBoxesStr);
    nvs_close(nvsHandle);
    heap_caps_free(storedBoxesStr);
    return ESP_OK;
}

/**
 * @brief  删除所有物料盒配置
 * @return esp_err_t
 */
esp_err_t deleteAllBoxParamFromNvs()
{
    esp_err_t err;
    nvs_handle nvsHandle;

    nvsHandle = nvsInit();
    err = nvs_erase_key(nvsHandle, NVS_BOX_PARAM_DATA);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
    {
        ESP_LOGE(TAG, "Failed to delete all box param from NVS, error=0x%x: %s", (int)err, esp_err_to_name(err));
        nvs_close(nvsHandle);
        return err;
    }
    ESP_ERROR_CHECK(nvs_commit(nvsHandle));
    ESP_LOGI(TAG, "Success to delete all box param from NVS.");
    nvs_close(nvsHandle);
    return ESP_OK;
}

/**
 * @brief  通过MQTT删除多个库位配置并存储到NVS 库位数据来自MQTT命令的data数组
 * @return esp_err_t
 */
esp_err_t mqttDeleteBoxInfoSaveToNvs(cJSON *data)
{

    cJSON *_boxListJson = getJSONobj(data, "box_list"); // 获取要删除的库位列表 !不需要调用cJSON_Delete，本函数返回后会被上一级删除
    if (_boxListJson == NULL)
    {
        return ESP_FAIL;
    }
    // 获取库位物料盒LIST长度
    uint16_t _boxListSize = cJSON_GetArraySize(_boxListJson);
    if (_boxListSize == 0) // 无物料盒，返回错误
    {
        ESP_LOGE(TAG, "No box list provided for deletion.");
        return ESP_FAIL;
    }

    esp_err_t err;
    nvs_handle nvsHandle;
    nvsHandle = nvsInit();
    size_t boxParamRequiredSize = MAX_BOX_PARAM_LEN;

    /* 读取现有的库位存储JSON 并融入新增的JSON数组*/
    char *storedBoxesStr = (char *)heap_caps_malloc(MAX_BOX_PARAM_LEN, MALLOC_CAP_SPIRAM); // NVS中已经存储的库位JSON字符串
    if (storedBoxesStr == NULL)
    {
        ESP_LOGE(TAG, "Failed to allocate PSRAM for storedBoxesStr");
        return ESP_ERR_NO_MEM;
    }
    memset(storedBoxesStr, 0, MAX_BOX_PARAM_LEN);

    err = nvs_get_blob(nvsHandle, NVS_BOX_PARAM_DATA, storedBoxesStr, &boxParamRequiredSize);
    if (err == ESP_ERR_NVS_NOT_FOUND) // 未曾存储过库位配置 第一次出厂烧录
    {
        ESP_LOGE(TAG, "NVS box param does not exist, Delete failed.");
        nvs_close(nvsHandle);
        heap_caps_free(storedBoxesStr);
        return ESP_ERR_NOT_FOUND;
    }

    cJSON *storedBoxParamJson = cJSON_Parse(storedBoxesStr); // 解析现有的库位JSON
    char *_boxName = NULL;
    for (size_t i = 0; i < _boxListSize; i++)
    {
        _boxName = cJSON_GetStringValue(cJSON_GetArrayItem(_boxListJson, i));
        cJSON *existBoxParamDataArray = cJSON_GetObjectItem(storedBoxParamJson, _boxName); // 从JSON中获取库位信息
        if (existBoxParamDataArray == NULL)                                                // 要删除的库位不存在
        {
            ESP_LOGE(TAG, "mqttDeleteBoxInfoSaveToNvs: Failed to read Box Name: %s param from NVS.", _boxName);
            cJSON_Delete(storedBoxParamJson);
            nvs_close(nvsHandle);
            heap_caps_free(storedBoxesStr);
            return ESP_ERR_NOT_FOUND;
        }
        else
        {
            cJSON_DeleteItemFromObject(storedBoxParamJson, _boxName); // 删除库位
            ESP_LOGI(TAG, "mqttDeleteBoxInfoSaveToNvs: Delete Box Name: %s param from NVS.", _boxName);
        }
    }
    uint16_t _afterDeletionBoxListSize = cJSON_GetArraySize(storedBoxParamJson); // 删除后的库位长度
    if (_afterDeletionBoxListSize == 0)                                          // 删除后库位长度为0 清除所有库位存储
    {
        err = nvs_erase_key(nvsHandle, NVS_BOX_PARAM_DATA);
        if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
        {
            ESP_LOGE(TAG, "Failed to delete all box param from NVS, error=0x%x: %s", (int)err, esp_err_to_name(err));
            cJSON_Delete(storedBoxParamJson);
            heap_caps_free(storedBoxesStr);
            nvs_close(nvsHandle);
            return err;
        }
        ESP_LOGW(TAG, "mqttDeleteBoxInfoSaveToNvs: All box param deleted.");
    }
    else
    {
        char *storedBoxesStrTemp = cJSON_PrintUnformatted(storedBoxParamJson);
        strcpy(storedBoxesStr, storedBoxesStrTemp);
        storedBoxParamJson = NULL;
        size_t len = strlen(storedBoxesStr) + 1; // 包括结束符
        err = nvs_set_blob(nvsHandle, NVS_BOX_PARAM_DATA, storedBoxesStr, len);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to save config to NVS, error=0x%x: %s", (int)err, esp_err_to_name(err));
            cJSON_free(storedBoxesStrTemp);
            cJSON_Delete(storedBoxParamJson);
            heap_caps_free(storedBoxesStr);
            nvs_close(nvsHandle);
            return err;
        }
    }
    ESP_ERROR_CHECK(nvs_commit(nvsHandle));
    ESP_LOGI(TAG, "Success to Delete box param from NVS.");
    ESP_LOGI(TAG, "All BoxParam: %s", storedBoxesStr);
    heap_caps_free(storedBoxesStr);
    cJSON_Delete(storedBoxParamJson);
    nvs_close(nvsHandle);
    return ESP_OK;
}
