#pragma once

#include "noncopyable.hpp"

namespace acl
{

class sha1 : public noncopyable
{
public:
    sha1();
    virtual ~sha1();

    /*
    *  Re-initialize the class
    */
    void reset();

    /*
    *  Returns the message digest, message_digest_array's length must >= 20
    */
    bool result(unsigned char *message_digest_array);
    bool result2(unsigned *message_digest_array);

    /*
    *  Provide input to SHA1
    */
    void input(const unsigned char *message_array, unsigned length);
    void input(const char *message_array, unsigned length);
    void input(unsigned char message_element);
    void input(char message_element);
    sha1& operator<<(const char *message_array);
    sha1& operator<<(const unsigned char *message_array);
    sha1& operator<<(const char message_element);
    sha1& operator<<(const unsigned char message_element);

private:
    /*
    *  Process the next 512 bits of the message
    */
    void process_message_block();

    /*
    *  Pads the current message block to 512 bits
    */
    void pad_message();

    /*
    *  Performs a circular left shift operation
    */
    inline unsigned circular_shift(int bits, unsigned word);

    unsigned h_[5];                     // Message digest buffers

    unsigned length_low_;               // Message length in bits
    unsigned length_high_;              // Message length in bits

    unsigned char message_block_[64];   // 512-bit message blocks
    int message_block_index_;           // Index into message block array

    bool computed_;                     // Is the digest computed?
    bool corrupted_;                    // Is the message digest corruped?
};
}
