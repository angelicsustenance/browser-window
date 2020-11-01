#include "cef.h"
#include "../assert.h"
#include "../application/cef.h"
#include "../browser_window.h"
#include "../cef/bw_handle_map.hpp"
#include "../cef/exception.hpp"
#include "../debug.h"

#include <string>
#include <include/base/cef_bind.h>
#include <include/cef_browser.h>
#include <include/cef_client.h>
#include <include/cef_v8.h>

// QUICKFIX: The win32 definition of GetMessage is getting through from somewhere...
// TODO: Find out where
#undef GetMessage



#ifdef BW_WIN32
RECT bw_BrowserWindow_window_rect( int width, int height );
#endif
// Sends the given Javascript code to the renderer process, expecting the code to be executed over there.
// script_id should be a script id obtained from storing a callback in the eval callback store.
void bw_BrowserWindow_sendJsToRendererProcess( bw_BrowserWindow* bw, CefRefPtr<CefBrowser>& cef_browser, CefString& code, bw_BrowserWindowJsCallbackFn cb, void* user_data );
char* bw_cef_errorMessage( bw_ErrCode code, const void* data );
void _bw_BrowserWindow_doCleanup( bw_BrowserWindow* bw );
void _bw_BrowserWindow_onResize( const bw_Window* window, unsigned int width, unsigned int height );
/// Constructs the platform-specific window info needed by CEF.
CefWindowInfo _bw_BrowserWindow_windowInfo( bw_Window* window, int width, int height );



/*void bw_BrowserWindow_close( bw_BrowserWindow* bw ) { BW_DEBUG("bw_BrowserWindow_close")
	// Actually close the brower
	CefRefPtr<CefBrowser>* cef_ptr = (CefRefPtr<CefBrowser>*)bw->inner.cef_ptr;
	(*cef_ptr)->GetHost()->CloseBrowser( true );
}*/

void bw_BrowserWindow_evalJs( bw_BrowserWindow* bw, bw_CStrSlice js, bw_BrowserWindowJsCallbackFn cb, void* user_data ) {

	// Wrap the JS code within a temporary function and execute it, and convert the return value to a string
	// This allows executing JS code that isn't terminated with a semicolon, and does the javascript value string conversion inside JS.
	std::string _code = "(function () { return ";
	_code.append( js.data, js.len );
	_code += "; })().toString()";
	CefString code = _code;
	// Note: For the sake of simplicity, I've used std::string to append some strings together.
	//       CefString unfortunately doesn't provide this functionality.
	//       There is some overhead because of this, but for now it is ok.

	// Execute the javascript on the renderer process, and invoke the callback from there:
	CefRefPtr<CefBrowser> cef_browser = *(CefRefPtr<CefBrowser>*)(bw->inner.cef_ptr);

	bw_BrowserWindow_sendJsToRendererProcess( bw, cef_browser, code, cb, user_data );
}

void _bw_BrowserWindow_doCleanup( const bw_Window* window ) {

	auto bw_ptr = (bw_BrowserWindow*)window->user_data;

	// Remove the link between our bw_BrowserWindow handle and the CefBrowser handle
	CefRefPtr<CefBrowser>* cef_ptr = (CefRefPtr<CefBrowser>*)bw_ptr->inner.cef_ptr;
	bw::bw_handle_map.drop( *cef_ptr );

	// Delete the CefBrowser pointer that we have stored in our bw_BrowserWindow handle
	delete cef_ptr;

	// Then free (a c feature) the browser window itself
	free( bw_ptr );
}

bw_Err bw_BrowserWindow_navigate( bw_BrowserWindow* bw, bw_CStrSlice url ) {

	// TODO: Check if bw_CStrSlice can be converted into CefString in one step.
	std::string std_str( url.data, url.len );
	CefString cef_str( std_str );

	CefRefPtr<CefBrowser>* cef_ptr = (CefRefPtr<CefBrowser>*)bw->inner.cef_ptr;
	(*cef_ptr)->GetMainFrame()->LoadURL( cef_str );

	BW_ERR_RETURN_SUCCESS;
}

