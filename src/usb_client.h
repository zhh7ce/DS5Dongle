//
// Created for DS5Dongle USB Client (Vendor Class)
//

#ifndef DS5_BRIDGE_USB_CLIENT_H
#define DS5_BRIDGE_USB_CLIENT_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize USB Client (Vendor Class) interface
 */
void usb_client_init(void);

/**
 * Process queued USB data and send to Bluetooth
 * Call this in main loop
 */
void usb_client_process_queue(void);

/**
 * Send data to USB host via Vendor interface
 * @param data Pointer to data buffer
 * @param len Length of data to send
 * @return Number of bytes sent, or 0 if failed
 */
uint32_t usb_client_send(const uint8_t* data, uint32_t len);

/**
 * Check if USB Client interface is mounted
 * @return true if mounted, false otherwise
 */
bool usb_client_mounted(void);

#ifdef __cplusplus
}
#endif

#endif //DS5_BRIDGE_USB_CLIENT_H
