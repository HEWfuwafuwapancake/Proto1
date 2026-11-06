#include "render/texture.h"
#include <wincodec.h>
#include <limits>
#include <vector>

// WIC の画像デコーダーから、B・G・R・A の画素を取り出してテクスチャにする。
static ID3D11ShaderResourceView* CreateFromDecoder(IWICImagingFactory* factory, IWICBitmapDecoder* decoder)
{
	ID3D11ShaderResourceView* view = nullptr;
	IWICBitmapFrameDecode* frame = nullptr;
	IWICFormatConverter* converter = nullptr;
	UINT width = 0;
	UINT height = 0;
	if (SUCCEEDED(decoder->GetFrame(0, &frame))
		&& SUCCEEDED(factory->CreateFormatConverter(&converter))
		&& SUCCEEDED(converter->Initialize(frame, GUID_WICPixelFormat32bppBGRA,
			WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom))
		&& SUCCEEDED(converter->GetSize(&width, &height))
		&& width > 0 && height > 0 && width <= D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION
		&& height <= D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION) {
		std::vector<BYTE> pixels(static_cast<size_t>(width) * height * 4);
		if (SUCCEEDED(converter->CopyPixels(nullptr, width * 4, static_cast<UINT>(pixels.size()), pixels.data())))
			view = TextureCreateBGRA(pixels.data(), width, height);
	}
	SAFE_RELEASE(converter);
	SAFE_RELEASE(frame);
	return view;
}

static IWICImagingFactory* CreateFactory(void)
{
	IWICImagingFactory* factory = nullptr;
	if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory))))
		return nullptr;
	return factory;
}

ID3D11ShaderResourceView* TextureLoadFromFile(const char* path)
{
	if (!path) return nullptr;
	const int length = MultiByteToWideChar(CP_ACP, 0, path, -1, nullptr, 0);
	if (length <= 0) return nullptr;
	std::vector<wchar_t> widePath(static_cast<size_t>(length));
	MultiByteToWideChar(CP_ACP, 0, path, -1, widePath.data(), length);
	IWICImagingFactory* factory = CreateFactory();
	if (!factory) return nullptr;
	ID3D11ShaderResourceView* view = nullptr;
	IWICBitmapDecoder* decoder = nullptr;
	if (SUCCEEDED(factory->CreateDecoderFromFilename(widePath.data(), nullptr, GENERIC_READ,
		WICDecodeMetadataCacheOnDemand, &decoder)))
		view = CreateFromDecoder(factory, decoder);
	SAFE_RELEASE(decoder);
	SAFE_RELEASE(factory);
	return view;
}

ID3D11ShaderResourceView* TextureLoadFromMemory(const void* data, size_t size)
{
	if (!data || size == 0 || size > (std::numeric_limits<DWORD>::max)()) return nullptr;
	IWICImagingFactory* factory = CreateFactory();
	if (!factory) return nullptr;
	ID3D11ShaderResourceView* view = nullptr;
	IWICStream* stream = nullptr;
	IWICBitmapDecoder* decoder = nullptr;
	if (SUCCEEDED(factory->CreateStream(&stream))
		&& SUCCEEDED(stream->InitializeFromMemory(static_cast<BYTE*>(const_cast<void*>(data)), static_cast<DWORD>(size)))
		&& SUCCEEDED(factory->CreateDecoderFromStream(stream, nullptr, WICDecodeMetadataCacheOnDemand, &decoder)))
		view = CreateFromDecoder(factory, decoder);
	SAFE_RELEASE(decoder);
	SAFE_RELEASE(stream);
	SAFE_RELEASE(factory);
	return view;
}

ID3D11ShaderResourceView* TextureCreateBGRA(const void* pixels, UINT width, UINT height)
{
	ID3D11Device* device = DirectXGetDevice();
	ID3D11DeviceContext* context = DirectXGetDeviceContext();
	if (!pixels || width == 0 || height == 0 || !device || !context) return nullptr;
	// 遠くで小さく描いてもちらつかないように、ミップマップを GPU で作る（GenerateMips は描画先にもなれるテクスチャが要る）。
	D3D11_TEXTURE2D_DESC desc{};
	desc.Width = width;
	desc.Height = height;
	desc.MipLevels = 0;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	desc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
	ID3D11Texture2D* texture = nullptr;
	ID3D11ShaderResourceView* view = nullptr;
	if (SUCCEEDED(device->CreateTexture2D(&desc, nullptr, &texture))
		&& SUCCEEDED(device->CreateShaderResourceView(texture, nullptr, &view))) {
		context->UpdateSubresource(texture, 0, nullptr, pixels, width * 4, 0);
		context->GenerateMips(view);
	}
	SAFE_RELEASE(texture);
	return view;
}
