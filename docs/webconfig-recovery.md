# Recovering rejected device credentials

Register recovery during setup, before starting the client. The callback must check a local operator action. Use the device-specific password provisioned during commissioning; keep it outside the sketch example and device logs.

~~~cpp
bool operatorPresent() {
    return digitalRead(recoveryButtonPin) == LOW;
}
// Configure recoveryButtonPin as INPUT_PULLUP in setup().
thing.set_credential_recovery(operatorPresent, devicePortalPassword);
~~~

The device polls the local action following an explicit authentication rejection. An unavailable server or transport error alone does not open the portal. Keep the button pressed during the next connection attempt. A blank replacement secret keeps the stored secret. A failed save leaves the live credentials unchanged. Fields supplied by the sketch are fixed. Pass nullptr for the callback and password to disable recovery.

The normal provisioning portal has its own settings. Set THINGER_DEVICE_SSID_PSWD before including the WebConfig header when provisioning a device-specific access-point password. Keep setup access physically controlled.
