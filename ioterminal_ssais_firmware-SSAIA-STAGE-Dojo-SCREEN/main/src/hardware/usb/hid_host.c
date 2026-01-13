/**
 * @file hid_host.c
 * @brief cherry_USB 的HID设备回调函数实现
 * @version 1.0
 *
 * @copyright Copyright (c) 2025  雅马哈发动机（厦门）信息系统有限公司
 *
 */

#include "common.h"

static char *TAG = "HID_HOST";

// HID报文数据
USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t hid_buffer[HID_MAX_BUFFER];

// 扫码枪输入队列
QueueHandle_t g_ScannerInputQueueHandler; // 扫码枪输入队列

// 扫码枪输入数据
static ScannerInputData_t s_sacnnerInputData = {0};

/**
 * @brief Key event
 */
typedef struct
{
    enum key_state
    {
        KEY_STATE_PRESSED = 0x00,
        KEY_STATE_RELEASED = 0x01
    } state;
    uint8_t modifier;
    uint8_t key_code;
} key_event_t;

/* Main char symbol for ENTER key */
#define KEYBOARD_ENTER_MAIN_CHAR '\r'

/**
 * @brief Scancode to ascii table
 */
const uint8_t keycode2ascii[57][2] = {
    {0, 0},                                               /* HID_KEY_NO_PRESS        */
    {0, 0},                                               /* HID_KEY_ROLLOVER        */
    {0, 0},                                               /* HID_KEY_POST_FAIL       */
    {0, 0},                                               /* HID_KBD_USAGE_ERRUNDEF */
    {'a', 'A'},                                           /* HID_KEY_A               */
    {'b', 'B'},                                           /* HID_KEY_B               */
    {'c', 'C'},                                           /* HID_KEY_C               */
    {'d', 'D'},                                           /* HID_KEY_D               */
    {'e', 'E'},                                           /* HID_KEY_E               */
    {'f', 'F'},                                           /* HID_KEY_F               */
    {'g', 'G'},                                           /* HID_KEY_G               */
    {'h', 'H'},                                           /* HID_KEY_H               */
    {'i', 'I'},                                           /* HID_KEY_I               */
    {'j', 'J'},                                           /* HID_KEY_J               */
    {'k', 'K'},                                           /* HID_KEY_K               */
    {'l', 'L'},                                           /* HID_KEY_L               */
    {'m', 'M'},                                           /* HID_KEY_M               */
    {'n', 'N'},                                           /* HID_KEY_N               */
    {'o', 'O'},                                           /* HID_KEY_O               */
    {'p', 'P'},                                           /* HID_KEY_P               */
    {'q', 'Q'},                                           /* HID_KEY_Q               */
    {'r', 'R'},                                           /* HID_KEY_R               */
    {'s', 'S'},                                           /* HID_KEY_S               */
    {'t', 'T'},                                           /* HID_KEY_T               */
    {'u', 'U'},                                           /* HID_KEY_U               */
    {'v', 'V'},                                           /* HID_KEY_V               */
    {'w', 'W'},                                           /* HID_KEY_W               */
    {'x', 'X'},                                           /* HID_KEY_X               */
    {'y', 'Y'},                                           /* HID_KEY_Y               */
    {'z', 'Z'},                                           /* HID_KEY_Z               */
    {'1', '!'},                                           /* HID_KEY_1               */
    {'2', '@'},                                           /* HID_KEY_2               */
    {'3', '#'},                                           /* HID_KEY_3               */
    {'4', '$'},                                           /* HID_KEY_4               */
    {'5', '%'},                                           /* HID_KEY_5               */
    {'6', '^'},                                           /* HID_KEY_6               */
    {'7', '&'},                                           /* HID_KEY_7               */
    {'8', '*'},                                           /* HID_KEY_8               */
    {'9', '('},                                           /* HID_KEY_9               */
    {'0', ')'},                                           /* HID_KEY_0               */
    {KEYBOARD_ENTER_MAIN_CHAR, KEYBOARD_ENTER_MAIN_CHAR}, /* HID_KEY_ENTER           */
    {0, 0},                                               /* HID_KEY_ESC             */
    {'\b', 0},                                            /* HID_KEY_DEL             */
    {0, 0},                                               /* HID_KEY_TAB             */
    {' ', ' '},                                           /* HID_KEY_SPACE           */
    {'-', '_'},                                           /* HID_KEY_MINUS           */
    {'=', '+'},                                           /* HID_KEY_EQUAL           */
    {'[', '{'},                                           /* HID_KEY_OPEN_BRACKET    */
    {']', '}'},                                           /* HID_KEY_CLOSE_BRACKET   */
    {'\\', '|'},                                          /* HID_KEY_BACK_SLASH      */
    {'\\', '|'},
    /* HID_KEY_SHARP           */ // HOTFIX: for NonUS Keyboards repeat HID_KEY_BACK_SLASH
    {';', ':'},                   /* HID_KEY_COLON           */
    {'\'', '"'},                  /* HID_KEY_QUOTE           */
    {'`', '~'},                   /* HID_KEY_TILDE           */
    {',', '<'},                   /* HID_KEY_LESS            */
    {'.', '>'},                   /* HID_KEY_GREATER         */
    {'/', '?'}                    /* HID_KBD_USAGE_QUESTION           */
};

