// ImGui GLFW binding with OpenGL3 + shaders
// In this binding, ImTextureID is used to store an OpenGL 'GLuint' texture identifier. Read the FAQ about ImTextureID in imgui.cpp.

// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you use this binding you'll need to call 4 functions: ImGui_ImplXXXX_Init(), ImGui_ImplXXXX_NewFrame(), ImGui::Render() and ImGui_ImplXXXX_Shutdown().
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

#include <stdio.h>

#include <imgui.h>
#include "imgui_impl_gdk3_cogl.h"

// Data
static GdkWindow*       g_Window = NULL;
static CoglFramebuffer* g_Framebuffer = NULL;
static guint64          g_Time = 0;
static bool             g_MousePressed[5] = { false, false, false, false, false };
static ImVec2           g_MousePosition = ImVec2(-1, -1);
static float            g_MouseWheel = 0.0f;
static CoglPipeline*    g_Pipeline = NULL;
static int              g_NumRedraws = 0;

// This is the main rendering function that you have to implement and provide to ImGui (via setting up 'RenderDrawListsFn' in the ImGuiIO structure)
// If text or lines are blurry when integrating ImGui in your engine:
// - in your Render function, try translating your projection matrix by (0.5f,0.5f) or (0.375f,0.375f)
void ImGui_ImplGdk3Cogl_RenderDrawLists(ImDrawData* draw_data)
{
    // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
    ImGuiIO& io = ImGui::GetIO();
    int fb_width = (int)(io.DisplaySize.x * io.DisplayFramebufferScale.x);
    int fb_height = (int)(io.DisplaySize.y * io.DisplayFramebufferScale.y);
    if (fb_width == 0 || fb_height == 0)
        return;
    draw_data->ScaleClipRects(io.DisplayFramebufferScale);

    cogl_framebuffer_orthographic(g_Framebuffer, 0, 0,
                                  io.DisplaySize.x, io.DisplaySize.y,
                                  -1, 1);

    CoglContext *context = cogl_framebuffer_get_context(g_Framebuffer);
    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        int idx_buffer_offset = 0;

        CoglAttributeBuffer *vertices =
            cogl_attribute_buffer_new(context,
                                      cmd_list->VtxBuffer.Size * sizeof(ImDrawVert),
                                      cmd_list->VtxBuffer.Data);

#define OFFSETOF(TYPE, ELEMENT) ((size_t)&(((TYPE *)0)->ELEMENT))
        CoglAttribute *attrs[3] = {
            cogl_attribute_new(vertices, "cogl_position_in",
                               sizeof(ImDrawVert), OFFSETOF(ImDrawVert, pos),
                               2, COGL_ATTRIBUTE_TYPE_FLOAT),
            cogl_attribute_new(vertices, "cogl_tex_coord0_in",
                               sizeof(ImDrawVert), OFFSETOF(ImDrawVert, uv),
                               2, COGL_ATTRIBUTE_TYPE_FLOAT),
            cogl_attribute_new(vertices, "cogl_color_in",
                               sizeof(ImDrawVert), OFFSETOF(ImDrawVert, col),
                               4, COGL_ATTRIBUTE_TYPE_UNSIGNED_BYTE)
        };
#undef OFFSETOF

        CoglPrimitive *primitive =
            cogl_primitive_new_with_attributes(COGL_VERTICES_MODE_TRIANGLES,
                                               cmd_list->VtxBuffer.Size,
                                               attrs, 3);

        CoglIndices *indices = cogl_indices_new(context,
                                                sizeof(ImDrawIdx) == 2 ?
                                                COGL_INDICES_TYPE_UNSIGNED_SHORT :
                                                COGL_INDICES_TYPE_UNSIGNED_INT,
                                                cmd_list->IdxBuffer.Data,
                                                cmd_list->IdxBuffer.Size);

        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];

            cogl_indices_set_offset(indices, sizeof(ImDrawIdx) * idx_buffer_offset);
            cogl_primitive_set_indices(primitive, indices, pcmd->ElemCount);

            if (pcmd->UserCallback)
            {
                pcmd->UserCallback(cmd_list, pcmd);
            }
            else
            {
                cogl_pipeline_set_layer_texture(g_Pipeline, 0,
                                                pcmd->TextureId ? COGL_TEXTURE(pcmd->TextureId) : NULL);

                cogl_framebuffer_push_scissor_clip(g_Framebuffer,
                                                   pcmd->ClipRect.x,
                                                   pcmd->ClipRect.y,
                                                   pcmd->ClipRect.z - pcmd->ClipRect.x,
                                                   pcmd->ClipRect.w - pcmd->ClipRect.y);
                cogl_primitive_draw(primitive, g_Framebuffer, g_Pipeline);
                cogl_framebuffer_pop_clip(g_Framebuffer);
            }
            idx_buffer_offset += pcmd->ElemCount;
        }

        for (int i = 0; i < 3; i++)
            cogl_object_unref(attrs[i]);
        cogl_object_unref(primitive);
        cogl_object_unref(vertices);
        cogl_object_unref(indices);
    }

    cogl_framebuffer_draw_rectangle(g_Framebuffer, g_Pipeline, 0, 0, 10, 10);
}

