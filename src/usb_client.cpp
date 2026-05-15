//
// USB Client (Vendor Class) Implementation
// Direct mode with BUFSIZE=0
//

#include "tusb.h"
#include "usb_client.h"
#include <stdio.h>
#include <string.h>

#if ENABLE_USB_CLIENT

// Callback invoked when data is received from host
// In direct mode (BUFSIZE=0), this is called immediately when data arrives
void tud_vendor_rx_cb(uint8_t itf, uint8_t const* buffer, uint16_t bufsize) {
    (void) itf;
    
    // TODO: Process received data here
    // For now, just print the received data length
    printf("[USB_CLIENT] Received %u bytes\n", bufsize);
    
    // Example: Echo back the received data
    // if (tud_vendor_mounted()) {
    //     tud_vendor_write(buffer, bufsize);  // Direct mode: no flush needed
    // }
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
