#pragma once

#include <asio2/config.hpp>

#if defined(ASIO_STANDALONE) || defined(ASIO2_HEADER_ONLY)
#include <asio/execution/start.hpp>
#else
#include <boost/asio/execution/start.hpp>
#endif