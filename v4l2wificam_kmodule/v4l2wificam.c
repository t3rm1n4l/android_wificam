#include <linux/version.h>
#include <linux/vmalloc.h>
#include <linux/mm.h>
#include <linux/time.h>
#include <linux/module.h>
#include <linux/videodev2.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-common.h>

#include <linux/sched.h>
#include <linux/slab.h>


MODULE_DESCRIPTION("V4L2 Wificam device driver");
MODULE_AUTHOR("Group - 9 CSB 2011");
MODULE_LICENSE("GPL");


struct v4l2wificam_private {
  int devicenr;
};



typedef struct v4l2wificam_private *priv_ptr;


/* Settings and state of camera device */
struct v4l2wificam_device {
  struct video_device *vdev;
  struct v4l2_pix_format pix_format;
  struct v4l2_captureparm capture_param;

  //Video buffers
  u8 *image;        
  unsigned long int imagesize; 
  int buffers_number;  
  struct v4l2_buffer *buffers;	
  int write_position; //Last written frame + 1
  long buffer_size;

  //Synchronisation
  atomic_t open_count;
  int ready_for_capture; //If somebody wrote, set to true
  wait_queue_head_t read_event;
};

//Operation
enum opener_type {
  UNNEGOTIATED = 0,
  READER = 1,
  WRITER = 2,
};


//State and type of operation
struct v4l2wificam_opener {
  enum opener_type type;
  int buffers_number;
  int position; 
  struct v4l2_buffer *buffers;
};



static int max_buffers_number = 8;

static int max_openers = 10;


#define MAX_DEVICES 8
static int devices = 1;


/* module constants */
#define MAX_MMAP_BUFFERS 100  

static __u32 validformats[] = {
  V4L2_PIX_FMT_YUYV,
  V4L2_PIX_FMT_YYUV,
  V4L2_PIX_FMT_YVYU,
  V4L2_PIX_FMT_UYVY,
  V4L2_PIX_FMT_VYUY,
  V4L2_PIX_FMT_YVU410,
  V4L2_PIX_FMT_YVU420,
  V4L2_PIX_FMT_YUV32,
  V4L2_PIX_FMT_YUV410,
  V4L2_PIX_FMT_YUV420,
  V4L2_PIX_FMT_BGR24,
  V4L2_PIX_FMT_RGB24,
  V4L2_PIX_FMT_BGR32,
  V4L2_PIX_FMT_RGB32,
  V4L2_PIX_FMT_GREY,
  V4L2_PIX_FMT_Y16
};

#define v4l2wificam_SIZE_MIN_WIDTH   48
#define v4l2wificam_SIZE_MIN_HEIGHT  32
#define v4l2wificam_SIZE_MAX_WIDTH   8192
#define v4l2wificam_SIZE_MAX_HEIGHT  8192

#define v4l2wificam_SIZE_DEFAULT_WIDTH   640
#define v4l2wificam_SIZE_DEFAULT_HEIGHT  480




static int v4l2l_checkformat(const __u32 format) {
  const __u32 numformats=sizeof(validformats)/sizeof(*validformats);
  __u32 i=0;
  for(i=0; i < numformats; i++) {
    if(format == validformats[i])
      return 1;
  }
 
  printk("Wificam: unsupported format\n");

  return 0;
}

static unsigned int v4l2l_getbytesperline(const __u32 format, 
					  __u32*_width, 
					  __u32*_height, 
					  __u32*_bytesperline ) {
  __u32 bytesperline=0;
  __u32 width=0;
  __u32 height=0;
  if(NULL == _width || NULL == _height)
    return 0;

  width=*_width;
  height=*_height;

  if(!v4l2l_checkformat(format))
    return 0;

  switch(format) {
  case(V4L2_PIX_FMT_YVU410): 
  case(V4L2_PIX_FMT_YUV410): 
    /* planar YUV(4:1:0): width/height need to be multiples of 4 */
    width =((width +3)>>2)<<2;
    height=((height+3)>>2)<<2;
    bytesperline=width*1;
    break;
  case(V4L2_PIX_FMT_YVU420): 
  case(V4L2_PIX_FMT_YUV420): 
    /* planar YUV(4:2:0): width/height need to be multiples of 2 */
    width =((width +1)>>1)<<1;
    height=((height+1)>>1)<<1;
    bytesperline=width*1;
    break;
  case(V4L2_PIX_FMT_YUYV): 
  case(V4L2_PIX_FMT_YYUV): 
  case(V4L2_PIX_FMT_YVYU): 
  case(V4L2_PIX_FMT_UYVY): 
  case(V4L2_PIX_FMT_VYUY): 
    /* packed YUV(4:2:22): width needs to be multiple of 2 */
    width =((width +1)>>1)<<1;
    bytesperline=width*2;
    break;
  case(V4L2_PIX_FMT_YUV32): 
    bytesperline=width*4;
    break;

  case(V4L2_PIX_FMT_BGR24):
  case(V4L2_PIX_FMT_RGB24):
    bytesperline=width*3;
    break;
    break;
  case(V4L2_PIX_FMT_BGR32): 
  case(V4L2_PIX_FMT_RGB32):
    bytesperline=width*4;
    break;
    break;

  case(V4L2_PIX_FMT_GREY): 
    bytesperline=width*1;
    break;
  case(V4L2_PIX_FMT_Y16):
    bytesperline=width*2; 
    break;
  default:
    return 0;
  }

  if(NULL != _bytesperline) {
    /* TODO: check whether we could actually use the given value */
    *_bytesperline=bytesperline;
  }

  *_width =width;
  *_height=height;
  
  return bytesperline;
}

