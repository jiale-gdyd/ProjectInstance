#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>

#include "SenderReport.h"

namespace xop{
SDESItem::SDESItem()
{

}

SDESItem::~SDESItem()
{

}

void SDESItem::Init(unsigned char tag, char *name)
{
    unsigned int length = strlen(name);
    if (length > 0xFF) {
        length = 0xFF;
    }

    fData[0] = tag;
    fData[1] = (unsigned char)length;

    memcpy(&fData[2], name, length);
}

uint32_t SDESItem::totalSize()
{
    return 2 + (unsigned)fData[1];
}

unsigned char *SDESItem::data()
{
    return fData;
}

SenderReport::SenderReport()
{
    char cname[256] = "";

    m_fCurOffset = 0;
    memset(m_fOutBuf, 0, sizeof(m_fOutBuf));

    gethostname(cname, sizeof(cname));
    m_fCNAME.Init(RTCP_SDES_CNAME, cname);
}

SenderReport::~SenderReport()
{

}

void SenderReport::addSR(uint32_t ssrc, struct timeval time_now, uint32_t rtpTimestamp)
{
    enqueueCommonReportPrefix(RTCP_PT_SR, ssrc, 5);

    enqueuedWord(time_now.tv_sec + 0x83AA7E80);

    double fractionalPart = (time_now.tv_usec / 15625.0) * 0x04000000;
    enqueuedWord((uint32_t)(fractionalPart + 0.5));

    enqueuedWord(rtpTimestamp);

    enqueuedWord(0);
    enqueuedWord(0);
}

void SenderReport::addSDES(uint32_t ssrc)
{
    unsigned int numBytes = 4;

    numBytes += m_fCNAME.totalSize();
    numBytes += 1;

    unsigned num4ByteWords = (numBytes + 3) / 4;

    unsigned rtcpHdr = 0x81000000;
    rtcpHdr |= (RTCP_PT_SDES<<16);
    rtcpHdr |= num4ByteWords;
    enqueuedWord(rtcpHdr);

    enqueuedWord(ssrc);

    enqueue(m_fCNAME.data(), m_fCNAME.totalSize());

    unsigned char zero = '\0';
    unsigned int numPaddingBytesNeeded = 4 - (m_fCurOffset % 4);

    while (numPaddingBytesNeeded-- > 0) {
        enqueue(&zero, 1);
    }
}

void SenderReport::enqueueCommonReportPrefix(uint8_t  packetType, uint32_t ssrc, uint32_t numExtraWords)
{
    uint32_t rtcpHdr = 0x80000000;
    uint32_t numReportingSources = 0;

    rtcpHdr |= (numReportingSources << 24);
    rtcpHdr |= (packetType << 16);
    rtcpHdr |= (1 + numExtraWords + 6 * numReportingSources);

    enqueuedWord(rtcpHdr);
    enqueuedWord(ssrc);
}

void SenderReport::enqueuedWord(uint32_t dword)
{
    uint32_t nvalue = htonl(dword);
    enqueue((unsigned char *)&nvalue, sizeof(uint32_t));
}

void SenderReport::enqueue(uint8_t *from, uint32_t numBytes)
{
    memcpy(m_fOutBuf + m_fCurOffset, from, numBytes);
    m_fCurOffset += numBytes;
}
}
