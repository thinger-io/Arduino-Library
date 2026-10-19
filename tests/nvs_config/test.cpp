// Real ThingerWifi, ESP32, WebConfig, PSON and the pinned ESP32 WiFiMulti.
// Only the transport, radio, NVS and filesystem operations are simulated.
#include <assert.h>
#include <iostream>
#include <vector>
#include <map>
#include "FS.h"
#include "nvs.h"
#include "thinger/pson.h"
#include "WiFi.h"

using namespace protoson;
protoson::dynamic_memory_allocator allocator;
protoson::memory_allocator& protoson::pool = allocator;
FakeFS SPIFFS, LittleFS;
int portal_calls = 0, wifi_status = 3;
bool native_wifi_saved = false;
unsigned long fake_millis = 0;
FakeWiFi WiFi;
const char* submitted_user = "new-user";
const char* submitted_device = "new-device";
const char* submitted_credential = "new-credential";
#define THINGER_DEBUG(a, b) do {} while(0)
#define THINGER_DEBUG_VALUE(a, b, c) do {} while(0)
#define ESP32
#define _DISABLE_TLS_
#define THINGER_CLIENT_H
class ThingerClient {
public:
    enum THINGER_ERROR { ERROR_UNKNOWN, NETWORK_NOT_CONFIGURED, NETWORK_NOT_FOUND,
                         NETWORK_AUTH_ERROR, NETWORK_NO_IP };
    template<class Client> ThingerClient(Client&, const char*, const char*, const char*) {}
    void set_error(THINGER_ERROR value) { error = value; }
    THINGER_ERROR error = ERROR_UNKNOWN;
};
#include "ThingerESP32WebConfig.h"

