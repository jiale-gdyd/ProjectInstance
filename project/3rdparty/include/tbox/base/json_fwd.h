#ifndef TBOX_BASE_JSON_FWD_H
#define TBOX_BASE_JSON_FWD_H

#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

//! 如果找到不该文件，则从 github 下载：
//! https://github.com/nlohmann/json/blob/v3.10.4/include/nlohmann/json_fwd.hpp
//! 下载链接：https://raw.githubusercontent.com/nlohmann/json/v3.10.4/include/nlohmann/json_fwd.hpp
//! 放置到 /usr/local/include/nlohmann/ 目录下

namespace tbox {

using Json = nlohmann::json;
using OrderedJson = nlohmann::ordered_json;

}

#endif
