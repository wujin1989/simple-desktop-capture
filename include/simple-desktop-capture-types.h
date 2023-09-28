_Pragma("once")

#if defined(_WIN32)
#include <dxgi1_5.h>
#include <d3d11.h>

typedef struct simple_desktop_capture_frame_s {
	DXGI_OUTDUPL_FRAME_INFO frame_info;
	ID3D11Texture2D*        frame;
}simple_desktop_capture_frame_t;
#endif

typedef enum simple_desktop_capture_type_e {
	SIMPLE_DESKTOP_CAPTURE_DXGI
}simple_desktop_capture_type_t;