struct FakeNVS {
    bool present = false;
    bool fail_open = false, fail_write = false, fail_commit = false, fail_read = false;
    std::vector<uint8_t> committed, pending;
    std::map<std::string, std::vector<uint8_t>> other;
    std::string pending_key, fail_key;
    int handles = 0, writes = 0, commits = 0;
} nv;
esp_err_t nvs_open(const char* name, nvs_open_mode_t mode, nvs_handle_t* h) {
    assert(std::string(name) == "thinger");
    if(nv.fail_open) return ESP_FAIL;
    if(mode == NVS_READONLY && !nv.present && nv.other.empty()) return ESP_ERR_NVS_NOT_FOUND;
    ++nv.handles; *h = 1; return ESP_OK;
}
void nvs_close(nvs_handle_t) { --nv.handles; }
esp_err_t nvs_get_blob(nvs_handle_t, const char* key, void* out, size_t* size) {
    const std::vector<uint8_t>* record;
    if(std::string(key) == "config") {
        if(!nv.present) return ESP_ERR_NVS_NOT_FOUND;
        record = &nv.committed;
    } else {
        auto it = nv.other.find(key);
        if(it == nv.other.end()) return ESP_ERR_NVS_NOT_FOUND;
        record = &it->second;
    }
    if(out && nv.fail_read && (nv.fail_key.empty() || nv.fail_key == key)) return ESP_FAIL;
    if(out && *size < record->size()) return ESP_ERR_NVS_INVALID_LENGTH;
    if(out) memcpy(out, record->data(), record->size());
    *size = record->size(); return ESP_OK;
}
esp_err_t nvs_set_blob(nvs_handle_t, const char* key, const void* bytes, size_t size) {
    ++nv.writes;
    if(nv.fail_write && (nv.fail_key.empty() || nv.fail_key == key)) return ESP_FAIL;
    nv.pending_key = key;
    nv.pending.assign(static_cast<const uint8_t*>(bytes), static_cast<const uint8_t*>(bytes) + size);
    return ESP_OK;
}
esp_err_t nvs_commit(nvs_handle_t) {
    ++nv.commits;
    if(nv.fail_commit && (nv.fail_key.empty() || nv.fail_key == nv.pending_key)) return ESP_FAIL;
    if(nv.pending_key == "config") { nv.committed = nv.pending; nv.present = true; }
    else nv.other[nv.pending_key] = nv.pending;
    return ESP_OK;
}
class Device : public ThingerESP32WebConfig {
public:
    using ThingerESP32WebConfig::load_configuration;
    using ThingerESP32WebConfig::save_configuration;
    using ThingerESP32WebConfig::configuration_storage_error;
    using ThingerESP32WebConfig::connect_network;
    using ThingerESP32WebConfig::ThingerESP32WebConfig;
    size_t network_count() const { return wifi_networks_; }
};
using Store = thinger_config::nvs_config;
FakeFS& legacy() {
#ifdef THINGER_ESP32_CONFIG_USE_LITTLEFS
    return LittleFS;
#else
    return SPIFFS;
#endif
}
void reset() {
    assert(nv.handles == 0);
    nv = FakeNVS(); SPIFFS = FakeFS(); LittleFS = FakeFS();
    portal_calls = 0; wifi_status = WL_CONNECTED;
    native_wifi_saved = false; WiFi = FakeWiFi(); fake_millis = 0;
}
std::vector<uint8_t> encode(pson& config) {
    pson_encoder counter; counter.encode(config);
    std::vector<uint8_t> bytes(counter.bytes_written());
    thinger_config::buffer_encoder encoder(bytes.data(), bytes.size()); encoder.encode(config);
    assert(encoder.valid()); return bytes;
}
void seed() {
    pson original;
    original["user"] = "legacy-user";
    original["device"] = "legacy-device";
    original["credential"] = "legacy-secret";
    original["ssid"] = "legacy-wifi";
    original["password"] = "legacy-password";
    original["custom"]["threshold"] = 42;
    original["custom"]["enabled"] = true;
    original["empty"] = "";
    uint8_t bytes[] = {0, 1, 2, 255}; original["binary"].set_bytes(bytes, sizeof(bytes));
    legacy().bytes = encode(original); legacy().present = true;
}
void test_migration() {
    reset(); seed(); const auto expected = legacy().bytes;
    Device d; pson config; assert(d.load_configuration(config));
    assert(!legacy().present && legacy().removals == 1);
    assert(legacy().mounts == legacy().unmounts);
    assert(std::vector<uint8_t>(nv.committed.begin()+8, nv.committed.end()) == expected);
    assert(encode(config) == expected);
    assert(std::string((const char*)config["ssid"]) == "legacy-wifi");
    assert((int)config["custom"]["threshold"] == 42);
    // Simulate reboot with a stale legacy file and a broken filesystem.
    seed(); legacy().mount_ok = false; int mounts = legacy().mounts;
    Device reboot; pson loaded; assert(reboot.load_configuration(loaded));
    assert(legacy().mounts == mounts && encode(loaded) == expected);
    assert(nv.writes == 1); // Reads/reconnections do not rewrite flash.
}
void test_failures() {
    for(int failure = 0; failure != 4; ++failure) {
        reset(); seed();
        nv.fail_open = failure == 0; nv.fail_write = failure == 1;
        nv.fail_commit = failure == 2; nv.fail_read = failure == 3;
        Device d; assert(!d.connect_network());
        assert(d.configuration_storage_error() && legacy().present && portal_calls == 0);
        assert(nv.handles == 0);
        nv.fail_open = nv.fail_write = nv.fail_commit = nv.fail_read = false;
        Device reboot; pson config; assert(reboot.load_configuration(config));
        assert(std::string((const char*)config["user"]) == "legacy-user");
    }
}
void test_cleanup_and_reset() {
    reset(); seed(); legacy().remove_ok = false;
    Device d; pson config; assert(d.load_configuration(config));
    assert(legacy().present); int mounts = legacy().mounts;
    assert(d.clean_credentials());
    Device reboot; pson empty; assert(!reboot.load_configuration(empty));
    assert(!reboot.configuration_storage_error() && legacy().mounts == mounts);
    assert(Store::load(empty) == Store::result::cleared);
    // Failure to clear preserves the previous committed config.
    assert(d.save_configuration(config)); auto committed = nv.committed;
    nv.fail_commit = true; assert(!d.clean_credentials()); assert(nv.committed == committed);
}
void test_bad_data() {
    reset(); seed(); legacy().bytes.pop_back();
    Device d; pson config; assert(!d.load_configuration(config));
    assert(d.configuration_storage_error() && legacy().present && !nv.present);
    reset(); seed(); legacy().bytes.resize(THINGER_NVS_CONFIG_MAX_SIZE+1);
    Device large; assert(!large.load_configuration(config)); assert(legacy().present);
    reset(); seed(); nv.present = true; nv.committed = {1, 2, 3};
    Device corrupt; assert(!corrupt.load_configuration(config));
    assert(corrupt.configuration_storage_error() && legacy().mounts == 0);
    reset(); seed(); legacy().mount_ok = false;
    Device unreadable; assert(!unreadable.connect_network());
    assert(legacy().present && portal_calls == 0 && nv.writes == 0);
}
void test_portal_preserves_custom_fields() {
    reset(); seed(); Device d; assert(d.connect_network());
    assert(portal_calls == 1);
    pson loaded; assert(Store::load(loaded) == Store::result::ready);
    assert((int)loaded["custom"]["threshold"] == 42);
    assert(std::string((const char*)loaded["password"]) == "legacy-password");
    reset(); nv.fail_write = true; Device failed;
    assert(!failed.connect_network() && failed.configuration_storage_error());
}
void test_new_device() {
    reset(); Device d; pson config;
    assert(!d.load_configuration(config) && !d.configuration_storage_error());
    assert(d.connect_network()); assert(portal_calls == 1);
    assert(Store::load(config) == Store::result::ready);
    assert(std::string((const char*)config["user"]) == submitted_user);
}

