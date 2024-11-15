/*
*
* Copyright 2015 Rockchip Electronics Co. LTD
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include <stdlib.h>
#include <string.h>
#include "rockchip/rkmpp/rk_type.h"
#include "../../osal/inc/mpp_mem.h"
#include "inc/mpp_bitread.h"

static MPP_RET update_curbyte_default(BitReadCtx_t *bitctx)
{
    if (bitctx->bytes_left_ < 1)
        return  MPP_ERR_READ_BIT;

    // Load a new byte and advance pointers.
    bitctx->curr_byte_ = *bitctx->data_++ & 0xff;
    --bitctx->bytes_left_;
    bitctx->num_remaining_bits_in_curr_byte_ = 8;
    bitctx->prev_two_bytes_ = (bitctx->prev_two_bytes_ << 8) | bitctx->curr_byte_;

    return MPP_OK;
}

static MPP_RET update_curbyte_h264(BitReadCtx_t *bitctx)
{
    if (bitctx->bytes_left_ < 1)
        return  MPP_ERR_READ_BIT;

    // Emulation prevention three-byte detection.
    // If a sequence of 0x000003 is found, skip (ignore) the last byte (0x03).
    if ((*bitctx->data_ == 0x03)
        && ((bitctx->prev_two_bytes_ & 0xffff) == 0)) {
        // Detected 0x000003, skip last byte.
        ++bitctx->data_;
        --bitctx->bytes_left_;
        ++bitctx->emulation_prevention_bytes_;
        bitctx->used_bits += 8;
        // Need another full three bytes before we can detect the sequence again.
        bitctx->prev_two_bytes_ = 0xffff;
        if (bitctx->bytes_left_ < 1)
            return  MPP_ERR_READ_BIT;
    }
    // Load a new byte and advance pointers.
    bitctx->curr_byte_ = *bitctx->data_++ & 0xff;
    --bitctx->bytes_left_;
    bitctx->num_remaining_bits_in_curr_byte_ = 8;
    bitctx->prev_two_bytes_ = (bitctx->prev_two_bytes_ << 8) | bitctx->curr_byte_;

    return MPP_OK;
}

static MPP_RET update_curbyte_h2645_sei(BitReadCtx_t *bitctx)
{
    if (bitctx->bytes_left_ < 1)
        return  MPP_ERR_READ_BIT;

    // Emulation prevention three-byte detection.
    // If a sequence of 0x000003 is found, skip (ignore) the last byte (0x03).
    if ((*bitctx->data_ == 0x03)
        && ((bitctx->prev_two_bytes_ & 0xffff) == 0)) {
        // Detected 0x000003, skip last byte.
        ++bitctx->data_;
        // Need another full three bytes before we can detect the sequence again.
        bitctx->prev_two_bytes_ = 0xffff;
    }
    // Load a new byte and advance pointers.
    bitctx->curr_byte_ = *bitctx->data_++ & 0xff;
    --bitctx->bytes_left_;
    bitctx->num_remaining_bits_in_curr_byte_ = 8;
    bitctx->prev_two_bytes_ = (bitctx->prev_two_bytes_ << 8) | bitctx->curr_byte_;

    return MPP_OK;
}

static MPP_RET update_curbyte_avs2(BitReadCtx_t *bitctx)
{
    if (bitctx->bytes_left_ < 1)
        return MPP_ERR_READ_BIT;

    if (*bitctx->data_ == 0x02 && ((bitctx->prev_two_bytes_ & 0xffff) == 0)) {
        // Detected 0x000002, get 2 bits from next byte
        bitctx->curr_byte_ = 0;
        bitctx->num_remaining_bits_in_curr_byte_ = 6;
        ++bitctx->data_;
    } else {
        bitctx->curr_byte_ = *bitctx->data_++ & 0xff;
        bitctx->num_remaining_bits_in_curr_byte_ = 8;
    }
    // Load a new byte and advance pointers.
    bitctx->prev_two_bytes_ = (bitctx->prev_two_bytes_ << 8) | bitctx->curr_byte_;
    --bitctx->bytes_left_;

    return MPP_OK;
}

/*!
***********************************************************************
* \brief
*   Read |num_bits| (1 to 31 inclusive) from the stream and return them
*   in |out|, with first bit in the stream as MSB in |out| at position
*   (|num_bits| - 1)
***********************************************************************
*/
MPP_RET mpp_read_bits(BitReadCtx_t *bitctx, RK_S32 num_bits, RK_S32 *out)
{
    RK_S32 bits_left = num_bits;
    *out = 0;
    if (num_bits > 31) {
        return  MPP_ERR_READ_BIT;
    }
    while (bitctx->num_remaining_bits_in_curr_byte_ < bits_left) {
        // Take all that's left in current byte, shift to make space for the rest.
        *out |= (bitctx->curr_byte_ << (bits_left - bitctx->num_remaining_bits_in_curr_byte_));
        bits_left -= bitctx->num_remaining_bits_in_curr_byte_;
        if (bitctx->update_curbyte(bitctx)) {
            return  MPP_ERR_READ_BIT;
        }
    }
    *out |= (bitctx->curr_byte_ >> (bitctx->num_remaining_bits_in_curr_byte_ - bits_left));
    *out &= ((1 << num_bits) - 1);
    bitctx->num_remaining_bits_in_curr_byte_ -= bits_left;
    bitctx->used_bits += num_bits;

    return MPP_OK;
}
/*!
***********************************************************************
* \brief
*   read more than 32 bits data
***********************************************************************
*/
MPP_RET mpp_read_longbits(BitReadCtx_t *bitctx, RK_S32 num_bits, RK_U32 *out)
{
    RK_S32 val = 0, val1 = 0;

    if (num_bits < 32)
        return mpp_read_bits(bitctx, num_bits, (RK_S32 *)out);

    if (mpp_read_bits(bitctx, 16, &val)) {
        return  MPP_ERR_READ_BIT;
    }
    if (mpp_read_bits(bitctx, (num_bits - 16), &val1)) {
        return  MPP_ERR_READ_BIT;
    }

    *out = (RK_U32)((val << 16) | val1);

    return MPP_OK;
}
/*!
***********************************************************************
* \brief
*   skip bits (0 - 31)
***********************************************************************
*/
MPP_RET mpp_skip_bits(BitReadCtx_t *bitctx, RK_S32 num_bits)
{
    RK_S32 bits_left = num_bits;

    while (bitctx->num_remaining_bits_in_curr_byte_ < bits_left) {
        // Take all that's left in current byte, shift to make space for the rest.
        bits_left -= bitctx->num_remaining_bits_in_curr_byte_;
        if (bitctx->update_curbyte(bitctx)) {
            return  MPP_ERR_READ_BIT;
        }
    }
    bitctx->num_remaining_bits_in_curr_byte_ -= bits_left;
    bitctx->used_bits += num_bits;

    return MPP_OK;
}
/*!
***********************************************************************
* \brief
*   skip bits long (0 - 32)
***********************************************************************
*/
MPP_RET mpp_skip_longbits(BitReadCtx_t *bitctx, RK_S32 num_bits)
{
    if (num_bits < 32)
        return mpp_skip_bits(bitctx, num_bits);

    if (mpp_skip_bits(bitctx, 16)) {
        return  MPP_ERR_READ_BIT;
    }
    if (mpp_skip_bits(bitctx, (num_bits - 16))) {
        return  MPP_ERR_READ_BIT;
    }
    return MPP_OK;
}
/*!
***********************************************************************
* \brief
*   show bits (0 - 31)
***********************************************************************
*/
MPP_RET mpp_show_bits(BitReadCtx_t *bitctx, RK_S32 num_bits, RK_S32 *out)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    BitReadCtx_t tmp_ctx = *bitctx;

    if (num_bits < 32)
        ret = mpp_read_bits(&tmp_ctx, num_bits, out);
    else
        ret = mpp_read_longbits(&tmp_ctx, num_bits, (RK_U32 *)out);

    return ret;
}
/*!
***********************************************************************
* \brief
*   show long bits (0 - 32)
***********************************************************************
*/
MPP_RET mpp_show_longbits(BitReadCtx_t *bitctx, RK_S32 num_bits, RK_U32 *out)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    BitReadCtx_t tmp_ctx = *bitctx;

    ret = mpp_read_longbits(&tmp_ctx, num_bits, out);

    return ret;
}
/*!
***********************************************************************
* \brief
*   read unsigned data
***********************************************************************
*/
MPP_RET mpp_read_ue(BitReadCtx_t *bitctx, RK_U32 *val)
{
    RK_S32 num_bits = -1;
    RK_S32 bit;
    RK_S32 rest;
    // Count the number of contiguous zero bits.
    do {
        if (mpp_read_bits(bitctx, 1, &bit)) {
            return  MPP_ERR_READ_BIT;
        }
        num_bits++;
    } while (bit == 0);
    if (num_bits > 31) {
        return  MPP_ERR_READ_BIT;
    }
    // Calculate exp-Golomb code value of size num_bits.
    *val = (1 << num_bits) - 1;
    if (num_bits > 0) {
        if (mpp_read_bits(bitctx, num_bits, &rest)) {
            return  MPP_ERR_READ_BIT;
        }
        *val += rest;
    }

    return MPP_OK;
}
/*!
***********************************************************************
* \brief
*   read signed data
***********************************************************************
*/
MPP_RET mpp_read_se(BitReadCtx_t *bitctx, RK_S32 *val)
{
    RK_U32 ue;

    if (mpp_read_ue(bitctx, &ue)) {
        return  MPP_ERR_READ_BIT;
    }
    if (ue % 2 == 0) { // odd
        *val = -(RK_S32)(ue >> 1);
    } else {
        *val = (RK_S32)((ue >> 1) + 1);
    }
    return MPP_OK;
}

