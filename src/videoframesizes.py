class VideoFramesize(object):
    def __init__(self, fmt):
        for key in fmt:
            setattr(self, key, fmt[key])

class VideoDescreteFramesize(VideoFramesize):
    size_x = 0
    size_y = 0

    def __init__(self, fmt):
        super(VideoDescreteFramesize, self).__init__(fmt)

    def __str__(self):
        return "%ix%i" % (self.size_x, self.size_y)

class VideoStepwiseFramesize(VideoFramesize):
    step_width = 0
    step_height = 0
    min_width = 0
    max_width = 0
    min_height = 0
    max_height = 0

    def __init__(self, fmt):
        super(VideoDescreteFramesize, self).__init__(fmt)

    def __str__(self):
        return "%ix%i - %ix%i [%ix%i]" % (self.min_width, self.min_height,
                                          self.max_width, self.max_height,
                                          self.step_width, self.step_height)


class VideoContinousFramesize(VideoStepwiseFramesize):
    def __init__(self, fmt):
        fmt["step_width"] = 1
        fmt["step_height"] = 1
        super(VideoContinousFramesize, self).__init__(fmt)

    def __str__(self):
        return "%ix%i - %ix%i" % (self.min_width, self.min_height,
                                  self.max_width, self.max_height,
                                  self.step_width, self.step_height)

class VideoFramesizes(object):
    fourcc = 0
    frames = None

    def __init__(self, fourcc, frame_list):
        self.fourcc = fourcc
        self.frames = []
        if "size_x" in frame_list[0]:
            for size in frame_list:
                self.frames.append(VideoDescreteFramesize(size))
        elif "step_width" in frame_list[0]:
            self.frames.append(VideoStepwiseFramesize(frame_list[0]))
        else:
            self.frames.append(VideoContinousFramesize(frame_list[0]))

    def __str__(self):
        return "%s: [%s]" % (self.fourcc,
                             ", ".join([str(frame) for frame in self.frames]))

