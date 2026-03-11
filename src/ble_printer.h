#ifndef BLE_PRINTER_H
#define BLE_PRINTER_H

#include <cstdint>
#include <cstddef>

// Forward declarations (NimBLE types)
class NimBLEClient;
class NimBLERemoteCharacteristic;
class NimBLEAdvertisedDevice;

class BlePrinter {
public:
    enum class State : uint8_t {
        Idle,
        Scanning,
        Connecting,
        Ready,
        Printing,
        Error
    };

    void init();
    void update();
    bool print(const char* text, const char* title = "OTP Key");
    void scanAndConnect();
    void disconnect();

    State state() const { return m_state; }
    bool isReady() const { return m_state == State::Ready; }
    const char* statusText() const;

    // Called by scan callback
    void onDeviceFound(NimBLEAdvertisedDevice* device);
    void onScanDone();

private:
    bool connectToDevice();
    bool sendEscPos(const char* text, const char* title);
    bool sendChunk(const uint8_t* data, size_t len);

    State m_state = State::Idle;
    NimBLEClient* m_client = nullptr;
    NimBLERemoteCharacteristic* m_writeChar = nullptr;
    NimBLEAdvertisedDevice* m_targetDevice = nullptr;

    uint32_t m_stateStartMs = 0;
    uint32_t m_statusClearMs = 0;
    bool m_statusTransient = false;
    bool m_scanDone = false;
    bool m_printFailed = false;
    size_t m_chunkSize = 20;

    static constexpr uint32_t SCAN_TIMEOUT_S = 5;
    static constexpr uint32_t STATUS_DISPLAY_MS = 2000;
    static constexpr size_t   BLE_CHUNK_SIZE = 20;
};

#endif // BLE_PRINTER_H
