#ifndef COLOR_H
#define COLOR_H

#define CSI_START                       "\033["     // 起始
#define CSI_END                         "\033[0m"   // 恢复

// 前景色
#define F_NULL
#define F_BLACK                         "30m"       // 黑色
#define F_RED                           "31m"       // 红色
#define F_GREEN                         "32m"       // 绿色
#define F_YELLOW                        "33m"       // 黄色
#define F_BLUE                          "34m"       // 蓝色
#define F_MAGENTA                       "35m"       // 紫红色
#define F_CYAN                          "36m"       // 青色
#define F_WHITE                         "37m"       // 白色

// 背景色
#define B_NULL
#define B_BLACK                         "40;"       // 黑色
#define B_RED                           "41;"       // 红色
#define B_GREEN                         "42;"       // 绿色
#define B_YELLOW                        "43;"       // 黄色
#define B_BLUE                          "44;"       // 蓝色
#define B_MAGENTA                       "45;"       // 紫红色
#define B_CYAN                          "46;"       // 青色
#define B_WHITE                         "47;"       // 白色

// ANSI控制码
#define C_NULL
#define C_HIGHLIGHT                     "1;"        // 设置高亮
#define C_UNDERLINE                     "4;"        // 下划线
#define C_FLICKER                       "5;"        // 闪烁
#define C_REVERSE_DISP                  "7;"        // 反显
#define C_BLANKING                      "8;"        // 消隐

#define LOG_COLOR_RESET                 CSI_END

#define LOG_COLOR_INFO                  CSI_START B_BLACK C_HIGHLIGHT F_GREEN
#define LOG_COLOR_WARN                  CSI_START B_BLACK C_HIGHLIGHT F_YELLOW
#define LOG_COLOR_DEBUG                 CSI_START B_BLACK C_HIGHLIGHT F_BLUE
#define LOG_COLOR_ERROR                 CSI_START B_BLACK C_HIGHLIGHT F_RED
#define LOG_COLOR_FATAL                 CSI_START B_BLACK C_FLICKER F_RED
#define LOG_COLOR_NOTICE                CSI_START B_BLACK C_HIGHLIGHT F_MAGENTA
#define LOG_COLOR_NORMAL                CSI_START B_WHITE C_NULL F_CYAN

#endif
