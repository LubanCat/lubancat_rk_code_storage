#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/of.h>               /*设备树*/
#include <linux/platform_device.h>  /*平台设备*/
#include <linux/gpio/consumer.h>    /*GPIO 接口描述符*/
#include <linux/input.h>
#include <linux/interrupt.h>


struct button_data {
	struct gpio_desc *button_input_gpiod; //GPIO
	struct input_dev *button_input_dev;   //按键输入子系统结构体
	struct platform_device *pdev;         //平台设备结构体
	int irq;                              //中断irq
};


/*按键中断处理函数*/
static irqreturn_t button_input_irq_hander(int irq, void *dev_id)
{
	struct button_data *priv = dev_id;
	int button_satus;

	/*读取按键引脚的电平，根据读取得到的结果输入按键状态*/
	button_satus = (gpiod_get_value(priv->button_input_gpiod) & 1);
	if(button_satus)
	{
		input_report_key(priv->button_input_dev, BTN_0, 1);
		input_sync(priv->button_input_dev);
	}
	else
	{
		input_report_key(priv->button_input_dev, BTN_0, 0);
		input_sync(priv->button_input_dev);
	}
	
	return IRQ_HANDLED;
}

static int btn_open(struct input_dev *i_dev)
{
	pr_info("input device opened()\n");
	return 0;
}

static void btn_close(struct input_dev *i_dev)
{
	pr_info("input device closed()\n");
}


static int button_probe(struct platform_device *pdev)
{
	struct button_data *priv;
	struct gpio_desc *gpiod;
	struct input_dev *i_dev;
	int ret;

	pr_info("button_probe\n");
	priv = devm_kzalloc(&pdev->dev,sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	i_dev = input_allocate_device();
	if (!i_dev)
		return -ENOMEM;

	i_dev->open = btn_open;
	i_dev->close = btn_close;
	i_dev->name = "key input";
	i_dev->dev.parent = &pdev->dev;
	priv->button_input_dev = i_dev;
	priv->pdev = pdev;

	set_bit(EV_KEY, i_dev->evbit);  /*设置要使用的输入事件类型*/
	set_bit(BTN_0, i_dev->keybit); /* 按钮 0*/

	gpiod = gpiod_get(&pdev->dev, "button", GPIOD_IN); /*获取gpio,并设置为输入*/
	if (IS_ERR(gpiod))
		return -ENODEV;

	priv->irq = gpiod_to_irq(gpiod);
	priv->button_input_gpiod = gpiod;

	ret = input_register_device(priv->button_input_dev);
	if (ret) {
		pr_err("Failed to register inputdevice\n");
		goto err_input;
	}

	platform_set_drvdata(pdev, priv);

	ret = request_any_context_irq(priv->irq, button_input_irq_hander,IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING, "input-button", priv);
	if (ret < 0) {
		dev_err(&pdev->dev,"请求gpio中断失败\n");
		goto err_btn;
	}

	return 0;

err_btn:
	gpiod_put(priv->button_input_gpiod);
err_input:
	input_free_device(priv->button_input_dev);
	return ret;
}

static int button_remove(struct platform_device *pdev)
{
	struct button_data *priv;
	priv = platform_get_drvdata(pdev);
	input_unregister_device(priv->button_input_dev);
	input_free_device(priv->button_input_dev);
	free_irq(priv->irq, priv);
	gpiod_put(priv->button_input_gpiod);
	return 0;

}

static const struct of_device_id button_dt_ids[] =
{
	{ .compatible = "input_button", },
	{ /*标记结束*/ }
};

static struct platform_driver button_input = {
	.probe = button_probe,
	.remove = button_remove,
	.driver = {
		.name = "button-input",
		.of_match_table = of_match_ptr(button_dt_ids),
		.owner = THIS_MODULE,
	},
};

module_platform_driver(button_input);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("llh<zgwinli555@163.com>");
MODULE_DESCRIPTION("Embedfire，Input key device");
