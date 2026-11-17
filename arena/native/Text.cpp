#include "Text.h"
#include <windows.h>
#include <dwrite_1.h>
#include <d2d1.h>
#include <wincodec.h>
#include <wrl/client.h>
#include <stdexcept>
using Microsoft::WRL::ComPtr;
namespace {
void check(HRESULT hr){if(FAILED(hr))throw std::runtime_error("DirectWrite/WIC failed");}
std::wstring wide(const std::string& s){int n=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,s.data(),(int)s.size(),nullptr,0);if(!n&&!s.empty())throw std::runtime_error("Invalid UTF-8");std::wstring out(n,0);MultiByteToWideChar(CP_UTF8,0,s.data(),(int)s.size(),out.data(),n);return out;}
}
std::vector<unsigned char> rasterText(const Json& j,int w,int h,double t){
 HRESULT co=CoInitializeEx(nullptr,COINIT_MULTITHREADED);
 struct Uninit{bool own;~Uninit(){if(own)CoUninitialize();}} uninit{SUCCEEDED(co)};
 ComPtr<IDWriteFactory> dw;check(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,__uuidof(IDWriteFactory),(IUnknown**)dw.GetAddressOf()));
 ComPtr<ID2D1Factory> d2;check(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,d2.GetAddressOf()));
 ComPtr<IWICImagingFactory> wic;check(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&wic)));
 ComPtr<IWICBitmap> bitmap;check(wic->CreateBitmap(w,h,GUID_WICPixelFormat32bppPBGRA,WICBitmapCacheOnLoad,&bitmap));
 ComPtr<ID2D1RenderTarget> rt;auto props=D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_SOFTWARE,D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM,D2D1_ALPHA_MODE_PREMULTIPLIED),96,96);
 check(d2->CreateWicBitmapRenderTarget(bitmap.Get(),props,&rt));
 auto font=wide(word(j,"font","Segoe UI"));auto text=wide(word(j,"text","PulseText"));
 if(word(j,"animation")=="Typewriter"){
  size_t count=(size_t)(t*number(j,"speed",1,0,10)*12);size_t end=0;
  while(end<text.size()&&count--){wchar_t c=text[end++];if(c>=0xD800&&c<=0xDBFF&&end<text.size())++end;}
  text.resize(end);
 }
 ComPtr<IDWriteTextFormat> format;check(dw->CreateTextFormat(font.c_str(),nullptr,flag(j,"bold")?DWRITE_FONT_WEIGHT_BOLD:DWRITE_FONT_WEIGHT_NORMAL,flag(j,"italic")?DWRITE_FONT_STYLE_ITALIC:DWRITE_FONT_STYLE_NORMAL,DWRITE_FONT_STRETCH_NORMAL,number(j,"size",96,8,500),L"",&format));
 auto align=word(j,"horizontal","Centre");format->SetTextAlignment(align=="Left"?DWRITE_TEXT_ALIGNMENT_LEADING:align=="Right"?DWRITE_TEXT_ALIGNMENT_TRAILING:DWRITE_TEXT_ALIGNMENT_CENTER);
 auto vertical=word(j,"vertical","Middle");format->SetParagraphAlignment(vertical=="Top"?DWRITE_PARAGRAPH_ALIGNMENT_NEAR:vertical=="Bottom"?DWRITE_PARAGRAPH_ALIGNMENT_FAR:DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
 float size=number(j,"size",96,8,500);format->SetLineSpacing(DWRITE_LINE_SPACING_METHOD_UNIFORM,size*number(j,"line_spacing",1.2f,.5f,3),size*.8f);
 ComPtr<IDWriteTextLayout> layout;check(dw->CreateTextLayout(text.c_str(),(UINT32)text.size(),format.Get(),(float)w,(float)h,&layout));
 ComPtr<IDWriteTextLayout1> layout1;if(SUCCEEDED(layout.As(&layout1)))layout1->SetCharacterSpacing(number(j,"letter_spacing",0,-20,100),0,0,DWRITE_TEXT_RANGE{0,(UINT32)text.size()});
 ComPtr<ID2D1SolidColorBrush> brush;check(rt->CreateSolidColorBrush(D2D1::ColorF(1,1,1,1),&brush));
 rt->BeginDraw();rt->Clear(D2D1::ColorF(0,0,0,0));rt->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);rt->DrawTextLayout(D2D1::Point2F(0,0),layout.Get(),brush.Get());check(rt->EndDraw());
 std::vector<unsigned char> pixels((size_t)w*h*4);check(bitmap->CopyPixels(nullptr,w*4,(UINT)pixels.size(),pixels.data()));return pixels;
}
