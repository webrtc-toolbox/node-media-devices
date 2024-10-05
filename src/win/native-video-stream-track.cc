#include "native-video-stream-track.h"
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <napi.h>
#include <thread>

Napi::Object NativeVideoStreamTrack::Init(Napi::Env env, Napi::Object exports) {
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
      isTsfnReleased(false) {
  InitializeMF();
}

NativeVideoStreamTrack::~NativeVideoStreamTrack() {
  UninitializeMF();
  if (!isTsfnReleased) {
    tsfn.Release();
  }
}

void NativeVideoStreamTrack::InitializeMF() {
  CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
  MFStartup(MF_VERSION);
}

void NativeVideoStreamTrack::UninitializeMF() {
  if (sourceReader) {
    sourceReader->Release();
    sourceReader = nullptr;
  }
  MFShutdown();
  CoUninitialize();
}

Napi::Value
NativeVideoStreamTrack::startCapture(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  if (info.Length() < 1 || !info[0].IsFunction()) {
    Napi::TypeError::New(env, "Function expected as first argument")
        .ThrowAsJavaScriptException();
    return env.Undefined();
  }

  Napi::Function jsCallback = info[0].As<Napi::Function>();
  tsfn = Napi::ThreadSafeFunction::New(env, jsCallback, "VideoCallback", 0, 1,
                                       [](Napi::Env) {
                                         // Finalize
                                       });

  IMFAttributes *pConfig = NULL;
  MFCreateAttributes(&pConfig, 1);
  pConfig->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
                   MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);

  IMFActivate **ppDevices = NULL;
  UINT32 count;
  MFEnumDeviceSources(pConfig, &ppDevices, &count);

  if (count > 0) {
    ppDevices[0]->ActivateObject(IID_PPV_ARGS(&sourceReader));
    for (UINT32 i = 0; i < count; i++) {
      ppDevices[i]->Release();
    }
    CoTaskMemFree(ppDevices);
  }

  pConfig->Release();

  if (sourceReader) {
    std::thread captureThread(&NativeVideoStreamTrack::ReadFrame, this);
    captureThread.detach();
  }

  return env.Undefined();
}

void NativeVideoStreamTrack::ReadFrame() {
  while (sourceReader) {
    IMFSample *pSample = NULL;
    DWORD streamIndex, flags;
    LONGLONG llTimeStamp;
    HRESULT hr =
        sourceReader->ReadSample(MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0,
                                 &streamIndex, &flags, &llTimeStamp, &pSample);

    if (SUCCEEDED(hr) && pSample) {
      IMFMediaBuffer *pBuffer = NULL;
      pSample->GetBufferByIndex(0, &pBuffer);

      BYTE *pBytes;
      DWORD maxLength, currentLength;
      pBuffer->Lock(&pBytes, &maxLength, &currentLength);

      // Get the pixel format
      IMFMediaType *pMediaType = NULL;
      GUID subtype;
      HRESULT hr = sourceReader->GetCurrentMediaType(
          MF_SOURCE_READER_FIRST_VIDEO_STREAM, &pMediaType);

      std::string format;

      if (SUCCEEDED(hr)) {
        hr = pMediaType->GetGUID(MF_MT_SUBTYPE, &subtype);
        if (SUCCEEDED(hr)) {
          // You can now use the subtype to determine the pixel format
          if (subtype == MFVideoFormat_RGB32) {
            format = "RGB32";
          } else if (subtype == MFVideoFormat_YUY2) {
            format = "YUY2";
          } else if (subtype == MFVideoFormat_NV12) {
            format = "NV12";
          } else {
            format = "Unknown";
          }
          // Add more format checks as needed
        } else {
          format = "Unknown";
        }
        pMediaType->Release();
      }

      auto callback = [](Napi::Env env, Napi::Function jsCallback, BYTE *data,
                         DWORD size) {
        Napi::Buffer<BYTE> buffer = Napi::Buffer<BYTE>::Copy(env, data, size);
        jsCallback.Call({buffer});
      };

      napi_status status = tsfn.BlockingCall(pBytes, currentLength, callback);
      if (status != napi_ok) {
        break;
      }

      pBuffer->Unlock();
      pBuffer->Release();
      pSample->Release();
    }
  }
}

Napi::Value
NativeVideoStreamTrack::stopCapture(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  if (sourceReader) {
    sourceReader->Release();
    sourceReader = nullptr;
  }

  isTsfnReleased = true;
  tsfn.Release();

  return env.Undefined();
}
