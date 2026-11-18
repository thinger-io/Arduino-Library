# Server authentication with TinyGSM

ThingerTinyGSM uses TinyGsmClientSecure by default. A TLS connection and verification of the server identity are separate capabilities. The adapter does not upload the certificate from ThingerClient::set_root_ca() to a modem. Configure a trusted CA and peer verification through a supported modem/driver before connecting. Verify that a trusted endpoint succeeds and an untrusted or mismatched certificate fails on the deployed hardware.

With TinyGSM 0.12.0 the SIM800 driver exposes a secure client. Certificate policy depends on modem firmware, provisioned trust and driver configuration. SIM900, A6, A7 and M590 in the example do not expose a secure client. Use a modem/driver supporting the required identity checks. Defining _DISABLE_TLS_ explicitly selects an unencrypted transport and sends application credentials over it.

[Driver selection](https://github.com/vshymanskyy/TinyGSM/blob/v0.12.0/src/TinyGsmClient.h) · [SIM800 implementation](https://github.com/vshymanskyy/TinyGSM/blob/v0.12.0/src/TinyGsmClientSIM800.h)
