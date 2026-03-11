#include <windows.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <glad/glad.h>

#include <util.hpp>
#include <vec.hpp>
#include <qsort_worker.hpp>
#include <bubble_sort_worker.hpp>
#include <randomizer_worker.hpp>

#pragma comment(lib, "opengl32.lib")

bool g_running = true;
Arena g_arena;
HDC g_hdc;
uint32 g_vao;
uint32 g_vert_vbo, g_sizes_vbo, g_pivots_vbo, g_height_coefs_vbo, g_color_indices_vbo;
uint32 g_colors_ubo;
uint32 g_shader_program;

const char* g_vertex_shader_source = R"(
#version 330 core

layout (location = 0) in vec2 vert;
layout (location = 1) in vec2 pivot;
layout (location = 2) in vec2 size;
layout (location = 3) in float height_coef;
layout (location = 4) in int in_color_index;

flat out int color_index;

void main()
{
    vec2 norm_size = size / 2.0;
    vec2 res = vert * norm_size;
    res.y *= height_coef;
    res += vec2(pivot.x, norm_size.y * height_coef + pivot.y); 

    gl_Position = vec4(res, 0.0, 1.0);
    color_index = in_color_index;
}
)";

const char* g_fragment_shader_source = R"(
#version 330 core

flat in int color_index;

layout(std140) uniform ColorData {
    vec4 colors[4];
};

out vec4 FragColor;

void main()
{
   FragColor = colors[color_index];
}
)";

float g_vertices[] = {
    -1.0f, -1.0f,
    -1.0f, 1.0f,
    1.0f, -1.0f,
    -1.0f, 1.0f,
    1.0f, 1.0f,
    1.0f, -1.0f
};

float g_colors[] = {
    1.0f, 1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 0.0f, 1.0f,
    1.0f, 0.7f, 0.0f, 1.0f,
    0.0f, 1.0f, 0.0f, 1.0f,
};

Vec<Worker*> g_workers;
Vec<SortWorker*> g_sort_workers;
Vec<float> g_height_coefs;
Vec<int32> g_color_indices;

void PrepareRender()
{
    Vec<float> pivots;
    Vec<float> sizes;
    for(int32 i = 0; i < g_sort_workers.GetCount(); ++i)
    {
        SortWorker* sort_worker = g_sort_workers[i];

        float height = sort_worker->rect_top_y - sort_worker->rect_bot_y;
        float width = (sort_worker->rect_right_x - sort_worker->rect_left_x) / float(sort_worker->arr.GetCount());
        float pos_x = sort_worker->rect_left_x + width / 2.0f;
        for(int32 i = 0; i < sort_worker->arr.GetCount(); ++i)
        {
            pivots.PushBack(&g_arena, pos_x);
            pivots.PushBack(&g_arena, sort_worker->rect_bot_y);

            sizes.PushBack(&g_arena, width * 0.5f);
            sizes.PushBack(&g_arena, height);

            g_height_coefs.PushBack(&g_arena, sort_worker->arr[i] / float(sort_worker->arr_max));
            g_color_indices.PushBack(&g_arena, 0);

            pos_x += width;
        }
    }

    uint32 vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &g_vertex_shader_source, NULL);
    glCompileShader(vertex_shader);

    int32 success;
    char info_log[512];
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
    if(!success)
    {
        glGetShaderInfoLog(vertex_shader, sizeof(info_log), NULL, info_log);
        OutputDebugStringA(info_log);
        assert(false);
    }

    uint32 fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &g_fragment_shader_source, NULL);
    glCompileShader(fragment_shader);

    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
    if(!success)
    {
        glGetShaderInfoLog(fragment_shader, sizeof(info_log), NULL, info_log);
        OutputDebugStringA(info_log);
        assert(false);
    }

    g_shader_program = glCreateProgram();
    glAttachShader(g_shader_program, vertex_shader);
    glAttachShader(g_shader_program, fragment_shader);
    glLinkProgram(g_shader_program);

    glGetProgramiv(g_shader_program, GL_LINK_STATUS, &success);
    if(!success) {
        glGetProgramInfoLog(g_shader_program, sizeof(info_log), NULL, info_log);
        OutputDebugStringA(info_log);
        assert(false);
    }
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    glGenVertexArrays(1, &g_vao);
    glGenBuffers(1, &g_vert_vbo);
    glGenBuffers(1, &g_pivots_vbo);
    glGenBuffers(1, &g_sizes_vbo);
    glGenBuffers(1, &g_height_coefs_vbo);
    glGenBuffers(1, &g_color_indices_vbo);
    glGenBuffers(1, &g_colors_ubo);

    glBindVertexArray(g_vao);

    glBindBuffer(GL_ARRAY_BUFFER, g_vert_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertices), g_vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, g_pivots_vbo);
    glBufferData(GL_ARRAY_BUFFER, pivots.GetCount() * sizeof(float), pivots.RawData(), GL_STATIC_DRAW);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glVertexAttribDivisor(1, 1);
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, g_sizes_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizes.GetCount() * sizeof(float), sizes.RawData(), GL_STATIC_DRAW);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glVertexAttribDivisor(2, 1);
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, g_height_coefs_vbo);
    glBufferData(GL_ARRAY_BUFFER, g_height_coefs.GetCount() * sizeof(float), g_height_coefs.RawData(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(float), (void*)0);
    glVertexAttribDivisor(3, 1);
    glEnableVertexAttribArray(3);

    glBindBuffer(GL_ARRAY_BUFFER, g_color_indices_vbo);
    glBufferData(GL_ARRAY_BUFFER, g_color_indices.GetCount() * sizeof(int), g_color_indices.RawData(), GL_DYNAMIC_DRAW);
    glVertexAttribIPointer(4, 1, GL_INT, sizeof(int), (void*)0);
    glVertexAttribDivisor(4, 1);
    glEnableVertexAttribArray(4);

    glBindBuffer(GL_UNIFORM_BUFFER, g_colors_ubo);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(g_colors), g_colors, GL_STATIC_DRAW);
    glUniformBlockBinding(g_shader_program, glGetUniformBlockIndex(g_shader_program, "ColorData"), 0);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, g_colors_ubo);

    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
}

