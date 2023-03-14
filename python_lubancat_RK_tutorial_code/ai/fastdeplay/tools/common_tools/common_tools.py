import argparse
import ast
import uvicorn


def argsparser():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        'tools', choices=['compress', 'convert', 'simple_serving'])
    ## argumentments for auto compression
    parser.add_argument(
        '--config_path',
        type=str,
        default=None,
        help="path of compression strategy config.")
    parser.add_argument(
        '--method',
        type=str,
        default=None,
        help="choose PTQ or QAT as quantization method")
    parser.add_argument(
        '--save_dir',
        type=str,
        default='./output',
        help="directory to save model.")
    parser.add_argument(
        '--devices',
        type=str,
        default='gpu',
        help="which device used to compress.")
    ## arguments for other x2paddle
    parser.add_argument(
        '--framework',
        type=str,
        default=None,
        help="define which deeplearning framework(tensorflow/caffe/onnx)")
    parser.add_argument(
        '--model',
        type=str,
        default=None,
        help="define model file path for tensorflow or onnx")
    parser.add_argument(
        "--prototxt",
        "-p",
        type=str,
        default=None,
        help="prototxt file of caffe model")
    parser.add_argument(
        "--weight",
        "-w",
        type=str,
        default=None,
        help="weight file of caffe model")
    parser.add_argument(
        "--caffe_proto",
        "-c",
        type=str,
        default=None,
        help="optional: the .py file compiled by caffe proto file of caffe model"
    )
    parser.add_argument(
       "--input_shape_dict",
       "-isd",
       type=str,
       default=None,
       help="define input shapes, e.g --input_shape_dict=\"{'image':[1, 3, 608, 608]}\" or" \
       "--input_shape_dict=\"{'image':[1, 3, 608, 608], 'im_shape': [1, 2], 'scale_factor': [1, 2]}\"")
    parser.add_argument(
        "--enable_code_optim",
        "-co",
        type=ast.literal_eval,
        default=False,
        help="Turn on code optimization")
    ## arguments for simple serving
    parser.add_argument(
        "--app",
        type=str,
        default="server:app",
        help="Simple serving app string")
    parser.add_argument(
        "--host",
        type=str,
        default="127.0.0.1",
        help="Simple serving host IP address")
    parser.add_argument(
        "--port", type=int, default=8000, help="Simple serving host port")
    ## arguments for other tools
    return parser


def main():
    args = argsparser().parse_args()
    if args.tools == "compress":
        from .auto_compression.fd_auto_compress.fd_auto_compress import auto_compress
        print("Welcome to use FastDeploy Auto Compression Toolkit!")
        auto_compress(args)
    if args.tools == "convert":
        try:
            import platform
            import logging
            v0, v1, v2 = platform.python_version().split('.')
            if not (int(v0) >= 3 and int(v1) >= 5):
                logging.info("[ERROR] python>=3.5 is required")
                return
            import paddle
            v0, v1, v2 = paddle.__version__.split('.')
            logging.info("paddle.__version__ = {}".format(paddle.__version__))
            if v0 == '0' and v1 == '0' and v2 == '0':
                logging.info(
                    "[WARNING] You are use develop version of paddlepaddle")
            elif int(v0) != 2 or int(v1) < 0:
                logging.info("[ERROR] paddlepaddle>=2.0.0 is required")
                return
            from x2paddle.convert import tf2paddle, caffe2paddle, onnx2paddle
            if args.framework == "tensorflow":
                assert args.model is not None, "--model should be defined while convert tensorflow model"
                tf2paddle(args.model, args.save_dir)
            elif args.framework == "caffe":
                assert args.prototxt is not None and args.weight is not None, "--prototxt and --weight should be defined while convert caffe model"
                caffe2paddle(args.prototxt, args.weight, args.save_dir,
                             args.caffe_proto)
            elif args.framework == "onnx":
                assert args.model is not None, "--model should be defined while convert onnx model"
                onnx2paddle(
                    args.model,
                    args.save_dir,
                    input_shape_dict=args.input_shape_dict)
            else:
                raise Exception(
                    "--framework only support tensorflow/caffe/onnx now")
        except ImportError:
            print(
                "Model convert failed! Please check if you have installed it!")
    if args.tools == "simple_serving":
        custom_logging_config = {
            "version": 1,
            "disable_existing_loggers": False,
            "formatters": {
                "default": {
                    "()": "uvicorn.logging.DefaultFormatter",
                    "fmt": "%(asctime)s %(levelprefix)s %(message)s",
                    'datefmt': '%Y-%m-%d %H:%M:%S',
                    "use_colors": None,
                },
            },
            "handlers": {
                "default": {
                    "formatter": "default",
                    "class": "logging.StreamHandler",
                    "stream": "ext://sys.stderr",
                },
                'null': {
                    "formatter": "default",
                    "class": 'logging.NullHandler'
                }
            },
            "loggers": {
                "": {
                    "handlers": ["null"],
                    "level": "DEBUG"
                },
                "uvicorn.error": {
                    "handlers": ["default"],
                    "level": "DEBUG"
                }
            },
        }
        uvicorn.run(args.app,
                    host=args.host,
                    port=args.port,
                    app_dir='.',
                    log_config=custom_logging_config)


if __name__ == '__main__':
    main()