class WifiDevice : public ThingerESP32 {
public:
    WifiDevice() : ThingerESP32("user", "device", "credential") {}
    using ThingerESP32::connect_network;
    size_t network_count() const { return wifi_networks_; }
};

void reboot_radio() {
    WiFi = FakeWiFi(); native_wifi_saved = false; wifi_status = WL_DISCONNECTED;
}

void test_wifi_reboot_and_selection() {
    reset();
    {
        WifiDevice first;
        char ssid[] = "network-a", password[] = "password-a";
        first.add_wifi(ssid, password); assert(first.wifi_storage_ok());
        memset(ssid, 'x', strlen(ssid)); memset(password, 'x', strlen(password));
        first.add_wifi("network-b", "password-b"); assert(first.wifi_storage_ok());
        first.add_wifi("open-network"); assert(first.wifi_storage_ok());
        assert(first.network_count() == 3);
        const int writes = nv.writes;
        first.add_wifi("network-a", "password-a");
        first.add_wifi("open-network", "");
        assert(nv.writes == writes && first.network_count() == 3);
    }
    const int writes = nv.writes;
    reboot_radio();
    WifiDevice reboot;
    WiFi.available = {{"network-a", "password-a", -70, WIFI_AUTH_WPA2_PSK},
                      {"network-b", "password-b", -40, WIFI_AUTH_WPA2_PSK},
                      {"unregistered", "", -10, WIFI_AUTH_OPEN}};
    assert(reboot.connect_network());
    assert(reboot.network_count() == 3 && WiFi.selected == "network-b");
    assert(WiFi.scans > 0 && WiFi.default_begins == 0 && nv.writes == writes);
    // The preferred AP disappears: the real WiFiMulti must select another.
    WiFi.disconnect(); WiFi.available.erase(WiFi.available.begin()+1);
    assert(reboot.connect_network() && WiFi.selected == "network-a");
    WiFi.disconnect(); WiFi.available = {{"open-network", "", -50, WIFI_AUTH_OPEN}};
    assert(reboot.connect_network() && WiFi.selected == "open-network");
    assert(WiFi.last_password.empty());
}

