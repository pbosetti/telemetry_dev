#ifndef DEFINES_H
#define DEFINES_H

#define APP_NAME "telemetry_dev"
#define APP_VERSION "0.1.0"
#define APP_DOMAIN "it.unitn.telemetry_dev"

#define SOCKET_TIMEOUT 200 // Milliseconds
#define CONNECT_DELAY 200000 // Microseconds

#define DEFAULT_ENDPOINT_BIND "tcp://*:9000"
#define DEFAULT_ENDPOINT "tcp://localhost:9000"
#define DEFAULT_TOPIC "telemetry"
#define FORMAT_PLAIN "plain"
#define FORMAT_COMPRESSED "snappy"



#endif // DEFINES_H
