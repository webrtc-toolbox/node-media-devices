#include "native-audio-stream-track.h"
#include <thread>
#include <iostream>

Napi::Object NativeAudioStreamTrack::Init(Napi::Env env, Napi::Object exports)
{
  Napi::Function func = DefineClass(
      env, "NativeAudioStreamTrack",
      {InstanceMethod("startCapture", &NativeAudioStreamTrack::startCapture),
       InstanceMethod("stopCapture", &NativeAudioStreamTrack::stopCapture)});

  Napi::FunctionReference *constructor = new Napi::FunctionReference();
  *constructor = Napi::Persistent(func);
  env.SetInstanceData(constructor);

  exports.Set("NativeAudioStreamTrack", func);
  return exports;
}

NativeAudioStreamTrack::NativeAudioStreamTrack(const Napi::CallbackInfo& info)
    : Napi::ObjectWrap<NativeAudioStreamTrack>(info), sourceReader(nullptr), isTsfnReleased(false) {
    InitializeMF();
}

NativeAudioStreamTrack::~NativeAudioStreamTrack() {
    UninitializeMF();
    if (!isTsfnReleased) {
        tsfn.Release();
    }
}

void NativeAudioStreamTrack::InitializeMF() {
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    MFStartup(MF_VERSION);
}

void NativeAudioStreamTrack::UninitializeMF() {
    if (sourceReader) {
        sourceReader->Release();
        sourceReader = nullptr;
    }
    MFShutdown();
    CoUninitialize();
}

Napi::Value NativeAudioStreamTrack::startCapture(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsFunction()) {
        Napi::TypeError::New(env, "Function expected as first argument").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    Napi::Function jsCallback = info[0].As<Napi::Function>();
    tsfn = Napi::ThreadSafeFunction::New(
        env,
        jsCallback,
        "AudioCallback",
        0,
        1,
        [this](Napi::Env) {
            isTsfnReleased = true;
        });

    std::thread captureThread(&NativeAudioStreamTrack::ReadFrame, this);
    captureThread.detach();

    return env.Undefined();
}

Napi::Value NativeAudioStreamTrack::stopCapture(const Napi::CallbackInfo& info) {
    if (!isTsfnReleased) {
        tsfn.Release();
    }
    return info.Env().Undefined();
}

void NativeAudioStreamTrack::ReadFrame() {
    HRESULT hr = S_OK;
    while (SUCCEEDED(hr) && !isTsfnReleased) {
        DWORD streamIndex, flags;
        LONGLONG timestamp;
        IMFSample *pSample = nullptr;

        hr = sourceReader->ReadSample(
            MF_SOURCE_READER_FIRST_AUDIO_STREAM,
            0,                // Flags
            &streamIndex,     // Receives the actual stream index 
            &flags,           // Receives status flags
            &timestamp,       // Receives the timestamp
            &pSample          // Receives the sample
        );

        if (SUCCEEDED(hr) && pSample) {
            ProcessSample(pSample);
            pSample->Release();
        }

        if (flags & MF_SOURCE_READERF_ENDOFSTREAM) {
            break;
        }
    }

    if (FAILED(hr)) {
        // Handle error
        tsfn.BlockingCall([](Napi::Env env, Napi::Function jsCallback) {
            jsCallback.Call({Napi::String::New(env, "Error reading audio sample")});
        });
    }
}

void NativeAudioStreamTrack::ProcessSample(IMFSample *pSample) {
    // Implement the logic to process each audio sample
    // This should extract the audio data from the sample
    // and use the tsfn to send it back to JavaScript
}

void NativeAudioStreamTrack::GetAudioFormat(WAVEFORMATEX *pWaveFormat) {
    // Implement the logic to get the audio format
    // This should populate the pWaveFormat structure with the current audio format
}

std::string NativeAudioStreamTrack::GetAudioEncoding() {
    // Implement the logic to get the audio encoding
    // This should return a string representing the current audio encoding
    return "pcm"; // placeholder, replace with actual implementation
}
