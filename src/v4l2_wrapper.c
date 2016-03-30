/**
 * @file v4l2_wrapper.c
 * \brief pyv4l2: Python extension for video4linux2
 *
 * 2009, 2010, 2011 Fredrik Portstrom
 * 2016 Jimmy Durand Wesolowski
 *
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <Python.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <sys/mman.h>

#ifdef USE_LIBV4L
#  include <libv4l2.h>
#else
#  include <sys/ioctl.h>
#  define v4l2_close close
#  define v4l2_ioctl ioctl
#  define v4l2_mmap mmap
#  define v4l2_munmap munmap
#  define v4l2_open open
#endif

#ifndef Py_TYPE
#  define Py_TYPE(ob) (((PyObject*)(ob))->ob_type)
#endif

#if PY_MAJOR_VERSION < 3
#  define PyLong_FromLong PyInt_FromLong

#  define PYOBJECT_HEAD_INIT(TYPE, SZ)	PyObject_HEAD_INIT(TYPE) SZ,
#  define INIT_V4L2_CAPTURE(X)		initpyv4l2(X)
#  define PYSTRING_FROM_STRING(NAME)	PyString_FromString(NAME)
#  define PYSTRING_FROM_STR_SZ(V, LEN)	PyString_FromStringAndSize(V, LEN)
#  define PYMODINIT_FUNC_RETURN(RET)
#else /* PY_MAJOR_VERSION >= 3 */
#  define PYOBJECT_HEAD_INIT(TYPE, SZ)	PyVarObject_HEAD_INIT(TYPE, SZ)
#  define INIT_V4L2_CAPTURE(X)		PyInit_pyv4l2(X)
#  define PYSTRING_FROM_STRING(NAME)	PyBytes_FromString(NAME)
#  define PYSTRING_FROM_STR_SZ(V, LEN)	PyBytes_FromStringAndSize(V, LEN)
#  define PYMODINIT_FUNC_RETURN(RET)	(RET)
#endif

#ifndef V4L2_CID_AUTO_WHITE_BALANCE
#  define V4L2_CID_AUTO_WHITE_BALANCE		-1
#endif /* !V4L2_CID_AUTO_WHITE_BALANCE */

#ifndef V4L2_CID_WHITE_BALANCE_TEMPERATURE
#  define V4L2_CID_WHITE_BALANCE_TEMPERATURE	-1
#endif /* !V4L2_CID_WHITE_BALANCE_TEMPERATURE */

#ifndef V4L2_CID_EXPOSURE_ABSOLUTE
#  define V4L2_CID_EXPOSURE_ABSOLUTE		-1
#endif /* !V4L2_CID_EXPOSURE_ABSOLUTE */

#ifndef V4L2_CID_EXPOSURE_AUTO
#  define V4L2_CID_EXPOSURE_AUTO		-1
#endif /* !V4L2_CID_EXPOSURE_AUTO */

#ifndef V4L2_CID_FOCUS_AUTO
#  define V4L2_CID_FOCUS_AUTO			-1
#endif /* !V4L2_CID_FOCUS_AUTO */

#define IS_ARRAY(arg)			__builtin_choose_expr(		\
		__builtin_types_compatible_p(typeof(arg[0]) [],		\
					     typeof(arg)), 1, 0)
#define CLEAR(x)							\
	do {								\
		assert(IS_ARRAY(&(x)));					\
		memset(&(x), 0, sizeof(x));				\
	} while (0)

#define CLAMP(c) ((c) <= 0 ? 0 : (c) >= 65025 ? 255 : (c) >> 8)

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#define PYDICT_SETITEM_SEC2STR(ELT, NAME, VALUE)			\
	PyDict_SetItemString(ELT, NAME, PyFloat_FromDouble(fract2sec(VALUE)))
#define PYDICT_SETITEM_FPS2STR(ELT, NAME, VALUE)			\
	PyDict_SetItemString(ELT, NAME, PyFloat_FromDouble(fract2fps(VALUE)))

#define DECLARE_METHODS(NAME, PARAM)					\
	static PyObject *video_device_set_ ## NAME(video_device *videodev, \
						   PyObject *args)	\
	{								\
		return video_device_set_helper(PARAM, videodev, args);	\
	}								\
									\
	static PyObject *video_device_get_ ## NAME(video_device *videodev) \
	{								\
		return video_device_get_helper(PARAM, videodev);	\
	}