/*!
***********************************************************************
* \brief
*   check whether has more rbsp data
***********************************************************************
*/
RK_U32 mpp_has_more_rbsp_data(BitReadCtx_t *bitctx)
{
    // remove tail byte which equal zero
    while (bitctx->bytes_left_ &&
           bitctx->data_[bitctx->bytes_left_ - 1] == 0)
        bitctx->bytes_left_--;

    // Make sure we have more bits, if we are at 0 bits in current byte
    // and updating current byte fails, we don't have more data anyway.
    if (bitctx->num_remaining_bits_in_curr_byte_ == 0 && bitctx->update_curbyte(bitctx))
        return 0;
    // On last byte?
    if (bitctx->bytes_left_)
        return 1;
    // Last byte, look for stop bit;
    // We have more RBSP data if the last non-zero bit we find is not the
    // first available bit.
    if (bitctx->num_remaining_bits_in_curr_byte_)
        return (bitctx->curr_byte_ &
                ((1 << (bitctx->num_remaining_bits_in_curr_byte_ - 1)) - 1)) != 0;
    else
        return 0;
}
/*!
***********************************************************************
* \brief
*   initialize bit read context
***********************************************************************
*/
void mpp_set_bitread_ctx(BitReadCtx_t *bitctx, RK_U8 *data, RK_S32 size)
{
    memset(bitctx, 0, sizeof(BitReadCtx_t));
    bitctx->data_ = data;
    bitctx->bytes_left_ = size;
    bitctx->num_remaining_bits_in_curr_byte_ = 0;
    // Initially set to 0xffff to accept all initial two-byte sequences.
    bitctx->prev_two_bytes_ = 0xffff;
    bitctx->emulation_prevention_bytes_ = 0;
    // add
    bitctx->buf = data;
    bitctx->buf_len = size;
    bitctx->used_bits = 0;
    mpp_set_bitread_pseudo_code_type(bitctx, PSEUDO_CODE_NONE);
}

