#define COBJMACROS
#include <dxgi1_5.h>
#include <d3d11.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>

#pragma warning(disable:4996) 
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "d3d11.lib")

#define VENDOR_ID_NVIDIA        0x10de
#define VENDOR_ID_AMD           0x1002
#define VENDOR_ID_MICROSOFT     0x1414
#define VENDOR_ID_INTEL         0x8086

#define SAFE_RELEASE(u) \
    do { if ((u) != NULL) (u)->lpVtbl->Release(u); (u) = NULL; } while(0)

int fps = 0;

typedef enum _dxgi_status {
	CAPTURER_STATUS_RECREATE_DUP,
	CAPTURER_STATUS_TIMEOUT,
	CAPTURER_STATUS_NO_UPDATE,
	CAPTURER_STATUS_FAILURE,
	CAPTURER_STATUS_SUCCESS
}dxgi_status;

typedef struct _d3d11_device_t {
	ID3D11Device*         dev;
	ID3D11DeviceContext*  ctx;
	uint32_t              vid; /* vendor-id */
}d3d11_device_t;

typedef struct _dxgi_dup_t {
	IDXGIOutputDuplication* dup;
	uint32_t                width;
	uint32_t                height;
}dxgi_dup_t;

typedef struct _dxgi_frame_t {
	DXGI_OUTDUPL_FRAME_INFO    frame_info;
	ID3D11Texture2D*           frame;
}dxgi_frame_t;

extern bool d3d11_create_device(d3d11_device_t* p_d3d11_device);
extern bool dxgi_create_duplicator(d3d11_device_t* p_d3d11_device, dxgi_dup_t* p_dxgi_dup);
extern int  dxgi_capture_frame(dxgi_dup_t* p_dxgi_dup, dxgi_frame_t* p_dxgi_frame);

#define USEC                        (1000000UL)
#define MSEC                        (1000UL)

uint64_t cdk_timespec_get(void) {

	struct timespec tsc;
	if (!timespec_get(&tsc, TIME_UTC)) { return 0; }

	return (tsc.tv_sec * MSEC + tsc.tv_nsec / USEC);
}

bool dxgi_create_duplicator(d3d11_device_t* p_d3d11_device, dxgi_dup_t* p_dxgi_dup) {

	IDXGIDevice*   p_dxgi_device;
	IDXGIAdapter*  p_dxgi_adapter;
	IDXGIOutput*   p_dxgi_output;
	IDXGIOutput1*  p_dxgi_output1;
	HRESULT        hr;

	SAFE_RELEASE(p_dxgi_dup->dup);

	if (p_dxgi_dup->width != 0 || p_dxgi_dup->height != 0) {
		p_dxgi_dup->width = 0;
		p_dxgi_dup->height = 0;
	}
	if (FAILED(hr = ID3D11Device_QueryInterface(p_d3d11_device->dev, &IID_IDXGIDevice, &p_dxgi_device)))
	{
		printf("query dxgi device failed: 0x%x.\n", hr);
		return false;
	}
	if (FAILED(hr = IDXGIDevice_GetAdapter(p_dxgi_device, &p_dxgi_adapter)))
	{
		printf("get dxgi adapter failed: 0x%x.\n", hr);
		
		SAFE_RELEASE(p_dxgi_device);
		return false;
	}
	if (FAILED(hr = IDXGIAdapter_EnumOutputs(p_dxgi_adapter, 0, &p_dxgi_output)))
	{
		printf("get dxgi output failed: 0x%x.\n", hr);
	
		SAFE_RELEASE(p_dxgi_device);
		SAFE_RELEASE(p_dxgi_adapter);
		return false;
	}

	if (FAILED(hr = IDXGIOutput_QueryInterface(p_dxgi_output, &IID_IDXGIOutput1, &p_dxgi_output1)))
	{
		printf("get dxgi output1 failed: 0x%x.\n", hr);

		SAFE_RELEASE(p_dxgi_device);
		SAFE_RELEASE(p_dxgi_adapter);
		SAFE_RELEASE(p_dxgi_output);
		return false;
	}
	if (FAILED(hr = IDXGIOutput1_DuplicateOutput(p_dxgi_output1, (IUnknown*)p_dxgi_device, &p_dxgi_dup->dup)))
	{
		printf("get dxgi dup failed: 0x%x.\n", hr);
		SAFE_RELEASE(p_dxgi_device);
		SAFE_RELEASE(p_dxgi_adapter);
		SAFE_RELEASE(p_dxgi_output);
		SAFE_RELEASE(p_dxgi_output1);
		
		return false;
	}
	DXGI_OUTDUPL_DESC dup_desc;
	memset(&dup_desc, 0, sizeof(dup_desc));
	IDXGIOutputDuplication_GetDesc(p_dxgi_dup->dup, &dup_desc);

	p_dxgi_dup->height = dup_desc.ModeDesc.Height;
	p_dxgi_dup->width  = dup_desc.ModeDesc.Width;

	SAFE_RELEASE(p_dxgi_device);
	SAFE_RELEASE(p_dxgi_adapter);
	SAFE_RELEASE(p_dxgi_output);
	SAFE_RELEASE(p_dxgi_output1);
	return true;
}