void test_wifi_password_update_and_single_network() {
    reset();
    {
        WifiDevice first;
        first.add_wifi("hidden-network", "old-password");
        first.add_wifi("hidden-network", "new-password");
        assert(first.wifi_storage_ok() && first.network_count() == 1);
        const int writes = nv.writes;
        first.add_wifi("hidden-network", "new-password"); assert(nv.writes == writes);
    }
    reboot_radio(); WifiDevice reboot;
    WiFi.available = {{"hidden-network", "new-password", -50, WIFI_AUTH_WPA2_PSK, true}};
    assert(reboot.connect_network() && reboot.network_count() == 1);
    assert(WiFi.scans == 0 && WiFi.last_password == "new-password" && WiFi.default_begins == 0);
    const auto saved = nv.other["wifi"];
    reboot.add_wifi("", "invalid"); assert(!reboot.wifi_storage_ok());
    assert(nv.other["wifi"] == saved);
    reboot.add_wifi(nullptr); assert(!reboot.wifi_storage_ok());
    assert(reboot.restore_wifi_networks());
    // Repeating setup after reboot does not duplicate or erase other networks.
    reboot.add_wifi("hidden-network", "new-password");
    reboot.add_wifi("another-network", "another-password");
    assert(reboot.wifi_storage_ok() && reboot.network_count() == 2);
}

void test_wifi_write_failures() {
    for(int failure = 0; failure != 3; ++failure) {
        reset(); WifiDevice first;
        first.add_wifi("old-network", "old-password"); assert(first.wifi_storage_ok());
        const auto saved = nv.other["wifi"];
        nv.fail_key = "wifi";
        nv.fail_write = failure == 0; nv.fail_commit = failure == 1; nv.fail_read = failure == 2;
        first.add_wifi("new-network", "new-password");
        assert(!first.wifi_storage_ok() && first.network_count() == 1);
        if(failure != 2) assert(nv.other["wifi"] == saved);
        nv.fail_write = nv.fail_commit = nv.fail_read = false;
        reboot_radio(); WifiDevice reboot;
        assert(reboot.restore_wifi_networks());
        // A failed verification can follow a successful commit; reboot trusts
        // that complete new record, never a partial list.
        assert(reboot.network_count() == (failure == 2 ? 2u : 1u));
        assert(nv.handles == 0);
    }
}

void test_wifi_cold_read_failure_and_retry() {
    reset(); { WifiDevice first; first.add_wifi("network-a", "password-a"); }
    reboot_radio(); WifiDevice reboot;
    nv.fail_key = "wifi"; nv.fail_read = true;
    WiFi.available = {{"network-a", "password-a", -40, WIFI_AUTH_WPA2_PSK}};
    assert(!reboot.connect_network() && !reboot.wifi_storage_ok() && WiFi.begins == 0);
    nv.fail_read = false;
    assert(reboot.connect_network() && reboot.wifi_storage_ok());
    pson incompatible; incompatible["version"] = 2;
    (void)static_cast<pson_array&>(incompatible["networks"]);
    assert(Store::save(incompatible, "wifi"));
    reboot_radio(); WifiDevice invalid;
    assert(!invalid.connect_network() && !invalid.wifi_storage_ok() && WiFi.begins == 0);
}

void test_explicit_credentials_without_pson() {
    reset(); Device first("user", "device", "credential");
    first.add_wifi("network-a", "password-a");
    first.add_wifi("network-b", "password-b");
    WiFi.available = {{"network-b", "password-b", -40, WIFI_AUTH_WPA2_PSK}};
    assert(first.connect_network() && portal_calls == 0 && !nv.present);
    reboot_radio(); Device reboot("user", "device", "credential");
    WiFi.available = {{"network-a", "password-a", -40, WIFI_AUTH_WPA2_PSK}};
    assert(reboot.connect_network() && portal_calls == 0 && reboot.network_count() == 2);
}

void test_portal_wifi_reboot() {
    reset();
    { Device first; assert(first.connect_network()); assert(portal_calls == 1); }
    thinger_config::wifi_store::networks networks;
    assert(thinger_config::wifi_store::load(networks));
    assert(networks.size() == 1 && networks[0].ssid == "portal-wifi");
    reboot_radio(); Device reboot;
    WiFi.available = {{"portal-wifi", "portal-password", -40, WIFI_AUTH_WPA2_PSK}};
    assert(reboot.connect_network() && portal_calls == 1 && WiFi.selected == "portal-wifi");
}