struct buffer {
	void *start;
	size_t length;
};

typedef struct {
	PyObject_HEAD int fd;
	const char *path;
	struct buffer *buffers;
	int buffer_count;
} video_device;

struct capability {
	int id;
	const char *name;
};

static struct capability capabilities[] = {
	{V4L2_CAP_ASYNCIO, "asyncio"},
	{V4L2_CAP_AUDIO, "audio"},
	{V4L2_CAP_HW_FREQ_SEEK, "hw_freq_seek"},
	{V4L2_CAP_RADIO, "radio"},
	{V4L2_CAP_RDS_CAPTURE, "rds_capture"},
	{V4L2_CAP_READWRITE, "readwrite"},
	{V4L2_CAP_SLICED_VBI_CAPTURE, "sliced_vbi_capture"},
	{V4L2_CAP_SLICED_VBI_OUTPUT, "sliced_vbi_output"},
	{V4L2_CAP_STREAMING, "streaming"},
	{V4L2_CAP_TUNER, "tuner"},
	{V4L2_CAP_VBI_CAPTURE, "vbi_capture"},
	{V4L2_CAP_VBI_OUTPUT, "vbi_output"},
	{V4L2_CAP_VIDEO_CAPTURE, "video_capture"},
	{V4L2_CAP_VIDEO_OUTPUT, "video_output"},
	{V4L2_CAP_VIDEO_OUTPUT_OVERLAY, "video_output_overlay"},
	{V4L2_CAP_VIDEO_OVERLAY, "video_overlay"}
};

#if PY_MAJOR_VERSION >= 3
static PyObject *initmodule(char *m_name, PyModuleDef_Slot *m_methods,
			    char *m_doc)
{
	static struct PyModuleDef moduledef = {
		PyModuleDef_HEAD_INIT,
		m_name,
		m_doc,
		-1,
		m_methods,
		NULL,
		NULL,
		NULL,
		NULL
	};

	return PyModule_Create(&moduledef);
}
#endif

static int open_check_err(video_device *videodev)
{
	if (0 > videodev->fd)
		PyErr_SetString(PyExc_ValueError,
				"I/O operation on closed file");

	return !videodev->fd;
}

static int my_ioctl(int fd, int request, void *arg)
{
	int result = -1;

	// Retry ioctl until it returns without being interrupted.
	while (result < 0) {
		result = v4l2_ioctl(fd, request, arg);
		if (result < 0 && errno != EINTR) {
			PyErr_SetFromErrno(PyExc_IOError);
			return 1;
		}
	}
	return 0;
}

static void video_device_unmap(video_device *videodev)
{
	int i;

	for (i = 0; i < videodev->buffer_count; i++)
		v4l2_munmap(videodev->buffers[i].start,
			    videodev->buffers[i].length);
}

static PyObject *video_device_open(video_device *videodev)
{
	videodev->fd = v4l2_open(videodev->path, O_RDWR | O_NONBLOCK);

	if (videodev->fd < 0) {
		PyErr_SetFromErrnoWithFilename(PyExc_IOError,
					       videodev->path);
		Py_RETURN_NONE;
	}

	Py_RETURN_NONE;
}

static PyObject *video_device_close(video_device *videodev)
{
	if (0 > videodev->fd)
		Py_RETURN_NONE;

	if (videodev->buffers)
		video_device_unmap(videodev);

	v4l2_close(videodev->fd);
	videodev->fd = -1;

	Py_RETURN_NONE;
}

static int video_device_init(video_device *videodev,
			     PyObject *args, PyObject *kwargs)
{
	if (!PyArg_ParseTuple(args, "s", &videodev->path))
		return -1;

	videodev->fd = -1;
	videodev->buffers = NULL;
	videodev->buffer_count = 0;

	return 0;
}

static void video_device_dealloc(video_device *videodev)
{
	if (0 <= videodev->fd) {
		if (videodev->buffers)
			video_device_unmap(videodev);

		v4l2_close(videodev->fd);
	}

	Py_TYPE(videodev)->tp_free(videodev);
}

