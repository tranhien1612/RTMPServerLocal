#include <glib.h>
#include <gst/gst.h>
#include <glib-unix.h>
#include <gst/app/gstappsink.h>
#include <mutex>
#include <iostream>

#ifndef INT64_C
#define INT64_C(c) (c ## LL)
#define UINT64_C(c) (c ## ULL)
#endif

GMainLoop* _loop;
GstBus* _bus;
GstElement* _pipeline;
GstElement* _source;
GstElement* _depay;
GstElement* _parse;
GstElement* _capsfilter;
GstElement* _queue1;
GstElement* _rtmpsink;
GstElement* _flvmux;
std::mutex _mutex;
bool _bStopPush;

void _onPadAdded(GstElement* src, GstPad* src_pad, gpointer user_data){
    GstPad* sink_pad = (GstPad*)user_data;
    gst_pad_link(src_pad, sink_pad);
}

void SetElesNull(){
    _loop = nullptr;
    _bus = nullptr;
    _pipeline = nullptr;
    _source = nullptr;
    _depay = nullptr;
    _parse = nullptr;
    _capsfilter = nullptr;
    _queue1 = nullptr;
    _rtmpsink = nullptr;
    _flvmux = nullptr;
}

bool Run(std::string strlocation, std::string strrequest)
{
    _bStopPush = false;

    while(!_bStopPush){
        SetElesNull();
        gboolean terminate = false;

        gst_init(nullptr, nullptr);

        _pipeline = gst_pipeline_new("pipeline");
        _source = gst_element_factory_make("rtspsrc", "src");
        _depay = gst_element_factory_make("rtph264depay", "depay");
        _parse = gst_element_factory_make("h264parse", "parse");
        _flvmux = gst_element_factory_make("flvmux", "flvmux");
        _queue1 = gst_element_factory_make("queue", "queue");
        _capsfilter = gst_element_factory_make("capsfilter", "filter");
        _rtmpsink = gst_element_factory_make("rtmpsink", "sink");

        g_object_set(_source, "location", strrequest.c_str(), NULL);
        g_object_set(_source, "latency", 0, NULL);
        g_object_set(_capsfilter, "caps-change-mode", 1, NULL);
        g_object_set(_rtmpsink, "location", strlocation.c_str(), NULL);

        gst_bin_add_many(GST_BIN(_pipeline), _source, _depay, _parse, _flvmux, _capsfilter, _queue1, _rtmpsink, NULL);
        g_signal_connect(_source, "pad-added", G_CALLBACK(&_onPadAdded), gst_element_get_static_pad(_depay, "sink"));
        gboolean bsuccess = gst_element_link_many(_depay, _parse, _flvmux, _capsfilter, _queue1, _rtmpsink, NULL);
        if(!bsuccess){
            gst_element_unlink_many(_depay, _parse, _flvmux, _capsfilter, _queue1, _rtmpsink, NULL);
            continue;
        }
        
        GstCaps* caps = gst_caps_new_simple(
            "video/x-raw",
            "format", G_TYPE_STRING, "rgb",
            "width", G_TYPE_INT, 426,
            "height", G_TYPE_INT, 240,
            "framerate", GST_TYPE_FRACTION, 25, 1,
            NULL
        );
        g_object_set(_capsfilter, "caps", caps, NULL);
        gst_caps_unref(caps);

        GstStateChangeReturn res = gst_element_set_state(_pipeline, GST_STATE_PLAYING);
        if(res == GST_STATE_CHANGE_FAILURE){
            gst_object_unref(_pipeline);
            continue;
        }

        GstMessage* msg;
        _bus = gst_pipeline_get_bus(GST_PIPELINE(_pipeline));
        do{
            msg = gst_bus_timed_pop_filtered(_bus, GST_CLOCK_TIME_NONE, GstMessageType(GST_MESSAGE_STATE_CHANGED | GST_MESSAGE_ERROR | GST_MESSAGE_EOS));
            if(msg != NULL){
                GError *err;
                gchar* debug_info;
                switch(GST_MESSAGE_TYPE(msg)){
                    case GST_MESSAGE_ERROR:
                        gst_message_parse_error(msg, &err, &debug_info);
                        g_printerr("Error received from element %s: %s", GST_OBJECT_NAME(msg->src), err->message);
                        g_printerr("Debugging information: %s", debug_info ? debug_info : "none");
                        g_clear_error(&err);
                        g_free(debug_info);
                        terminate = true;
                        break;
                    case GST_MESSAGE_EOS:
                        g_printerr("End of Stream");
                        terminate = true;
                        break;
                    case GST_MESSAGE_STATE_CHANGED:
                        GstState old_state, new_state, pending_state;
                        gst_message_parse_state_changed(msg, &old_state, &new_state, &pending_state);
                        g_printerr("Pipeline state change from %s to %s: \n", gst_element_state_get_name(old_state), gst_element_state_get_name(new_state));
                        if(pending_state == GST_STATE_NULL){
                            terminate = true;
                        }
                        break;
                    default:
                        g_printerr("Unexpected message received.\n");
                        break;
                }
                gst_message_unref(msg);
            }
        }while (!terminate);

        try{
            gst_object_unref(_bus);
            gst_element_set_state(_pipeline, GST_STATE_PAUSED);
            gst_element_set_state(_pipeline, GST_STATE_READY);
            gst_element_set_state(_pipeline, GST_STATE_NULL);
            gst_object_unref(_pipeline);
        }catch(std::exception &e){
            std::cout << e.what();
            return true;
        }catch(...){
            return true;
        }
        
    }

    return true;
}


int main(){
    std::string strRtmp = "rtmp://127.0.0.1/live/stream";
    std::string strRtsp = "rtsp://127.0.0.1:8554/test";
    bool bSuccess = Run(strRtmp, strRtsp);

    return 0;
}