static const char* ImGui_ImplGdk3Cogl_GetClipboardText(void* user_data)
{
    return "";
}

static void ImGui_ImplGdk3Cogl_SetClipboardText(void* user_data, const char* text)
{
    // glfwSetClipboardString((GdkWindow*)user_data, text);
}

bool ImGui_ImplGdk3Cogl_CreateFontsTexture()
{
    // Build texture atlas
    ImGuiIO& io = ImGui::GetIO();
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);   // Load as RGBA 32-bits (75% of the memory is wasted, but default font is so small) because it is more likely to be compatible with user's existing shaders. If your ImTextureId represent a higher-level concept than just a GL texture id, consider calling GetTexDataAsAlpha8() instead to save on GPU memory.

    g_clear_pointer(&io.Fonts->TexID, cogl_object_unref);
    io.Fonts->TexID =
        cogl_texture_2d_new_from_data(cogl_framebuffer_get_context(g_Framebuffer),
                                      width, height,
                                      COGL_PIXEL_FORMAT_RGBA_8888,
                                      width * 4, pixels, NULL);

    return true;
}

bool ImGui_ImplGdk3Cogl_CreateDeviceObjects()
{
    g_Pipeline = cogl_pipeline_new(cogl_framebuffer_get_context(g_Framebuffer));

    CoglError *error = NULL;
    if (!cogl_pipeline_set_blend(g_Pipeline,
                                 "RGB = ADD(SRC_COLOR*(SRC_COLOR[A]), DST_COLOR*(1-SRC_COLOR[A]))"
                                 "A = ADD(SRC_COLOR[A], DST_COLOR*(1-SRC_COLOR[A]))",
                                 &error))
    {
        g_warning("Blending: %s", error->message);
        g_error_free(error);
        return false;
    }
    cogl_pipeline_set_cull_face_mode(g_Pipeline, COGL_PIPELINE_CULL_FACE_MODE_NONE);

    CoglDepthState depth_state;

    cogl_depth_state_init(&depth_state);
    cogl_depth_state_set_test_enabled(&depth_state, FALSE);
    if (!cogl_pipeline_set_depth_state(g_Pipeline, &depth_state, &error))
    {
        g_warning("Depth: %s", error->message);
        g_error_free(error);
        return false;
    }

    ImGui_ImplGdk3Cogl_CreateFontsTexture();

    /* Disable depth buffer since we're not using it. */
    cogl_framebuffer_set_depth_write_enabled(g_Framebuffer, FALSE);

    return true;
}

void    ImGui_ImplGdk3Cogl_InvalidateDeviceObjects()
{
    g_clear_pointer(&g_Pipeline, cogl_object_unref);
    g_clear_pointer(&ImGui::GetIO().Fonts->TexID, cogl_object_unref);
}