/* global module data */
struct v4l2wificam_device *devs[MAX_DEVICES];

static struct v4l2wificam_device*v4l2wificam_getdevice(struct file*f) {
  struct video_device *loopdev = video_devdata(f);

  priv_ptr ptr = (priv_ptr)video_get_drvdata(loopdev);

  int nr = ptr->devicenr;
  if(nr<0 || nr>=devices){printk(KERN_ERR "v4l2-wificam: illegal device %d\n",nr);return NULL;}
  return devs[nr];
}

/* forward declarations */
static void init_buffers(struct v4l2wificam_device *dev);
static int allocate_buffers(struct v4l2wificam_device *dev);
static int free_buffers(struct v4l2wificam_device *dev);
static const struct v4l2_file_operations v4l2wificam_fops;
static const struct v4l2_ioctl_ops v4l2wificam_ioctl_ops;


/* Queue helpers */
/* next functions sets buffer flags and adjusts counters accordingly */
static inline void set_done(struct v4l2_buffer *buffer)
{
  buffer->flags &= ~V4L2_BUF_FLAG_QUEUED;
  buffer->flags |= V4L2_BUF_FLAG_DONE;
}

static inline void set_queued(struct v4l2_buffer *buffer)
{
  buffer->flags &= ~V4L2_BUF_FLAG_DONE;
  buffer->flags |= V4L2_BUF_FLAG_QUEUED;
}

static inline void unset_flags(struct v4l2_buffer *buffer)
{
  buffer->flags &= ~V4L2_BUF_FLAG_QUEUED;
  buffer->flags &= ~V4L2_BUF_FLAG_DONE;
}
/* V4L2 ioctl caps and params calls */
/* returns device capabilities
 * called on VIDIOC_QUERYCAP
 */
static int vidioc_querycap(struct file *file,
			   void *priv, struct v4l2_capability *cap)
{
  strlcpy(cap->driver, "wificam", sizeof(cap->driver));
  strlcpy(cap->card, "wificam-dev", sizeof(cap->card));
  cap->bus_info[0]=0;

  cap->version = 1;
  cap->capabilities =
    V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_VIDEO_OUTPUT |
    V4L2_CAP_STREAMING |
    V4L2_CAP_READWRITE
    ;

  memset(cap->reserved, 0, sizeof(cap->reserved));

  return 0;
}

static int vidioc_enum_framesizes(struct file *file, void *fh, 
                                  struct v4l2_frmsizeenum *argp)
{
  struct v4l2wificam_device *dev=v4l2wificam_getdevice(file);
  if (dev->ready_for_capture) {
    /* format has already been negotiated
     * cannot change during runtime
     */
    struct  v4l2_frmsize_discrete discr=argp->discrete;

    if (argp->index)
      return -EINVAL;
    
    argp->type=V4L2_FRMSIZE_TYPE_DISCRETE;
    
    discr.width=dev->pix_format.width;
    discr.height=dev->pix_format.height;
  } else {


    if (argp->index)
      return -EINVAL;

    argp->type=V4L2_FRMSIZE_TYPE_CONTINUOUS;

    argp->stepwise.min_width=v4l2wificam_SIZE_MIN_WIDTH;
    argp->stepwise.min_height=v4l2wificam_SIZE_MIN_HEIGHT;

    argp->stepwise.max_width=v4l2wificam_SIZE_MAX_WIDTH;
    argp->stepwise.max_height=v4l2wificam_SIZE_MAX_HEIGHT;

    argp->stepwise.step_width=1;
    argp->stepwise.step_height=1;
  }
  return 0;
}

/* ------------------ CAPTURE ----------------------- */

/* returns device formats
 * called on VIDIOC_ENUM_FMT, with v4l2_buf_type set to V4L2_BUF_TYPE_VIDEO_CAPTURE
 */
static int vidioc_enum_fmt_cap(struct file *file, void *fh,
			       struct v4l2_fmtdesc *f)
{
  struct v4l2wificam_device *dev=v4l2wificam_getdevice(file);
  if (f->index)
    return -EINVAL;
  if (dev->ready_for_capture) {
    const __u32 format = dev->pix_format.pixelformat;
    //  strlcpy(f->description, "current format", sizeof(f->description));

    snprintf(f->description, sizeof(f->description),
	     "[%c%c%c%c]",
	     (format>> 0) & 0xFF,
	     (format>> 8) & 0xFF,
	     (format>>16) & 0xFF,
	     (format>>24) & 0xFF);

    f->pixelformat = dev->pix_format.pixelformat;
  } else {
    return -EINVAL;
  }
  f->flags=0;

  return 0;
}

/* returns current video format format fmt
 * called on VIDIOC_G_FMT, with v4l2_buf_type set to V4L2_BUF_TYPE_VIDEO_CAPTURE
 */
