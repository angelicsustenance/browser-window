use browser_window::application::*;
use browser_window::browser::*;
use serde_json;
use std::env;
use std::io::prelude::*;
use std::process::{Command, exit, Stdio};



async fn execute_command( bw: BrowserWindowHandle, line: &str ) {
	let working_dir = bw.eval_js("working_dir").await.expect("Unable to obtain working dir from JavaScript!");

	let cmd = Command::new("cmd")
		.arg("/C")
		.arg( line )
		.current_dir( working_dir )
		.stdout( Stdio::piped() )
		.stderr( Stdio::piped() )
		//.kill_on_drop(true)
		.spawn()
		.expect("Command failed to run!");

	// Read the output
	let mut stdout = cmd.stdout.unwrap();
	let mut stderr = cmd.stderr.unwrap();
	let mut buffer: [u8;  1024] = [0xFF; 1024];
	loop {
		let stdout_empty = read_stream( bw, &mut stdout, &mut buffer, "onOutputReceived" );
		let stderr_empty = read_stream( bw, &mut stderr, &mut buffer, "onErrorOutputReceived" );

		if !stdout_empty && !stderr_empty {
			break;
		}
	}

	// Notify the terminal that it can type commands again
	bw.exec_js("onExecutionEnded()");
}

fn read_stream<R>( bw: BrowserWindowHandle, reader: &mut R, buffer: &mut [u8], js_func: &str ) -> bool where
	R: Read
{
	match reader.read( buffer ) {
		Err(e) => eprintln!("Command error: {}", e),
		Ok( read ) => {
			if read == 0 {
				return false;
			}

			// Convert to string
			let string = String::from_utf8_lossy( &buffer[0..read] );
			// Sanitize string input for JavaScript
			let js_string = serde_json::to_string( &*string ).unwrap();

			bw.exec_js(&(js_func.to_owned() + "(" + js_string.as_str() + ")"));
		}
	}

	true
}

fn main() {
	let application = Application::initialize();
	let runtime = application.start();

	let exit_code = runtime.run_async( |app| async move {

		let working_dir = env::current_dir().unwrap();
		let mut html_file = working_dir.clone();
		html_file.push( "resources/terminal.html" );

		let bw = BrowserWindowBuilder::new( Source::File( html_file ) )
			.title("Terminal Example")
			.async_handler(|handle, cmd, args| async move {

				match cmd.as_str() {
					"exec" => {
						let cmd_line = &args[0];

						execute_command( handle, cmd_line ).await;
					},
					other => {
						eprintln!("Received unsupported command: {}", other);
					}
				}
			})
			.build( app ).await;

		// Initialize the script with our working directory.
		// Make sure that it is initializes whether document has been loaded already or not.
		let working_dir_js = serde_json::to_string( working_dir.to_str().unwrap() ).expect("Invalid working directory characters!");
		match bw.eval_js( format!("initialize({})", &working_dir_js ).as_str() ).await {
			Err(_) => bw.exec_js( format!("window.onload = () => {{ initialize({}) }}", &working_dir_js ).as_str() ),
			Ok(_) => {}
		};
	} );

	// Return exit code
	exit( exit_code );
}