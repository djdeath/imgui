// ImGui - standalone example application for Glfw + OpenGL 3, using programmable pipeline
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.

#include <imgui.h>
#include "imgui_impl_gdk3_cogl.h"
#include <stdio.h>

#if defined(GDK_WINDOWING_WAYLAND)
#include <gdk/gdkwayland.h>
#endif
#if defined(GDK_WINDOWING_X11)
#include <gdk/gdkx.h>
#endif
#if defined(GDK_WINDOWING_WIN32)
#include <gdk/win32/gdkwin32.h>
#endif

#if defined(COGL_HAS_XLIB_SUPPORT)
#include <cogl/cogl-xlib.h>
#endif

#define EVENT_MASK \
    (GDK_STRUCTURE_MASK |            \
     GDK_FOCUS_CHANGE_MASK |	     \
     GDK_EXPOSURE_MASK |             \
     GDK_PROPERTY_CHANGE_MASK |	     \
     GDK_ENTER_NOTIFY_MASK |	     \
     GDK_LEAVE_NOTIFY_MASK |	     \
     GDK_KEY_PRESS_MASK |            \
     GDK_KEY_RELEASE_MASK |	     \
     GDK_BUTTON_PRESS_MASK |	     \
     GDK_BUTTON_RELEASE_MASK |	     \
     GDK_POINTER_MOTION_MASK |       \
     GDK_TOUCH_MASK |                \
     GDK_SCROLL_MASK)


struct backend_callbacks;

struct context
{
    GdkWindow *window;
    CoglOnscreen *onscreen;
    const struct backend_callbacks *callbacks;
};

static void repaint_window(GdkFrameClock *clock, gpointer user_data)
{
    static bool show_test_window = true;
    static bool show_another_window = false;
    static bool show_metrics_window = false;
    static ImVec4 clear_color = ImColor(114, 144, 154);
    struct context *ctx = (struct context *) user_data;

    ImGui_ImplGdk3Cogl_NewFrame();

    // 1. Show a simple window
    // Tip: if we don't call ImGui::Begin()/ImGui::End() the widgets appears in a window automatically called "Debug"
    {
        static float f = 0.0f;
        ImGui::Text("Hello, world!");
        ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
        ImGui::ColorEdit3("clear color", (float*)&clear_color);
        if (ImGui::Button("Test Window")) show_test_window ^= 1;
        if (ImGui::Button("Metrics Window")) show_metrics_window ^= 1;
        if (ImGui::Button("Another Window")) show_another_window ^= 1;
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    }

    // 2. Show another simple window, this time using an explicit Begin/End pair
    if (show_another_window)
    {
        ImGui::SetNextWindowSize(ImVec2(200,100), ImGuiCond_FirstUseEver);
        ImGui::Begin("Another Window", &show_another_window);
        ImGui::Text("Hello");
        ImGui::End();
    }

    {
        ImGui::Begin("Plop Plop 42");
        ImGui::Text("Hello, world2!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
        ImGui::End();
    }

    // 3. Show the ImGui test window. Most of the sample code is in ImGui::ShowTestWindow()
    if (show_test_window)
    {
        ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiCond_FirstUseEver);
        ImGui::ShowTestWindow(&show_test_window);
    }

    if (show_metrics_window)
    {
        ImGui::SetNextWindowPos(ImVec2(650, 200), ImGuiCond_FirstUseEver);
        ImGui::ShowMetricsWindow(&show_metrics_window);
    }


    // Rendering
    CoglFramebuffer *fb = COGL_FRAMEBUFFER(ctx->onscreen);
    cogl_framebuffer_set_viewport(fb,
                                  0, 0,
                                  cogl_framebuffer_get_width(fb),
                                  cogl_framebuffer_get_height(fb));

    cogl_framebuffer_clear4f(fb, COGL_BUFFER_BIT_COLOR | COGL_BUFFER_BIT_DEPTH,
                             clear_color.x, clear_color.y, clear_color.z, 1.0);
    ImGui::Render();
    cogl_onscreen_swap_buffers(ctx->onscreen);
}