static int vidioc_g_fmt_cap(struct file *file,
			    void *priv, struct v4l2_format *fmt)
{
  struct v4l2wificam_device *dev=v4l2wificam_getdevice(file);

  if (dev->ready_for_capture == 0) {
    return -EINVAL;
  }
  fmt->fmt.pix = dev->pix_format;
  return 0;
}

/* checks if it is OK to change to format fmt;
 * actual check is done by inner_try_fmt_cap
 * just checking that pixelformat is OK and set other parameters, app should
 * obey this decision 
 * called on VIDIOC_TRY_FMT, with v4l2_buf_type set to V4L2_BUF_TYPE_VIDEO_CAPTURE
 */
static int vidioc_try_fmt_cap(struct file *file,
			      void *priv, struct v4l2_format *fmt)
{
  struct v4l2wificam_opener *opener = file->private_data;
  struct v4l2wificam_device *dev=v4l2wificam_getdevice(file);
  opener->type = READER;

  if (0 == dev->ready_for_capture) {
    return -EBUSY;
  }

  if (fmt->fmt.pix.pixelformat != dev->pix_format.pixelformat)
    return -EINVAL;

  fmt->fmt.pix = dev->pix_format;
  return 0;
}

/* sets new output format, if possible
 */
static int vidioc_s_fmt_cap(struct file *file,
			    void *priv, struct v4l2_format *fmt)
{
  return vidioc_try_fmt_cap(file, priv, fmt);
}


/* ------------------ OUTPUT ----------------------- */

/* returns device formats;
 * LATER: allow all formats 
 * called on VIDIOC_ENUM_FMT, with v4l2_buf_type set to V4L2_BUF_TYPE_VIDEO_OUTPUT
 */
static int vidioc_enum_fmt_out(struct file *file, void *fh,
			       struct v4l2_fmtdesc *f)
{
  struct v4l2wificam_device *dev=v4l2wificam_getdevice(file);

  if (dev->ready_for_capture) {
    const __u32 format = dev->pix_format.pixelformat;

    if (f->index)
      return -EINVAL;



    snprintf(f->description, sizeof(f->description),
	     "[%c%c%c%c]",
	     (format>> 0) & 0xFF,
	     (format>> 8) & 0xFF,
	     (format>>16) & 0xFF,
	     (format>>24) & 0xFF);

    f->pixelformat = dev->pix_format.pixelformat;
  } else {
    __u32 format;

    if(f->index < 0 || 
       f->index >= (sizeof(validformats)/sizeof(*validformats))) 
      return -EINVAL;

    f->pixelformat=validformats[f->index];
    format = f->pixelformat;



    snprintf(f->description, sizeof(f->description),
	     "(%c%c%c%c)",
	     (format>> 0) & 0xFF,
	     (format>> 8) & 0xFF,
	     (format>>16) & 0xFF,
	     (format>>24) & 0xFF);

  }
  f->flags=0;

  return 0;
}

/* returns current video format format fmt */
/* NOTE: this is called from the producer
 * so if format has not been ngotiated yet, 
 * it should return ALL of available formats, 
 * called on VIDIOC_G_FMT, with v4l2_buf_type set to V4L2_BUF_TYPE_VIDEO_OUTPUT
 */
