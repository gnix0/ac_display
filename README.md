# AC Display

A dual-microcontroller real-time signal monitoring system with visualization. 
Built on **ESP-IDF**, the system features an **ESP32-C6 (Master)** sampling analog voltage signals via ADC and streaming waveforms through structured packets over an I2C bus to an **ESP32-S3 LilyGo AMOLED (Slave)**, which executes the real-time chart rendering via **LVGL**.

---

## System Architecture

### 1. Master Node (`/master`)
* Acts as the data acquisition unit.
* Runs a dedicated FreeRTOS task that captures input signals, packs them into a rigid structure, and transmits them over the I2C bus at 400 kbits/s.
* Controlled transmission intervals (80ms) to prevent bus flooding and respect slave processing latencies.

### 2. Slave Display Node (`/slave`)
* Acts as the dedicated human-machine interface (HMI).
* Incoming raw bytes are caught by an optimized ISR, validated for exact sizing, and pushed into a fixed-length FreeRTOS queue using an overwrite strategy to guarantee close to zero-latency freshness.
* **Display Engine:** Leverages LVGL 8 with PSRAM double-buffering driven via an AMOLED controller layer.

---

## Data Protocol & Packet Layout

To prevent bus corruption, partial-frame artifacts _("franken-integers")_, and graphical tearing, communication relies on a strictly bounded, packed binary structure:

```c
typedef struct {
    uint32_t voltages[64];
} __attribute__((packed)) display_packet_t;
```
* **Payload Size:** **256 bytes** (64 points $\times$ 4 bytes per unsigned 32-bit integer).
* **Validation:** The slave driver enforces a strict bounds check. Any malformed, aborted, or truncated transaction (e.g., due to SPI bus contention during high-load screen refreshes) is dropped instantly at the ISR/queue boundary.

---

## Concurrency & Thread Safety

The slave application employs a prevention to race conditions between the asynchronous I2C reception and the LVGL rendering loop:

* **Mutex Encapsulation ** A static mutex is encapsulated strictly inside the display component. 
* **Safe Updates:** the wave update function locks the mutex internally before updating chart point IDs, ensuring the LVGL timer handler never reads half-written or corrupted arrays.

---

## Hardware Pin Mapping & Configuration

| Interface / Bus | MCU Target | Signal Name | GPIO Pin | Configuration Notes |
| :--- | :--- | :--- | :--- | :--- |
| **I2C Master** | ESP32-C6 | SCL | GPIO 2 | Master clock output |
| **I2C Master** | ESP32-C6 | SDA | GPIO 3 | Master data line |
| **I2C Slave** | ESP32-S3 | SCL | GPIO 1 | Slave clock (with clock-stretch support) |
| **I2C Slave** | ESP32-S3 | SDA | GPIO 2 | Slave data line |
| **I2C Address** | ESP32-S3 | Device ID | `0x32` | 7-bit slave address |
