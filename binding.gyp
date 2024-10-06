{
  "targets": [
    {
      "target_name": "media-devices-native",
      "cflags!": [ "-fno-exceptions" ],
      "cflags_cc!": [ "-fno-exceptions" ],
      "sources": [ 
          "src/native/media-devices-native.cc",
          # "src/native/native-media-stream.mm",
          # "src/native/native-media-recorder.mm",
          # "src/native/video-frame-delegate.mm",
          # "src/native/native-video-stream-track.mm",
          # "src/native/native-audio-stream-track.mm"
        ],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")"
      ],
      'defines': [ 'NAPI_DISABLE_CPP_EXCEPTIONS' ],
      'conditions': [
        ['OS=="mac"', {
          'xcode_settings': {
            'MACOSX_DEPLOYMENT_TARGET': '10.15',
            "OTHER_CPLUSPLUSFLAGS" : ['-ObjC++'],
            # "OTHER_CFLAGS":["-fobjc-arc",'-fmodules']
            "OTHER_LDFLAGS": ["-framework AVFoundation","-framework Cocoa"],
          },
          "sources": [
            "src/mac/native-video-stream-track.mm",
            "src/mac/native-audio-stream-track.mm"
          ]
        }],
        ['OS=="win"', {
          'sources': [
            "src/win/native-video-stream-track.cc",
            "src/win/native-audio-stream-track.cc"
          ],
          'libraries': [
            '-lmf.lib',
            '-lmfplat.lib',
            '-lmfreadwrite.lib',
            '-lmfuuid.lib'
          ],
          'msvs_settings': {
            'VCCLCompilerTool': {
              'AdditionalOptions': ['/EHsc']
            }
          }
        }]
      ]
    }
  ]
}