static PyObject *video_device_get_info(video_device *videodev)
{
	int idx = 0;
	PyObject *set = Py_None;
	PyObject *elt = Py_None;
	struct v4l2_capability caps;

	if (0 != open_check_err(videodev))
		Py_RETURN_NONE;

	if (my_ioctl(videodev->fd, VIDIOC_QUERYCAP, &caps))
		Py_RETURN_NONE;

	if (0 == (set = PySet_New(NULL)))
		Py_RETURN_NONE;

	while (idx < ARRAY_SIZE(capabilities)) {
		if (caps.capabilities & capabilities[idx].id) {
			elt = PYSTRING_FROM_STRING(capabilities[idx].name);

			if (!elt) {
				Py_DECREF(set);
				Py_RETURN_NONE;
			}

			PySet_Add(set, elt);
		}

		idx++;
	}

	return Py_BuildValue("sssO", caps.driver, caps.card, caps.bus_info,
			     set);
}

static PyObject *video_device_set_format(video_device *videodev,
					 PyObject *args, PyObject *keywds)
{
	int size_x = 0;
	int size_y = 0;
	int yuv420 = 0;
	int fourcc = 0;
	int fourcc_len = 0;
	const char *fourcc_str;
	static char *kwlist[] = {
		"size_x",
		"size_y",
		"yuv420",
		"fourcc",
		NULL
	};
	struct v4l2_format format;

	if (!PyArg_ParseTupleAndKeywords(args, keywds, "ii|is#", kwlist,
					 &size_x, &size_y, &yuv420,
					 &fourcc_str, &fourcc_len))
		Py_RETURN_NONE;

	CLEAR(format);
	format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	/* Get the current format */
	if (my_ioctl(videodev->fd, VIDIOC_G_FMT, &format))
		Py_RETURN_NONE;

#ifdef USE_LIBV4L
	format.fmt.pix.pixelformat =
		yuv420 ? V4L2_PIX_FMT_YUV420 : V4L2_PIX_FMT_RGB24;
#else
	format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
#endif
	format.fmt.pix.field = V4L2_FIELD_INTERLACED;

	if (fourcc_len == 4) {
		fourcc = v4l2_fourcc(fourcc_str[0],
				     fourcc_str[1], fourcc_str[2],
				     fourcc_str[3]);
		format.fmt.pix.pixelformat = fourcc;
		format.fmt.pix.field = V4L2_FIELD_ANY;
	}

	format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	format.fmt.pix.width = size_x;
	format.fmt.pix.height = size_y;
	format.fmt.pix.bytesperline = 0;

	if (my_ioctl(videodev->fd, VIDIOC_S_FMT, &format)) {
		Py_RETURN_NONE;
	}

	return Py_BuildValue("ii", format.fmt.pix.width,
			     format.fmt.pix.height);
}

static PyObject *video_device_set_fps(video_device *videodev,
				      PyObject *args)
{
	int fps;
	struct v4l2_streamparm setfps;

	if (!PyArg_ParseTuple(args, "i", &fps))
		Py_RETURN_NONE;

	CLEAR(setfps);
	setfps.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	setfps.parm.capture.timeperframe.numerator = 1;
	setfps.parm.capture.timeperframe.denominator = fps;

	if (my_ioctl(videodev->fd, VIDIOC_S_PARM, &setfps))
		Py_RETURN_NONE;

	return Py_BuildValue("i", setfps.parm.capture.timeperframe.denominator);
}

static void get_fourcc_str(char *fourcc_str, int fourcc)
{
	if (fourcc_str == NULL)
		return;

	fourcc_str[0] = (char) (fourcc & 0xFF);
	fourcc_str[1] = (char) ((fourcc >> 8) & 0xFF);
	fourcc_str[2] = (char) ((fourcc >> 16) & 0xFF);
	fourcc_str[3] = (char) ((fourcc >> 24) & 0xFF);
	fourcc_str[4] = 0;
}

static PyObject *video_device_get_format(video_device *videodev)
{
	struct v4l2_format format;
	char current_fourcc[5];

	CLEAR(format);
	format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	/* Get the current format */
	if (my_ioctl(videodev->fd, VIDIOC_G_FMT, &format))
		Py_RETURN_NONE;

	get_fourcc_str(current_fourcc, format.fmt.pix.pixelformat);

	return Py_BuildValue("iis", format.fmt.pix.width,
			     format.fmt.pix.height,
			     current_fourcc);
}

