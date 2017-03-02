#pragma once


#ifndef MINGW_STD_THREADS
#include <thread>
#include <mutex>
#include <condition_variable>
#else
#include <mingw.thread.h>
#include <mingw.mutex.h>
#include <mingw.condition_variable.h>
#endif