void   ImGui_ImplGdk3Cogl_HandleEvent(GdkEvent *event)
{
    ImGuiIO& io = ImGui::GetIO();

    GdkEventType type = gdk_event_get_event_type(event);
    switch (type)
    {
    case GDK_MOTION_NOTIFY:
    {
        gdouble x = 0.0f, y = 0.0f;
        if (gdk_event_get_coords(event, &x, &y))
            g_MousePosition = ImVec2(x, y);
        break;
    }
    case GDK_BUTTON_PRESS:
    case GDK_BUTTON_RELEASE:
    {
        guint button = 0;
        if (gdk_event_get_button(event, &button) && button > 0 && button <= 5)
        {
            if (type == GDK_BUTTON_PRESS)
                g_MousePressed[button - 1] = true;
        }
        break;
    }
    case GDK_SCROLL:
    {
        GdkScrollDirection direction = GDK_SCROLL_UP;
        if (gdk_event_get_scroll_direction(event, &direction))
            g_MouseWheel = direction == GDK_SCROLL_UP ? 1.0 : -1.0;
        break;
    }
    case GDK_KEY_PRESS:
    case GDK_KEY_RELEASE:
    {
        GdkModifierType modifiers;
        if (gdk_event_get_state(event, &modifiers))
        {
            io.KeyCtrl = (modifiers & GDK_CONTROL_MASK) != 0;
            io.KeyShift = (modifiers & GDK_SHIFT_MASK) != 0;
            io.KeyAlt = (modifiers & GDK_META_MASK) != 0;
            io.KeySuper = (modifiers & GDK_SUPER_MASK) != 0;
        }

        guint keyval;
        guint32 unicode;
        if (gdk_event_get_keyval(event, &keyval))
        {
            static const struct
            {
                enum ImGuiKey_ imgui;
                guint gdk;
            } gdk_key_to_imgui_key[ImGuiKey_COUNT] =
                  {
                      { ImGuiKey_Tab, GDK_KEY_Tab },
                      { ImGuiKey_LeftArrow, GDK_KEY_Left },
                      { ImGuiKey_RightArrow, GDK_KEY_Right },
                      { ImGuiKey_UpArrow, GDK_KEY_Up },
                      { ImGuiKey_DownArrow, GDK_KEY_Down },
                      { ImGuiKey_PageUp, GDK_KEY_Page_Up },
                      { ImGuiKey_PageDown, GDK_KEY_Page_Down },
                      { ImGuiKey_Home, GDK_KEY_Home },
                      { ImGuiKey_End, GDK_KEY_End },
                      { ImGuiKey_Delete, GDK_KEY_Delete },
                      { ImGuiKey_Backspace, GDK_KEY_BackSpace },
                      { ImGuiKey_Enter, GDK_KEY_Return },
                      { ImGuiKey_Escape, GDK_KEY_Escape },
                      { ImGuiKey_A, GDK_KEY_a },
                      { ImGuiKey_C, GDK_KEY_c },
                      { ImGuiKey_V, GDK_KEY_v },
                      { ImGuiKey_X, GDK_KEY_x },
                      { ImGuiKey_Y, GDK_KEY_y },
                      { ImGuiKey_Z, GDK_KEY_z },
                  };
            for (int i = 0; i < ImGuiKey_COUNT; i++)
            {
                if (keyval == gdk_key_to_imgui_key[i].gdk)
                    io.KeysDown[gdk_key_to_imgui_key[i].imgui] = type == GDK_KEY_PRESS;
            }

            if ((unicode = gdk_keyval_to_unicode(keyval)) != 0)
            {
                char *unicode_chars = (char *) &unicode;
                io.AddInputCharactersUTF8(unicode_chars);
            }
        }
        break;
    }
    default:
        break;
    }

    // We trigger 2 subsequent redraws for each event because of the
    // way some ImGui widgets work. For example a Popup menu will only
    // appear a frame after a click happened.
    g_NumRedraws = 2;

    GdkFrameClock *clock = gdk_window_get_frame_clock(g_Window);
    gdk_frame_clock_request_phase(clock, GDK_FRAME_CLOCK_PHASE_PAINT);
}