static PyObject *video_device_get_fourcc(video_device *videodev,
					 PyObject *args)
{
	char *fourcc_str;
	int size = 0;
	int fourcc = 0;

	if (!PyArg_ParseTuple(args, "s#", &fourcc_str, &size))
		Py_RETURN_NONE;

	if (size < 4)
		Py_RETURN_NONE;

	fourcc = v4l2_fourcc(fourcc_str[0], fourcc_str[1],
			     fourcc_str[2], fourcc_str[3]);
	return Py_BuildValue("i", fourcc);
}

static PyObject *video_device_get_framesizes(video_device *videodev,
					     PyObject *args)
{
	int size = 0;
	PyObject *ret = Py_None;
	char *fourcc_str = NULL;
	struct v4l2_frmsizeenum frmsize;

	if (0 != open_check_err(videodev))
		Py_RETURN_NONE;

	CLEAR(frmsize);
	if (!PyArg_ParseTuple(args, "s#", &fourcc_str, &size))
		Py_RETURN_NONE;

	if (size != 4)
		Py_RETURN_NONE;

	frmsize.pixel_format = v4l2_fourcc(fourcc_str[0],
					   fourcc_str[1],
					   fourcc_str[2], fourcc_str[3]);
	frmsize.index = 0;
	ret = PyList_New(0);
	while (!my_ioctl(videodev->fd, VIDIOC_ENUM_FRAMESIZES, &frmsize)) {
		PyObject *cap = PyDict_New();
		switch (frmsize.type) {
		case V4L2_FRMSIZE_TYPE_DISCRETE:
			PyDict_SetItemString(cap, "size_x",
					     PyLong_FromLong(frmsize.discrete.
							     width));
			PyDict_SetItemString(cap, "size_y",
					     PyLong_FromLong(frmsize.discrete.
							     height));

			break;
		case V4L2_FRMSIZE_TYPE_STEPWISE:
			PyDict_SetItemString(cap, "min_width",
					     PyLong_FromLong(frmsize.stepwise.
							     min_width));
			PyDict_SetItemString(cap, "min_height",
					     PyLong_FromLong(frmsize.stepwise.
							     min_height));
			PyDict_SetItemString(cap, "max_width",
					     PyLong_FromLong(frmsize.stepwise.
							     max_width));
			PyDict_SetItemString(cap, "max_height",
					     PyLong_FromLong(frmsize.stepwise.
							     max_height));
			PyDict_SetItemString(cap, "step_width",
					     PyLong_FromLong(frmsize.stepwise.
							     step_width));
			PyDict_SetItemString(cap, "step_height",
					     PyLong_FromLong(frmsize.stepwise.
							     step_height));
			break;
		default:
			PyErr_SetString(PyExc_ValueError,
					"Unknow format type");
			break;

		}
		PyList_Append(ret, cap);
		frmsize.index++;
	}
	PyErr_Clear();

	return ret;
}

static double fract2sec(const struct v4l2_fract *f)
{
	return (double)f->numerator / f->denominator;
}

static double fract2fps(const struct v4l2_fract *f)
{
	return (double)f->denominator / f->numerator;
}

