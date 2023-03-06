#ifndef RTSP_RTSPSERVER_RTSPRINGBUFFER_HPP
#define RTSP_RTSPSERVER_RTSPRINGBUFFER_HPP

#include <tuple>
#include <mutex>
#include <cstdint>
#include <cstring>
#include <cstdbool>

namespace rtsp {
class RTSPRingBuffer;

class RTSPRingElement {
public:
    RTSPRingElement() {
        this->pBuf = nullptr;
        this->nSize = 0;
        this->nChannel = 0;
        this->bIFrame = false;
        this->nPts = 0;
        this->nRefCount = 0;
        this->pParent = nullptr;
        this->pHeadBuf = nullptr;
        this->nHeadSize = 0;
    }

    RTSPRingElement(uint8_t *pBuf, uint32_t nSize, uint32_t nChn, uint64_t u64PTS = 0, bool isIFrame = false, uint8_t *pHeadBuf = nullptr, uint32_t nHeadSize = 0) {
        this->nIndex = -1;
        this->pBuf = pBuf;
        this->nSize = nSize;
        this->nChannel = nChn;
        this->nPts = u64PTS;
        this->bIFrame = isIFrame;
        this->nRefCount = 0;
        this->pParent = nullptr;
        this->pHeadBuf = pHeadBuf;
        this->nHeadSize = nHeadSize;
    }

    void CopyFrom(RTSPRingElement &element) {
        if (this->pBuf) {
            if (element.pHeadBuf && (element.nHeadSize > 0)) {
                memcpy(this->pBuf, element.pHeadBuf, element.nHeadSize);
                memcpy(this->pBuf + element.nHeadSize, element.pBuf, element.nSize);
            } else {
                memcpy(this->pBuf, element.pBuf, element.nSize);
            }
        }

        this->nSize = element.nHeadSize + element.nSize;
        this->nChannel = element.nChannel;
        this->nPts = element.nPts;
        this->bIFrame = element.bIFrame;

        this->nRefCount = 0;
        this->pParent = nullptr;
    }

    int IncreaseRefCount() {
        return ++nRefCount;
    }

    int DecreaseRefCount(bool bForceClear) {
        if (bForceClear) {
            nRefCount = 0;
        } else {
            --nRefCount;
            if (nRefCount < 0) {
               nRefCount = 0;
            }
        }

        return nRefCount;
    }

    int GetRefCount() {
        return nRefCount;
    }

    int GetIndex() {
        return nIndex;
    }

    int SetIndex(int nIndex) {
        return this->nIndex = nIndex;
    }

private:
    void Clear() {
        this->nSize = 0;
        this->nChannel = 0;
        this->bIFrame = false;
        this->nPts = 0;
        this->nRefCount = 0;
    }

public:
    uint8_t        *pBuf;
    uint32_t       nSize;
    uint32_t       nChannel;
    uint64_t       nPts;
    bool           bIFrame;
    uint8_t        *pHeadBuf;
    uint32_t       nHeadSize;
    RTSPRingBuffer *pParent;

private:
    int            nIndex;
    int            nRefCount;
};

class RTSPRingBuffer {
public:
    RTSPRingBuffer(uint32_t nElementBuffSize, uint32_t nElementCount, const char *pszName = nullptr ) {
        m_nElementCount = nElementCount;
        m_nElementBuffSize = nElementBuffSize;
        m_pRing = new RTSPRingElement[m_nElementCount];

        for (uint32_t i = 0; i < m_nElementCount; i++) {
            m_pRing[i].pBuf = new uint8_t[m_nElementBuffSize];
            memset((void *)m_pRing[i].pBuf, 0x0, m_nElementBuffSize);
            m_pRing[i].nSize = 0;
            m_pRing[i].pParent = this;
            m_pRing[i].SetIndex(i);
        }

        m_nHeader = 0;
        m_nTail = 0;
        m_bHasLost = false;
        m_szName[0] = 0;
        if (pszName && strlen(pszName)) {
            strncpy(m_szName, pszName, sizeof(m_szName));
        }
    }