bool    ImGui_ImplGdk3Cogl_Init(GdkWindow* window, CoglFramebuffer *framebuffer, bool install_callbacks)
{
    g_clear_pointer(&g_Window, g_object_unref);
    g_Window = GDK_WINDOW(g_object_ref(window));

    g_clear_pointer(&g_Framebuffer, cogl_object_unref);
    g_Framebuffer = COGL_FRAMEBUFFER(cogl_object_ref(framebuffer));

    ImGuiIO& io = ImGui::GetIO();
    for (int i = 0; i < ImGuiKey_COUNT; i++)
    {
        io.KeyMap[i] = i;
    }

    io.RenderDrawListsFn = ImGui_ImplGdk3Cogl_RenderDrawLists;
    io.SetClipboardTextFn = ImGui_ImplGdk3Cogl_SetClipboardText;
    io.GetClipboardTextFn = ImGui_ImplGdk3Cogl_GetClipboardText;
    io.ClipboardUserData = g_Window;

    return true;
}

void ImGui_ImplGdk3Cogl_Shutdown()
{
    ImGui_ImplGdk3Cogl_InvalidateDeviceObjects();
    ImGui::Shutdown();
}

void ImGui_ImplGdk3Cogl_NewFrame()
{
    if (!g_Pipeline)
        ImGui_ImplGdk3Cogl_CreateDeviceObjects();

    if (g_NumRedraws > 0)
    {
        GdkFrameClock *clock = gdk_window_get_frame_clock(g_Window);
        gdk_frame_clock_request_phase(clock, GDK_FRAME_CLOCK_PHASE_PAINT);
        g_NumRedraws--;
    }

    ImGuiIO& io = ImGui::GetIO();

    // Setup display size (every frame to accommodate for window resizing)
    int w, h;
    gdk_window_get_geometry(g_Window, NULL, NULL, &w, &h);
    io.DisplaySize = ImVec2((float)w, (float)h);
    int scale_factor = gdk_window_get_scale_factor(g_Window);
    io.DisplayFramebufferScale = ImVec2(scale_factor, scale_factor);

    // Setup time step
    guint64 current_time =  g_get_monotonic_time();
    io.DeltaTime = g_Time > 0 ? ((float)(current_time - g_Time) / 1000000) : (float)(1.0f/60.0f);
    g_Time = current_time;

    // Setup inputs
    if (gdk_window_get_state(g_Window) & GDK_WINDOW_STATE_FOCUSED)
    {
        io.MousePos = g_MousePosition;   // Mouse position in screen coordinates (set to -1,-1 if no mouse / on another screen, etc.)
    }
    else
    {
        io.MousePos = ImVec2(-1,-1);
    }

    GdkSeat *seat = gdk_display_get_default_seat(gdk_window_get_display(g_Window));
    GdkDevice *pointer = gdk_seat_get_pointer(seat);
    GdkModifierType modifiers;
    gdk_device_get_state(pointer, g_Window, NULL, &modifiers);

    for (int i = 0; i < 3; i++)
    {
        io.MouseDown[i] = g_MousePressed[i] || (modifiers & (GDK_BUTTON1_MASK << i)) != 0;
        g_MousePressed[i] = false;
    }

    io.MouseWheel = g_MouseWheel;
    g_MouseWheel = 0.0f;

    // Hide OS mouse cursor if ImGui is drawing it
    GdkDisplay *display = gdk_window_get_display(g_Window);
    GdkCursor *cursor =
      gdk_cursor_new_from_name(display, io.MouseDrawCursor ? "none" : "default");
    gdk_window_set_cursor(g_Window, cursor);
    g_object_unref(cursor);

    // Start the frame
    ImGui::NewFrame();
}
