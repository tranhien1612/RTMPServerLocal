# RTSP2RTMP

# Compiler and Run
Install gstreamer for this device

Compiler
```
  g++ -o main main.cpp $(pkg-config --cflags --libs gstreamer-1.0)
```

Change information of rtsp and rtmp link in `main.cpp` file and run it:
```
  ./main
```
