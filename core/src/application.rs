pub mod c;


pub use c::ApplicationImpl;

use crate::error::CbwResult;

use std::{
	path::PathBuf,
	os::raw::{c_char, c_int}
};



pub trait ApplicationExt: Copy {
	/// Asserts if not on the GUI thread
	fn assert_correct_thread( &self );
	/// Dispatches work to be executed on the GUI thread.
	fn dispatch( &self, work: unsafe fn(ApplicationImpl, *mut ()), data: *mut () ) -> bool;
	/// Causes the main loop to exit and lets it return the given code.
	fn exit( &self, exit_code: i32 );
	/// Same as `exit`, but is thread-safe.
	fn exit_threadsafe( self: &Self, exit_code: i32 );
	/// Shuts down all application processes and performs necessary clean-up code.
	fn finish( &self ) {}
	fn initialize( argc: c_int, argv: *mut *mut c_char, settings: &ApplicationSettings ) -> CbwResult<ApplicationImpl>;
	/// Runs the main loop.
	/// This blocks until the application is exitting.
	fn run( &self, on_ready: unsafe fn(ApplicationImpl, *mut ()), data: *mut () ) -> i32;
}

pub struct ApplicationSettings {
	pub engine_seperate_executable_path: Option<PathBuf>,
	pub resource_dir: Option<String>
}



impl Default for ApplicationSettings {
	fn default() -> Self {
		Self {
			engine_seperate_executable_path: None,
			resource_dir: None
		}
	}
}