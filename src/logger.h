//
//  logger.h
//  NewGlslRenderer
//
//  Created by Skatto on 2018/03/16.
//  Copyright © 2018年 Skatto. All rights reserved.
//

#ifndef logger_h20180316
#define logger_h20180316

#include <cstring>
#include <iostream>
#include <sys/time.h>
#include <time.h>

#define kFILE_NAME                                                             \
  (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#ifndef NDEBUG
#define DEBUG_OUT(var)                                                         \
  Log(Now(), kFILE_NAME, ":", __LINE__, ": ", #var, " = ", var)
#define DEBUG_LOG(...) Log(Now(), kFILE_NAME, ":", __LINE__, ": ", __VA_ARGS__);
#else
#define DEBUG_OUT(var)
#define DEBUG_LOG(...)
#endif

#define LOG_INFO(...) Log(Now(), kFILE_NAME, ":", __LINE__, ": ", __VA_ARGS__);

static const std::string Now() {
  struct timeval myTime;
  gettimeofday(&myTime, nullptr);
  struct tm* time_st = localtime(&myTime.tv_sec);

  return (std::string("[") + std::to_string(time_st->tm_year + 1900) +
          std::string("/") + std::to_string(time_st->tm_mon + 1) +
          std::string("/") + std::to_string(time_st->tm_mday) +
          std::string(" ") + std::to_string(time_st->tm_hour) +
          std::string(":") + std::to_string(time_st->tm_min) +
          std::string(":") + std::to_string(time_st->tm_sec) +
          std::string(".") + std::to_string(myTime.tv_usec) +
          std::string("] "));
}

static void Log() { std::cout << std::endl; }

template <class Head, class... Tail>
static void Log(Head&& head, Tail&&... tail) {
  std::cout << head;
  Log(std::move(tail)...);
}

#endif /* logger_h20180316 */
