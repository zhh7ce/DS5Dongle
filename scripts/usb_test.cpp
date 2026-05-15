// USB Client Test Program
// Sends sinusoidal waveform data to DS5Dongle Vendor interface

#include <libusb-1.0/libusb.h>
#include <iostream>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <thread>
#include <chrono>

#define VENDOR_ID       0x054C
#define PRODUCT_ID      0x0CE6
#define INTERFACE_NUM   4
#define PACKET_SIZE     64
#define PACKET_COUNT    1000

int main() {
    libusb_context* ctx = nullptr;
    libusb_device_handle* handle = nullptr;
    int rc;

    // Initialize libusb
    rc = libusb_init(&ctx);
    if (rc < 0) {
        std::cerr << "Failed to initialize libusb: " << libusb_error_name(rc) << std::endl;
        return 1;
    }

    // Open device
    handle = libusb_open_device_with_vid_pid(ctx, VENDOR_ID, PRODUCT_ID);
    if (!handle) {
        std::cerr << "Device not found (VID:0x" << std::hex << VENDOR_ID 
                  << ", PID:0x" << PRODUCT_ID << ")" << std::endl;
        libusb_exit(ctx);
        return 1;
    }

    std::cout << "Device opened successfully" << std::endl;

    // Claim interface
    rc = libusb_claim_interface(handle, INTERFACE_NUM);
    if (rc < 0) {
        std::cerr << "Failed to claim interface " << INTERFACE_NUM << ": " 
                  << libusb_error_name(rc) << std::endl;
        libusb_close(handle);
        libusb_exit(ctx);
        return 1;
    }

    std::cout << "Interface " << INTERFACE_NUM << " claimed" << std::endl;

    // Find OUT endpoint
    libusb_endpoint_descriptor ep_desc;
    bool found_out_ep = false;
    uint8_t out_ep_addr = 0;

    libusb_config_descriptor* config = nullptr;
    rc = libusb_get_active_config_descriptor(libusb_get_device(handle), &config);
    if (rc == 0) {
        for (int i = 0; i < config->bNumInterfaces; i++) {
            const libusb_interface& iface = config->interface[i];
            for (int j = 0; j < iface.num_altsetting; j++) {
                const libusb_interface_descriptor& altsetting = iface.altsetting[j];
                if (altsetting.bInterfaceNumber == INTERFACE_NUM) {
                    for (int k = 0; k < altsetting.bNumEndpoints; k++) {
                        const libusb_endpoint_descriptor& ep = altsetting.endpoint[k];
                        if ((ep.bEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK) == LIBUSB_ENDPOINT_OUT) {
                            out_ep_addr = ep.bEndpointAddress;
                            found_out_ep = true;
                            std::cout << "Found OUT endpoint: 0x" << std::hex 
                                      << (int)out_ep_addr << std::dec << std::endl;
                            break;
                        }
                    }
                    break;
                }
            }
            if (found_out_ep) break;
        }
        libusb_free_config_descriptor(config);
    }

    if (!found_out_ep) {
        std::cerr << "OUT endpoint not found on interface " << INTERFACE_NUM << std::endl;
        libusb_release_interface(handle, INTERFACE_NUM);
        libusb_close(handle);
        libusb_exit(ctx);
        return 1;
    }

    // Generate sinusoidal waveform data
    uint8_t data[PACKET_SIZE];
    for (int i = 0; i < PACKET_SIZE; i++) {
        // Sinusoidal wave: -127 -> 0 -> +127 -> 0 -> -127
        double value = 127 * sin(2.0 * M_PI * i / PACKET_SIZE);
        data[i] = static_cast<uint8_t>(static_cast<int>(value) & 0xFF);
    }

    std::cout << "Starting to send " << PACKET_COUNT << " packets (" 
              << PACKET_SIZE << " bytes each)..." << std::endl;

    // Send packets
    int success_count = 0;
    int fail_count = 0;
    auto start_time = std::chrono::steady_clock::now();

    for (int i = 0; i < PACKET_COUNT; i++) {
        int transferred = 0;
        rc = libusb_bulk_transfer(handle, out_ep_addr, data, PACKET_SIZE, 
                                  &transferred, 1000); // 1 second timeout

        if (rc == 0 && transferred == PACKET_SIZE) {
            success_count++;
        } else {
            fail_count++;
            std::cerr << "Error at packet " << i << ": " << libusb_error_name(rc) 
                      << " (transferred: " << transferred << ")" << std::endl;
            break;
        }

        // Progress report
        if (i % 100 == 0) {
            std::cout << "Sent " << i << " packets..." << std::endl;
        }

        // Small delay to avoid overwhelming the device
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    std::cout << "\nDone!" << std::endl;
    std::cout << "Success: " << success_count << " packets" << std::endl;
    std::cout << "Failed:  " << fail_count << " packets" << std::endl;
    std::cout << "Time:    " << duration.count() << " ms" << std::endl;
    if (success_count > 0) {
        std::cout << "Rate:    " << (success_count * 1000.0 / duration.count()) 
                  << " packets/sec" << std::endl;
    }

    // Cleanup
    libusb_release_interface(handle, INTERFACE_NUM);
    libusb_close(handle);
    libusb_exit(ctx);

    return 0;
}
