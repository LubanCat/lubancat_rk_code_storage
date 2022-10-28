#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/of_platform.h>
#include <linux/leds.h>
#include <linux/err.h>
#include <linux/pwm.h>
#include <linux/leds_pwm.h>
#include <linux/slab.h>

struct pwm_device	*pwm_demo;  //定义pwm设备结构体

/*简单驱动*/
static int pwm_demo_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct device *dev = &pdev->dev;
	struct device_node *child; // 保存子节点
	printk("match success \n");

	child = of_get_next_child(dev->of_node, NULL);
	if (child)
	{
		pwm_demo = devm_of_pwm_get(dev, child, NULL);
		if (IS_ERR(pwm_demo)) 
		{
			printk(KERN_ERR" pwm_demo，get pwm error!!\n");
			return -1;
		}
	}
	else
	{
		printk(KERN_ERR" pwm_demoof_get_next_child error!!\n");
		return -1;
	}

	/*配置频率100KHz 占空比80%*/
	pwm_config(pwm_demo, 1000, 5000);
	/*反相 频率100KHz 占空比20%*/
	pwm_set_polarity(pwm_demo, PWM_POLARITY_INVERSED);
	pwm_enable(pwm_demo);

	return ret;
}

static int pwm_demo_remove(struct platform_device *pdev)
{
	pwm_config(pwm_demo, 0, 5000);
    pwm_free(pwm_demo);
	return 0;
}

static const struct of_device_id of_pwm_leds_match[] = {
	{.compatible = "pwm_demo",},
	{},
};

static struct platform_driver pwm_demo_driver = {
	.probe		= pwm_demo_probe,
	.remove		= pwm_demo_remove,
	.driver		= {
		.name	= "pwm_demo",
		.of_match_table = of_pwm_leds_match,
	},
};

module_platform_driver(pwm_demo_driver);

MODULE_AUTHOR("llh <zgwinli555@163.com>");
MODULE_DESCRIPTION(" Embedfire PWM DEMO driver");
MODULE_LICENSE("GPL v2");


