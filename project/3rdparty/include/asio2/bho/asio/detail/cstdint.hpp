#pragma once

#include <asio2/config.hpp>

#if defined(ASIO_STANDALONE) || defined(ASIO2_HEADER_ONLY)
#include <asio/detail/cstdint.hpp>
#else
#include <boost/asio/detail/cstdint.hpp>
#endif