void bw_BrowserWindow_new(
	bw_Application* app,
	const bw_Window* parent,
	bw_BrowserWindowSource source,
	bw_CStrSlice _title,
	int width, int height,
	const bw_WindowOptions* window_options,
	const bw_BrowserWindowOptions* browser_window_options,
	bw_BrowserWindowHandlerFn external_handler,	/// A function that gets invoked when javascript the appropriate call is made in javascript.
	void* user_data,	/// The data that will be passed to the above handler function when it is invoked.
	bw_BrowserWindowCreationCallbackFn callback,
	void* callback_data
) {
	CefString title( std::string( _title.data, _title.len ) );

	CefWindowInfo info;
	CefBrowserSettings settings;

	// Platform-specific default parent handle:
#ifdef BW_WIN32
	HWND parent_handle = HWND_DESKTOP;
#endif

	if ( parent != 0 ) {
		bw_BrowserWindow* parent_bw = (bw_BrowserWindow*)parent->user_data;
		CefRefPtr<CefBrowser>* parent_cef_ptr = (CefRefPtr<CefBrowser>*)(parent_bw->inner.cef_ptr);

		parent_handle = (*parent_cef_ptr)->GetHost()->GetWindowHandle();
	}

/*#ifdef BW_WIN32
	// On Windows, the following is required.
	// Possibly because CreateWindowEx doesn't work without a parent handle if it is not a popup.
	info.SetAsPopup( 0, title );

	RECT rect = bw_BrowserWindow_window_rect( width, height );

	//info.SetAsChild( parent_handle, bw_BrowserWindow_window_rect( width, height ) );
#endif*/

	// Set up a CefString with the source
	CefString source_string;
	if ( !source.is_html ) {
		source_string = CefString( std::string( source.data.data, source.data.len ) );
	}
	else {
		std::string data = "data:text/html,";
		data.append( source.data.data, source.data.len );
		source_string = CefString( data );
	}

	bw_Window* window = bw_Window_new( app, parent, _title, width, height, window_options, 0 );
	window->callbacks.do_cleanup = _bw_BrowserWindow_doCleanup;
	window->callbacks.on_resize = _bw_BrowserWindow_onResize;

	// Update window size in CefWindowInfo
#ifdef BW_WIN32
	RECT rect;
	GetClientRect( window->handle, &rect );
	info.SetAsChild( window->handle, rect );
#endif

	// Create the browser window handle
	bw_BrowserWindow* bw = new bw_BrowserWindow;
	bw->window = window;
	bw->inner.cef_ptr = 0;
	bw->external_handler = external_handler;
	bw->user_data = user_data;
	// Store a pointer of our browser window into the window
	bw->window->user_data = (void*)bw;
	_bw_BrowserWindow_initWindowCallbacks( bw );

	// Create a CefDictionary containing the bw_BrowserWindow pointer to pass along CreateBrowser
	CefRefPtr<CefDictionaryValue> dict = CefDictionaryValue::Create();
	dict->SetBinary( "handle", CefBinaryValue::Create( (const void*)&bw, sizeof(bw) ) );
	dict->SetBinary( "callback", CefBinaryValue::Create( (const void*)&callback, sizeof(callback) ) );
	dict->SetBinary( "callback-data", CefBinaryValue::Create( (const void*)&callback_data, sizeof(callback_data) ) );

	// Create the browser
	CefRefPtr<CefClient>* cef_client = (CefRefPtr<CefClient>*)app->engine_data->cef_client;
	bool success = CefBrowserHost::CreateBrowser( info, *cef_client, source_string, settings, dict, NULL );
	BW_ASSERT( success, "CefBrowserHost::CreateBrowser failed!\n" );
}

void bw_BrowserWindow_sendJsToRendererProcess( bw_BrowserWindow* bw, CefRefPtr<CefBrowser>& cef_browser, CefString& code, bw_BrowserWindowJsCallbackFn cb, void* user_data ) {
	CefRefPtr<CefProcessMessage> msg = CefProcessMessage::Create("eval-js");
	CefRefPtr<CefListValue> args = msg->GetArgumentList();

	// eval-js message arguments
	args->SetString( 0, code );
	// Conver the callback data into binary blobs so we can send them to the renderer process
	CefRefPtr<CefBinaryValue> bw_bin = CefBinaryValue::Create( (const void*)&bw, sizeof( bw ) );
	args->SetBinary( 1, bw_bin );
	CefRefPtr<CefBinaryValue> cb_bin = CefBinaryValue::Create( (const void*)&cb, sizeof( cb ) );
	args->SetBinary( 2, cb_bin );
	CefRefPtr<CefBinaryValue> user_data_bin = CefBinaryValue::Create( (const void*)&user_data, sizeof( user_data ) );
	args->SetBinary( 3, user_data_bin );

	cef_browser->GetMainFrame()->SendProcessMessage( PID_RENDERER, msg );
}

#ifdef BW_WIN32
RECT bw_BrowserWindow_window_rect( int width, int height) {

	RECT desktop_rect;
	GetClientRect( GetDesktopWindow(), &desktop_rect );
	LONG desktop_width = desktop_rect.right - desktop_rect.left;
	LONG desktop_height = desktop_rect.bottom - desktop_rect.top;

	RECT rect;
	rect.left = 0;
	rect.top = 0;
	rect.right = width;
	rect.bottom = height;

	return rect;

	if ( width != -1 ) {
		rect.left = ( desktop_width - width ) / 2;
		rect.right = rect.left + width;
	}
	if ( height != -1 ) {
		rect.bottom = ( desktop_height - height ) / 2;
		rect.top = rect.bottom + height;
	}

	return rect;
}
#endif

void _bw_BrowserWindow_onResize( const bw_Window* window, unsigned int width, unsigned int height ) {
	bw_BrowserWindow* bw = (bw_BrowserWindow*)window->user_data;

	if ( bw != 0 ) {
		CefRefPtr<CefBrowser> cef = *(CefRefPtr<CefBrowser>*)bw->inner.cef_ptr;

#ifdef BW_WIN32
		SetWindowPos( cef->GetHost()->GetWindowHandle(), 0, 0, 0, width, height, SWP_SHOWWINDOW | SWP_NOZORDER | SWP_NOACTIVATE );
#endif
	}
}
