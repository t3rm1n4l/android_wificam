#!/usr/bin/env python

import gst
import time
import gobject

pipe = gst.Pipeline("wificam")

tcpserver = gst.element_factory_make("tcpserversrc","TCPServer")
tcpserver.set_property("host","192.168.1.133")
tcpserver.set_property("port",5000)

caps = gst.Caps("video/x-h263, framerate=(fraction)80/11")
q1 = gst.element_factory_make("queue","prequeue")
q2 = gst.element_factory_make("queue","postqueue")
capsfilter = gst.element_factory_make("capsfilter","format")
capsfilter.set_property("caps",caps)
decoder = gst.element_factory_make("ffdec_h263","decoder")
ffmpeg = gst.element_factory_make("ffmpegcolorspace","colorfilter")
camdev = gst.element_factory_make("v4l2wificam","camdev")

pipe.add(tcpserver,capsfilter,decoder,ffmpeg,camdev)
gst.element_link_many(tcpserver,capsfilter,decoder,ffmpeg,camdev)

pipe.set_state(gst.STATE_PLAYING)

loop = gobject.MainLoop()
loop.run()