void test_complete_migration_with_multiple_wifi() {
    reset(); seed(); const auto expected = legacy().bytes;
    {
        Device first;
        // Registering Wi-Fi first must not suppress the separate PSON import.
        first.add_wifi("network-a", "password-a");
        first.add_wifi("network-b", "password-b");
        WiFi.available = {{"network-b", "password-b", -40, WIFI_AUTH_WPA2_PSK}};
        assert(first.connect_network() && portal_calls == 0);
        assert(!legacy().present && first.network_count() == 2);
    }
    assert(std::vector<uint8_t>(nv.committed.begin()+8, nv.committed.end()) == expected);
    const auto wifi_record = nv.other["wifi"];
    seed(); legacy().mount_ok = false; const int mounts = legacy().mounts;
    reboot_radio(); Device reboot;
    WiFi.available = {{"network-a", "password-a", -40, WIFI_AUTH_WPA2_PSK}};
    assert(reboot.connect_network() && portal_calls == 0);
    assert(reboot.network_count() == 2 && WiFi.selected == "network-a");
    assert(legacy().mounts == mounts && nv.other["wifi"] == wifi_record);
    pson config; assert(Store::load(config) == Store::result::ready);
    assert(encode(config) == expected);
    // Invalid Wi-Fi records must not reopen the portal or access legacy FS.
    nv.other["wifi"] = {0, 1, 2}; reboot_radio(); Device corrupt;
    assert(!corrupt.connect_network() && portal_calls == 0 && legacy().mounts == mounts);
    assert(nv.committed.size() == expected.size()+8);
}

void test_enterprise_coexistence() {
#if CONFIG_ESP_WIFI_ENTERPRISE_SUPPORT
    reset(); WifiDevice first;
    first.add_wifi_enterprise("enterprise", "username", "enterprise-password");
    first.add_wifi("personal", "personal-password");
    first.add_wifi("personal", "updated-password");
    assert(first.network_count() == 2 && first.wifi_storage_ok());
    WiFi.available = {{"enterprise", "enterprise-password", -40, WIFI_AUTH_WPA2_ENTERPRISE}};
    assert(first.connect_network() && WiFi.selected == "enterprise");
    thinger_config::wifi_store::networks networks;
    assert(thinger_config::wifi_store::load(networks) && networks.size() == 1);
    reboot_radio(); WifiDevice reboot; assert(reboot.restore_wifi_networks());
    assert(reboot.network_count() == 1); // add_wifi_enterprise remains RAM-only.
#endif
}

void test_list_and_remove_wifi() {
    reset(); WifiDevice first;
    std::vector<std::string> ssids = {"sentinel"};
    assert(first.list_wifi(ssids) && ssids.empty() && nv.writes == 0);
    first.add_wifi("network-a", "password-a");
    first.add_wifi("network-b", "password-b");
    first.add_wifi("network-c");
    const int writes = nv.writes;
    assert(first.list_wifi(ssids));
    assert((ssids == std::vector<std::string>{"network-a", "network-b", "network-c"}));
    assert(nv.writes == writes);
    ssids[0] = "modified-snapshot";
    assert(first.list_wifi(ssids) && ssids[0] == "network-a");
    assert(!first.remove_wifi("Network-b") && !first.remove_wifi(nullptr) && !first.remove_wifi(""));
    assert(first.wifi_storage_ok() && nv.writes == writes);
    WiFi.selected = "network-a";
    assert(first.remove_wifi("network-b") && first.network_count() == 2);
    assert(WiFi.selected == "network-a" && WiFi.disconnects == 0);
    assert(!first.remove_wifi("network-b") && first.wifi_storage_ok() && nv.writes == writes+1);
    reboot_radio(); WifiDevice reboot;
    assert(reboot.list_wifi(ssids));
    assert((ssids == std::vector<std::string>{"network-a", "network-c"}));
    WiFi.available = {{"network-b", "password-b", -10, WIFI_AUTH_WPA2_PSK},
                      {"network-a", "password-a", -70, WIFI_AUTH_WPA2_PSK},
                      {"network-c", "", -40, WIFI_AUTH_OPEN}};
    assert(reboot.connect_network() && WiFi.selected == "network-c");
}

