#include "native-audio-stream-track.h"
#include <thread>
#include <iostream>
#include <memory>

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

NativeAudioStreamTrack::NativeAudioStreamTrack(const Napi::CallbackInfo &info)
    : Napi::ObjectWrap<NativeAudioStreamTrack>(info), sourceReader(nullptr), isTsfnReleased(false)
{
    InitializeMF();
}

NativeAudioStreamTrack::~NativeAudioStreamTrack()
{
    UninitializeMF();

    if (!isTsfnReleased)
    {
        tsfn.Release();
    }
}

void NativeAudioStreamTrack::InitializeMF()
{
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    MFStartup(MF_VERSION);
}

void NativeAudioStreamTrack::UninitializeMF()
{
    // Release the source reader if it exists
    if (sourceReader)
    {
        sourceReader->Release();
        sourceReader = nullptr;
    }

    // Shut down Media Foundation
    HRESULT hr = MFShutdown();
    if (FAILED(hr))
    {
        // Handle or log the error
        std::cerr << "MFShutdown failed: " << std::hex << hr << std::endl;
    }

    // Uninitialize COM
    CoUninitialize();
}

Napi::Value NativeAudioStreamTrack::startCapture(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsFunction())
    {
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
        [](Napi::Env)
        {
            // Finalize
        });

    // Create attributes for audio capture
    IMFAttributes *pConfig = nullptr;
    HRESULT hr = MFCreateAttributes(&pConfig, 1);
    if (FAILED(hr))
    {
        Napi::Error::New(env, "Failed to create attributes").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    // Set the device type to audio capture
    hr = pConfig->SetGUID(
        MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
        MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_AUDCAP_GUID);
    if (FAILED(hr))
    {
        pConfig->Release();
        Napi::Error::New(env, "Failed to set device type").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    // Enumerate devices
    IMFActivate **ppDevices = nullptr;
    UINT32 count;
    hr = MFEnumDeviceSources(pConfig, &ppDevices, &count);
    pConfig->Release();

    if (FAILED(hr) || count == 0)
    {
        Napi::Error::New(env, "No audio capture devices found").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    // Activate the first device
    IMFMediaSource *pSource = nullptr;
    hr = ppDevices[0]->ActivateObject(IID_PPV_ARGS(&pSource));

    // Clean up device enumeration
    for (UINT32 i = 0; i < count; i++)
    {
        ppDevices[i]->Release();
    }
    CoTaskMemFree(ppDevices);

    if (FAILED(hr))
    {
        Napi::Error::New(env, "Failed to activate audio capture device").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    // Create a source reader
    hr = MFCreateSourceReaderFromMediaSource(
        pSource,
        nullptr,
        &sourceReader);
    pSource->Release();

    if (FAILED(hr))
    {
        Napi::Error::New(env, "Failed to create source reader").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    std::thread captureThread(&NativeAudioStreamTrack::ReadFrame, this);
    captureThread.detach();

    return env.Undefined();
}

Napi::Value NativeAudioStreamTrack::stopCapture(const Napi::CallbackInfo &info)
{
    if (!isTsfnReleased)
    {
        isTsfnReleased = true;
        tsfn.Release();
    }
    return info.Env().Undefined();
}

void NativeAudioStreamTrack::ReadFrame()
{
    HRESULT hr = S_OK;
    while (sourceReader && SUCCEEDED(hr) && !isTsfnReleased)
    {
        DWORD streamIndex, flags;
        LONGLONG timestamp;
        IMFSample *pSample = nullptr;

        hr = sourceReader->ReadSample(
            MF_SOURCE_READER_FIRST_AUDIO_STREAM,
            0,            // Flags
            &streamIndex, // Receives the actual stream index
            &flags,       // Receives status flags
            &timestamp,   // Receives the timestamp
            &pSample      // Receives the sample
        );

        if (SUCCEEDED(hr) && pSample)
        {
            ProcessSample(pSample);
            pSample->Release();
        }

        if (flags & MF_SOURCE_READERF_ENDOFSTREAM)
        {
            break;
        }
    }

    if (FAILED(hr))
    {
        tsfn.BlockingCall([](Napi::Env env, Napi::Function jsCallback)
                          { jsCallback.Call({Napi::String::New(env, "Error reading audio sample")}); });
    }
}

void NativeAudioStreamTrack::ProcessSample(IMFSample *pSample)
{
    IMFMediaBuffer *pBuffer = nullptr;
    HRESULT hr = pSample->ConvertToContiguousBuffer(&pBuffer);
    if (FAILED(hr))
    {
        return;
    }

    BYTE *pAudioData = nullptr;
    DWORD cbCurrentLength = 0;
    hr = pBuffer->Lock(&pAudioData, nullptr, &cbCurrentLength);

    WAVEFORMATEX wav_format{GetAudioFormat()};
    auto encoding{GetAudioEncoding()};

    if (SUCCEEDED(hr) && !isTsfnReleased)
    {
        tsfn.BlockingCall([pAudioData, cbCurrentLength, wav_format, encoding](Napi::Env env, Napi::Function jsCallback)
                          {
            Napi::Object audioData = Napi::Object::New(env);
            audioData.Set("data", Napi::Buffer<BYTE>::Copy(env, pAudioData, cbCurrentLength));
            audioData.Set("sampleRate", Napi::Number::New(env, wav_format.nSamplesPerSec));
            audioData.Set("bitsPerSample", Napi::Number::New(env, wav_format.wBitsPerSample));
            audioData.Set("channels", Napi::Number::New(env, wav_format.nChannels));
            audioData.Set("encoding", Napi::String::New(env, encoding));
            jsCallback.Call({audioData}); });

        pBuffer->Unlock();

        pBuffer->Release();
    }
}

WAVEFORMATEX NativeAudioStreamTrack::GetAudioFormat()
{
    WAVEFORMATEX waveFormat = {0};

    if (!sourceReader)
    {
        return waveFormat;
    }

    IMFMediaType *pMediaType = nullptr;
    HRESULT hr = sourceReader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, &pMediaType);
    if (SUCCEEDED(hr))
    {
        UINT32 size = 0;
        WAVEFORMATEX *pTempWaveFormat = nullptr;
        hr = MFCreateWaveFormatExFromMFMediaType(pMediaType, &pTempWaveFormat, &size);
        if (SUCCEEDED(hr) && pTempWaveFormat)
        {
            waveFormat = *pTempWaveFormat;
            CoTaskMemFree(pTempWaveFormat);
        }
        pMediaType->Release();
    }

    return waveFormat;
}

std::string NativeAudioStreamTrack::GetAudioEncoding()
{
    if (!sourceReader)
    {
        return "unknown";
    }

    IMFMediaType *pMediaType = nullptr;
    HRESULT hr = sourceReader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, &pMediaType);
    if (SUCCEEDED(hr))
    {
        GUID subtype;
        hr = pMediaType->GetGUID(MF_MT_SUBTYPE, &subtype);
        if (SUCCEEDED(hr))
        {
            if (subtype == MFAudioFormat_PCM)
            {
                return "pcm";
            }
            else if (subtype == MFAudioFormat_Float)
            {
                return "float";
            }
            else if (subtype == MFAudioFormat_AAC)
            {
                return "aac";
            }
            // Add more format checks as needed
        }
        pMediaType->Release();
    }

    return "unknown";
}
