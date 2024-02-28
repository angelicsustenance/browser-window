#ifndef BW_WINDOW_WIN32_H
#define BW_WINDOW_WIN32_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../window.h"

#include <stdbool.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>



struct bw_WindowDispatchData {
	bw_WindowDispatchFn func;
	bw_Window* window;
	void* data;

};



LRESULT CALLBACK bw_Window_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);


#ifdef __cplusplus
}
#endif

#endif//BW_WINDOW_WIN32_H