struct backend_callbacks
{
    CoglWinsysID winsys;
    void (*set_window)(CoglOnscreen* onscreen, GdkWindow *window);
    void (*handle_event)(GdkEvent *event, const struct context *context);
};

static cairo_region_t *get_window_region(GdkWindow *window)
{
    cairo_rectangle_int_t rect;
    gdk_window_get_geometry(window,
                            &rect.x, &rect.y,
                            &rect.width, &rect.height);
    return cairo_region_create_rectangle (&rect);
}

#if defined(GDK_WINDOWING_WAYLAND) && defined(COGL_HAS_EGL_PLATFORM_WAYLAND_SUPPORT)
static void wayland_set_window(CoglOnscreen* onscreen, GdkWindow *window)
{
    cogl_wayland_onscreen_set_foreign_surface(onscreen,
                                              gdk_wayland_window_get_wl_surface(GDK_WAYLAND_WINDOW(window)));

}
#endif

#if defined(GDK_WINDOWING_X11) && defined(COGL_HAS_XLIB_SUPPORT)
static void x11_window_update_foreign_event_mask(CoglOnscreen *onscreen,
                                                 guint32 event_mask,
                                                 void *user_data)
{
    GdkWindow *window = GDK_WINDOW(user_data);

    /* we assume that a GDK event mask is bitwise compatible with X11
       event masks */
    gdk_window_set_events(window, (GdkEventMask) (event_mask | EVENT_MASK));
}

static GdkFilterReturn
cogl_gdk_filter (GdkXEvent  *xevent,
                 GdkEvent   *event,
                 gpointer    data)
{
  CoglOnscreen *onscreen = (CoglOnscreen *) data;
  CoglRenderer *renderer =
      cogl_context_get_renderer(cogl_framebuffer_get_context(COGL_FRAMEBUFFER(onscreen)));
  CoglFilterReturn ret;

  ret = cogl_xlib_renderer_handle_event (renderer, (XEvent *) xevent);
  switch (ret)
    {
    case COGL_FILTER_REMOVE:
      return GDK_FILTER_REMOVE;

    case COGL_FILTER_CONTINUE:
    default:
      return GDK_FILTER_CONTINUE;
    }

  return GDK_FILTER_CONTINUE;
}

static void x11_set_window(CoglOnscreen* onscreen, GdkWindow *window)
{
    cogl_x11_onscreen_set_foreign_window_xid(onscreen,
                                             GDK_WINDOW_XID(window),
                                             x11_window_update_foreign_event_mask,
                                             window);
    gdk_window_add_filter(window, cogl_gdk_filter, onscreen);
}

static void x11_handle_event(GdkEvent *event, const struct context *context)
{
    if (gdk_event_get_event_type(event) == GDK_CONFIGURE)
    {
        cairo_region_t *region = get_window_region(context->window);
        gdk_window_set_opaque_region(context->window, region);
        cairo_region_destroy(region);
    }
}
#endif

#if defined(GDK_WINDOWING_WIN32) && defined(COGL_HAS_WIN32_SUPPORT)
static void win32_set_window(CoglOnscreen* onscreen, GdkWindow *window)
{
    cogl_win32_onscreen_set_foreign_window(onscreen,
                                           gdk_win32_window_get_handle(window));
}
#endif

static void noop_set_window(CoglOnscreen* onscreen, GdkWindow *window)
{
    /* NOOP */
}

static void noop_handle_event(GdkEvent *event, const struct context *context)
{
    /* NOOP */
}