static PyObject *video_device_get_frameintervals(video_device *videodev,
						 PyObject *args)
{
	int size = 0;
	char *fourcc_str = NULL;
	PyObject *ret = Py_None;
	PyObject *cap = Py_None;
	struct v4l2_frmivalenum frmival;

	if (0 != open_check_err(videodev))
		Py_RETURN_NONE;

	CLEAR(frmival);

	if (!PyArg_ParseTuple(args, "s#ii", &fourcc_str, &size, &frmival.width,
			      &frmival.height))
		Py_RETURN_NONE;

	if (size != 4)
		Py_RETURN_NONE;

	frmival.pixel_format = v4l2_fourcc(fourcc_str[0],
					   fourcc_str[1],
					   fourcc_str[2], fourcc_str[3]);
	frmival.index = 0;
	ret = PyList_New(0);

	while (!my_ioctl(videodev->fd, VIDIOC_ENUM_FRAMEINTERVALS, &frmival)) {
		cap = PyDict_New();

		switch (frmival.type) {
		case V4L2_FRMIVAL_TYPE_DISCRETE:
			PYDICT_SETITEM_SEC2STR(cap, "interval",
					       &frmival.discrete);
			PYDICT_SETITEM_FPS2STR(cap, "fps", &frmival.discrete);
			break;
		case V4L2_FRMIVAL_TYPE_CONTINUOUS:
			PYDICT_SETITEM_SEC2STR(cap, "interval_min",
					       &frmival.stepwise.min);
			PYDICT_SETITEM_SEC2STR(cap, "interval_max",
					       &frmival.stepwise.max);
			PYDICT_SETITEM_FPS2STR(cap, "fps_max",
					       &frmival.stepwise.max);
			PYDICT_SETITEM_FPS2STR(cap, "fps_min",
					       &frmival.stepwise.min);
			break;
		case V4L2_FRMIVAL_TYPE_STEPWISE:
			PYDICT_SETITEM_SEC2STR(cap, "interval_min",
					       &frmival.stepwise.min);
			PYDICT_SETITEM_SEC2STR(cap, "interval_max",
					       &frmival.stepwise.max);
			PYDICT_SETITEM_SEC2STR(cap, "interval_step",
					       &frmival.stepwise.step);
			PYDICT_SETITEM_FPS2STR(cap, "fps_max",
					       &frmival.stepwise.max);
			PYDICT_SETITEM_FPS2STR(cap, "fps_min",
					       &frmival.stepwise.min);
			break;
		}
		PyList_Append(ret, cap);
		frmival.index++;
	}

	PyErr_Clear();

	return ret;
}

static PyObject *video_device_start(video_device *videodev)
{
	enum v4l2_buf_type type;

	if (0 != open_check_err(videodev))
		Py_RETURN_NONE;

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (my_ioctl(videodev->fd, VIDIOC_STREAMON, &type))
		Py_RETURN_NONE;

	Py_RETURN_NONE;
}

static PyObject *video_device_stop(video_device *videodev)
{
	enum v4l2_buf_type type;

	if (0 != open_check_err(videodev))
		Py_RETURN_NONE;

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (my_ioctl(videodev->fd, VIDIOC_STREAMOFF, &type))
		Py_RETURN_NONE;

	Py_RETURN_NONE;
}

static PyObject *video_device_create_buffers(video_device *videodev,
					     PyObject *args)
{
	int buffer_count = 0;
	int i = 0;
	struct v4l2_requestbuffers reqbuf;
	struct v4l2_buffer buffer;

	if (!PyArg_ParseTuple(args, "I", &buffer_count))
		Py_RETURN_NONE;

	if (0 != open_check_err(videodev))
		Py_RETURN_NONE;

	if (videodev->buffers) {
		PyErr_SetString(PyExc_ValueError, "Buffers are already created");
		Py_RETURN_NONE;
	}

	reqbuf.count = buffer_count;
	reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	reqbuf.memory = V4L2_MEMORY_MMAP;

	if (my_ioctl(videodev->fd, VIDIOC_REQBUFS, &reqbuf))
		Py_RETURN_NONE;

	if (!reqbuf.count) {
		PyErr_SetString(PyExc_IOError, "Not enough buffer memory");
		Py_RETURN_NONE;
	}

	videodev->buffers = malloc(reqbuf.count * sizeof(struct buffer));

	if (!videodev->buffers) {
		PyErr_NoMemory();
		Py_RETURN_NONE;
	}

	for (i = 0; i < reqbuf.count; i++) {
		buffer.index = i;
		buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buffer.memory = V4L2_MEMORY_MMAP;

		if (my_ioctl(videodev->fd, VIDIOC_QUERYBUF, &buffer)) {
			Py_RETURN_NONE;
		}

		videodev->buffers[i].length = buffer.length;
		videodev->buffers[i].start = v4l2_mmap(NULL, buffer.length,
						       PROT_READ | PROT_WRITE,
						       MAP_SHARED,
						       videodev->fd,
						       buffer.m.offset);

		if (videodev->buffers[i].start == MAP_FAILED) {
			PyErr_SetFromErrno(PyExc_IOError);
			Py_RETURN_NONE;
		}
	}

	videodev->buffer_count = i;

	Py_RETURN_NONE;
}

