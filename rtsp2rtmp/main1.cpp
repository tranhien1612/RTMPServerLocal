#include <glib.h>
#include <gst/gst.h>
#include <glib-unix.h>
#include <gst/app/gstappsink.h>
#include <mutex>
#include <iostream>

/*
g++ -o main1 main1.c $(pkg-config --cflags --libs gstreamer-1.0)
*/

void _onPadAdded(GstElement* src, GstPad* src_pad, gpointer user_data){
    GstPad* sink_pad = (GstPad*)user_data;
    gst_pad_link(src_pad, sink_pad);
}

int main(int argc, char *argv[]) {

    /* Initialize GStreamer */
    gst_init(&argc, &argv);
    GMainLoop* loop = g_main_loop_new (NULL, FALSE);

    // video
    GstElement *pipeline = gst_pipeline_new("rtsp-to-rtmp");
    GstElement *rtspsrc = gst_element_factory_make("rtspsrc", "rtspsrc");
    GstElement *depay = gst_element_factory_make("rtph264depay", "depay");
    GstElement *parse = gst_element_factory_make("h264parse", "h264parse");
    GstElement *flvmux = gst_element_factory_make("flvmux", "flvmux");
    GstElement *queue = gst_element_factory_make("queue", "queue");
    GstElement *rtmpsink = gst_element_factory_make("rtmpsink", "rtmpsink");

    // audio
    GstElement *audiotestsrc = gst_element_factory_make("audiotestsrc", "audiotestsrc");
    GstElement *audioconvert = gst_element_factory_make("audioconvert", "audioconvert");
    GstElement *audioresample = gst_element_factory_make("audioresample", "audioresample");
    GstElement *voaacenc = gst_element_factory_make("voaacenc", "voaacenc");
    GstElement *aacparse = gst_element_factory_make("aacparse", "aacparse");

    /* Set properties video*/
    g_object_set(rtspsrc, "location", "rtsp://127.0.0.1:8554/test", NULL);
    g_object_set(rtspsrc, "latency", 0, NULL);
    g_object_set(rtmpsink, "location", "rtmp://127.0.0.1/live/stream", NULL);

    /* Add all elements to the pipeline */
    gst_bin_add_many(GST_BIN(pipeline), rtspsrc, depay, parse, flvmux, queue, rtmpsink, 
                    audiotestsrc, audioconvert, audioresample, voaacenc, aacparse, NULL );
    g_signal_connect(rtspsrc, "pad-added", G_CALLBACK(&_onPadAdded), gst_element_get_static_pad(depay, "sink"));
    gst_element_link_many(depay, parse, flvmux, queue, rtmpsink, NULL);

    /* Build audio chain */
    g_object_set(audiotestsrc, "is-live", TRUE, NULL);
    g_object_set(voaacenc, "bitrate", 96000, NULL);
    GstCaps *audio_caps = gst_caps_from_string("audio/x-raw,rate=48000");
    GstCaps *aac_caps = gst_caps_from_string("audio/mpeg,mpegversion=4");
    gst_element_link(audiotestsrc, audioconvert);
    gst_element_link(audioconvert, audioresample);
    gst_element_link_filtered(audioresample, voaacenc, audio_caps);
    gst_element_link(voaacenc, aacparse);
    gst_element_link_filtered(aacparse, flvmux, aac_caps);
    gst_caps_unref(audio_caps);
    gst_caps_unref(aac_caps);

    /* Start playing */
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    g_main_loop_run (loop);
    g_print("Stop\n");

    gst_element_set_state (pipeline, GST_STATE_NULL);
    gst_object_unref (GST_OBJECT (pipeline));

    gst_deinit ();

    return 0;
}
