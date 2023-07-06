#ifndef _HELPER_H
#define _HELPER_H
#define _POSIX_C_SOURCE 200112L
#include <getopt.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <wayland-server-core.h>

#define MAX(a, b) ((a > b) ? (a) : (b))
#define MIN(a, b) ((a < b) ? (a) : (b))
#define TITLEBAR_HEIGHT 8 /* TODO: Get this from the theme */
#include <wlr/version.h>
#define WLR_CHECK_VERSION(major, minor, micro) (WLR_VERSION_NUM >= ((major << 16) | (minor << 8) | (micro)))

#include <wlr/backend.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_input_device.h>
#include <wlr/types/wlr_keyboard.h>
#include <wlr/types/wlr_matrix.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_pointer.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/render/allocator.h>
#include <wlr/types/wlr_gamma_control_v1.h>
#include <wlr/types/wlr_idle_notify_v1.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_screencopy_v1.h>
#include <wlr/types/wlr_subcompositor.h>
#include <wlr/types/wlr_xdg_output_v1.h>
#include <wlr/util/log.h>
#include <xkbcommon/xkbcommon.h>

struct bk_server{
    struct wl_display *wl_display;
    struct wl_event_loop *wl_event_loop;
    struct wlr_backend *backend;
    struct wlr_renderer *renderer;
    struct wlr_output_layout *output_layout;
    struct wlr_compositor *compositor;
    struct wlr_xdg_shell *xdg_shell;
    struct wlr_seat *seat;
    struct wlr_idle_notifier_v1 *idle_notifier;
    struct wlr_scene *scene;
    struct wlr_allocator *allocator;
    struct wlr_subcompositor *subcompositor;

    struct wl_listener new_output;
    struct wl_listener new_xdg_surface;
    struct wl_listener new_input;
    struct wl_listener request_set_selection;

    struct wl_list outputs;
    struct wl_list views;
    struct wl_list keyboards;
};

struct bk_output {
    struct wlr_output *wlr_output;
    struct bk_server *server;
    struct timespec last_frame;
    struct wl_listener destroy;
    struct wl_list link;
    struct wl_listener frame;
    struct wlr_box geometry;
};

struct bk_view {
	struct wl_list link;
	struct bk_server *server;
	struct wlr_xdg_surface *xdg_surface;

	struct wl_listener map;
	struct wl_listener unmap;
	struct wl_listener destroy;
	struct wl_listener request_move;
	struct wl_listener request_resize;
	bool mapped;
	int x, y;
};

struct bk_keyboard {
	struct wl_list link;
	struct bk_server *server;
	struct wlr_keyboard *keyboard;

	struct wl_listener destroy;
	struct wl_listener modifiers;
	struct wl_listener key;
};

struct render_data {
	struct wlr_output *output;
	struct wlr_renderer *renderer;
	struct bk_view *view;
	struct timespec *when;
};

void output_destroy_notify(struct wl_listener *, void *);
void render_surface(struct wlr_surface *surface,int sx, int sy, void *data);
void output_frame_notify(struct wl_listener *listener, void *data);
void focus_view(struct bk_view *view, struct wlr_surface *surface);
void keyboard_handle_destroy(struct wl_listener *listener, void *data);
void keyboard_handle_modifiers(struct wl_listener *listener, void *data);
bool handle_keybinding(struct bk_server *server, xkb_keysym_t sym);
void keyboard_handle_key(struct wl_listener *listener, void *data);
void server_new_keyboard(struct bk_server *server,struct wlr_input_device *device);
void server_new_input(struct wl_listener *listener, void *data);
void new_output_notify(struct wl_listener *listener, void *data);
void xdg_surface_map(struct wl_listener *listener, void *data) ;
void xdg_surface_unmap(struct wl_listener *listener, void *data);
void xdg_surface_destroy(struct wl_listener *listener, void *data);
void server_new_xdg_surface(struct wl_listener *listener, void *data);
#endif /* helper.h */