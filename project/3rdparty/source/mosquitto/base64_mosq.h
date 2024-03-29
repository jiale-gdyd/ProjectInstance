#ifndef MOSQUITTO_BASE64_MOSQ_H
#define MOSQUITTO_BASE64_MOSQ_H

#ifdef __cplusplus
extern "C" {
#endif

int base64__encode(const unsigned char *in, size_t in_len, char **encoded);
int base64__decode(const char *in, unsigned char **decoded, unsigned int *decoded_len);

#ifdef __cplusplus
}
#endif

#endif