void InitGL(HWND h_wnd)
{
    int32 ret;
    PIXELFORMATDESCRIPTOR pfd = {
        sizeof(PIXELFORMATDESCRIPTOR),    // size of this pfd  
        1,                                // version number  
        PFD_DRAW_TO_WINDOW |              // support window  
        PFD_SUPPORT_OPENGL |              // support OpenGL  
        PFD_DOUBLEBUFFER,                 // double buffered  
        PFD_TYPE_RGBA,                    // RGBA type  
        24,                               // 24-bit color depth  
        0, 0, 0, 0, 0, 0,                 // color bits ignored  
        0,                                // no alpha buffer  
        0,                                // shift bit ignored  
        0,                                // no accumulation buffer  
        0, 0, 0, 0,                       // accum bits ignored  
        32,                               // 32-bit z-buffer      
        0,                                // no stencil buffer  
        0,                                // no auxiliary buffer  
        PFD_MAIN_PLANE,                   // main layer  
        0,                                // reserved  
        0, 0, 0                           // layer masks ignored  
    };

    g_hdc = GetDC(h_wnd);
    assert(g_hdc != NULL);
    int32 pixel_format = ChoosePixelFormat(g_hdc, &pfd);
    assert(pixel_format != 0);
    ret = SetPixelFormat(g_hdc, pixel_format, &pfd);
    assert(ret);

    HGLRC hglrc = wglCreateContext(g_hdc);
    assert(hglrc != NULL);
    ret = wglMakeCurrent(g_hdc, hglrc);
    assert(ret);

    ret = gladLoadGL();
    assert(ret != 0);
}

void Render()
{
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    for(int32 i = 0; i < g_sort_workers.GetCount(); ++i)
    {
        SortWorker* sort_worker = g_sort_workers[i];

        EnterCriticalSection(&sort_worker->cs);
        for(int32 j = 0; j < sort_worker->arr.GetCount(); ++j)
        {
            int32 gl_ind = sort_worker->gl_buff_offset + j;
            g_height_coefs[gl_ind] = sort_worker->arr[j] / float(sort_worker->arr_max);
            switch(sort_worker->states[j])
            {
            case State::IDLE:
                g_color_indices[gl_ind] = 0;
                break;
            case State::COMPARED:
                g_color_indices[gl_ind] = 1;
                break;
            case State::INSERT_POS:
                g_color_indices[gl_ind] = 2;
                break;
            case State::SORTED:
                g_color_indices[gl_ind] = 3;
                break;
            default:
                g_color_indices[gl_ind] = 0;
            }
        }
        LeaveCriticalSection(&sort_worker->cs);
    }

    glBindVertexArray(g_vao);

    glBindBuffer(GL_ARRAY_BUFFER, g_height_coefs_vbo);
    glBufferData(GL_ARRAY_BUFFER, g_height_coefs.GetCount() * sizeof(float), g_height_coefs.RawData(), GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, g_color_indices_vbo);
    glBufferData(GL_ARRAY_BUFFER, g_color_indices.GetCount() * sizeof(int32), g_color_indices.RawData(), GL_DYNAMIC_DRAW);

    glUseProgram(g_shader_program);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, g_height_coefs.GetCount());

    SwapBuffers(g_hdc);
}

DWORD WINAPI ThreadProc(
    LPVOID lp_parameter
)
{
    Worker* worker = (Worker*)lp_parameter;
    worker->Work();
    return 0;
}

