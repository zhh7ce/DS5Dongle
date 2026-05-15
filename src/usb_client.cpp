//
// USB Client (Vendor Class) Implementation
// Direct mode with BUFSIZE=0
//

#include "tusb.h"
#include "usb_client.h"
#include <stdio.h>
#include <string.h>

#if ENABLE_USB_CLIENT

#include "bt.h"

#define REPORT_SIZE       398
#define REPORT_ID         0x36
#define SAMPLE_SIZE       64
static uint8_t reportSeqCounter = 0;
static uint8_t packetCounter = 0;

// Callback invoked when data is received from host
// In direct mode (BUFSIZE=0), this is called immediately when data arrives
void tud_vendor_rx_cb(uint8_t itf, uint8_t const* buffer, uint32_t bufsize) {
    (void) itf;

    if (264 != bufsize && 64 != bufsize) {
        printf("[USB_CLIENT] Invalid size: %lu\n", bufsize);
        return;
    }

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
    pkt[11] = 0x12 | 0 << 6 | 1 << 7;
    pkt[12] = SAMPLE_SIZE;
    memcpy(pkt + 13, buffer, SAMPLE_SIZE);

#if !DISABLE_SPEAKER_PROC
    if (bufsize == 264) {
        extern bool plug_headset;
        pkt[77] = (plug_headset ? 0x16 : 0x13) | 0 << 6 | 1 << 7; // Speaker: 0x13
        // L Headset Mono: 0x14
        // L Headset R Speaker: 0x15
        // Headset: 0x16
        pkt[78] = 200;
        memcpy(pkt + 79, buffer+SAMPLE_SIZE, 200);
    }
#endif

    bt_write(pkt, sizeof(pkt), true);
}

// Callback invoked when data transmission is complete
void tud_vendor_tx_complete_cb(uint8_t itf, uint32_t sent_bytes) {
    (void) itf;
    (void) sent_bytes;
    
    // TODO: Handle transmit completion if needed
    // printf("[USB_CLIENT] Sent %lu bytes complete\n", sent_bytes);
}

void usb_client_init(void) {
    printf("[USB_CLIENT] Initialized\n");
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
