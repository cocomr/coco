#pragma once

#if defined(_WIN32) && defined(COCO_BUILDING)
#define COCOEXPORT __declspec(dllexport)
#else
#define COCOEXPORT 
#endif

#ifndef MINGW_STD_THREADS
#include <thread>
#include <mutex>
#include <condition_variable>
#else
#include <mingw.thread.h>
#include <mingw.mutex.h>
#include <mingw.condition_variable.h>
#endif
