import platform
import random
import cv2
from copy import copy
import numpy as np
from rknnlite.api import RKNNLite

OBJ_THRESH = 0.25
NMS_THRESH = 0.45

IMG_SIZE = (640, 640)  # (width, height), such as (1280, 736)

# decice tree for RK356x/RK3576/RK3588
DEVICE_COMPATIBLE_NODE = '/proc/device-tree/compatible'

# 
RK3566_RK3568_RKNN_MODEL_PATH = './yolov10_rk356x.rknn'
RK3588_RKNN_MODEL_PATH = './yolov10_rk3588.rknn'
RK3562_RKNN_MODEL_PATH = './yolov10_rk3562.rknn'
RK3576_RKNN_MODEL_PATH = './yolov10_rk3576.rknn'

IMG_PATH = "./bus.jpg"

CLASSES = ("person", "bicycle", "car","motorbike ","aeroplane ","bus ","train","truck ","boat","traffic light",
           "fire hydrant","stop sign ","parking meter","bench","bird","cat","dog ","horse ","sheep","cow","elephant",
           "bear","zebra ","giraffe","backpack","umbrella","handbag","tie","suitcase","frisbee","skis","snowboard","sports ball","kite",
           "baseball bat","baseball glove","skateboard","surfboard","tennis racket","bottle","wine glass","cup","fork","knife ",
           "spoon","bowl","banana","apple","sandwich","orange","broccoli","carrot","hot dog","pizza ","donut","cake","chair","sofa",
           "pottedplant","bed","diningtable","toilet ","tvmonitor","laptop	","mouse	","remote ","keyboard ","cell phone","microwave ",
           "oven ","toaster","sink","refrigerator ","book","clock","vase","scissors ","teddy bear ","hair drier", "toothbrush ")

CLASS_COLORS = [[random.randint(0, 255) for _ in range(3)] for _ in range(len(CLASSES))]

def get_host():
    # get platform and device type
    system = platform.system()
    machine = platform.machine()
    os_machine = system + '-' + machine
    if os_machine == 'Linux-aarch64':
        try:
            with open(DEVICE_COMPATIBLE_NODE) as f:
                device_compatible_str = f.read()
                if 'rk3562' in device_compatible_str:
                    host = 'RK3562'
                elif 'rk3576' in device_compatible_str:
                    host = 'RK3576'
                elif 'rk3588' in device_compatible_str:
                    host = 'RK3588'
                else:
                    host = 'RK3566_RK3568'
        except IOError:
            print('Read device node {} failed.'.format(DEVICE_COMPATIBLE_NODE))
            exit(-1)
    else:
        host = os_machine
    return host

def filter_boxes(boxes, box_confidences, box_class_probs):
    """Filter boxes with object threshold.
    """
    box_confidences = box_confidences.reshape(-1)
    candidate, class_num = box_class_probs.shape

    class_max_score = np.max(box_class_probs, axis=-1)
    classes = np.argmax(box_class_probs, axis=-1)

    _class_pos = np.where(class_max_score* box_confidences >= OBJ_THRESH)
    scores = (class_max_score* box_confidences)[_class_pos]

    boxes = boxes[_class_pos]
    classes = classes[_class_pos]
    return boxes, classes, scores

def nms_boxes(boxes, scores):
    """Suppress non-maximal boxes.
    # Returns
        keep: ndarray, index of effective boxes.
    """
    x = boxes[:, 0]
    y = boxes[:, 1]
    w = boxes[:, 2] - boxes[:, 0]
    h = boxes[:, 3] - boxes[:, 1]

    areas = w * h
    order = scores.argsort()[::-1]

    keep = []
    while order.size > 0:
        i = order[0]
        keep.append(i)

        xx1 = np.maximum(x[i], x[order[1:]])
        yy1 = np.maximum(y[i], y[order[1:]])
        xx2 = np.minimum(x[i] + w[i], x[order[1:]] + w[order[1:]])
        yy2 = np.minimum(y[i] + h[i], y[order[1:]] + h[order[1:]])

        w1 = np.maximum(0.0, xx2 - xx1 + 0.00001)
        h1 = np.maximum(0.0, yy2 - yy1 + 0.00001)
        inter = w1 * h1

        ovr = inter / (areas[i] + areas[order[1:]] - inter)
        inds = np.where(ovr <= NMS_THRESH)[0]
        order = order[inds + 1]
    keep = np.array(keep)
    return keep

def dfl(position):
    # Distribution Focal Loss (DFL)
    n,c,h,w = position.shape
    p_num = 4
    mc = c//p_num
    y = position.reshape(n,p_num,mc,h,w)
    e_y = np.exp(y - np.max(y, axis=2, keepdims=True))
    y = e_y / np.sum(e_y, axis=2, keepdims=True)
    acc_metrix = np.arange(mc).reshape(1,1,mc,1,1)
    y = (y*acc_metrix).sum(2)
    return y

