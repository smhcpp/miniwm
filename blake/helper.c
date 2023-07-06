#include "helper.h"
void output_destroy_notify(struct wl_listener *listener, void *data) {
    struct bk_output *output = wl_container_of(listener, output, destroy);
    wl_list_remove(&output->link);
    wl_list_remove(&output->destroy.link);
    wl_list_remove(&output->frame.link);
    free(output);
}

void render_surface(struct wlr_surface *surface,int sx, int sy, void *data) {
	/* This function is called for every surface that needs to be rendered. */
	struct render_data *rdata = data;
	struct bk_view *view = rdata->view;
	struct wlr_output *output = rdata->output;
	struct wlr_texture *texture = wlr_surface_get_texture(surface);
	if (texture == NULL) {
		return;
	}

	double ox = 0, oy = 0;
	wlr_output_layout_output_coords(view->server->output_layout, output, &ox, &oy);
	ox += view->x + sx, oy += view->y + sy;
	struct wlr_box box = {
		.x = ox * output->scale,
		.y = oy * output->scale,
		.width = surface->current.width * output->scale,
		.height = surface->current.height * output->scale,
	};

	float matrix[9];
	enum wl_output_transform transform = wlr_output_transform_invert(surface->current.transform);
	wlr_matrix_project_box(matrix, &box, transform, 0,output->transform_matrix);
	wlr_render_texture_with_matrix(rdata->renderer, texture, matrix, 1);
	wlr_surface_send_frame_done(surface, rdata->when);
}

void output_frame_notify(struct wl_listener *listener, void *data) {
	/* This function is called every time an output is ready to display a frame,
	 * generally at the output's refresh rate (e.g. 60Hz). */
	struct bk_output *output = wl_container_of(listener, output, frame);
	struct wlr_scene *scene = output->server->scene;
	struct wlr_scene_output *scene_output = wlr_scene_get_scene_output(scene, output->wlr_output);

	wlr_output_layout_get_box(output->server->output_layout,output->wlr_output, &output->geometry);
    if (!wlr_output_test(output->wlr_output)) {
        wlr_output_rollback(output->wlr_output);
	}


	/* Render the scene if needed and commit the output */
	wlr_scene_output_commit(scene_output);

	/* This lets the client know that we've displayed that frame and it can
	 * prepare another one now if it likes. */
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	wlr_scene_output_send_frame_done(scene_output, &now);
}

void focus_view(struct bk_view *view, struct wlr_surface *surface) {
	/* Note: this function only deals with keyboard focus. */
	if (view == NULL) {
		return;
	}
	struct bk_server *server = view->server;
	struct wlr_seat *seat = server->seat;
	struct wlr_surface *prev_surface = seat->keyboard_state.focused_surface;
	if (prev_surface == surface) {
		/* Don't re-focus an already focused surface. */
		return;
	}
	if (prev_surface) {
		/*
		 * Deactivate the previously focused surface. This lets the client know
		 * it no longer has focus and the client will repaint accordingly, e.g.
		 * stop displaying a caret.
		 */
        //struct wlr_xdg_toplevel *previous
		//struct wlr_xdg_surface *previous = wlr_xdg_surface_from_wlr_surface(seat->keyboard_state.focused_surface);
		//wlr_xdg_toplevel_set_activated(previous, false);
	}
	struct wlr_keyboard *keyboard = wlr_seat_get_keyboard(seat);
	/* Move the view to the front */
	wl_list_remove(&view->link);
	wl_list_insert(&server->views, &view->link);
	/* Activate the new surface */
	//wlr_xdg_toplevel_set_activated(view->xdg_surface, true);
	/*
	 * Tell the seat to have the keyboard enter this surface. wlroots will keep
	 * track of this and automatically send key events to the appropriate
	 * clients without additional work on your part.
	 */
	wlr_seat_keyboard_notify_enter(seat, view->xdg_surface->surface,keyboard->keycodes, keyboard->num_keycodes, &keyboard->modifiers);
}

void keyboard_handle_destroy(struct wl_listener *listener, void *data) {
	/* This event is raised by the keyboard base wlr_input_device to signal
	 * the destruction of the wlr_keyboard. It will no longer receive events
	 * and should be destroyed.
	 */
	struct bk_keyboard *keyboard = wl_container_of(listener, keyboard, destroy);
	wl_list_remove(&keyboard->destroy.link);
	wl_list_remove(&keyboard->key.link);
	wl_list_remove(&keyboard->modifiers.link);
	wl_list_remove(&keyboard->link);
	free(keyboard);
}