static PyObject *video_device_queue_all_buffers(video_device *videodev)
{
	int i = 0;
	int buffer_count = videodev->buffer_count;
	struct v4l2_buffer buffer;

	if (0 != open_check_err(videodev))
		Py_RETURN_NONE;

	if (!videodev->buffers) {
		PyErr_SetString(PyExc_ValueError, "Buffers have not been created");
		Py_RETURN_NONE;
	}

	for (i = 0; i < buffer_count; i++) {
		buffer.index = i;
		buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buffer.memory = V4L2_MEMORY_MMAP;

		if (my_ioctl(videodev->fd, VIDIOC_QBUF, &buffer))
			Py_RETURN_NONE;
	}

	Py_RETURN_NONE;
}

#ifdef USE_LIBV4L
static PyObject *video_device_yuyv2rgb(video_device *videodev,
				       struct v4l2_buffer *buffer,
				       int length)
{
	return PYSTRING_FROM_STR_SZ(videodev->buffers[buffer->index].start,
				    buffer->bytesused);
}
#else /* !USE_LIBV4L */
static PyObject *video_device_yuyv2rgb(video_device *videodev,
				       struct v4l2_buffer *buffer,
				       int length)
{
	int u = 0;
	int v = 0;
	int uv = 0;
	int y = 0;
	char *rgb = NULL;
	char *rgb_max = NULL;
	unsigned char *yuyv = NULL;
	PyObject *result = Py_None;

	result = PYSTRING_FROM_STR_SZ(NULL, length);

	// Convert buffer from YUYV to RGB.
	// For the byte order, see: http://v4l2spec.bytesex.org/spec/r4339.htm
	// For the color conversion, see: http://v4l2spec.bytesex.org/spec/x2123.htm

	if (!result)
		Py_RETURN_NONE;

	rgb = PyString_AS_STRING(result);
	rgb_max = rgb + length;
	yuyv = videodev->buffers[buffer->index].start;

	while (rgb < rgb_max) {
		u = yuyv[1] - 128;
		v = yuyv[3] - 128;
		uv = 100 * u + 208 * v;
		u *= 516;
		v *= 409;

		y = 298 * (yuyv[0] - 16);
		rgb[0] = CLAMP(y + v);
		rgb[1] = CLAMP(y - uv);
		rgb[2] = CLAMP(y + u);

		y = 298 * (yuyv[2] - 16);
		rgb[3] = CLAMP(y + v);
		rgb[4] = CLAMP(y - uv);
		rgb[5] = CLAMP(y + u);

		rgb += 6;
		yuyv += 4;
	}
}
#endif /* USE_LIBV4L */

static PyObject *video_device_read_internal(video_device *videodev,
					    int queue)
{
	int length = 0;
	PyObject *result = Py_None;
	struct v4l2_buffer buffer;

	if (!videodev->buffers) {
		PyErr_SetString(PyExc_ValueError, "Buffers have not been created");
		Py_RETURN_NONE;
	}

	buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buffer.memory = V4L2_MEMORY_MMAP;

	if (my_ioctl(videodev->fd, VIDIOC_DQBUF, &buffer)) {
		Py_RETURN_NONE;
	}

	length = buffer.bytesused * 6 / 4;

	result = video_device_yuyv2rgb(videodev, &buffer, length);

	if (!result)
		Py_RETURN_NONE;

	if (queue && my_ioctl(videodev->fd, VIDIOC_QBUF, &buffer))
		Py_RETURN_NONE;

	return result;
}

static PyObject *video_device_read(video_device *videodev)
{
	return video_device_read_internal(videodev, 0);
}

static PyObject *video_device_read_and_queue(video_device *videodev)
{
	return video_device_read_internal(videodev, 1);
}

static PyObject *video_device_set_helper(int id,
					 video_device *videodev,
					 PyObject *args)
{
	int value = 0;
	struct v4l2_control ctrl;

	CLEAR(ctrl);

	if (0 > id)
		Py_RETURN_NONE;

	if (!PyArg_ParseTuple(args, "i", &value))
		Py_RETURN_NONE;

	ctrl.id = id;
	ctrl.value = value;

	if (my_ioctl(videodev->fd, VIDIOC_S_CTRL, &ctrl))
		Py_RETURN_NONE;

	return Py_BuildValue("i", ctrl.value);
}

