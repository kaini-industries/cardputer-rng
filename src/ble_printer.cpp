#include "ble_printer.h"
#include <Arduino.h>
#include <NimBLEDevice.h>

// Common BLE thermal printer service/characteristic UUIDs
static const NimBLEUUID PRINTER_SVC_UUID("18f0");
static const NimBLEUUID PRINTER_CHAR_UUID("2af1");

// Alternative UUIDs used by some printers (e.g. Cat/PeriPage style)
static const NimBLEUUID ALT_SVC_UUID("e7810a71-73ae-499d-8c15-faa9aef0c3f2");
static const NimBLEUUID ALT_CHAR_UUID("bef8d6c9-9c21-4c9e-b632-bd58c1009f9f");

// Global pointer so the callback can reach the BlePrinter instance
static BlePrinter* s_instance = nullptr;

// Scan callback for NimBLE 1.4.x API
class PrinterScanCallbacks : public NimBLEAdvertisedDeviceCallbacks {
    void onResult(NimBLEAdvertisedDevice* device) override {
        if (!s_instance) return;
        // Accept any device advertising a known printer service
        if (device->isAdvertisingService(PRINTER_SVC_UUID) ||
            device->isAdvertisingService(ALT_SVC_UUID)) {
            s_instance->onDeviceFound(device);
            NimBLEDevice::getScan()->stop();
        }
    }
};

static PrinterScanCallbacks s_scanCb;

// Scan complete callback (free function for NimBLE 1.4.x)
static void scanCompleteCB(NimBLEScanResults results) {
    if (s_instance) {
        s_instance->onScanDone();
    }
}

void BlePrinter::init() {
    s_instance = this;
    NimBLEDevice::init("Cardputer");
    NimBLEDevice::setPower(ESP_PWR_LVL_P3);
    m_state = State::Idle;
}

void BlePrinter::onDeviceFound(NimBLEAdvertisedDevice* device) {
    if (m_targetDevice) {
        delete m_targetDevice;
    }
    m_targetDevice = new NimBLEAdvertisedDevice(*device);
}

void BlePrinter::onScanDone() {
    m_scanDone = true;
}

void BlePrinter::update() {
    uint32_t now = millis();

    // Auto-clear transient status messages
    if (m_statusTransient && now >= m_statusClearMs) {
        m_statusTransient = false;
        if (m_state == State::Error) {
            m_state = State::Idle;
        }
    }

    switch (m_state) {
    case State::Scanning:
        if (m_targetDevice) {
            m_state = State::Connecting;
            m_stateStartMs = now;
        } else if (m_scanDone) {
            m_state = State::Error;
            m_statusTransient = true;
            m_statusClearMs = now + 3000;
            Serial.println("[BLE] No printer found");
        }
        break;

    case State::Connecting:
        if (connectToDevice()) {
            m_state = State::Ready;
            Serial.println("[BLE] Printer connected");
        } else {
            m_state = State::Error;
            m_statusTransient = true;
            m_statusClearMs = now + 3000;
            Serial.println("[BLE] Connection failed");
        }
        break;

    case State::Ready:
        if (m_client && !m_client->isConnected()) {
            m_writeChar = nullptr;
            NimBLEDevice::deleteClient(m_client);
            m_client = nullptr;
            m_state = State::Idle;
            Serial.println("[BLE] Printer disconnected");
        }
        break;

    default:
        break;
    }
}

bool BlePrinter::print(const char* text, const char* title) {
    if (m_state != State::Ready || !m_writeChar) return false;

    m_state = State::Printing;
    bool ok = sendEscPos(text, title);
    m_state = State::Ready;

    m_printFailed = !ok;
    m_statusTransient = true;
    m_statusClearMs = millis() + STATUS_DISPLAY_MS;
    if (!ok) {
        Serial.println("[BLE] Print failed");
    }
    return ok;
}

void BlePrinter::scanAndConnect() {
    if (m_state == State::Scanning || m_state == State::Connecting) return;

    if (m_targetDevice) {
        delete m_targetDevice;
        m_targetDevice = nullptr;
    }
    m_scanDone = false;

    NimBLEScan* scan = NimBLEDevice::getScan();
    scan->setAdvertisedDeviceCallbacks(&s_scanCb, false);
    scan->setActiveScan(true);
    scan->setInterval(100);
    scan->setWindow(99);
    scan->start(SCAN_TIMEOUT_S, scanCompleteCB, false);

    m_state = State::Scanning;
    m_stateStartMs = millis();
    Serial.println("[BLE] Scanning for printer...");
}

void BlePrinter::disconnect() {
    if (m_client) {
        if (m_client->isConnected()) {
            m_client->disconnect();
        }
        NimBLEDevice::deleteClient(m_client);
        m_client = nullptr;
    }
    m_writeChar = nullptr;
    if (m_targetDevice) {
        delete m_targetDevice;
        m_targetDevice = nullptr;
    }
    m_state = State::Idle;
}