void keyboard_handle_modifiers(struct wl_listener *listener, void *data) {
	struct bk_keyboard *keyboard =wl_container_of(listener, keyboard, modifiers);
	wlr_seat_set_keyboard(keyboard->server->seat, keyboard->keyboard);
	wlr_seat_keyboard_notify_modifiers(keyboard->server->seat,&keyboard->keyboard->modifiers);
}

bool handle_keybinding(struct bk_server *server, xkb_keysym_t sym) {
	/*
	 * Here we handle compositor keybindings. This is when the compositor is
	 * processing keys, rather than passing them on to the client for its own
	 * processing.
	 *
	 * This function assumes Alt is held down.
	 */
	switch (sym) {
	case XKB_KEY_Escape:
		wl_display_terminate(server->wl_display);
		break;
	case XKB_KEY_F1:
		/* Cycle to the next view */
		if (wl_list_length(&server->views) < 2) {
			break;
		}
		struct bk_view *current_view = wl_container_of(server->views.next, current_view, link);
		struct bk_view *next_view = wl_container_of(current_view->link.next, next_view, link);
		focus_view(next_view, next_view->xdg_surface->surface);
		/* Move the previous view to the end of the list */
		wl_list_remove(&current_view->link);
		wl_list_insert(server->views.prev, &current_view->link);
		break;
	default:
		return false;
	}
	return true;
}

void keyboard_handle_key(struct wl_listener *listener, void *data) {
	/* This event is raised when a key is pressed or released. */
	struct bk_keyboard *keyboard =wl_container_of(listener, keyboard, key);
	struct bk_server *server = keyboard->server;
	struct wlr_keyboard_key_event *event = data;
	struct wlr_seat *seat = server->seat;

	/* Translate libinput keycode -> xkbcommon */
	uint32_t keycode = event->keycode + 8;
	/* Get a list of keysyms based on the keymap for this keyboard */
	const xkb_keysym_t *syms;
	int nsyms = xkb_state_key_get_syms(keyboard->keyboard->xkb_state, keycode, &syms);

	bool handled = false;
	uint32_t modifiers = wlr_keyboard_get_modifiers(keyboard->keyboard);
	if ((modifiers & WLR_MODIFIER_ALT) && event->state == WL_KEYBOARD_KEY_STATE_PRESSED) {
		/* If alt is held down and this button was _pressed_, we attempt to
		 * process it as a compositor keybinding. */
		for (int i = 0; i < nsyms; i++) {
			handled = handle_keybinding(server, syms[i]);
		}
	}
    if (!handled) {
		/* Otherwise, we pass it along to the client. */
		wlr_seat_set_keyboard(seat, keyboard->keyboard);
		wlr_seat_keyboard_notify_key(seat, event->time_msec,event->keycode, event->state);
	}
	wlr_idle_notifier_v1_notify_activity(server->idle_notifier, seat);
}

void server_new_keyboard(struct bk_server *server,struct wlr_input_device *device) {
	struct bk_keyboard *keyboard =calloc(1, sizeof(struct bk_keyboard));
	keyboard->server = server;
	keyboard->keyboard = wlr_keyboard_from_input_device(device);

	/* We need to prepare an XKB keymap and assign it to the keyboard. */
	/* here goes keyboard config part!*/
	struct xkb_context *context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
	struct xkb_keymap *keymap = xkb_keymap_new_from_names(context, NULL,XKB_KEYMAP_COMPILE_NO_FLAGS);

	wlr_keyboard_set_keymap(keyboard->keyboard, keymap);
	xkb_keymap_unref(keymap);
	xkb_context_unref(context);
	wlr_keyboard_set_repeat_info(keyboard->keyboard, 25, 600);


	/* Here we set up listeners for keyboard events. */
	keyboard->destroy.notify = keyboard_handle_destroy;
	wl_signal_add(&device->events.destroy, &keyboard->destroy);
	keyboard->modifiers.notify = keyboard_handle_modifiers;
	wl_signal_add(&keyboard->keyboard->events.modifiers, &keyboard->modifiers);
	keyboard->key.notify = keyboard_handle_key;
	wl_signal_add(&keyboard->keyboard->events.key, &keyboard->key);

	wlr_seat_set_keyboard(server->seat, keyboard->keyboard);

	/* And add the keyboard to our list of keyboards */
	wl_list_insert(&server->keyboards, &keyboard->link);
}