bool d3d11_create_device(d3d11_device_t* p_d3d11_device) {

	HRESULT          hr;
	IDXGIFactory5*   p_dxgi_factory5; /* using five version to control tearing */
	IDXGIAdapter*    p_dxgi_adapter;

	p_dxgi_factory5 = NULL;
	p_dxgi_adapter  = NULL;

	SAFE_RELEASE(p_d3d11_device->dev);
	SAFE_RELEASE(p_d3d11_device->ctx);
	if (p_d3d11_device->vid != 0) {
		p_d3d11_device->vid = 0;
	}
	/**
	 * using CreateDXGIFactory1 not CreateDXGIFactory for IDXGIOutput1_DuplicateOutput success.
	 * https://docs.microsoft.com/en-us/windows/win32/api/dxgi1_2/nf-dxgi1_2-idxgioutput1-duplicateoutput
	 */
	if (FAILED(hr = CreateDXGIFactory1(&IID_IDXGIFactory5, &p_dxgi_factory5)))
	{
		printf("create dxgi factory failed: 0x%x.\n", hr);
		return false;
	}
	IDXGIFactory5_EnumAdapters(p_dxgi_factory5, 0, &p_dxgi_adapter);

	DXGI_ADAPTER_DESC dxgi_desc;
	IDXGIAdapter_GetDesc(p_dxgi_adapter, &dxgi_desc);

	if (dxgi_desc.VendorId == VENDOR_ID_NVIDIA
	 || dxgi_desc.VendorId == VENDOR_ID_AMD
	 || dxgi_desc.VendorId == VENDOR_ID_MICROSOFT
	 || dxgi_desc.VendorId == VENDOR_ID_INTEL) {

		p_d3d11_device->vid = dxgi_desc.VendorId;
	}
	else {
		printf("not supported dxgi adapter.\n");
	
		SAFE_RELEASE(p_dxgi_factory5);
		SAFE_RELEASE(p_dxgi_adapter);

		return false;
	}
	/**
	 * when adapter not null, driver type must be D3D_DRIVER_TYPE_UNKNOWN.
	 * https://docs.microsoft.com/en-us/windows/win32/api/d3d11/nf-d3d11-d3d11createdevice
	 */
	hr = D3D11CreateDevice(p_dxgi_adapter,
		D3D_DRIVER_TYPE_UNKNOWN,
		NULL,
		0,
		NULL,
		0,
		D3D11_SDK_VERSION,
		&p_d3d11_device->dev,
		NULL,
		&p_d3d11_device->ctx);

	if (FAILED(hr)) {
		printf("d3d11 create device and context failed: 0x%x.\n", hr);

		SAFE_RELEASE(p_dxgi_factory5);
		SAFE_RELEASE(p_dxgi_adapter);
		return false;
	}

	SAFE_RELEASE(p_dxgi_factory5);
	SAFE_RELEASE(p_dxgi_adapter);
	return true;
}

