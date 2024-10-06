#include "native-video-stream-track.h"
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <napi.h>
#include <thread>
#include <iostream>

Napi::Object NativeVideoStreamTrack::Init(Napi::Env env, Napi::Object exports)
{
  Napi::Function func = DefineClass(
      env, "NativeVideoStreamTrack",
      {InstanceMethod("startCapture", &NativeVideoStreamTrack::startCapture),
       InstanceMethod("stopCapture", &NativeVideoStreamTrack::stopCapture)});

  Napi::FunctionReference *constructor = new Napi::FunctionReference();
  *constructor = Napi::Persistent(func);
  env.SetInstanceData(constructor);

  exports.Set("NativeVideoStreamTrack", func);
  return exports;
}

NativeVideoStreamTrack::NativeVideoStreamTrack(const Napi::CallbackInfo &info)
    : Napi::ObjectWrap<NativeVideoStreamTrack>(info), sourceReader(nullptr),
      isTsfnReleased(false)
{
  InitializeMF();
}

NativeVideoStreamTrack::~NativeVideoStreamTrack()
{
  // UninitializeMF();
  // if (!isTsfnReleased)
  // {
  //   tsfn.Release();
  // }
}

void NativeVideoStreamTrack::InitializeMF()
{
  CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
  MFStartup(MF_VERSION);
}

void NativeVideoStreamTrack::UninitializeMF()
{
  if (sourceReader)
  {
    sourceReader->Release();
    sourceReader = nullptr;
  }
  MFShutdown();
  CoUninitialize();
}

Napi::Value
NativeVideoStreamTrack::startCapture(const Napi::CallbackInfo &info)
{
  Napi::Env env = info.Env();

  if (info.Length() < 1 || !info[0].IsFunction())
  {
    Napi::TypeError::New(env, "Function expected as first argument")
        .ThrowAsJavaScriptException();
    return env.Undefined();
  }

  Napi::Function jsCallback = info[0].As<Napi::Function>();
  tsfn = Napi::ThreadSafeFunction::New(env, jsCallback, "VideoCallback", 0, 1,
                                       [](Napi::Env)
                                       {
                                         // Finalize
                                       });

  IMFAttributes *pConfig = NULL;
  MFCreateAttributes(&pConfig, 1);
  pConfig->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
                   MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);

  IMFActivate **ppDevices = NULL;
  UINT32 count;
  MFEnumDeviceSources(pConfig, &ppDevices, &count);

  if (count > 0)
  {
    std::cout << "Count: " << count << std::endl;

    // Create the media source
    IMFMediaSource *pSource = NULL;
    HRESULT hr = ppDevices[0]->ActivateObject(IID_PPV_ARGS(&pSource));
    if (FAILED(hr))
    {
      std::cerr << "Failed to activate media source. HRESULT: 0x" << std::hex << hr << std::dec << std::endl;
    }
    else
    {
      std::cout << "Media source activated successfully." << std::endl;

      // Create the source reader
      hr = MFCreateSourceReaderFromMediaSource(pSource, NULL, &sourceReader);
      if (FAILED(hr))
      {
        std::cerr << "Failed to create source reader. HRESULT: 0x" << std::hex << hr << std::dec << std::endl;
      }
      else
      {
        std::cout << "Source reader created successfully." << std::endl;
      }

      // Release the media source as it's no longer needed
      pSource->Release();
    }

    for (UINT32 i = 0; i < count; i++)
    {
      ppDevices[i]->Release();
    }
    CoTaskMemFree(ppDevices);
  }
  else
  {
    std::cerr << "No video capture devices found." << std::endl;
  }

  pConfig->Release();

  if (sourceReader)
  {
    std::cout << "Starting capture thread" << std::endl;
    std::thread captureThread(&NativeVideoStreamTrack::ReadFrame, this);
    captureThread.detach();
  }
  else
  {
    std::cerr << "Source reader is null, cannot start capture thread." << std::endl;
  }

  return env.Undefined();
}

void NativeVideoStreamTrack::ReadFrame()
{
  while (sourceReader && !isTsfnReleased)
  {
    IMFSample *pSample = nullptr;
    DWORD streamIndex, flags;
    LONGLONG llTimeStamp;
    HRESULT hr = sourceReader->ReadSample(MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0,
                                          &streamIndex, &flags, &llTimeStamp, &pSample);

    if (FAILED(hr) || !pSample)
    {
      continue;
    }

    ProcessSample(pSample);
    pSample->Release();
  }
}

void NativeVideoStreamTrack::ProcessSample(IMFSample *pSample)
{
  IMFMediaBuffer *pBuffer = nullptr;
  HRESULT hr = pSample->GetBufferByIndex(0, &pBuffer);
  if (FAILED(hr))
  {
    return;
  }

  BYTE *pBytes;
  DWORD maxLength, currentLength;
  hr = pBuffer->Lock(&pBytes, &maxLength, &currentLength);
  if (FAILED(hr) || pBytes == nullptr || currentLength == 0)
  {
    pBuffer->Release();
    return;
  }

  BYTE *bufferData = new BYTE[currentLength];
  memcpy(bufferData, pBytes, currentLength);

  std::string format = GetPixelFormat();
  UINT32 width, height;
  GetFrameSize(&width, &height);

  tsfn.BlockingCall([=](Napi::Env env, Napi::Function jsCallback)
  {
    Napi::Buffer<BYTE> buffer = Napi::Buffer<BYTE>::New(env, bufferData, currentLength, 
        [](Napi::Env, BYTE* data) { delete[] data; });

    Napi::Object frameData = Napi::Object::New(env);
    frameData.Set("data", buffer);
    frameData.Set("format", Napi::String::New(env, format));
    frameData.Set("width", Napi::Number::New(env, width));
    frameData.Set("height", Napi::Number::New(env, height));

    jsCallback.Call({frameData});
  });

  pBuffer->Unlock();
  pBuffer->Release();
}

void NativeVideoStreamTrack::GetFrameSize(UINT32 *width, UINT32 *height)
{
  IMFMediaType *pMediaType = nullptr;
  HRESULT hr = sourceReader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, &pMediaType);
  if (FAILED(hr))
  {
    *width = 0;
    *height = 0;
    return;
  }

  hr = MFGetAttributeSize(pMediaType, MF_MT_FRAME_SIZE, width, height);
  pMediaType->Release();

  if (FAILED(hr))
  {
    *width = 0;
    *height = 0;
  }
}

std::string NativeVideoStreamTrack::GetPixelFormat()
{
  IMFMediaType *pMediaType = nullptr;
  HRESULT hr = sourceReader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, &pMediaType);
  if (FAILED(hr))
  {
    return "Unknown";
  }

  GUID subtype;
  hr = pMediaType->GetGUID(MF_MT_SUBTYPE, &subtype);
  pMediaType->Release();

  if (FAILED(hr))
  {
    return "Unknown";
  }

  if (subtype == MFVideoFormat_RGB32)
    return "RGB32";
  if (subtype == MFVideoFormat_YUY2)
    return "YUY2";
  if (subtype == MFVideoFormat_NV12)
    return "NV12";

  return "Unknown";
}

Napi::Value
NativeVideoStreamTrack::stopCapture(const Napi::CallbackInfo &info)
{
  Napi::Env env = info.Env();

  // if (sourceReader)
  // {
  //   sourceReader->Release();
  //   sourceReader = nullptr;
  // }

  isTsfnReleased = true;
  tsfn.Release();

  return env.Undefined();
}