void server_new_input(struct wl_listener *listener, void *data) {
	/* This event is raised by the backend when a new input device becomes
	 * available. */
	struct bk_server *server =wl_container_of(listener, server, new_input);
	struct wlr_input_device *device = data;
    wlr_log(WLR_INFO, "%s: %s", "New pointer detected", device->name);

	switch (device->type) {
	case WLR_INPUT_DEVICE_KEYBOARD:
		server_new_keyboard(server, device);
	    break;
	case WLR_INPUT_DEVICE_POINTER:
	//	server_new_pointer(server, device);
		break;
	default:
		break;
	}
	/* We need to let the wlr_seat know what our capabilities are, which is
	 * communiciated to the client. In Blake we always have a cursor, even if
	 * there are no pointer devices, so we always include that capability. */
	uint32_t caps = WL_SEAT_CAPABILITY_POINTER;
	if (!wl_list_empty(&server->keyboards)) {
		caps |= WL_SEAT_CAPABILITY_KEYBOARD;
	}
	wlr_seat_set_capabilities(server->seat, caps);
}

void new_output_notify(struct wl_listener *listener, void *data) {
    struct bk_server *server = wl_container_of(listener, server, new_output);
    struct wlr_output *wlr_output = data;
    wlr_log(WLR_INFO, "%s: %s", "New output device detected", wlr_output->name);
    wlr_output_init_render(wlr_output, server->allocator, server->renderer);
    // some backends do not have modes.
    if (!wl_list_empty(&wlr_output->modes)) {
		struct wlr_output_mode *mode = wlr_output_preferred_mode(wlr_output);
		wlr_output_set_mode(wlr_output, mode);
		wlr_output_enable(wlr_output, true);
		if (!wlr_output_commit(wlr_output)) {
			return;
		}
	}

    struct bk_output *output = calloc(1, sizeof(struct bk_output));
    //clock_gettime(CLOCK_MONOTONIC, &output->last_frame);
    output->wlr_output = wlr_output;
    output->server = server;

    output->frame.notify = output_frame_notify;
    wl_signal_add(&wlr_output->events.frame, &output->frame);
    wl_list_insert(&server->outputs, &output->link);

    //output->destroy.notify = output_destroy_notify;
    //wl_signal_add(&wlr_output->events.destroy, &output->destroy);
    
    wlr_output_layout_add_auto(server->output_layout, wlr_output);
    //wlr_output_create_global(wlr_output);
}

void xdg_surface_map(struct wl_listener *listener, void *data) {
	/* Called when the surface is mapped, or ready to display on-screen. */
	struct bk_view *view = wl_container_of(listener, view, map);
	view->mapped = true;
	focus_view(view, view->xdg_surface->surface);
}

void xdg_surface_unmap(struct wl_listener *listener, void *data) {
	/* Called when the surface is unmapped, and should no longer be shown. */
	struct bk_view *view = wl_container_of(listener, view, unmap);
	view->mapped = false;
}

void xdg_surface_destroy(struct wl_listener *listener, void *data) {
	/* Called when the surface is destroyed and should never be shown again. */
	struct bk_view *view = wl_container_of(listener, view, destroy);
	wl_list_remove(&view->link);
	free(view);
}

void server_new_xdg_surface(struct wl_listener *listener, void *data) {
    struct bk_server *server = wl_container_of(listener, server, new_xdg_surface);
	struct wlr_xdg_surface *xdg_surface = data;
	if (xdg_surface->role != WLR_XDG_SURFACE_ROLE_TOPLEVEL) {
		return;
	}

    struct bk_view *view = calloc(1, sizeof(struct bk_view));
	view->server = server;
	view->xdg_surface = xdg_surface;

    view->map.notify = xdg_surface_map;
	wl_signal_add(&xdg_surface->events.map, &view->map);
	view->unmap.notify = xdg_surface_unmap;
	wl_signal_add(&xdg_surface->events.unmap, &view->unmap);
	view->destroy.notify = xdg_surface_destroy;
	wl_signal_add(&xdg_surface->events.destroy, &view->destroy);
/*
    struct wlr_xdg_toplevel *toplevel = xdg_surface->toplevel;
	view->request_move.notify = xdg_toplevel_request_move;
	wl_signal_add(&toplevel->events.request_move, &view->request_move);
	view->request_resize.notify = xdg_toplevel_request_resize;
	wl_signal_add(&toplevel->events.request_resize, &view->request_resize);
*/
    wl_list_insert(&server->views, &view->link);
}