void mpp_set_bitread_pseudo_code_type(BitReadCtx_t *bitctx, PseudoCodeType type)
{
    bitctx->prevention_type = type;
    switch (type) {
    case PSEUDO_CODE_H264_H265:
        bitctx->update_curbyte = update_curbyte_h264;
        break;
    case PSEUDO_CODE_H264_H265_SEI:
        bitctx->update_curbyte = update_curbyte_h2645_sei;
        break;
    case PSEUDO_CODE_AVS2:
        bitctx->update_curbyte = update_curbyte_avs2;
        break;
    default:
        bitctx->update_curbyte = update_curbyte_default;
        break;
    }
}

/*!
***********************************************************************
* \brief
*   align data and get current point
***********************************************************************
*/
RK_U8 *mpp_align_get_bits(BitReadCtx_t *bitctx)
{
    int n = bitctx->num_remaining_bits_in_curr_byte_;
    if (n)
        mpp_skip_bits(bitctx, n);
    return bitctx->data_;
}

RK_S32 mpp_get_bits_left(BitReadCtx_t *bitctx)
{
    return  bitctx->bytes_left_ * 8 + bitctx->num_remaining_bits_in_curr_byte_;
}

RK_S32 mpp_get_bits_count(BitReadCtx_t *bitctx)
{
    return bitctx->used_bits;
}
