#ifndef MOSQ_SOCKS_MOSQ_H
#define MOSQ_SOCKS_MOSQ_H

int socks5__send(struct mosquitto *mosq);
int socks5__read(struct mosquitto *mosq);

#endif