LRESULT CALLBACK WindowProc(
    HWND h_wnd,
    UINT message,
    WPARAM w_param,
    LPARAM l_param
)
{
    switch(message)
    {
    case WM_CREATE:
    {
        InitGL(h_wnd);
        PrepareRender();
        break;
    }

    case WM_SIZE:
    {
        int32 width = LOWORD(l_param);
        int32 height = HIWORD(l_param);
        glViewport(0, 0, width, height);
        break;
    }

    case WM_GETMINMAXINFO:
    {
        LPMINMAXINFO lpmmi = (LPMINMAXINFO)l_param;
        lpmmi->ptMinTrackSize.x = 400;
        lpmmi->ptMinTrackSize.y = 300;
        break;
    }

    case WM_PAINT:
    {
        Render();
        break;
    }

    case WM_KEYDOWN:
    {
        if(w_param == VK_ESCAPE)
        {
            g_running = false;
            PostQuitMessage(0);
        }
        break;
    }

    case WM_CLOSE:
    {
        g_running = false;
        PostQuitMessage(0);
        break;
    }

    default:
    {
        return DefWindowProcW(h_wnd, message, w_param, l_param);
    }
    }
    return 0;
}

RandomizerWorker g_randomizer_worker;
BubbleSortWorker g_bubble_sort_worker;
QsortWorker g_qsort_worker;

int32 WINAPI wWinMain(
    HINSTANCE h_instance,
    HINSTANCE h_prev_instance,
    LPWSTR lp_cmd_line,
    int32 n_cmd_show
)
{
    int64 ret;

    srand((uint32)time(NULL));

    uint32 memory_capacity = 1 * 1024 * 1024;
    void* memory = VirtualAlloc(NULL, memory_capacity, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    assert(memory != NULL);
    g_arena.base = memory;
    g_arena.used = 0;
    g_arena.capacity = memory_capacity;

    g_workers.PushBack(&g_arena, &g_randomizer_worker);
    InitializeCriticalSection(&g_randomizer_worker.cs);
    InitializeConditionVariable(&g_randomizer_worker.ready_to_be_filled_cv);

    g_sort_workers.PushBack(&g_arena, &g_bubble_sort_worker);
    g_bubble_sort_worker.rect_left_x = -0.9f;
    g_bubble_sort_worker.rect_right_x = -0.1f;
    g_bubble_sort_worker.rect_bot_y = -0.9f;
    g_bubble_sort_worker.rect_top_y = 0.9f;

    g_sort_workers.PushBack(&g_arena, &g_qsort_worker);
    g_qsort_worker.rect_left_x = 0.1f;
    g_qsort_worker.rect_right_x = 0.9f;
    g_qsort_worker.rect_bot_y = -0.9f;
    g_qsort_worker.rect_top_y = 0.9f;

    int32 gl_offset = 0;
    for(int32 i = 0; i < g_sort_workers.GetCount(); ++i)
    {
        SortWorker* sort_worker = g_sort_workers[i];

        InitializeCriticalSection(&sort_worker->cs);
        InitializeConditionVariable(&sort_worker->filled_by_randomizer_cv);
        int32 count = 45 + rand() % 11;
        sort_worker->arr.Init(&g_arena, count);
        sort_worker->states.Init(&g_arena, count);
        FillRandomValues(sort_worker);
        sort_worker->gl_buff_offset = gl_offset;
        sort_worker->arena = &g_arena;
        sort_worker->randomizer_worker = &g_randomizer_worker;
        sort_worker->filled_by_randomizer = false;
        g_workers.PushBack(&g_arena, sort_worker);

        gl_offset += count;
    }

    HANDLE h_thread;
    h_thread = GetCurrentThread();
    ret = SetThreadAffinityMask(h_thread, 0x1LL);
    assert(ret != 0);
    for(int32 i = 0; i < g_workers.GetCount(); ++i)
    {
        DWORD thread_id;
        h_thread = CreateThread(
            NULL,
            0,
            ThreadProc,
            g_workers[i],
            0,
            &thread_id
        );
        assert(h_thread != NULL);

        ret = SetThreadAffinityMask(h_thread, 0x1LL << (i + 1));
        assert(ret != 0);
    }

    const wchar class_name[] = L"SortVisWndClass";
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = h_instance;
    wc.lpszClassName = class_name;
    wc.style = CS_OWNDC;

    ret = RegisterClassW(&wc);
    assert(ret != 0);

    HWND h_wnd = CreateWindowExW(
        0,
        class_name,
        L"Bubble Sort vs Quick Sort - Press ESC to exit",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1600, 900,
        NULL,
        NULL,
        h_instance,
        NULL
    );
    assert(h_wnd != NULL);

    ShowWindow(h_wnd, n_cmd_show);

    MSG msg = {};
    while(g_running)
    {
        if(PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    VirtualFree(memory, 0, MEM_RELEASE);
    return (int32)msg.wParam;
}