static int vidioc_g_fmt_out(struct file *file,
			    void *priv, struct v4l2_format *fmt)
{
  struct v4l2wificam_device *dev=v4l2wificam_getdevice(file);
  struct v4l2wificam_opener *opener = file->private_data;
  opener->type = WRITER;

#if 1
  if (0 == dev->ready_for_capture) {
#else
  if (0 == dev->pix_format.sizeimage) {
#endif
    dev->pix_format.width=0; /* v4l2wificam_SIZE_DEFAULT_WIDTH; */
    dev->pix_format.height=0; /* v4l2wificam_SIZE_DEFAULT_HEIGHT; */
    dev->pix_format.pixelformat=validformats[0];
    dev->pix_format.colorspace=V4L2_COLORSPACE_SRGB;
    dev->pix_format.field=V4L2_FIELD_NONE;

    dev->pix_format.bytesperline=v4l2l_getbytesperline(dev->pix_format.pixelformat,
						       &dev->pix_format.width,
						       &dev->pix_format.height,
						       NULL);
    dev->pix_format.sizeimage=dev->pix_format.bytesperline*dev->pix_format.height;

    dev->buffer_size = PAGE_ALIGN(dev->pix_format.sizeimage);
    allocate_buffers(dev);
    init_buffers(dev);
  }
  fmt->fmt.pix = dev->pix_format;
  return 0;
}

/* checks if it is OK to change to format fmt;
 * if format is negotiated do not change it 
 * called on VIDIOC_TRY_FMT with v4l2_buf_type set to V4L2_BUF_TYPE_VIDEO_OUTPUT
 */
static int vidioc_try_fmt_out(struct file *file,
			      void *priv, struct v4l2_format *fmt)
{
  struct v4l2wificam_opener *opener = file->private_data;
  struct v4l2wificam_device *dev=v4l2wificam_getdevice(file);
  opener->type = WRITER;

  if (dev->ready_for_capture) {
    fmt->fmt.pix = dev->pix_format;
  } else {
    __u32 w=fmt->fmt.pix.width;
    __u32 h=fmt->fmt.pix.height;
    __u32 bpl=fmt->fmt.pix.bytesperline;
    __u32 pixfmt=fmt->fmt.pix.pixelformat;
    
    if(w<1)
      w=v4l2wificam_SIZE_DEFAULT_WIDTH;

    if(h<1)
      h=v4l2wificam_SIZE_DEFAULT_HEIGHT;

    if(0 == v4l2l_getbytesperline(pixfmt,
				  &w,
				  &h,
				  &bpl)) {
      /* try again, with safe default */
      pixfmt=validformats[0];
      if(0 == v4l2l_getbytesperline(pixfmt,
				    &w,
				    &h,
				    &bpl)) {
	return -EINVAL;
      }
    }

    if (bpl * h == 0)
      return -EINVAL;

    fmt->fmt.pix.pixelformat = pixfmt;
    fmt->fmt.pix.width =w;
    fmt->fmt.pix.height=h;
    
    fmt->fmt.pix.bytesperline=bpl;

    fmt->fmt.pix.sizeimage=bpl*h;
    fmt->fmt.pix.colorspace=V4L2_COLORSPACE_SRGB;

    if(V4L2_FIELD_ANY == fmt->fmt.pix.field)
      fmt->fmt.pix.field=V4L2_FIELD_NONE;

    dev->pix_format = fmt->fmt.pix;
  }
  return 0;
}

/* sets new output format, if possible;
 * allocate data here because we do not know if it will be streaming or
 * read/write IO 
 * called on VIDIOC_S_FMT with v4l2_buf_type set to V4L2_BUF_TYPE_VIDEO_OUTPUT
 */
static int vidioc_s_fmt_out(struct file *file,
			    void *priv, struct v4l2_format *fmt)
{
  int ret = vidioc_try_fmt_out(file, priv, fmt);
  struct v4l2wificam_device *dev=v4l2wificam_getdevice(file);

  if (ret < 0)
    return ret;


  if (dev->ready_for_capture == 0) {
    dev->buffer_size = PAGE_ALIGN(dev->pix_format.sizeimage);
    fmt->fmt.pix.sizeimage = dev->buffer_size;
    init_buffers(dev);
  }
  return ret;
}

//#define V4L2L_OVERLAY
#ifdef V4L2L_OVERLAY
/* ------------------ OVERLAY ----------------------- */
/* currently unsupported */
/* GSTreamer's v4l2sink is buggy, as it requires the overlay to work
 * while it should only require it, if overlay is requested
 * once the gstreamer element is fixed, remove the overlay dummies
 */
#warning OVERLAY dummies
static int vidioc_g_fmt_overlay(struct file *file,
				void *priv, struct v4l2_format *fmt)
{
  return 0;
}
static int vidioc_s_fmt_overlay(struct file *file,
				void *priv, struct v4l2_format *fmt)
{
  return 0;
}
#endif /* V4L2L_OVERLAY */


/* ------------------ PARAMs ----------------------- */

/* get some data flaw parameters, only capability, fps and readbuffers has
 * effect on this driver
 * called on VIDIOC_G_PARM 
 */
static int vidioc_g_parm(struct file *file, void *priv,
			 struct v4l2_streamparm *parm)
{
  /* do not care about type of opener, hope this enums would always be
   * compatible */
  struct v4l2wificam_device *dev=v4l2wificam_getdevice(file);
  parm->parm.capture = dev->capture_param;
  return 0;
}

/* get some data flaw parameters, only capability, fps and readbuffers has
 * effect on this driver
 * called on VIDIOC_S_PARM 
 */
static int vidioc_s_parm(struct file *file, void *priv,
			 struct v4l2_streamparm *parm)
{
  struct v4l2wificam_device *dev=v4l2wificam_getdevice(file);

  switch (parm->type) {
  case V4L2_BUF_TYPE_VIDEO_CAPTURE:
    parm->parm.capture = dev->capture_param;
    return 0;
  case V4L2_BUF_TYPE_VIDEO_OUTPUT:

    parm->parm.capture = dev->capture_param;
    return 0;
  default:
    return -1;
  }
}

/* sets a tv standard, actually we do not need to handle this any special way
 * added to support effecttv
 * called on VIDIOC_S_STD
 */
static int vidioc_s_std(struct file *file, void *private_data,
			v4l2_std_id *norm)
{
  return 0;
}


/* gets a fake video standard
 * called on VIDIOC_G_STD
 */
static int vidioc_g_std(struct file *file, void *private_data,
			v4l2_std_id *norm)
{
  if(norm)
    *norm=V4L2_STD_ALL;
  return 0;
}
/* gets a fake video standard
 * called on VIDIOC_QUERYSTD
 */
static int vidioc_querystd(struct file *file, void *private_data,
			v4l2_std_id *norm)
{
  if(norm)
    *norm=V4L2_STD_UNKNOWN;
  return 0;
}




/* returns set of device outputs, in our case there is only one
 * called on VIDIOC_ENUMOUTPUT
 */
static int vidioc_enum_output(struct file *file, void *fh,
			      struct v4l2_output *outp)
{
  struct v4l2wificam_device *dev=v4l2wificam_getdevice(file);
  if (outp->index) {
    return -EINVAL;
  }
  
  if (dev->ready_for_capture)
    return -EINVAL;
 
  strlcpy(outp->name, "loopback in", sizeof(outp->name));
  outp->type = V4L2_OUTPUT_TYPE_ANALOG;
  outp->audioset = 0;
  outp->modulator = 0;
  outp->std = V4L2_STD_ALL;
  return 0;
}

/* returns set of device inputs, in our case there is only one, 
 * but later I may add more
 * called on VIDIOC_ENUMINPUT 
 */
static int vidioc_enum_input(struct file *file, void *fh,
			     struct v4l2_input *inp)
{
  struct v4l2wificam_device *dev=v4l2wificam_getdevice(file);
  if (dev->ready_for_capture == 0)
    return -EINVAL;
  if (inp->index == 0) {
    strlcpy(inp->name, "loopback", sizeof(inp->name));
    inp->type = V4L2_INPUT_TYPE_CAMERA;
    inp->audioset = 0;
    inp->tuner = 0;
    inp->std = V4L2_STD_ALL;
    inp->status = 0;
    return 0;
  }
  return -EINVAL;
}




/* which input is currently active, 
 * called on VIDIOC_G_INPUT 
 */
int vidioc_g_input(struct file *file, void *fh, unsigned int *i)
{
  *i = 0;
  return 0;
}

/* set input, can make sense if we have more than one video src,
 * called on VIDIOC_S_INPUT 
 */
int vidioc_s_input(struct file *file, void *fh, unsigned int i)
{
  if (i == 0)
    return 0;
  return -EINVAL;
}

/* --------------- V4L2 ioctl buffer related calls ----------------- */

/* negotiate buffer type
 * only mmap streaming supported 
 * called on VIDIOC_REQBUFS
 */
static int vidioc_reqbufs(struct file *file, void *fh,
			  struct v4l2_requestbuffers *b)
{
  struct v4l2wificam_device *dev=v4l2wificam_getdevice(file);

  init_buffers(dev);
  switch (b->memory) {
  case V4L2_MEMORY_MMAP:
    /* do nothing here, buffers are always allocated*/
    if (b->count == 0)
      return 0;
    b->count = dev->buffers_number;
    return 0;
  default:
    return -EINVAL;
  }
}

/* returns buffer asked for;
 * give app as many buffers as it wants, if it less than MAX,
 * but map them in our inner buffers 
 * called on VIDIOC_QUERYBUF
 */
static int vidioc_querybuf(struct file *file, void *fh,
			   struct v4l2_buffer *b)
{
  enum v4l2_buf_type type = b->type;
  int index = b->index;
  struct v4l2wificam_device *dev=v4l2wificam_getdevice(file);

  if ((b->type != V4L2_BUF_TYPE_VIDEO_CAPTURE) &&
      (b->type != V4L2_BUF_TYPE_VIDEO_OUTPUT)) {
    return -EINVAL;
  }
  if (b->index > MAX_MMAP_BUFFERS)
    return -EINVAL;
  *b = dev->buffers[b->index % dev->buffers_number];
  b->type = type;
  b->index = index;

  return 0;
}

/* put buffer to queue
 * called on VIDIOC_QBUF
 */
static int vidioc_qbuf(struct file *file, void *private_data,
		       struct v4l2_buffer *buf)
{
  struct v4l2wificam_device *dev=v4l2wificam_getdevice(file);
  int index = buf->index % dev->buffers_number;

  if (buf->index > MAX_MMAP_BUFFERS)
    return -EINVAL;
  switch (buf->type) {
  case V4L2_BUF_TYPE_VIDEO_CAPTURE:
    set_queued(&dev->buffers[index]);
    return 0;
  case V4L2_BUF_TYPE_VIDEO_OUTPUT:
    do_gettimeofday(&dev->buffers[index].timestamp);
    set_done(&dev->buffers[index]);
    wake_up_all(&dev->read_event);
    return 0;
  default:
    return -EINVAL;
  }
}

/* put buffer to dequeue
 * called on VIDIOC_DQBUF
 */
static int vidioc_dqbuf(struct file *file, void *private_data,
			struct v4l2_buffer *buf)
{
  struct v4l2wificam_device *dev=v4l2wificam_getdevice(file);
  int index;
  struct v4l2wificam_opener *opener = file->private_data;

  switch (buf->type) {
  case V4L2_BUF_TYPE_VIDEO_CAPTURE:
    if ((dev->write_position <= opener->position) &&
	(file->f_flags&O_NONBLOCK))
      return -EAGAIN;
    wait_event_interruptible(dev->read_event, (dev->write_position >
					       opener->position));
    if (dev->write_position > opener->position+2)
      opener->position = dev->write_position - 1;
    index = opener->position % dev->buffers_number;
    if (!(dev->buffers[index].flags&V4L2_BUF_FLAG_MAPPED)) {

      return -EINVAL;
    }
    ++opener->position;
    unset_flags(&dev->buffers[index]);
    *buf = dev->buffers[index];
    return 0;
  case V4L2_BUF_TYPE_VIDEO_OUTPUT:
    index = dev->write_position % dev->buffers_number;
    unset_flags(&dev->buffers[index]);
    *buf = dev->buffers[index];
    ++dev->write_position;
    return 0;
  default:
    return -EINVAL;
  }
}

/* ------------- STREAMING ------------------- */

/* start streaming
 * called on VIDIOC_STREAMON
 */
static int vidioc_streamon(struct file *file, void *private_data,
			   enum v4l2_buf_type type)
{
  struct v4l2wificam_device *dev=v4l2wificam_getdevice(file);
  int ret;
  switch (type) {
  case V4L2_BUF_TYPE_VIDEO_OUTPUT:
    if (dev->ready_for_capture == 0) {
      ret = allocate_buffers(dev);
      if (ret < 0)
	return ret;
      init_buffers(dev);
      dev->ready_for_capture = 1;
    }
    return 0;
  case V4L2_BUF_TYPE_VIDEO_CAPTURE:
    if (dev->ready_for_capture == 0)
      return -EIO;
    return 0;
  default:
    return -EINVAL;
  }
}

/* stop streaming
 * called on VIDIOC_STREAMOFF
 */
static int vidioc_streamoff(struct file *file, void *private_data,
			    enum v4l2_buf_type type)
{
  return 0;
}

#ifdef CONFIG_VIDEO_V4L1_COMPAT
int vidiocgmbuf(struct file *file, void *fh, struct video_mbuf *p)
{
  struct v4l2wificam_device *dev=v4l2wificam_getdevice(file);
  p->frames = dev->buffers_number;
  p->offsets[0] = 0;
  p->offsets[1] = 0;
  p->size = dev->buffer_size;
  return 0;
}
#endif


/* file operations */
static void vm_open(struct vm_area_struct *vma)
{

}

static void vm_close(struct vm_area_struct *vma)
{

}

static struct vm_operations_struct vm_ops = {
  .open = vm_open,
  .close = vm_close,
};

static int v4l2wificam_mmap(struct file *file,
			      struct vm_area_struct *vma)
{

  struct page *page = NULL;
  unsigned long addr;
  unsigned long start = (unsigned long) vma->vm_start;
  unsigned long size = (unsigned long) (vma->vm_end - vma->vm_start);
  struct v4l2wificam_device *dev=v4l2wificam_getdevice(file);


  if (size > dev->buffer_size) {
    return -EINVAL;
  }
  if ((vma->vm_pgoff << PAGE_SHIFT) >
      dev->buffer_size * (dev->buffers_number - 1)) {
    return -EINVAL;
  }

  if(NULL==dev->image) {
    if(allocate_buffers(dev)<0) {
      return -EINVAL;
    }
    init_buffers(dev);
  }

  addr = (unsigned long) dev->image + (vma->vm_pgoff << PAGE_SHIFT);

  while (size > 0) {

    page = (void *) vmalloc_to_page((void *) addr);

    if (vm_insert_page(vma, start, page) < 0)
      return -EAGAIN;

    start += PAGE_SIZE;
    addr += PAGE_SIZE;
    size -= PAGE_SIZE;
  }

  vma->vm_ops = &vm_ops;
  vma->vm_private_data = 0;
  dev->buffers[(vma->vm_pgoff<<PAGE_SHIFT)/dev->buffer_size].flags |=
    V4L2_BUF_FLAG_MAPPED;

  vm_open(vma);

  return 0;
}

static unsigned int v4l2wificam_poll(struct file *file,
				       struct poll_table_struct *pts)
{
  struct v4l2wificam_opener *opener = file->private_data;
  struct v4l2wificam_device *dev=v4l2wificam_getdevice(file);
  int ret_mask = 0;

  switch (opener->type) {
  case WRITER:
    ret_mask = POLLOUT | POLLWRNORM;
    break;
  case READER:
    poll_wait(file, &dev->read_event, pts);
    if (dev->write_position > opener->position)
      ret_mask =  POLLIN | POLLRDNORM;
    break;
  default:
    ret_mask = -POLLERR;
  }
  return ret_mask;
}

/* do not want to limit device opens, it can be as many readers as user want,
 * writers are limited by means of setting writer field */
static int v4l_loopback_open(struct file *file)
{
  struct v4l2wificam_opener *opener;
  struct v4l2wificam_device *dev=v4l2wificam_getdevice(file);


  if (dev->open_count.counter >= max_openers)
    return -EBUSY;
  /* kfree on close */
  opener = kzalloc(sizeof(*opener), GFP_KERNEL);
  if (opener == NULL)
    return -ENOMEM;
  file->private_data = opener;
  atomic_inc(&dev->open_count);
  return 0;
}

static int v4l_loopback_close(struct file *file)
{
  struct v4l2wificam_opener *opener = file->private_data;
  struct v4l2wificam_device *dev=v4l2wificam_getdevice(file);


  atomic_dec(&dev->open_count);

  if (dev->open_count.counter == 0) {
    free_buffers(dev);
    dev->ready_for_capture = 0;
    dev->buffer_size = 0;
  }
  kfree(opener);
  return 0;
}

static ssize_t v4l_loopback_read(struct file *file, char __user *buf,
				 size_t count, loff_t *ppos)
{
  int read_index;
  struct v4l2wificam_opener *opener = file->private_data;
  struct v4l2wificam_device *dev=v4l2wificam_getdevice(file);

  if ((dev->write_position <= opener->position) &&
      (file->f_flags&O_NONBLOCK)) {
    return -EAGAIN;
  }
  wait_event_interruptible(dev->read_event,
			   (dev->write_position > opener->position));
  if (count > dev->buffer_size)
    count = dev->buffer_size;
  if (dev->write_position > opener->position+2)
    opener->position = dev->write_position - 1;
  read_index = opener->position % dev->buffers_number;
  if (copy_to_user((void *) buf, (void *) (dev->image +
					   dev->buffers[read_index].m.offset), count)) {
    printk(KERN_ERR "v4l2-loopback: "
	   "failed copy_from_user() in write buf\n");
    return -EFAULT;
  }
  ++opener->position;
  return count;
}

static ssize_t v4l_loopback_write(struct file *file,
				  const char __user *buf, size_t count,
				  loff_t *ppos)
{
  struct v4l2wificam_device *dev=v4l2wificam_getdevice(file);
  int write_index = dev->write_position % dev->buffers_number;
  int ret;

  if (dev->ready_for_capture == 0) {
    ret = allocate_buffers(dev);
    if (ret < 0)
      return ret;
    init_buffers(dev);
    dev->ready_for_capture = 1;
  }

  if (count > dev->buffer_size)
    count = dev->buffer_size;
  if (copy_from_user(
		     (void *) (dev->image + dev->buffers[write_index].m.offset),
		     (void *) buf, count)) {
    printk(KERN_ERR "v4l2-loopback: "
	   "failed copy_from_user() in write buf, could not write %d\n",
	   count);
    return -EFAULT;
  }
  do_gettimeofday(&dev->buffers[write_index].timestamp);
  dev->buffers[write_index].sequence = dev->write_position++;
  wake_up_all(&dev->read_event);

  return count;
}

/* init functions */
/* frees buffers, if already allocated */
static int free_buffers(struct v4l2wificam_device *dev)
{
  if(dev->image) {
    vfree(dev->image);
    dev->image=NULL;
  }
  dev->imagesize=0;

  return 0;
}


/* allocates buffers, if buffer_size is set */
static int allocate_buffers(struct v4l2wificam_device *dev)
{

  if (dev->buffer_size == 0)
    return -EINVAL;

  if (dev->image) {

    if(dev->buffer_size * dev->buffers_number == dev->imagesize)
      return 0;

    /* if there is only one writer, no problem should occur */
    if (dev->open_count.counter==1) 
      free_buffers(dev);
    else
      return -EINVAL;
  }
  dev->imagesize=dev->buffer_size * dev->buffers_number;
  dev->image = vmalloc(dev->imagesize);

  if (dev->image == NULL)
    return -ENOMEM;

  return 0;
}
/* init inner buffers, they are capture mode and flags are set as
 * for capture mod buffers */
static void init_buffers(struct v4l2wificam_device *dev)
{
  int i;
  int buffer_size=dev->buffer_size;
  int bytesused = dev->pix_format.sizeimage;
  for (i = 0; i < dev->buffers_number; ++i) {
    dev->buffers[i].bytesused         = bytesused;
    dev->buffers[i].length            = buffer_size;
    dev->buffers[i].field             = V4L2_FIELD_NONE;
    dev->buffers[i].flags             = 0;
    dev->buffers[i].index             = i;
    dev->buffers[i].input             = 0;
    dev->buffers[i].m.offset          = i * buffer_size;
    dev->buffers[i].memory            = V4L2_MEMORY_MMAP;
    dev->buffers[i].sequence          = 0;
    dev->buffers[i].timestamp.tv_sec  = 0;
    dev->buffers[i].timestamp.tv_usec = 0;
    dev->buffers[i].type              = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  }
  dev->write_position = 0;
}

/* fills and register video device */
static void init_vdev(struct video_device *vdev)
{
  strlcpy(vdev->name, "Loopback video device", sizeof(vdev->name));
  vdev->tvnorms      = V4L2_STD_ALL;
  vdev->current_norm = V4L2_STD_ALL;
  vdev->vfl_type     = VFL_TYPE_GRABBER;
  vdev->fops         = &v4l2wificam_fops;
  vdev->ioctl_ops    = &v4l2wificam_ioctl_ops;
  vdev->release      = &video_device_release;
  vdev->minor        = -1;

}

/* init default capture parameters, only fps may be changed in future */
static void init_capture_param(struct v4l2_captureparm *capture_param)
{
  capture_param->capability               = 0;
  capture_param->capturemode              = 0;
  capture_param->extendedmode             = 0;
  capture_param->readbuffers              = max_buffers_number;
  capture_param->timeperframe.numerator   = 1;
  capture_param->timeperframe.denominator = 30;
}

/* init loopback main structure */
static int v4l2wificam_init(struct v4l2wificam_device *dev, int nr)
{
  dev->vdev = video_device_alloc();
  if (dev->vdev == NULL)
    return -ENOMEM;

  video_set_drvdata(dev->vdev, kzalloc(sizeof(struct v4l2wificam_private),
				       GFP_KERNEL));

  if (video_get_drvdata(dev->vdev) == NULL) {

      kfree(dev->vdev);
      return -ENOMEM;
    }

    ((priv_ptr)video_get_drvdata(dev->vdev))->devicenr = nr;

    init_vdev(dev->vdev);
    init_capture_param(&dev->capture_param);
    dev->buffers_number = max_buffers_number;
    atomic_set(&dev->open_count, 0);
    dev->ready_for_capture = 0;
    dev->buffer_size = 0;
    dev->image = NULL;
    dev->imagesize = 0;

    /* kfree on module release */
    dev->buffers =
      kzalloc(sizeof(*dev->buffers) * dev->buffers_number,
	      GFP_KERNEL);
    if (dev->buffers == NULL) {

      kfree(video_get_drvdata(dev->vdev));
 
      kfree(dev->vdev);
      return -ENOMEM;
    }
    init_waitqueue_head(&dev->read_event);
    return 0;
  };

  /* LINUX KERNEL */
  static const struct v4l2_file_operations v4l2wificam_fops = {
    .owner   = THIS_MODULE,
    .open    = v4l_loopback_open,
    .release = v4l_loopback_close,
    .read    = v4l_loopback_read,
    .write   = v4l_loopback_write,
    .poll    = v4l2wificam_poll,
    .mmap    = v4l2wificam_mmap,
    .ioctl   = video_ioctl2,
  };

  static const struct v4l2_ioctl_ops v4l2wificam_ioctl_ops = {
    .vidioc_querycap         = &vidioc_querycap,
    .vidioc_enum_framesizes  = &vidioc_enum_framesizes,

    .vidioc_enum_output       = &vidioc_enum_output,

    .vidioc_enum_input       = &vidioc_enum_input,
    .vidioc_g_input          = &vidioc_g_input,
    .vidioc_s_input          = &vidioc_s_input,

    .vidioc_enum_fmt_vid_cap = &vidioc_enum_fmt_cap,
    .vidioc_g_fmt_vid_cap    = &vidioc_g_fmt_cap,
    .vidioc_s_fmt_vid_cap    = &vidioc_s_fmt_cap,
    .vidioc_try_fmt_vid_cap  = &vidioc_try_fmt_cap,

    .vidioc_enum_fmt_vid_out = &vidioc_enum_fmt_out,
    .vidioc_s_fmt_vid_out    = &vidioc_s_fmt_out,
    .vidioc_g_fmt_vid_out    = &vidioc_g_fmt_out,
    .vidioc_try_fmt_vid_out  = &vidioc_try_fmt_out,

#ifdef V4L2L_OVERLAY
    .vidioc_s_fmt_vid_overlay= &vidioc_s_fmt_overlay,
    .vidioc_g_fmt_vid_overlay= &vidioc_g_fmt_overlay,
#endif

    .vidioc_s_std            = &vidioc_s_std,
    .vidioc_g_std            = &vidioc_g_std,
    .vidioc_querystd         = &vidioc_querystd,

    .vidioc_g_parm           = &vidioc_g_parm,
    .vidioc_s_parm           = &vidioc_s_parm,

    .vidioc_reqbufs          = &vidioc_reqbufs,
    .vidioc_querybuf         = &vidioc_querybuf,
    .vidioc_qbuf             = &vidioc_qbuf,
    .vidioc_dqbuf            = &vidioc_dqbuf,

    .vidioc_streamon         = &vidioc_streamon,
    .vidioc_streamoff        = &vidioc_streamoff,

#ifdef CONFIG_VIDEO_V4L1_COMPAT
    .vidiocgmbuf             = &vidiocgmbuf,
#endif
  };

  static void zero_devices (void) {
    int i=0;
    for(i=0; i<MAX_DEVICES; i++) {
      devs[i]=NULL;
    }
  }

  static void free_devices (void) {
    int i=0;
    for(i=0; i<devices; i++) {
      if(NULL!=devs[i]) {

	kfree(video_get_drvdata(devs[i]->vdev));

	video_unregister_device(devs[i]->vdev);
	kfree(devs[i]->buffers);
	kfree(devs[i]);
	devs[i]=NULL;
      }
    }
  }

  int __init init_module()
  {
    int ret;
    int i=0;
    zero_devices();
    if (devices == -1) 
      devices = 1;

    if (devices > MAX_DEVICES) {
      devices = MAX_DEVICES;
      printk(KERN_INFO "number of devices is limited to: %d\n", MAX_DEVICES);
    }


    /* kfree on module release */
    for(i=0; i<devices; i++) {

      printk("Wificam: creating device #%d\n", i);
      devs[i] = kzalloc(sizeof(*devs[i]), GFP_KERNEL);
      if (devs[i] == NULL) {
	free_devices();
	return -ENOMEM;
      }
      ret = v4l2wificam_init(devs[i], i);
      if (ret < 0) {
	free_devices();
	return ret;
      }


      // register the device -> it creates /dev/video* 
      if (video_register_device(devs[i]->vdev, VFL_TYPE_GRABBER, -1) < 0) {
	video_device_release(devs[i]->vdev);
	printk(KERN_ERR "failed video_register_device()\n");
	free_devices();
	return -EFAULT;
      }
    }

    printk("Inserted wificam module\n");


    return 0;
  }

  void __exit cleanup_module()
  {

    //unregister the device -> it deletes /dev/video*
    free_devices();
    printk("Wificam module removed\n");
  }