/**
 * @brief HID Keyboard modifier verification for capitalization application (right or left shift)
 *
 * @param[in] modifier
 * @return true  Modifier was pressed (left or right shift)
 * @return false Modifier was not pressed (left or right shift)
 *
 */
static inline bool hid_keyboard_is_modifier_shift(uint8_t modifier)
{
    if (((modifier & HID_MODIFER_LSHIFT) == HID_MODIFER_LSHIFT) ||
        ((modifier & HID_MODIFER_RSHIFT) == HID_MODIFER_RSHIFT))
    {
        return true;
    }
    return false;
}

/**
 * @brief HID Keyboard get char symbol from key code
 *
 * @param[in] modifier  Keyboard modifier data
 * @param[in] key_code  Keyboard key code
 * @param[in] key_char  Pointer to key char data
 *
 * @return true  Key scancode converted successfully
 * @return false Key scancode unknown
 */
static inline bool hid_keyboard_get_char(uint8_t modifier,
                                         uint8_t key_code,
                                         unsigned char *key_char)
{
    uint8_t mod = (hid_keyboard_is_modifier_shift(modifier)) ? 1 : 0;

    if ((key_code >= HID_KBD_USAGE_A) && (key_code <= HID_KBD_USAGE_QUESTION))
    {
        *key_char = keycode2ascii[key_code][mod];
    }
    else
    {
        // All other key pressed
        return false;
    }

    return true;
}

/**
 * @brief HID Keyboard print char symbol
 *
 * @param[in] key_char  Keyboard char to stdout
 */
static inline void hid_keyboard_print_char(unsigned int key_char)
{
    if (!!key_char)
    {
        if (key_char == KEYBOARD_ENTER_MAIN_CHAR)
        {
            // 收到回车键，输出完整的扫码数据
            s_sacnnerInputData.data[s_sacnnerInputData.dataLen] = '\0';
            USB_LOG_RAW("Scanned Code: %s\r\n", s_sacnnerInputData.data);
            xQueueSendFromISR(g_ScannerInputQueueHandler,&s_sacnnerInputData,NULL);
            s_sacnnerInputData.dataLen = 0; // 重置缓冲区
        }
        else if (s_sacnnerInputData.dataLen < SCANNER_INPUT_DATA_MAX_SIZE - 1)
        {
            // 将字符存入缓冲区
            s_sacnnerInputData.data[s_sacnnerInputData.dataLen++] = key_char;
        }
    }
}

/**
 * @brief Key Event. Key event with the key code, state and modifier.
 *
 * @param[in] key_event Pointer to Key Event structure
 *
 */
static void key_event_callback(key_event_t *key_event)
{
    unsigned char key_char;

    if (KEY_STATE_PRESSED == key_event->state)
    {
        if (hid_keyboard_get_char(key_event->modifier,
                                  key_event->key_code, &key_char))
        {

            hid_keyboard_print_char(key_char);
        }
    }
}

/**
 * @brief Key buffer scan code search.
 *
 * @param[in] src       Pointer to source buffer where to search
 * @param[in] key       Key scancode to search
 * @param[in] length    Size of the source buffer
 */
static inline bool key_found(const uint8_t *const src,
                             uint8_t key,
                             unsigned int length)
{
    for (unsigned int i = 0; i < length; i++)
    {
        if (src[i] == key)
        {
            return true;
        }
    }
    return false;
}

/**
 * @brief  扫码枪回调
 * @param  arg
 * @param  nbytes
 */