void test_remove_active_and_last_wifi() {
    reset(); WifiDevice first;
    pson config; config["custom"]["threshold"] = 42; assert(Store::save(config));
    const auto saved_config = nv.committed;
    first.add_wifi("network-a", "password-a");
    first.add_wifi("network-b", "password-b");
    WiFi.available = {{"network-a", "password-a", -40, WIFI_AUTH_WPA2_PSK},
                      {"network-b", "password-b", -70, WIFI_AUTH_WPA2_PSK}};
    assert(first.connect_network() && WiFi.selected == "network-a");
    const int disconnects = WiFi.disconnects;
    assert(first.remove_wifi("network-a") && first.network_count() == 1);
    assert(WiFi.disconnects == disconnects+1 && WiFi.erases == 1);
    const int scans = WiFi.scans;
    assert(first.connect_network() && WiFi.selected == "network-b");
    assert(WiFi.scans == scans); // The direct-connect pointer now refers to B.
    assert(first.remove_wifi("network-b") && first.network_count() == 0);
    assert(WiFi.erases == 2 && nv.other.count("wifi") == 1);
    assert(nv.committed == saved_config); // PSON data is never rewritten.
    reboot_radio(); WifiDevice reboot;
    // Even if a driver credential survived independently, do not resurrect it.
    native_wifi_saved = true; WiFi.native_ssid = "network-b"; WiFi.native_password = "password-b";
    WiFi.available = {{"network-b", "password-b", -40, WIFI_AUTH_WPA2_PSK}};
    std::vector<std::string> ssids;
    assert(reboot.list_wifi(ssids) && ssids.empty());
    assert(!reboot.connect_network() && WiFi.default_begins == 0 && WiFi.begins == 0);
    assert(reboot.error == ThingerClient::NETWORK_NOT_CONFIGURED);
    // Explicitly registering another network still works after an empty list.
    reboot.add_wifi("network-c");
    assert(reboot.wifi_storage_ok() && reboot.list_wifi(ssids) && ssids.size() == 1);
}

void test_remove_failure_and_list_error() {
    for(int failure = 0; failure != 3; ++failure) {
        reset(); WifiDevice first;
        first.add_wifi("network-a", "password-a"); first.add_wifi("network-b", "password-b");
        const auto saved = nv.other["wifi"];
        WiFi.selected = "network-a";
        nv.fail_key = "wifi";
        nv.fail_write = failure == 0; nv.fail_commit = failure == 1; nv.fail_read = failure == 2;
        assert(!first.remove_wifi("network-a") && !first.wifi_storage_ok());
        assert(first.network_count() == 2 && WiFi.disconnects == 0);
        if(failure != 2) assert(nv.other["wifi"] == saved);
        nv.fail_write = nv.fail_commit = nv.fail_read = false;
        reboot_radio(); WifiDevice reboot;
        std::vector<std::string> ssids;
        assert(reboot.list_wifi(ssids) && ssids.size() == (failure == 2 ? 1u : 2u));
        assert(ssids.back() == "network-b");
    }
    reset(); { WifiDevice first; first.add_wifi("network-a", "password-a"); }
    nv.fail_key = "wifi"; nv.fail_read = true; WifiDevice reboot;
    std::vector<std::string> ssids = {"preserved-output"};
    const int writes = nv.writes;
    assert(!reboot.list_wifi(ssids) && !reboot.wifi_storage_ok());
    assert(ssids.size() == 1 && ssids[0] == "preserved-output" && nv.writes == writes);
    nv.fail_read = false;
    assert(reboot.list_wifi(ssids) && ssids[0] == "network-a");
}

void test_webconfig_after_last_removal() {
    reset();
    { WifiDevice first; first.add_wifi("deleted-network", "deleted-password"); assert(first.remove_wifi("deleted-network")); }
    reboot_radio(); native_wifi_saved = true;
    WiFi.native_ssid = "deleted-network"; WiFi.native_password = "deleted-password";
    WiFi.available = {{"deleted-network", "deleted-password", -40, WIFI_AUTH_WPA2_PSK}};
    Device reboot("user", "device", "credential");
    assert(reboot.connect_network() && portal_calls == 1 && WiFi.default_begins == 0);
    std::vector<std::string> ssids;
    assert(reboot.list_wifi(ssids) && ssids.size() == 1 && ssids[0] == "portal-wifi");
}

