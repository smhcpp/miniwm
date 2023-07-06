#include "helper.h"

int main(int argc, char **argv) {
    wlr_log_init(WLR_DEBUG, NULL);
    //wlr_log(WLR_INFO, "%s", "Successfully started server");

    struct bk_server server;
    server.wl_display = wl_display_create();
    server.wl_event_loop = wl_display_get_event_loop(server.wl_display);
    server.backend = wlr_backend_autocreate(server.wl_display);
    server.renderer= wlr_renderer_autocreate(server.backend);
    wlr_renderer_init_wl_display(server.renderer, server.wl_display);

// new
    server.allocator = wlr_allocator_autocreate(server.backend, server.renderer);
	if (server.allocator == NULL) {
		wlr_log(WLR_ERROR, "%s", "Failed to create allocator");
		return false;
	}
//

    server.compositor = wlr_compositor_create(server.wl_display,server.renderer);
    wlr_data_device_manager_create(server.wl_display);

    server.output_layout = wlr_output_layout_create();
// new
    server.scene = wlr_scene_create();
    server.subcompositor = wlr_subcompositor_create(server.wl_display);
	wlr_scene_attach_output_layout(server.scene, server.output_layout);
    server.idle_notifier = wlr_idle_notifier_v1_create(server.wl_display);
//
    wl_list_init(&server.outputs);
    server.new_output.notify = new_output_notify;
    wl_signal_add(&server.backend->events.new_output, &server.new_output);

    wl_list_init(&server.views);
	server.xdg_shell = wlr_xdg_shell_create(server.wl_display,3);
	server.new_xdg_surface.notify = server_new_xdg_surface;
	wl_signal_add(&server.xdg_shell->events.new_surface,&server.new_xdg_surface);

    /* Here we should do cursor initialization!*/
 
    wl_list_init(&server.keyboards);
    server.new_input.notify = server_new_input;
    wl_signal_add(&server.backend->events.new_input, &server.new_input);
    server.seat = wlr_seat_create(server.wl_display, "seat0");

    //server.request_cursor.notify = seat_request_cursor;
	//wl_signal_add(&server.seat->events.request_set_cursor,&server.request_cursor);

    //server.request_set_selection.notify = seat_request_set_selection;
	//wl_signal_add(&server.seat->events.request_set_selection,&server.request_set_selection);

    const char *socket = wl_display_add_socket_auto(server.wl_display);
    if (!socket) {
		wlr_backend_destroy(server.backend);
        wlr_log(WLR_INFO, "%s","Failed on getting right socket for the connection!");
		return 1;
	}

    if (!wlr_backend_start(server.backend)) {
        wlr_log(WLR_INFO, "%s", "Failed to start the backend.");
        wlr_backend_destroy(server.backend);
        wl_display_destroy(server.wl_display);
        return 1;
    }
// new
    //init_xdg_decoration(server);
	//init_layer_shell(server);
	//init_xdg_shell(server);
    //wlr_idle_create(server.wl_display);
//
    wlr_log(WLR_INFO, "%s: WAYLAND_DISPLAY=%s", "Running Wayland compositor on Wayland display", socket);
    setenv("WAYLAND_DISPLAY", socket, true);
    wl_display_run(server.wl_display);
    wl_display_destroy(server.wl_display);
    
    return 0;
}