static void usbh_hid_barcode_scanner_report_callback(void *arg, int nbytes)
{
    struct usb_hid_kbd_report *kb_report = (struct usb_hid_kbd_report *)hid_buffer;
    if (nbytes < sizeof(struct usb_hid_kbd_report))
    {
        return;
    }
    static uint8_t prev_keys[sizeof(kb_report->key)] = {0};
    key_event_t key_event;

    // 只处理键盘按键按下事件，忽略释放事件(防止扫码枪输入字符重复)
    for (int i = 0; i < sizeof(kb_report->key); i++)
    {
        // key has been pressed verification
        if (kb_report->key[i] > HID_KBD_USAGE_ERRUNDEF &&
            !key_found(prev_keys, kb_report->key[i], sizeof(prev_keys)))
        {
            key_event.key_code = kb_report->key[i];
            key_event.modifier = kb_report->modifier;
            key_event.state = KEY_STATE_PRESSED;
            key_event_callback(&key_event);
        }
    }

    memcpy(prev_keys, &kb_report->key, sizeof(kb_report->key));
}

/**
 * @brief  红外触摸框回调
 * @param  arg
 * @param  nbytes
 * @return * void
 */
static void usbh_hid_infrared_touch_report_callback(void *arg, int nbytes)
{
    infraredTouchReportHandel(hid_buffer);
}

/**
 * @brief  其他不认识的设备回调
 * @param  arg
 * @param  nbytes
 */
static void usbh_hid_generic_report_callback(void *arg, int nbytes)
{
    for (size_t i = 0; i < nbytes; i++)
    {
        USB_LOG_RAW("0x%02x ", hid_buffer[i]);
    }
    USB_LOG_RAW("nbytes:%d\r\n", nbytes);
}

static void usbh_hid_report_callback(void *arg, int nbytes)
{
    struct usbh_hid *hid_class = (struct usbh_hid *)arg;
    if (nbytes <= 0)
    {
        if (nbytes == -USB_ERR_NAK)
        {
            usbh_submit_urb(&hid_class->intin_urb);
        }
        return;
    }
    uint8_t sub_class = hid_class->hport->config.intf[hid_class->intf].altsetting[0].intf_desc.bInterfaceSubClass;
    uint8_t protocol = hid_class->hport->config.intf[hid_class->intf].altsetting[0].intf_desc.bInterfaceProtocol;

    usbh_complete_callback_t complete_cb = usbh_hid_generic_report_callback;
    if (sub_class == HID_SUBCLASS_BOOTIF && protocol == HID_PROTOCOL_KEYBOARD)
    {
        complete_cb = usbh_hid_barcode_scanner_report_callback;
    }
    if (sub_class == HID_SUBCLASS_NONE && nbytes == INFRARED_TOUCH_DATA_LENGTH)
    {
        complete_cb = usbh_hid_infrared_touch_report_callback;
    }
    complete_cb(arg, nbytes);
    usbh_submit_urb(&hid_class->intin_urb);
}

/**
 * @brief  USBH 初始化
 * @param  argument
 */
void usbh_hid_thread(void *argument)
{
    int ret;
    struct usbh_hid *hid_class = (struct usbh_hid *)argument;

    /* test with only one buffer, if you have more hid class, modify by yourself */

    /* Suggest you to use timer for int transfer and use ep interval */
    uint8_t sub_class = hid_class->hport->config.intf[hid_class->intf].altsetting[0].intf_desc.bInterfaceSubClass;
    uint8_t protocol = hid_class->hport->config.intf[hid_class->intf].altsetting[0].intf_desc.bInterfaceProtocol;
    ESP_LOGI(TAG, "intf %u, SubClass %u, Protocol %u", hid_class->intf, sub_class, protocol);
    if (sub_class == HID_SUBCLASS_BOOTIF)
    {
        int usbh_hid_set_protocol(struct usbh_hid * hid_class, uint8_t protocol);
        ret = usbh_hid_set_protocol(hid_class, HID_PROTOCOL_BOOT);
        if (ret < 0)
        {
            goto delete;
        }
    }
    if (hid_class->intf == 0 && sub_class == 0 && protocol == 0)
    {
        //  红外触摸框多点触摸配置报文 （wireshark抓包得到）
        uint8_t rep[3] = {0x04, 0x02, 0x00};
        usbh_hid_set_report(hid_class, HID_REPORT_FEATURE, 0x04, rep, 0x03);
    }

    if (hid_class->intin == NULL)
    {
        ESP_LOGW(TAG, "no intin ep desc");
        goto delete;
    }

    usbh_int_urb_fill(&hid_class->intin_urb, hid_class->hport, hid_class->intin, hid_buffer, hid_class->intin->wMaxPacketSize, 0, usbh_hid_report_callback, hid_class);
    ret = usbh_submit_urb(&hid_class->intin_urb);
    if (ret < 0)
    {
        goto delete;
    }
    // clang-format off
delete:
    usb_osal_thread_delete(NULL);
    // clang-format on
}