void test_legacy_native_fallback_without_registry() {
    reset(); reboot_radio();
    WiFi.native_ssid = "legacy-network"; WiFi.native_password = "legacy-password";
    WiFi.available = {{"legacy-network", "legacy-password", -40, WIFI_AUTH_WPA2_PSK}};
    WifiDevice first;
    assert(first.connect_network() && WiFi.default_begins == 1 && nv.other.empty());
}

void test_remove_preserves_enterprise() {
#if CONFIG_ESP_WIFI_ENTERPRISE_SUPPORT
    reset(); WifiDevice first;
    first.add_wifi_enterprise("enterprise", "username", "enterprise-password");
    first.add_wifi("personal", "personal-password");
    assert(first.remove_wifi("personal") && first.network_count() == 1);
    std::vector<std::string> ssids;
    assert(first.list_wifi(ssids) && ssids.empty());
    assert(!first.remove_wifi("enterprise") && first.wifi_storage_ok());
    WiFi.available = {{"enterprise", "enterprise-password", -40, WIFI_AUTH_WPA2_ENTERPRISE}};
    assert(first.connect_network() && WiFi.selected == "enterprise");
#endif
}

void test_clear_wifi_preserves_other_settings() {
    reset(); seed();
    assert(Store::save_legacy(legacy().bytes.data(), legacy().bytes.size()));
    const auto config = nv.committed;
    nv.other["unrelated"] = {9, 8, 7};
    WifiDevice first;
    first.add_wifi("network-a", "password-a");
    first.add_wifi("network-b", "password-b"); first.add_wifi("open-network");
    WiFi.available = {{"network-b", "password-b", -40, WIFI_AUTH_WPA2_PSK}};
    assert(first.connect_network() && WiFi.selected == "network-b");
    const int erases = WiFi.erases;
    assert(first.clear_wifi() && first.wifi_storage_ok() && first.network_count() == 0);
    assert(WiFi.erases == erases+1 && wifi_status == WL_DISCONNECTED);
    std::vector<std::string> ssids = {"sentinel"};
    assert(first.list_wifi(ssids) && ssids.empty());
    assert(nv.committed == config && (nv.other["unrelated"] == std::vector<uint8_t>{9, 8, 7}));
    assert(nv.other.count("wifi") == 1 && !nv.other["wifi"].empty());
    reboot_radio(); WifiDevice reboot;
    native_wifi_saved = true; WiFi.native_ssid = "network-b"; WiFi.native_password = "password-b";
    WiFi.available = {{"network-b", "password-b", -40, WIFI_AUTH_WPA2_PSK}};
    assert(reboot.list_wifi(ssids) && ssids.empty());
    assert(!reboot.connect_network() && WiFi.begins == 0 && WiFi.default_begins == 0);
    assert(reboot.clear_wifi() && reboot.list_wifi(ssids) && ssids.empty());
    assert(nv.committed == config && legacy().present); // Never reads/deletes the old PSON file.
    reboot.add_wifi("new-network", "new-password");
    assert(reboot.wifi_storage_ok() && reboot.list_wifi(ssids) && ssids.size() == 1 && ssids[0] == "new-network");
}

void test_clear_wifi_without_a_registry() {
    reset(); WifiDevice first;
    native_wifi_saved = true; WiFi.selected = WiFi.native_ssid = "legacy-network";
    WiFi.native_password = "legacy-password";
    assert(first.clear_wifi() && WiFi.erases == 1 && nv.other.count("wifi") == 1);
    reboot_radio(); WifiDevice reboot;
    assert(!reboot.connect_network() && WiFi.default_begins == 0);
    assert(reboot.clear_wifi()); // Clearing an already empty registry succeeds.
}

