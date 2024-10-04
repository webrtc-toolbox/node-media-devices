{
  "targets": [
    {
      "target_name": "media-devices-native",
      "cflags!": [ "-fno-exceptions" ],
      "cflags_cc!": [ "-fno-exceptions" ],
      # "sources": [ "src/native/media-devices-native.cc","src/native/native-media-stream.mm","src/native/native-media-recorder.mm","src/native/video-frame-delegate.mm"],
      "sources": ["src/native/foo.mm"],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")"
      ],
      'defines': [ 'NAPI_DISABLE_CPP_EXCEPTIONS' ],
      'xcode_settings': {
        'MACOSX_DEPLOYMENT_TARGET': '10.15',
        "OTHER_CPLUSPLUSFLAGS" : ['-ObjC++'],
        # "OTHER_CFLAGS":["-fobjc-arc",'-fmodules']
        "OTHER_LDFLAGS": ["-framework AVFoundation","-framework Cocoa"],
      }
    }
  ]
}