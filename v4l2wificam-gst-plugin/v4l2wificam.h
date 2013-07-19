#ifndef __GST_V4L2_WIFICAM_H__
#define __GST_V4L2_WIFICAM_H__

#include <gst/gst.h>

#include <linux/videodev2.h>

G_BEGIN_DECLS

/* #defines don't like whitespacey bits */
#define GST_TYPE_V4L2_WIFICAM \
  (gst_v4l2_wificam_get_type())
#define GST_V4L2_WIFICAM(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_V4L2_WIFICAM,GstV4L2Wificam))
#define GST_V4L2_WIFICAM_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_V4L2_WIFICAM,GstV4L2WificamClass))
#define GST_IS_V4L2_WIFICAM(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_V4L2_WIFICAM))
#define GST_IS_V4L2_WIFICAM_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_V4L2_WIFICAM))

typedef struct _GstV4L2Wificam      GstV4L2Wificam;
typedef struct _GstV4L2WificamClass GstV4L2WificamClass;

struct _GstV4L2Wificam
{
  GstVideoSink videosink;

  /*< private >*/
  char *videodev;   /* the video device */
  gint video_fd;    /* the video-device's file descriptor */

  struct v4l2_capability vcap;  /* the video device's capabilities */
  struct v4l2_format     vformat; /* the v4l2 format */
  GstCaps *current_caps;        /* the current negotiated caps */

  unsigned int width, height; /* width and height of the video */
};

struct _GstV4L2WificamClass 
{
  GstVideoSinkClass parent_class;
};

GType gst_v4l2_wificam_get_type (void);

G_END_DECLS

#endif /* __GST_V4L2_WIFICAM_H__ */
