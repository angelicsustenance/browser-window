use std::sync::{atomic::AtomicI32, Arc};

use gtk::prelude::{GtkWindowExt, WidgetExt, WindowExtManual};

use super::{WindowExt, WindowOptions};
use crate::{core::application::ApplicationImpl, prelude::*};

#[derive(Clone)]
pub struct WindowImpl(pub gtk::Window);

impl WindowImpl {
	pub fn new(
		app: ApplicationImpl, parent: Self, title: &str, width: Option<u32>, height: Option<u32>,
		options: &WindowOptions, mut user_data: *mut (),
	) -> Self {
		let mut builder = gtk::Window::builder()
			.application(&app.inner)
			.parent(&parent.0)
			.destroy_with_parent(true)
			.title(title);

		builder = builder
			.border_width(options.borders as _)
			.resizable(options.resizable);

		if let Some(w) = width {
			builder = builder.width_request(w as _);
		}
		if let Some(h) = height {
			builder = builder.height_request(h as _);
		}

		let inner = builder.build();
		Self(inner)
	}
}

impl Default for WindowImpl {
	fn default() -> Self { Self(gtk::Window::new(gtk::WindowType::Toplevel)) }
}

impl WindowExt for WindowImpl {
	fn app(&self) -> ApplicationImpl {
		ApplicationImpl {
			inner: self.0.application().unwrap(),
			exit_code: Arc::new(AtomicI32::new(0)),
		}
	}

	fn destroy(&self) { self.0.close(); }

	fn drop(&self) {}

	fn get_content_dimensions(&self) -> Dims2D {
		unimplemented!();
	}

	fn get_opacity(&self) -> u8 { 0 }

	fn get_position(&self) -> Pos2D {
		let (x, y) = self.0.position();
		Pos2D {
			x: x as _,
			y: y as _,
		}
	}

	fn get_title(&self) -> String {
		self.0
			.title()
			.map(|g| g.to_string())
			.unwrap_or(String::new())
	}

	fn get_window_dimensions(&self) -> Dims2D {
		let (w, h) = self.0.size();
		Dims2D {
			width: w as _,
			height: h as _,
		}
	}

	fn hide(&self) { self.0.hide(); }

	fn set_content_dimensions(&self, dimensions: Dims2D) {}

	fn set_opacity(&self, opacity: u8) {}

	fn set_position(&self, position: Pos2D) {
		unimplemented!();
	}

	fn set_title(&self, title: &str) { self.0.set_title(title); }

	fn set_window_dimensions(&self, dimensions: Dims2D) {
		self.0
			.set_size_request(dimensions.width as _, dimensions.height as _);
	}

	fn show(&self) { self.0.show_all(); }
}