void test_clear_wifi_failures_and_corrupt_record() {
    for(int failure = 0; failure != 4; ++failure) {
        reset(); WifiDevice first;
        first.add_wifi("network-a", "password-a"); first.add_wifi("network-b", "password-b");
        const auto old = nv.other["wifi"];
        WiFi.selected = "network-a";
        nv.fail_key = "wifi";
        nv.fail_open = failure == 0; nv.fail_write = failure == 1;
        nv.fail_commit = failure == 2; nv.fail_read = failure == 3;
        assert(!first.clear_wifi() && !first.wifi_storage_ok());
        assert(first.network_count() == 2 && WiFi.disconnects == 0);
        if(failure != 3) assert(nv.other["wifi"] == old);
        nv.fail_open = nv.fail_write = nv.fail_commit = nv.fail_read = false;
        reboot_radio(); WifiDevice reboot; std::vector<std::string> ssids;
        assert(reboot.list_wifi(ssids) && ssids.size() == (failure == 3 ? 0u : 2u));
        assert(nv.handles == 0);
    }
    reset(); seed(); assert(Store::save_legacy(legacy().bytes.data(), legacy().bytes.size()));
    const auto config = nv.committed; nv.other["wifi"] = {0, 1, 2};
    WifiDevice corrupt; std::vector<std::string> ssids = {"sentinel"};
    assert(!corrupt.list_wifi(ssids) && ssids[0] == "sentinel");
    assert(corrupt.clear_wifi() && corrupt.wifi_storage_ok());
    assert(corrupt.list_wifi(ssids) && ssids.empty() && nv.committed == config);
}

void test_clear_wifi_preserves_enterprise() {
#if CONFIG_ESP_WIFI_ENTERPRISE_SUPPORT
    reset(); WifiDevice first;
    first.add_wifi_enterprise("enterprise", "username", "enterprise-password");
    first.add_wifi("personal", "personal-password");
    WiFi.available = {{"enterprise", "enterprise-password", -40, WIFI_AUTH_WPA2_ENTERPRISE}};
    assert(first.connect_network());
    const int disconnects = WiFi.disconnects;
    assert(first.clear_wifi() && first.network_count() == 1 && WiFi.disconnects == disconnects);
    std::vector<std::string> ssids;
    assert(first.list_wifi(ssids) && ssids.empty() && first.connect_network());
    assert(WiFi.selected == "enterprise");
    reset(); WifiDevice with_ca;
    with_ca.add_wifi_enterprise("enterprise-ca", "username", "enterprise-password", "", "test-ca");
    with_ca.add_wifi("personal", "personal-password");
    WiFi.available = {{"enterprise-ca", "enterprise-password", -40, WIFI_AUTH_WPA2_ENTERPRISE}};
    assert(with_ca.connect_network());
    const int ca_disconnects = WiFi.disconnects;
    assert(with_ca.clear_wifi() && WiFi.disconnects == ca_disconnects);
    assert(with_ca.has_wifi_networks() && with_ca.connect_network() && WiFi.selected == "enterprise-ca");
#endif
}

int main() {
    test_wifi_reboot_and_selection(); test_wifi_password_update_and_single_network();
    test_wifi_write_failures(); test_enterprise_coexistence(); test_portal_wifi_reboot();
    test_wifi_cold_read_failure_and_retry(); test_explicit_credentials_without_pson();
    test_list_and_remove_wifi(); test_remove_active_and_last_wifi();
    test_remove_failure_and_list_error(); test_webconfig_after_last_removal(); test_remove_preserves_enterprise();
    test_legacy_native_fallback_without_registry();
    test_clear_wifi_preserves_other_settings(); test_clear_wifi_without_a_registry();
    test_clear_wifi_failures_and_corrupt_record(); test_clear_wifi_preserves_enterprise();
    std::cout << "PASS: clear_wifi persistent empty registry, other settings/enterprise preserved, failures and recovery\n";
    std::cout << "PASS: SSID snapshots, persistent deletion, active/last removal, no native resurrection, failure recovery\n";
    std::cout << "PASS: real WiFiMulti restore/selection/fallback/open/hidden/update, NVS failures, portal integration\n";
#ifdef THINGER_ESP32_CONFIG_SKIP_LEGACY_MIGRATION
    reset(); legacy().mount_ok = false; Device d;
    assert(d.connect_network() && legacy().mounts == 0);
    std::cout << "PASS: fresh NVS-only device\n";
#else
    test_migration(); test_failures(); test_cleanup_and_reset();
    test_bad_data(); test_portal_preserves_custom_fields(); test_new_device();
    test_complete_migration_with_multiple_wifi();
    std::cout << "PASS: migration, reboot, write/commit/read errors, cleanup, reset, invalid data, custom fields, provisioning\n";
#endif
    assert(nv.handles == 0);
}