def box_process(position):
    grid_h, grid_w = position.shape[2:4]
    col, row = np.meshgrid(np.arange(0, grid_w), np.arange(0, grid_h))  # 
    col = col.reshape(1, 1, grid_h, grid_w)
    row = row.reshape(1, 1, grid_h, grid_w)
    grid = np.concatenate((col, row), axis=1)
    stride = np.array([IMG_SIZE[1]//grid_h, IMG_SIZE[0]//grid_w]).reshape(1,2,1,1) # 

    position = dfl(position)
    box_xy  = grid +0.5 -position[:,0:2,:,:]
    box_xy2 = grid +0.5 +position[:,2:4,:,:]
    xyxy = np.concatenate((box_xy*stride, box_xy2*stride), axis=1)

    return xyxy

def post_process(input_data):
    # input_data[0], input_data[3], and input_data[6] detection box information
    # input_data[1], input_data[4], and input_data[7] category score information
    # input_data[2], input_data[5], and input_data[8] category score sum 
    boxes, scores, classes_conf = [], [], []
    defualt_branch=3
    pair_per_branch = len(input_data)//defualt_branch # 3

    # Python 忽略模型class score_sum 输出
    for i in range(defualt_branch):
        boxes.append(box_process(input_data[pair_per_branch*i]))
        classes_conf.append(input_data[pair_per_branch*i+1])
        scores.append(np.ones_like(input_data[pair_per_branch*i+1][:,:1,:,:], dtype=np.float32))

    def sp_flatten(_in):
        ch = _in.shape[1]
        _in = _in.transpose(0,2,3,1)
        return _in.reshape(-1, ch)

    boxes = [sp_flatten(_v) for _v in boxes]
    classes_conf = [sp_flatten(_v) for _v in classes_conf]
    scores = [sp_flatten(_v) for _v in scores]

    boxes = np.concatenate(boxes)
    classes_conf = np.concatenate(classes_conf)
    scores = np.concatenate(scores)

    # filter according to threshold
    boxes, classes, scores = filter_boxes(boxes, scores, classes_conf)
    return boxes, classes, scores


def draw(image, boxes, scores, classes):
    for box, score, cl in zip(boxes, scores, classes):
        top, left, right, bottom = [int(_b) for _b in box]
        print("%s @ (%d %d %d %d) %.3f" % (CLASSES[cl], top, left, right, bottom, score))
        cv2.rectangle(image, (top, left), (right, bottom), CLASS_COLORS[cl], 2)
        cv2.putText(image, '{0} {1:.2f}'.format(CLASSES[cl], score),
                    (top, left - 6), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 0, 255), 2)

def letter_box(im, new_shape, color=(0, 0, 0)):
    # Resize and pad image while meeting stride-multiple constraints
    shape = im.shape[:2]  # current shape [height, width]
    if isinstance(new_shape, int):
        new_shape = (new_shape, new_shape)

    # Scale ratio (new / old)
    r = min(new_shape[0] / shape[0], new_shape[1] / shape[1])

    # Compute padding
    ratio = r  # width, height ratios
    new_unpad = int(round(shape[1] * r)), int(round(shape[0] * r))
    dw, dh = new_shape[1] - new_unpad[0], new_shape[0] - new_unpad[1]  # wh padding

    dw /= 2  # divide padding into 2 sides
    dh /= 2

    if shape[::-1] != new_unpad:  # resize
        im = cv2.resize(im, new_unpad, interpolation=cv2.INTER_LINEAR)
    top, bottom = int(round(dh - 0.1)), int(round(dh + 0.1))
    left, right = int(round(dw - 0.1)), int(round(dw + 0.1))
    im = cv2.copyMakeBorder(im, top, bottom, left, right, cv2.BORDER_CONSTANT, value=color)  # add border
    return im, ratio, (dw, dh)

def get_real_box(src_shape, box, dw, dh, ratio):
    bbox = copy(box)
    # unletter_box result
    bbox[:,0] -= dw
    bbox[:,0] /= ratio
    bbox[:,0] = np.clip(bbox[:,0], 0, src_shape[1])

    bbox[:,1] -= dh
    bbox[:,1] /= ratio
    bbox[:,1] = np.clip(bbox[:,1], 0, src_shape[0])

    bbox[:,2] -= dw
    bbox[:,2] /= ratio
    bbox[:,2] = np.clip(bbox[:,2], 0, src_shape[1])

    bbox[:,3] -= dh
    bbox[:,3] /= ratio
    bbox[:,3] = np.clip(bbox[:,3], 0, src_shape[0])
    return bbox

if __name__ == '__main__':

    # Get device information
    host_name = get_host()
    if host_name == 'RK3566_RK3568':
        rknn_model = RK3566_RK3568_RKNN_MODEL_PATH
    elif host_name == 'RK3562':
        rknn_model = RK3562_RKNN_MODEL_PATH
    elif host_name == 'RK3576':
        rknn_model = RK3576_RKNN_MODEL_PATH
    elif host_name == 'RK3588':
        rknn_model = RK3588_RKNN_MODEL_PATH
    else:
        print("This demo cannot run on the current platform: {}".format(host_name))
        exit(-1)

    rknn_lite = RKNNLite()

    # 加载rknn模型
    ret = rknn_lite.load_rknn(rknn_model)
    if ret != 0:
        print('Load RKNN model failed')
        exit(ret)

    # 设置运行环境，目标默认是rk3588,选择npu0单核
    if host_name in ['RK3576', 'RK3588']:
        ret = rknn_lite.init_runtime(core_mask=RKNNLite.NPU_CORE_0)
    else:
        ret = rknn_lite.init_runtime()
    if ret != 0:
        print('Init runtime environment failed')
        exit(ret)
    print('done')

    # 输入图像
    img_src = cv2.imread(IMG_PATH)
    src_shape = img_src.shape[:2]
    img, ratio, (dw, dh) = letter_box(img_src, IMG_SIZE)
    img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
    img = np.expand_dims(img, 0)

    # 推理运行
    print('--> Running model')
    outputs = rknn_lite.inference(inputs=[img])

    # 后处理
    boxes, classes, scores = post_process(outputs)

    img_p = img_src.copy()
    if boxes is not None:
        draw(img_p, get_real_box(src_shape, boxes, dw, dh, ratio), scores, classes)
        cv2.imwrite("result.jpg", img_p)
