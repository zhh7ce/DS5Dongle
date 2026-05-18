//
// USB Client (Vendor Class) Implementation
// Direct mode with BUFSIZE=0, using queue for buffering
//

#include "tusb.h"
#include "usb_client.h"
#include <stdio.h>
#include <string.h>

#if ENABLE_USB_CLIENT

#include "bt.h"
#include "state_mgr.h"
#include "pico/cyw43_arch.h"
#include "pico/util/queue.h"

#define REPORT_SIZE       398
#define REPORT_ID         0x36
#define SAMPLE_SIZE       64
#define PACKET_SIZE       264
#define USB_RX_QUEUE_SIZE 8  // Queue depth, can hold 8 packets of 264 bytes

static uint8_t reportSeqCounter = 0;
static uint8_t packetCounter = 0;

// USB receive data queue element
struct usb_rx_element {
    uint8_t data[PACKET_SIZE];  // Always 264 bytes
};

static usb_rx_element rx_element;
static size_t rx_offset = 0;

static queue_t usb_rx_queue;

// Initialize USB Client
void usb_client_init(void) {
    queue_init(&usb_rx_queue, sizeof(usb_rx_element), USB_RX_QUEUE_SIZE);
    printf("[USB_CLIENT] Initialized with queue depth %d\n", USB_RX_QUEUE_SIZE);
}

// Callback invoked when data is received from host
// In direct mode (BUFSIZE=0), this is called immediately when data arrives
void tud_vendor_rx_cb(uint8_t itf, uint8_t const* buffer, uint32_t bufsize) {
    (void) itf;

    if (rx_offset + bufsize >= PACKET_SIZE) {
        size_t copy_size = PACKET_SIZE - rx_offset;
        memcpy(rx_element.data + rx_offset, buffer, copy_size);

        if (!queue_try_add(&usb_rx_queue, &rx_element)) {
            // Queue full, drop oldest data
            usb_rx_element old_element;
            queue_try_remove(&usb_rx_queue, &old_element);
            printf("[USB_CLIENT] Queue full, dropped old packet\n");
            queue_try_add(&usb_rx_queue, &rx_element);
        }

        rx_offset = bufsize - copy_size;
        memcpy(rx_element.data, buffer + copy_size, rx_offset);

    } else {
        memcpy(rx_element.data + rx_offset, buffer, bufsize);
        rx_offset += bufsize;
    }
}

// Process queued data and send to Bluetooth
void usb_client_process_queue(void) {
    usb_rx_element element;
    
    while (queue_try_remove(&usb_rx_queue, &element)) {
        uint8_t pkt[REPORT_SIZE]{};
        pkt[0] = REPORT_ID;
        pkt[1] = reportSeqCounter << 4;
        reportSeqCounter = (reportSeqCounter + 1) & 0x0F;
        pkt[2] = 0x11 | 0 << 6 | 1 << 7;
        pkt[3] = 7;
        pkt[4] = 0b11111110;
        const auto buf_len = SAMPLE_SIZE;
        pkt[5] = buf_len;
        pkt[6] = buf_len;
        pkt[7] = buf_len;
        pkt[8] = buf_len; // 这 4 个字节的作用未知，调整没有效果
        pkt[9] = buf_len; // audio buffer length 只有调整这个字节生效。
        pkt[10] = packetCounter++;
        // SetStateData
        pkt[11] = 0x10 | 0 << 6 | 1 << 7;
        pkt[12] = 63;
        state_set(pkt + 13,63);
        // Haptics Audio Data
        pkt[76] = 0x12 | 0 << 6 | 1 << 7;
        pkt[77] = SAMPLE_SIZE;
        memcpy(pkt + 78, element.data, SAMPLE_SIZE);

        // Speaker Audio Data
        extern bool plug_headset;
        pkt[142] = (plug_headset ? 0x16 : 0x13) | 0 << 6 | 1 << 7; // Speaker: 0x13
        // L Headset Mono: 0x14
        // L Headset R Speaker: 0x15
        // Headset: 0x16
        pkt[143] = 200;
        memcpy(pkt + 144, element.data + SAMPLE_SIZE, 200);

        bt_write(pkt, sizeof(pkt));
    }
}

// Callback invoked when data transmission is complete
void tud_vendor_tx_complete_cb(uint8_t itf, uint32_t sent_bytes) {
    (void) itf;
    (void) sent_bytes;
}

uint32_t usb_client_send(const uint8_t* data, uint32_t len) {
    if (!tud_vendor_mounted()) {
        return 0;
    }
    
    // In direct mode (BUFSIZE=0), tud_vendor_write() sends data directly
    // No need to call flush
    return tud_vendor_write(data, len);
}

bool usb_client_mounted(void) {
    return tud_vendor_mounted();
}

#else

// Stub implementations when USB_CLIENT is disabled
void usb_client_init(void) {
    // Do nothing
}

uint32_t usb_client_send(const uint8_t* data, uint32_t len) {
    (void) data;
    (void) len;
    return 0;
}

bool usb_client_mounted(void) {
    return false;
}

#endif // ENABLE_USB_CLIENT
