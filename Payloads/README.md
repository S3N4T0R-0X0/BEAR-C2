## Payloads

This file is for payload testing only and is designed to help you understand the BEAR C2 architecture and customize payloads for adversary simulation purposes. Provides C++/Rust payloads that can be used as customizable templates for environments where a pre-generated payload is not required.

### Payload Configuration

The payload configuration should provide input fields for the following parameters:

* **Authentication ID**
  The identifier used to associate the payload with the configured Add Listener instance.

* **Server Host**
  The IP address or hostname of the Add Listener endpoint.

* **Server Port**
  The network port exposed by the Add Listener for the selected communication protocol.

* **Encryption Key**
  Required when the selected transport uses encryption. The value must match the encryption configuration used by the Add Listener. If encryption is disabled, this field is not required.

* **User-Agent**
  The HTTP client identification value used when establishing the initial HTTP/HTTPS communication. The payload should use a User-Agent supported by the corresponding Add Listener configuration.

The User-Agent does not need to be identical across different Add Listener configurations. A payload can use any User-Agent defined as supported by the selected Add Listener profile, as long as the resulting configuration is compatible with the server-side transport settings.

### Example Configuration

The following example shows a sample HTTPS transport configuration with authentication, server addressing, encryption, and User-Agent parameters:

```cpp
const string AUTH_ID = "YOUR_AUTH_ID";
const string SERVER_HOST = "YOUR_SERVER_HOST";
const int SERVER_PORT = YOUR_SERVER_PORT;
const string KEY = "YOUR_ENCRYPTION_KEY";
const string DEFAULT_USER_AGENT = "YOUR_USER_AGENT";
bool VERIFY_SSL = true;
```

This configuration represents an HTTPS transport with encryption enabled. The values shown above are placeholders and should be replaced with the parameters defined by the corresponding Add Listener configuration.

The C++/Rust sample is intended to provide a starting point for customization. Users can modify the configuration and transport-related parameters according to the Add Listener profile they are testing, then compile the customized payload for their authorized simulation environment.
