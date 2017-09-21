// ImGui - standalone example application for Glfw + OpenGL 3, using programmable pipeline
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.

#include <imgui.h>
#include "imgui_impl_gdk3_cogl.h"
#include <stdio.h>

static void repaint_window(GdkFrameClock *clock, gpointer user_data)
{
    static bool show_test_window = true;
    static bool show_another_window = false;
    static bool show_metrics_window = false;
    static ImVec4 clear_color = ImColor(114, 144, 154);
    CoglOnscreen *onscreen = (CoglOnscreen *) user_data;

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
    CoglFramebuffer *fb = COGL_FRAMEBUFFER(onscreen);
    cogl_framebuffer_set_viewport(fb,
                                  0, 0,
                                  cogl_framebuffer_get_width(fb),
                                  cogl_framebuffer_get_height(fb));

    cogl_framebuffer_clear4f(fb, COGL_BUFFER_BIT_COLOR | COGL_BUFFER_BIT_DEPTH,
                             clear_color.x, clear_color.y, clear_color.z, 1.0);
    ImGui::Render();
    cogl_onscreen_swap_buffers(onscreen);
}

static void handle_gdk_event(GdkEvent *event, void *data)
{
    switch (gdk_event_get_event_type(event)) {
    case GDK_DELETE:
    case GDK_DESTROY:
    {
        GMainLoop *loop = (GMainLoop *) data;
        ImGui_ImplGdk3Cogl_Shutdown();
        g_main_loop_quit(loop);
        break;
    }
    default:
        ImGui_ImplGdk3Cogl_HandleEvent(event);
        break;
    }
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

    GdkWindow *window =
        gdk_window_new(NULL, &win_attrs, GDK_WA_TITLE | GDK_WA_WMCLASS | GDK_WA_VISUAL);
    CoglOnscreen *onscreen = ImGui_ImplGdk3Cogl_Init(window, false);

    GMainLoop *loop = g_main_loop_new(NULL, FALSE);

    gdk_event_handler_set(handle_gdk_event, loop, NULL);
    gdk_window_show(window);

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
    GdkFrameClock *clock = gdk_window_get_frame_clock(window);
    g_signal_connect(clock, "paint", G_CALLBACK(repaint_window), onscreen);

    gdk_frame_clock_request_phase(clock, GDK_FRAME_CLOCK_PHASE_PAINT);

    g_main_loop_run(loop);

    return 0;
}
