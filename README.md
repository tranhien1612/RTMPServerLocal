# RTMP Server Local

## Current situation
* Support Windows and Linux platforms
* Support RTMP, HTTP-FLV protocols
* Support H.264 and AAC forwarding
* Support GOP cache
* Support RTMP streaming

## Compile and run
```
  # compile
  make
  
  # run
  ./rtmp_server
```

## Server test
### Using ffmpeg:
```
  # stream video with input is rtspsrc
  ffmpeg -rtsp_transport tcp -i "rtsp://127.0.0.1:8554/test" -f flv -r 25 -an rtmp://127.0.0.1/live/stream

  # stream video with input is video file
  ffmpeg -re -i test.h264 -f flv rtmp://127.0.0.1/live/stream

  # display
  ffplay rtmp://127.0.0.1:1935/live/stream
```


### Using gstreamer:
```
  # only videotestsrc
  gst-launch-1.0 videotestsrc is-live=true ! videoconvert ! x264enc ! flvmux streamable=true ! rtmpsink location="rtmp://127.0.0.1/live/stream"
  
  # videotestsrc + audiotestsrc
  gst-launch-1.0 videotestsrc is-live=true ! videoconvert ! x264enc ! flvmux streamable=true name=mux ! rtmpsink location="rtmp://127.0.0.1/live/stream" audiotestsrc is-live=true ! audioconvert ! audioresample ! audio/x-raw,rate=48000 ! voaacenc bitrate=96000 ! audio/mpeg ! aacparse ! audio/mpeg, mpegversion=4 ! mux.
  
  # rtspsrc
  gst-launch-1.0 rtspsrc location=rtsp://x.x.x.x latency=0 ! rtph264depay ! h264parse ! flvmux streamable=true ! queue ! rtmpsink location="rtmp://127.0.0.1/live/stream"

```

