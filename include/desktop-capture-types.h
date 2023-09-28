_Pragma("once")

#if defined(_WIN32)
typedef struct _desktop_frame_t {
	DXGI_OUTDUPL_FRAME_INFO frame_info;
	ID3D11Texture2D*        frame;
}desktop_frame_t;
#endif