#ifndef TBOX_CRYPTO_AES_H
#define TBOX_CRYPTO_AES_H

#include <cstdint>

namespace tbox {
namespace crypto {

/**
 * AES 加密 & 解密，仅实现单个块的运算
 *
 * 示例：
 * char key[16] = { ... };          //! 密钥，任意16字节
 * char plain_text[16] = { ... };   //! 明文，任意16字节
 * char crypto_text[16];            //! 密文
 * AES aes(key);
 * aes.cipher(plain_text, crypto);
 */
class AES {
  public:
    AES(const uint8_t *key);

    /**
     * \brief   设置密钥
     * \param   key     密钥地址，16字节长度
     */
    void setKey(const uint8_t *key);

    /**
     * \brief   加密
     * \param   input   明文输入地址，16字节长度
     * \param   output  密文输出地址，16字节长度
     */
    void cipher(const uint8_t *input, uint8_t *output);

    /**
     * \brief   解密
     *
     * \param   input   密文输入地址，16字节长度
     * \param   output  明文输出地址，16字节长度
     */
    void invcipher(const uint8_t *input, uint8_t *output);

  private:
    void keyExpansion(const uint8_t *key);

  private:
    uint8_t w[11][4][4];
};

}
}

#endif //TBOX_CRYPTO_AES_H_20221219
