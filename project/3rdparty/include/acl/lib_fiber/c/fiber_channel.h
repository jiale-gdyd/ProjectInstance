#ifndef ACL_LIB_FIBER_FIBER_CHANNEL_H
#define ACL_LIB_FIBER_FIBER_CHANNEL_H

#include "fiber_define.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Channel communication */

/**
 * The fiber channel type definition
 */
typedef struct ACL_CHANNEL ACL_CHANNEL;

/**
 * Create one fiber channel
 * @param elemsize {int} the fixed object size transfered in fiber channel
 * @param bufsize {int} the buffered of objects in fiber channel
 * @return {ACL_CHANNNEL*}
 */
FIBER_API ACL_CHANNEL* acl_channel_create(int elemsize, int bufsize);

/**
 * Free fiber channel created by acl_channel_create
 * @param c {ACL_CHANNEL*} created by acl_channel_create
 */
FIBER_API void acl_channel_free(ACL_CHANNEL* c);

/**
 * Send object to specified fiber channel in block mode
 * @param c {ACL_CHANNEL*} created by acl_channel_create
 * @param v {void*} the object to be transfered
 * @return {int} value (>= 0) returned
 */
FIBER_API int acl_channel_send(ACL_CHANNEL* c, void* v);

/**
 * Send object to specified fiber channel in non-block mode, one new object
 * copied from which will be created internal
 * @param c {ACL_CHANNEL*} created by acl_channel_create
 * @param v {void*} the object to be transfered
 * @return {int} value (>= 0) returned
 */
FIBER_API int acl_channel_send_nb(ACL_CHANNEL* c, void* v);

/**
 * Read one object from specified channel in block mode
 * @param c {ACL_CHANNEL*} created by acl_channel_create
 * @param v {void*} will store the result
 * @return {int} value(>= 0) returned if get one object
 */
FIBER_API int acl_channel_recv(ACL_CHANNEL* c, void* v);

/**
 * Read one object from specified channel in non-block ode
 * @param c {ACL_CHANNEL*} created by acl_channel_create
 * @param v {void*} will store the result
 * @return {int} value(>= 0) returned if get one object, or NULL returned if
 *  none object been got
 */
FIBER_API int acl_channel_recv_nb(ACL_CHANNEL* c, void* v);

/**
 * Send object's addr to specified channel in block mode
 * @param c {ACL_CHANNEL*} created by acl_channel_create
 * @param v {void*} the addr of the object to be transfered
 * @return {int} value (>= 0) returned
 */
FIBER_API int acl_channel_sendp(ACL_CHANNEL* c, void* v);

/**
 * Get object's addr from specified channel in block mode
 * @param c {ACL_CHANNEL*} created by acl_channel_create
 * @return {void*} non-NULL addr returned
 */
FIBER_API void *acl_channel_recvp(ACL_CHANNEL* c);

/**
 * Send the object's addr to specified channel in non-block mode
 * @param c {ACL_CHANNEL*} created by acl_channel_create
 * @param v {void*} the addr of the object to be transfered
 * @return {int} value which is >= 0 returned
 */
FIBER_API int acl_channel_sendp_nb(ACL_CHANNEL* c, void* v);

/**
 * Get the object's addr form specified channel in non-block mode
 * @param c {ACL_CHANNEL*} created by acl_channel_create
 * @return {void*} * non-NULL returned when got one, or NULL returned
 */
FIBER_API void *acl_channel_recvp_nb(ACL_CHANNEL* c);

/**
 * Send unsigned integer to specified channel in block mode
 * @param c {ACL_CHANNEL*} created by acl_channel_create
 * @param val {unsigned long} the integer to be sent
 * @return {int} value (>= 0) returned
 */
FIBER_API int acl_channel_sendul(ACL_CHANNEL* c, unsigned long val);

/**
 * Get unsigned integer from specified channel in block mode
 * @param c {ACL_CHANNEL*} created by acl_channel_create
 * @return {unsigned long}
 */
FIBER_API unsigned long acl_channel_recvul(ACL_CHANNEL* c);

/**
 * Sent unsigned integer to specified channel in non-block mode
 * @param c {ACL_CHANNEL*} created by acl_channel_create
 * @param val {unsigned long} integer to be sent
 * @return {int} value(>= 0) returned
 */
FIBER_API int acl_channel_sendul_nb(ACL_CHANNEL* c, unsigned long val);

/**
 * Get one unsigned integer from specified channel in non-block mode
 * @param c {ACL_CHANNEL*} created by acl_channel_create
 * @return {unsigned long}
 */
FIBER_API unsigned long acl_channel_recvul_nb(ACL_CHANNEL* c);

#ifdef __cplusplus
}
#endif

#endif