const char* BlePrinter::statusText() const {
    switch (m_state) {
    case State::Idle:       return nullptr;
    case State::Scanning:   return "Scanning...";
    case State::Connecting: return "Connecting...";
    case State::Ready:
        if (m_statusTransient) return m_printFailed ? "Print failed" : "Printed!";
        return "BLE Ready";
    case State::Printing:   return "Printing...";
    case State::Error:      return "No printer found";
    }
    return nullptr;
}

bool BlePrinter::connectToDevice() {
    if (!m_targetDevice) return false;

    m_client = NimBLEDevice::createClient();
    if (!m_client->connect(m_targetDevice)) {
        NimBLEDevice::deleteClient(m_client);
        m_client = nullptr;
        return false;
    }

    // Try primary service UUID first, then alternative
    NimBLERemoteService* svc = m_client->getService(PRINTER_SVC_UUID);
    if (svc) {
        m_writeChar = svc->getCharacteristic(PRINTER_CHAR_UUID);
    }

    if (!m_writeChar) {
        svc = m_client->getService(ALT_SVC_UUID);
        if (svc) {
            m_writeChar = svc->getCharacteristic(ALT_CHAR_UUID);
        }
    }

    if (!m_writeChar) {
        // Last resort: iterate services looking for a writable characteristic
        auto* services = m_client->getServices(true);
        if (services) {
            for (auto* s : *services) {
                auto* chars = s->getCharacteristics(true);
                if (!chars) continue;
                for (auto* c : *chars) {
                    if (c->canWrite() || c->canWriteNoResponse()) {
                        m_writeChar = c;
                        break;
                    }
                }
                if (m_writeChar) break;
            }
        }
    }

    if (!m_writeChar) {
        m_client->disconnect();
        NimBLEDevice::deleteClient(m_client);
        m_client = nullptr;
        return false;
    }

    // Use negotiated MTU for chunk size (MTU minus 3-byte ATT header)
    uint16_t mtu = m_client->getMTU();
    m_chunkSize = (mtu > 3) ? (mtu - 3) : BLE_CHUNK_SIZE;
    Serial.printf("[BLE] MTU=%u, chunk=%u\n", mtu, (unsigned)m_chunkSize);

    return true;
}

bool BlePrinter::sendEscPos(const char* text, const char* title) {
    // ESC @ — Reset printer
    static const uint8_t cmdReset[] = {0x1B, 0x40};
    if (!sendChunk(cmdReset, sizeof(cmdReset))) return false;

    // ESC a 1 — Center align
    static const uint8_t cmdCenter[] = {0x1B, 0x61, 0x01};
    if (!sendChunk(cmdCenter, sizeof(cmdCenter))) return false;

    // ESC E 1 — Bold on
    static const uint8_t cmdBoldOn[] = {0x1B, 0x45, 0x01};
    if (!sendChunk(cmdBoldOn, sizeof(cmdBoldOn))) return false;

    // Title
    char titleLine[32];
    snprintf(titleLine, sizeof(titleLine), "%s\n", title);
    if (!sendChunk((const uint8_t*)titleLine, strlen(titleLine))) return false;

    // ESC E 0 — Bold off
    static const uint8_t cmdBoldOff[] = {0x1B, 0x45, 0x00};
    if (!sendChunk(cmdBoldOff, sizeof(cmdBoldOff))) return false;

    // ESC a 0 — Left align
    static const uint8_t cmdLeft[] = {0x1B, 0x61, 0x00};
    if (!sendChunk(cmdLeft, sizeof(cmdLeft))) return false;

    // Separator
    const char* sep = "--------------------------------\n";
    if (!sendChunk((const uint8_t*)sep, strlen(sep))) return false;

    // Content — send in BLE-sized chunks
    size_t textLen = strlen(text);
    for (size_t offset = 0; offset < textLen; offset += m_chunkSize) {
        size_t chunkLen = textLen - offset;
        if (chunkLen > m_chunkSize) chunkLen = m_chunkSize;
        if (!sendChunk((const uint8_t*)(text + offset), chunkLen)) return false;
    }

    // Trailing newline + separator
    const char* tail = "\n--------------------------------\n";
    if (!sendChunk((const uint8_t*)tail, strlen(tail))) return false;

    // ESC d 3 — Feed 3 lines
    static const uint8_t cmdFeed[] = {0x1B, 0x64, 0x03};
    if (!sendChunk(cmdFeed, sizeof(cmdFeed))) return false;

    return true;
}

bool BlePrinter::sendChunk(const uint8_t* data, size_t len) {
    if (!m_writeChar) return false;

    for (size_t offset = 0; offset < len; offset += m_chunkSize) {
        size_t chunkLen = len - offset;
        if (chunkLen > m_chunkSize) chunkLen = m_chunkSize;

        bool ok;
        if (m_writeChar->canWriteNoResponse()) {
            ok = m_writeChar->writeValue(data + offset, chunkLen, false);
            delay(10); // Brief pacing for write-no-response
        } else {
            ok = m_writeChar->writeValue(data + offset, chunkLen, true);
            // Write-with-response already blocks until ACK
        }

        if (!ok) return false;
    }
    return true;
}