    ~RTSPRingBuffer(void) {
        m_nHeader = 0;
        m_nTail = 0;

        if (nullptr == m_pRing) {
            return;
        }

        m_mutex.lock();
        for (uint32_t i = 0; i < m_nElementCount; i++) {
            if (nullptr != m_pRing[i].pBuf) {
                delete[] m_pRing[i].pBuf;
            }
        }

        delete[] m_pRing;
        m_mutex.unlock();
    }

    bool IsFull() {
        std::lock_guard<std::mutex> lck(m_mutex);
        return (m_nTail - m_nHeader) == m_nElementCount ? true : false;
    }

    bool IsEmpty() {
        std::lock_guard<std::mutex> lck(m_mutex);
        return (m_nTail == m_nHeader) ? true : false;
    }

    bool Put(RTSPRingElement &element) {
        std::lock_guard<std::mutex> lck(m_mutex);

        if (!m_pRing || ((element.nSize + element.nHeadSize) > m_nElementBuffSize)) {
            if (element.bIFrame) {
                m_bHasLost = true;
            }

            return false;
        }

        if ((m_nTail - m_nHeader) == m_nElementCount) {
            if (element.bIFrame) {
                uint64_t nIndex = (m_nTail - 1) % m_nElementCount;
                int nRefCount= m_pRing[nIndex].GetRefCount();
                if (nRefCount == 0) {
                    m_nTail--;
                    m_bHasLost = false;
                } else {
                    m_bHasLost = true;
                    return false;
                }
            } else {
                m_bHasLost = true;
                return false;
            }
        } else {
            if (element.bIFrame)  {
                m_bHasLost = false;
            } else {
                if (m_bHasLost)  {
                    return false;
                }
            }
        }

        uint64_t nIndex = m_nTail % m_nElementCount;
        int nRefCount= m_pRing[nIndex].GetRefCount();
        if (nRefCount != 0) {
            m_bHasLost = true;
            return false;
        }

        m_pRing[nIndex].CopyFrom(element);
        m_pRing[nIndex].IncreaseRefCount();
        m_pRing[nIndex].pParent = this;
        m_nTail++;

        return true;
    }

    RTSPRingElement *Get() {
        RTSPRingElement *element = nullptr;

        std::lock_guard<std::mutex> lck(m_mutex);
        if (m_nHeader == m_nTail) {
            return element;
        }

        uint64_t nIndex = m_nHeader % m_nElementCount;
        m_pRing[nIndex].IncreaseRefCount();
        element = &m_pRing[nIndex];

        return element;
    }

    bool Pop(bool bForce = true) {
        std::lock_guard<std::mutex> lck(m_mutex);
        if (m_nHeader == m_nTail) {
            return false;
        }

        uint64_t nIndex = m_nHeader % m_nElementCount;
        m_pRing[nIndex].DecreaseRefCount(bForce);
        m_nHeader++;

        return true;
    }

    void Free(RTSPRingElement *ele, bool bForce = false) {
        if (!ele) {
            return;
        }

        std::lock_guard<std::mutex> lck(m_mutex);
        int nIndex = ele->GetIndex();
        if ((nIndex >= 0) && (nIndex < (int)m_nElementCount)) {
            m_pRing[nIndex].DecreaseRefCount(bForce);
        }
    }

    uint32_t Size() {
        if (m_nTail < m_nHeader) {
            return 0;
        }

        return m_nTail-m_nHeader;
    }

private:
    RTSPRingElement *m_pRing;
    uint32_t        m_nElementCount;
    uint32_t        m_nElementBuffSize;
    uint64_t        m_nHeader;
    uint64_t        m_nTail;
    bool            m_bHasLost;
    std::mutex      m_mutex;
    char            m_szName[64];
};
}

#endif
