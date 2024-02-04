#define UNICODE
#define NOMINMAX            1
#define WIN32_LEAN_AND_MEAN 1
#define WIN32_MEAN_AND_LEAN 1
#define VC_EXTRALEAN        1
#include <windows.h>
#undef NOMINMAX
#undef WIN32_LEAN_AND_MEAN
#undef WIN32_MEAN_AND_LEAN
#undef VC_EXTRALEAN
#undef far
#undef near

#include "link.h"

#define NORMAL_WINDOW_STYLES (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE)
#define FULLSCREEN_WINDOW_STYLES (WS_VISIBLE)

struct
{
  void* backbuffer;
  HWND window;

  bool is_fullscreen;
  u8 backbuffer_dim_idx;
  u8 normal_dim_idx;
  u8 monitor_max_dim_idx;
  HMONITOR monitor;
  MONITORINFO monitor_info;
  WINDOWPLACEMENT normal_placement;
} PlatformState = {0};

// NOTE: Resolutions must be sorted by the area of their bounding box
V2S SupportedResolutions[] = {
  { 640,   360},
  {1280,   720},
  {1920,  1080},
  {2560,  1440},
};

void
RefitToMonitor()
{
  PlatformState.monitor = MonitorFromWindow(PlatformState.window, MONITOR_DEFAULTTOPRIMARY);

  PlatformState.monitor_info = (MONITORINFO){ .cbSize = sizeof(MONITORINFO) };
  BOOL monitor_info_ok = GetMonitorInfoW(PlatformState.monitor, &PlatformState.monitor_info);
  ASSERT(monitor_info_ok); // TODO

  V2S monitor_res = V2S(PlatformState.monitor_info.rcMonitor.right  - PlatformState.monitor_info.rcMonitor.left,
                        PlatformState.monitor_info.rcMonitor.bottom - PlatformState.monitor_info.rcMonitor.top);

  PlatformState.monitor_max_dim_idx = ARRAY_SIZE(SupportedResolutions)-1;
  for (; PlatformState.monitor_max_dim_idx > 0; --PlatformState.monitor_max_dim_idx)
  {
    if (SupportedResolutions[PlatformState.monitor_max_dim_idx].x <= monitor_res.x &&
        SupportedResolutions[PlatformState.monitor_max_dim_idx].y <= monitor_res.y)
    {
      break;
    }
  }

  PlatformState.backbuffer_dim_idx = MIN(PlatformState.backbuffer_dim_idx, PlatformState.monitor_max_dim_idx);
  PlatformState.normal_dim_idx     = MIN(PlatformState.normal_dim_idx,     PlatformState.monitor_max_dim_idx);

  if (PlatformState.is_fullscreen)
  {
    SetWindowLongW(PlatformState.window, GWL_STYLE, FULLSCREEN_WINDOW_STYLES);
    SetWindowPos(PlatformState.window, HWND_TOP, PlatformState.monitor_info.rcMonitor.left,
                 PlatformState.monitor_info.rcMonitor.top,
                 PlatformState.monitor_info.rcMonitor.right  - PlatformState.monitor_info.rcMonitor.left,
                 PlatformState.monitor_info.rcMonitor.bottom - PlatformState.monitor_info.rcMonitor.top,
                 SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
  }
  else
  {
    V2S normal_dim = SupportedResolutions[PlatformState.normal_dim_idx];

    SetWindowLongW(PlatformState.window, GWL_STYLE, NORMAL_WINDOW_STYLES);
    SetWindowPos(PlatformState.window, 0, 0, 0, normal_dim.x, normal_dim.y,
                 SWP_NOMOVE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
  }
}

void
ToggleFullscreen()
{
  if (PlatformState.is_fullscreen)
  {
    PlatformState.is_fullscreen      = false;
    PlatformState.backbuffer_dim_idx = PlatformState.normal_dim_idx;

    SetWindowPlacement(PlatformState.window, &PlatformState.normal_placement);
  }
  else
  {
    PlatformState.normal_placement = (WINDOWPLACEMENT){ .length = sizeof(WINDOWPLACEMENT) };
    GetWindowPlacement(PlatformState.window, &PlatformState.normal_placement);

    PlatformState.is_fullscreen      = true;
    PlatformState.backbuffer_dim_idx = PlatformState.monitor_max_dim_idx;
  }

  RefitToMonitor();
}

LRESULT
Wndproc(HWND window, UINT msg, WPARAM wparam, LPARAM lparam)
{
  LRESULT result;

  if (msg == WM_CLOSE)
  {
    DestroyWindow(window);
    PostQuitMessage(0);
    result = 0;
  }
  else if (msg == WM_EXITSIZEMOVE)
  {
    RefitToMonitor();
    result = 0;
  }
  else if (msg == WM_PAINT)
  {
    PAINTSTRUCT paint = {0};
    HDC dc = BeginPaint(window, &paint);

    // TODO

    EndPaint(window, &paint);
    result = 0;
  }
  else result = DefWindowProcW(window, msg, wparam, lparam);

  return result;
}

int
wWinMain(HINSTANCE instance, HINSTANCE pre_instance, PWSTR cmd_line, int show_cmd)
{
  int result = 0;

  { /// Setup PlatformState
    V2S max_res = SupportedResolutions[ARRAY_SIZE(SupportedResolutions)-1];
    PlatformState.backbuffer = VirtualAlloc(0, max_res.x*max_res.y*4, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    ASSERT(PlatformState.backbuffer != 0); // TODO

    PlatformState.normal_dim_idx     = ARRAY_SIZE(SupportedResolutions)-1;
    PlatformState.backbuffer_dim_idx = ARRAY_SIZE(SupportedResolutions)-1;

    WNDCLASSW window_class = {
        .style         = CS_OWNDC,
        .lpfnWndProc   = Wndproc,
        .cbClsExtra    = 0,
        .cbWndExtra    = 0,
        .hInstance     = instance,
        .hIcon         = 0,
        .hCursor       = 0,
        .hbrBackground = 0,
        .lpszMenuName  = 0,
        .lpszClassName = L"P2PUNCH",
    };

    ATOM window_atom = RegisterClassW(&window_class);
    ASSERT(window_atom != 0); // TODO

    PlatformState.window = CreateWindowW(L"P2PUNCH", L"P2PUNCH", NORMAL_WINDOW_STYLES & ~WS_VISIBLE,
                                         CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                         0, 0, 0, 0);
    ASSERT(PlatformState.window != 0); // TODO
    
    PlatformState.normal_placement = (WINDOWPLACEMENT){ .length = sizeof(WINDOWPLACEMENT) };
    GetWindowPlacement(PlatformState.window, &PlatformState.normal_placement);

    RefitToMonitor();
  }

  ShowWindow(PlatformState.window, SW_SHOW);

  for (;;)
  {
    for (MSG msg; PeekMessageW(&msg, 0, 0, 0, PM_REMOVE); )
    {
      if (msg.message == WM_QUIT)
      {
        result = (int)msg.wParam;
        goto end; // TODO
      }
      else if (msg.message == WM_SYSKEYDOWN && msg.wParam == VK_RETURN && ((msg.wParam>>30)&1) == 0 ||
               msg.message == WM_KEYDOWN    && msg.wParam == VK_F11    && ((msg.wParam>>30)&1) == 0)
      {
        ToggleFullscreen();
      }
      else Wndproc(PlatformState.window, msg.message, msg.wParam, msg.lParam);
    }

    HDC dc = GetDC(PlatformState.window);
    V2S backbuffer_dim = SupportedResolutions[PlatformState.backbuffer_dim_idx];

    BITMAPINFO bitmap_info = {
      .bmiHeader = {
        .biSize          = sizeof(BITMAPINFOHEADER),
        .biWidth         = backbuffer_dim.x,
        .biHeight        = -backbuffer_dim.y,
        .biPlanes        = 1,
        .biBitCount      = 32,
        .biCompression   = BI_RGB,
      },
    };

    RECT window_rect;
    GetWindowRect(PlatformState.window, &window_rect);

    V2S window_dim = V2S(window_rect.right  - window_rect.left, window_rect.bottom - window_rect.top);
    ASSERT(backbuffer_dim.y <= window_dim.y);

    SetDIBitsToDevice(dc, 0, 0, backbuffer_dim.x, backbuffer_dim.y, 0, 0, 0, backbuffer_dim.y,
                     PlatformState.backbuffer, &bitmap_info, DIB_RGB_COLORS);

    ReleaseDC(PlatformState.window, dc);
  }

  end:;
  return result;
}