static PyObject *video_device_get_helper(int id,
					 video_device *videodev)
{
	struct v4l2_control ctrl;

	CLEAR(ctrl);
	ctrl.id = id;

	if (my_ioctl(videodev->fd, VIDIOC_G_CTRL, &ctrl))
		Py_RETURN_NONE;

	return Py_BuildValue("i", ctrl.value);
}

DECLARE_METHODS(auto_wb, V4L2_CID_AUTO_WHITE_BALANCE);
DECLARE_METHODS(wb_temperature, V4L2_CID_WHITE_BALANCE_TEMPERATURE);
DECLARE_METHODS(exposure_absolute, V4L2_CID_EXPOSURE_ABSOLUTE);
DECLARE_METHODS(exposure_auto, V4L2_CID_EXPOSURE_AUTO);
DECLARE_METHODS(focus_auto, V4L2_CID_FOCUS_AUTO);

static PyMethodDef video_device_methods[] = {
	{"open", (PyCFunction) video_device_open, METH_NOARGS,
	 "open()\n\n"
	 "Open the video device."},
	{"close", (PyCFunction) video_device_close, METH_NOARGS,
	 "close()\n\n"
	 "Close the video device."},
	{"get_info", (PyCFunction) video_device_get_info, METH_NOARGS,
	 "get_info() -> driver, card, bus_info, capabilities\n\n"
	 "Returns three strings with information about the video device, and one "
	 "set containing strings identifying the capabilities of the video "
	 "device."},
	{"get_fourcc", (PyCFunction) video_device_get_fourcc, METH_VARARGS,
	 "get_fourcc(fourcc_string) -> fourcc_int\n\n"
	 "Return the fourcc string encoded as int."},
	{"get_format", (PyCFunction) video_device_get_format, METH_NOARGS,
	 "get_format() -> size_x, size_y, fourcc\n\n"
	 "Request the current video format."},
	{"set_format", (PyCFunction) video_device_set_format,
	 METH_VARARGS | METH_KEYWORDS,
	 "set_format(size_x, size_y, yuv420 = 0, fourcc='MJPEG') -> size_x, size_y\n\n"
	 "Request the video device to set image size and format. The device may "
	 "choose another size than requested and will return its choice. The "
	 "image format will be RGB24 if yuv420 is zero (default) or YUV420 if "
	 "yuv420 is 1, if fourcc keyword is set that will be the fourcc pixel format used."},
	{"set_fps", (PyCFunction) video_device_set_fps, METH_VARARGS,
	 "set_fps(fps) -> fps \n\n"
	 "Request the video device to set frame per seconds.The device may "
	 "choose another frame rate than requested and will return its choice. "},
	{"set_auto_white_balance",
	 (PyCFunction) video_device_set_auto_wb, METH_VARARGS,
	 "set_auto_white_balance(autowb) -> autowb \n\n"
	 "Request the video device to set auto white balance to value. The device may "
	 "choose another value than requested and will return its choice. "},
	{"get_auto_white_balance",
	 (PyCFunction) video_device_get_auto_wb, METH_NOARGS,
	 "get_auto_white_balance() -> autowb \n\n"
	 "Request the video device to get auto white balance value. "},
	{"set_white_balance_temperature",
	 (PyCFunction) video_device_set_wb_temperature, METH_VARARGS,
	 "set_white_balance_temperature(temp) -> temp \n\n"
	 "Request the video device to set white balance tempature to value. The device may "
	 "choose another value than requested and will return its choice. "},
	{"get_white_balance_temperature",
	 (PyCFunction) video_device_get_wb_temperature, METH_NOARGS,
	 "get_white_balance_temperature() -> temp \n\n"
	 "Request the video device to get white balance temperature value. "},
	{"set_exposure_auto", (PyCFunction) video_device_set_exposure_auto,
	 METH_VARARGS,
	 "set_exposure_auto(autoexp) -> autoexp \n\n"
	 "Request the video device to set auto exposure to value. The device may "
	 "choose another value than requested and will return its choice. "},
	{"get_exposure_auto", (PyCFunction) video_device_get_exposure_auto,
	 METH_NOARGS,
	 "get_exposure_auto() -> autoexp \n\n"
	 "Request the video device to get auto exposure value. "},
	{"set_exposure_absolute", (PyCFunction) video_device_set_exposure_absolute,
	 METH_VARARGS,
	 "set_exposure_absolute(exptime) -> exptime \n\n"
	 "Request the video device to set exposure time to value. The device may "
	 "choose another value than requested and will return its choice. "},
	{"get_exposure_absolute", (PyCFunction) video_device_get_exposure_absolute,
	 METH_NOARGS,
	 "get_exposure_absolute() -> exptime \n\n"
	 "Request the video device to get exposure time value. "},
	{"set_focus_auto", (PyCFunction) video_device_set_focus_auto, METH_VARARGS,
	 "set_auto_focus_auto(autofocus) -> autofocus \n\n"
	 "Request the video device to set auto focuse on or off. The device may "
	 "choose another value than requested and will return its choice. "},
	{"get_focus_auto", (PyCFunction) video_device_get_focus_auto, METH_NOARGS,
	 "get_focus_auto() -> autofocus \n\n"
	 "Request the video device to get auto focus value. "},
	{"get_framesizes", (PyCFunction) video_device_get_framesizes, METH_VARARGS,
	 "get_framesizes() -> framesizes \n\n"
	 "Request the framesizes suported by the device. "},
	{"get_frameintervals", (PyCFunction) video_device_get_frameintervals,
	 METH_VARARGS,
	 "get_frameintervals() -> frameintervals \n\n"
	 "Request the frameintervals suported by the device. "},
	{"start", (PyCFunction) video_device_start, METH_NOARGS,
	 "start()\n\n" "Start video capture."},
	{"stop", (PyCFunction) video_device_stop, METH_NOARGS,
	 "stop()\n\n" "Stop video capture."},
	{"create_buffers", (PyCFunction) video_device_create_buffers, METH_VARARGS,
	 "create_buffers(count)\n\n"
	 "Create buffers used for capturing image data. Can only be called once "
	 "for each video device object."},
	{"queue_all_buffers", (PyCFunction) video_device_queue_all_buffers,
	 METH_NOARGS,
	 "queue_all_buffers()\n\n"
	 "Let the video device fill all buffers created."},
	{"read", (PyCFunction) video_device_read, METH_NOARGS,
	 "read() -> string\n\n"
	 "Reads image data from a buffer that has been filled by the video "
	 "device. The image data is in RGB och YUV420 format as decided by "
	 "'set_format'. The buffer is removed from the queue. Fails if no buffer "
	 "is filled. Use select.select to check for filled buffers."},
	{"read_and_queue", (PyCFunction) video_device_read_and_queue, METH_NOARGS,
	 "read_and_queue()\n\n"
	 "Same as 'read', but adds the buffer back to the queue so the video "
	 "device can fill it again."},
	{NULL}
};

