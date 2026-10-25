#pragma once
#include <stdio.h>

#define LOG_LEVEL_ERR 1
#define LOG_LEVEL_INF 2
#define LOG_LEVEL_DBG 3

#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_DBG
#endif

#define LOG_ERR(...) do { if (LOG_LEVEL >= LOG_LEVEL_ERR) printf(__VA_ARGS__); } while (0)
#define LOG_INF(...) do { if (LOG_LEVEL >= LOG_LEVEL_INF) printf(__VA_ARGS__); } while (0)
#define LOG_DBG(...) do { if (LOG_LEVEL >= LOG_LEVEL_DBG) printf(__VA_ARGS__); } while (0)