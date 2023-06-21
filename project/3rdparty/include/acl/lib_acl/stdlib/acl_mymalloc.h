#ifndef ACL_LIBACL_STDLIB_ACL_MYMALLOC_H
#define ACL_LIBACL_STDLIB_ACL_MYMALLOC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "acl_define.h"
#include "acl_malloc.h"

/**
 * 动态分配内存的宏定义，不初始化新分配的内存空间
 * @param size {size_t} 分配长度
 * @return {void *}
 */
#define acl_mymalloc(size) acl_malloc_glue(__FILE__, __LINE__, size)

/**
 * 动态分配内存的宏定义，初始化新分配的内存空间为零
 * @param nmemb {size_t} 元素个数
 * @param size {size_t} 每个元素的长度
 * @return {void *}
 */
#define acl_mycalloc(nmemb, size) acl_calloc_glue(__FILE__, __LINE__, nmemb, size)

/**
 * 重新动态分配内存的宏定义
 * @param ptr {void*} 原内存地址
 * @param size {size_t} 新分配内存时要求的长度
 * @return {void *}
 */
#define acl_myrealloc(ptr, size) acl_realloc_glue(__FILE__, __LINE__, (ptr), size)

/**
 * 动态复制字符串宏定义
 * @param str {const char*} 源字符串
 * @return {char*} 新的字符串，需用 acl_myfree 释放
 */
#define acl_mystrdup(str) acl_strdup_glue(__FILE__, __LINE__, (str))

/**
 * 动态复制字符串宏定义，并限定最大内存空间大小
 * @param str {const char*} 源字符串
 * @param len {size_t} 新字符串最大内存空间大小限制值
 * @return {char*} 新的字符串，需用 acl_myfree 释放
 */
#define acl_mystrndup(str, len) acl_strndup_glue(__FILE__, __LINE__, (str), len)

/**
 * 动态复制内存宏定义
 * @param ptr {const void*} 源内存地址
 * @param len {size_t} 源内存大小
 * @return {void*} 新的字符串，需用 acl_myfree 释放
 */
#define acl_mymemdup(ptr, len) acl_memdup_glue(__FILE__, __LINE__, (ptr), len)

/**
 * 释放动态分配的内存空间
 * @param _ptr_ {void*} 动态内存地址
 */
#define acl_myfree(_ptr_) \
    do {  \
        if (_ptr_) {  \
            acl_free_glue(__FILE__, __LINE__, (_ptr_));  \
            (_ptr_) = NULL;  \
        }  \
    } while (0)

/**
 * XXX: 因为该函数用于回调函数，所以无法进行宏定义转换, 将来再完善此函数
 */
#define acl_myfree_fn acl_free_fn_glue

#ifdef __cplusplus
}
#endif

#endif