static PyTypeObject video_device_type = {
	PYOBJECT_HEAD_INIT(NULL, 0)
	.tp_name = "V4L2VideoDevice",
	.tp_basicsize = sizeof(video_device),
	.tp_dealloc = (destructor)video_device_dealloc,
	.tp_flags = Py_TPFLAGS_DEFAULT,
	.tp_doc = "video_device(path)\n\nOpens the video device at "
	"the given path and returns an object that can capture images. The "
	"constructor and all methods except close may raise IOError.",
	.tp_methods = video_device_methods,
	.tp_init = (initproc)video_device_init
};

static PyMethodDef module_methods[] = {
	{NULL, NULL, 0, NULL}
};


PyMODINIT_FUNC INIT_V4L2_CAPTURE(void)
{
	PyObject *module;

	video_device_type.tp_new = PyType_GenericNew;

	if (PyType_Ready(&video_device_type) < 0)
		return PYMODINIT_FUNC_RETURN(NULL);

	module = Py_InitModule3("pyv4l2", module_methods,
				"Video with video4linux2.");

	if (!module)
		return PYMODINIT_FUNC_RETURN(NULL);

	Py_INCREF(&video_device_type);
	PyModule_AddObject(module, "V4L2VideoDevice",
			   (PyObject *)&video_device_type);

	return PYMODINIT_FUNC_RETURN(module);
}