int dxgi_capture_frame(dxgi_dup_t* p_dxgi_dup, dxgi_frame_t* p_dxgi_frame) {

	HRESULT           hr;
	IDXGIResource*    p_dxgi_res;

	p_dxgi_res = NULL;

	IDXGIOutputDuplication_ReleaseFrame(p_dxgi_dup->dup);

	hr = IDXGIOutputDuplication_AcquireNextFrame(p_dxgi_dup->dup,
		500,
		&p_dxgi_frame->frame_info,
		&p_dxgi_res);
	if (FAILED(hr)) {

		if (hr == DXGI_ERROR_WAIT_TIMEOUT)
		{
			printf("wait frame timeout.\n");
			return CAPTURER_STATUS_TIMEOUT;
		}
		if (hr == DXGI_ERROR_ACCESS_LOST || hr == DXGI_ERROR_INVALID_CALL)
		{
			printf("get frame failed, maybe resolution changed or desktop switched. please re-create desktop-duplicator.\n");
			return CAPTURER_STATUS_RECREATE_DUP;
		}
	}

	/* no desktop image update, only cursor move. */
	if (p_dxgi_frame->frame_info.AccumulatedFrames == 0 || p_dxgi_frame->frame_info.LastPresentTime.QuadPart == 0)
	{
		printf("no desktop image update, only cursor move.\n");

		SAFE_RELEASE(p_dxgi_res);
		return CAPTURER_STATUS_NO_UPDATE;
	}
	SAFE_RELEASE(p_dxgi_frame->frame);

	if (FAILED(IDXGIResource_QueryInterface(p_dxgi_res, &IID_ID3D11Texture2D, &p_dxgi_frame->frame)))
	{
		printf("get d3d11 texture2D failed.\n");
		SAFE_RELEASE(p_dxgi_res);
		return CAPTURER_STATUS_FAILURE;
	}
	SAFE_RELEASE(p_dxgi_res);
	return CAPTURER_STATUS_SUCCESS;
}

void dump_bgra(dxgi_frame_t* frame, d3d11_device_t* d3d_dev, dxgi_dup_t* dup) {
	static FILE* bgra;
	D3D11_TEXTURE2D_DESC d3d11_texture_desc;
	ID3D11Texture2D* p_frame;
	IDXGISurface* p_dxgi_surface;
	DXGI_MAPPED_RECT     dxgi_mapped_rect;

	if (!bgra) {
		bgra = fopen("dump.bgra", "ab+");
	}

	ID3D11Texture2D_GetDesc(frame->frame, &d3d11_texture_desc);
	d3d11_texture_desc.MipLevels = 1;
	d3d11_texture_desc.ArraySize = 1;
	d3d11_texture_desc.SampleDesc.Count = 1;
	d3d11_texture_desc.Usage = D3D11_USAGE_STAGING;
	d3d11_texture_desc.BindFlags = 0;
	d3d11_texture_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	d3d11_texture_desc.MiscFlags = 0;

	ID3D11Device_CreateTexture2D(d3d_dev->dev, &d3d11_texture_desc, NULL, &p_frame);
	ID3D11DeviceContext_CopyResource(d3d_dev->ctx, (ID3D11Resource*)p_frame, (ID3D11Resource*)frame->frame);

	ID3D11Texture2D_QueryInterface(p_frame, &IID_IDXGISurface, &p_dxgi_surface);
	if (p_frame) {
		ID3D11Texture2D_Release(p_frame);
		p_frame = NULL;
	}
	IDXGISurface_Map(p_dxgi_surface, &dxgi_mapped_rect, DXGI_MAP_READ);

	for (int i = 0; i < (int)dup->height; i++) {
		fwrite(dxgi_mapped_rect.pBits + i * dxgi_mapped_rect.Pitch, dxgi_mapped_rect.Pitch, 1, bgra);
	}
	IDXGISurface_Unmap(p_dxgi_surface);

	if (p_dxgi_surface) {
		IDXGISurface_Release(p_dxgi_surface);
		p_dxgi_surface = NULL;
	}
}

bool exec_once = true;

int main(void) {

	d3d11_device_t d3d_dev;
	dxgi_dup_t     dup;
	dxgi_frame_t   frame;

	memset(&d3d_dev, 0, sizeof(d3d11_device_t));
	memset(&dup, 0, sizeof(dxgi_dup_t));
	memset(&frame, 0, sizeof(dxgi_frame_t));

	d3d11_create_device(&d3d_dev);
	dxgi_create_duplicator(&d3d_dev, &dup);

	int r;
	while (true) {
		static uint64_t base;
		if (exec_once) {
			base = cdk_timespec_get();
			exec_once = false;
		}

		r = dxgi_capture_frame(&dup, &frame);
		if (r == CAPTURER_STATUS_TIMEOUT || r == CAPTURER_STATUS_NO_UPDATE || r == CAPTURER_STATUS_FAILURE) {
			continue;
		}
		if (r == CAPTURER_STATUS_RECREATE_DUP) {
			printf("need re-create dup.\n");
			dxgi_create_duplicator(&d3d_dev, &dup);
			continue;
		}
		uint64_t end = cdk_timespec_get();
		if (end - base >= 1000) {
			printf("fps: %d\n", fps);
			base = end;
			fps = 0;
		}
		else {
			fps++;
		}
		//dump_bgra(&frame, &d3d_dev, &dup);
	}
	return 0;
}