static const struct backend_callbacks *
get_backend_callbacks(GdkWindow *window)
{
#if defined(GDK_WINDOWING_WAYLAND) && defined(COGL_HAS_EGL_PLATFORM_WAYLAND_SUPPORT)
    if (GDK_IS_WAYLAND_WINDOW(window)) {
        static const struct backend_callbacks cbs =
            {
                //COGL_WINSYS_ID_EGL_WAYLAND,
                COGL_WINSYS_ID_GLX,
                wayland_set_window,
                noop_handle_event,
            };
        return &cbs;
    }
#endif
#if defined(GDK_WINDOWING_X11) && defined(COGL_HAS_XLIB_SUPPORT)
    if (GDK_IS_X11_WINDOW(window)) {
        static const struct backend_callbacks cbs =
            {
                COGL_WINSYS_ID_EGL_XLIB,
                x11_set_window,
                x11_handle_event,
            };
        return &cbs;
    }
#endif
#if defined(GDK_WINDOWING_WIN32) && defined(COGL_HAS_WIN32_SUPPORT)
    if (GDK_IS_WIN32_WINDOW(window)) {
        static const struct backend_callbacks cbs =
            {
                COGL_WINSYS_ID_WGL,
                win32_set_window,
                noop_handle_event,
            };
        return &cbs;
    }
#endif

    static const struct backend_callbacks cbs =
    {
        COGL_WINSYS_ID_EGL_WAYLAND,
        noop_set_window,
        noop_handle_event,
    };
    return &cbs;
}

static void gdk_event_handler(GdkEvent *event, gpointer data)
{
    const struct context *context = (const struct context *) data;

    // Apply any preprocessing required by the different window
    // systems.
    context->callbacks->handle_event(event, context);

    // Then forward the event to ImGui's Gdk backend.
    ImGui_ImplGdk3Cogl_HandleEvent(event);
}

int main(int argc, char** argv)
{
    gdk_init(&argc, &argv);

    GdkWindowAttr win_attrs;
    memset(&win_attrs, 0, sizeof(win_attrs));
    win_attrs.title = "ImGui Gdk3 Cogl example";
    win_attrs.width = 1280;
    win_attrs.height = 720;
    win_attrs.wclass = GDK_INPUT_OUTPUT;
    win_attrs.window_type = GDK_WINDOW_TOPLEVEL;
    win_attrs.visual =
        gdk_screen_get_rgba_visual(gdk_display_get_default_screen(gdk_display_get_default()));
    win_attrs.event_mask = EVENT_MASK;

    struct context ctx;
    ctx.window = gdk_window_new(NULL, &win_attrs, GDK_WA_TITLE | GDK_WA_WMCLASS | GDK_WA_VISUAL);
    ctx.callbacks = get_backend_callbacks(ctx.window);

    int scale = gdk_window_get_scale_factor(ctx.window);
    int width, height;
    gdk_window_get_geometry(ctx.window, NULL, NULL, &width, &height);

    cairo_region_t *region = get_window_region(ctx.window);
    gdk_window_set_opaque_region(ctx.window, region);
    cairo_region_destroy(region);

    gdk_event_handler_set(gdk_event_handler, &ctx, NULL);

    gdk_window_ensure_native(ctx.window);

    CoglRenderer *renderer = cogl_renderer_new();
    cogl_renderer_set_winsys_id(renderer, ctx.callbacks->winsys);
    cogl_xlib_renderer_set_foreign_display(renderer,
                                           gdk_x11_display_get_xdisplay(gdk_window_get_display(ctx.window)));

    CoglContext *context = cogl_context_new(cogl_display_new(renderer, NULL), NULL);
    ctx.onscreen = cogl_onscreen_new(context, width * scale, height * scale);
    cogl_onscreen_set_resizable(ctx.onscreen, TRUE);
    cogl_object_unref(renderer);
    ctx.callbacks->set_window(ctx.onscreen, ctx.window);

    gdk_window_show(ctx.window);

    // Setup ImGui binding
    ImGui_ImplGdk3Cogl_Init(ctx.window, COGL_FRAMEBUFFER(ctx.onscreen), true);

    // Load Fonts
    // (there is a default font, this is only if you want to change it. see extra_fonts/README.txt for more details)
    //ImGuiIO& io = ImGui::GetIO();
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("../../extra_fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../../extra_fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../extra_fonts/ProggyClean.ttf", 13.0f);
    //io.Fonts->AddFontFromFileTTF("../../extra_fonts/ProggyTiny.ttf", 10.0f);
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());

    // Main loop
    GdkFrameClock *clock = gdk_window_get_frame_clock(ctx.window);
    g_signal_connect(clock, "paint", G_CALLBACK(repaint_window), &ctx);

    gdk_frame_clock_request_phase(clock, GDK_FRAME_CLOCK_PHASE_PAINT);

    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(loop);

    return 0;
}
