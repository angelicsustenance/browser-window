#ifndef BW_BROWSER_WINDOW_H
#define BW_BROWSER_WINDOW_H

#ifdef BW_WEBVIEW2
#include "browser_window/webview2.h"
#endif
#ifdef BW_CEF
#include "browser_window/cef.h"
#endif




#include "application.h"
#include "err.h"
#include "string.h"
#include "window.h"



#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>



typedef struct bw_BrowserWindow bw_BrowserWindow;

typedef struct bw_BrowserWindowInner bw_BrowserWindowInner;

typedef struct bw_BrowserWindowCallbacks {
	void (*do_cleanup)( bw_BrowserWindow* bw );
	/// Fired when a browser window has finished loading its window and its browser
	void (*on_close)( bw_BrowserWindow* bw );
	void (*on_destroy)( bw_BrowserWindow* bw );
	void (*on_loaded)( bw_BrowserWindow* bw );
} bw_BrowserWindowCallbacks;

typedef void (*bw_BrowserWindowHandlerFn)( bw_BrowserWindow* window, const char* cmd, const char** args, size_t arg_count );
typedef void (*bw_BrowserWindowJsCallbackFn)( bw_BrowserWindow* window, void* user_data, const char* result, const bw_Err* err );

typedef struct bw_BrowserWindowOptions {
	bool dev_tools;
} bw_BrowserWindowOptions;

typedef struct bw_BrowserWindowSource {
	bw_CStrSlice data;
	bool is_html;
} bw_BrowserWindowSource;



struct bw_BrowserWindow {
	bw_Window* window;
	bw_BrowserWindowInner inner;
	bw_BrowserWindowHandlerFn handler;
	void* user_data;
	bw_BrowserWindowCallbacks callbacks;
};



void bw_BrowserWindow_close( bw_BrowserWindow* bw );

void _bw_BrowserWindow_doCleanup( bw_BrowserWindow* bw );

/// Same as bw_Window_drop, but for bw_BrowserWindow.
void bw_BrowserWindow_drop( bw_BrowserWindow* bw );

void bw_BrowserWindow_eval_js( bw_BrowserWindow* bw, bw_CStrSlice js, bw_BrowserWindowJsCallbackFn callback, void* cb_data );

const bw_Application* bw_BrowserWindow_get_app( bw_BrowserWindow* bw );
void* bw_BrowserWindow_get_user_data( bw_BrowserWindow* bw );

/// Sets up the callbacks of the internal window handle to redirect the callbacks of the browser window callbacks.
void _bw_BrowserWindow_init_window_callbacks( bw_BrowserWindow* bw );

bw_Err bw_BrowserWindow_navigate( bw_BrowserWindow* bw, bw_CStrSlice url );

/// Creates a new browser window
bw_BrowserWindow* bw_BrowserWindow_new(
	const bw_Application* app,
	const bw_Window* parent,
	bw_BrowserWindowSource source,
	bw_CStrSlice _title,
	int width, int height,
	const bw_WindowOptions* window_options,
	const bw_BrowserWindowOptions* browser_window_options,
	bw_BrowserWindowHandlerFn handler,	/// A function that gets invoked when javascript the appropriate call is made in javascript.
	void* user_data	/// The data that will be passed to the above handler function when it is invoked.
);



#ifdef __cplusplus
} // extern "C"
#endif

#endif//BW_BROWSER_WINDOW_H
