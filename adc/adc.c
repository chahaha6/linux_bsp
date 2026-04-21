#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/iio/iio.h>
#include <linux/iio/consumer.h>

static const struct of_device_id sun_match_table[] = {
    { .compatible = "sun_sensor" }, 
    { } // 结束标记
};
MODULE_DEVICE_TABLE(of, sun_match_table);

static const struct platform_device_id sun_idtable[] = {
    { "sun_sensor", 0 }, 
    { } // 结束标记
};
MODULE_DEVICE_TABLE(platform, sun_idtable);

static int sun_probe(struct platform_device *pdev)
{
    struct iio_channel *adc_chan;
    int val, ret;
    printk("probe is donging\n");
    adc_chan = devm_iio_channel_get(&pdev->dev, "adc_in");
    if (IS_ERR(adc_chan)) {
        dev_err(&pdev->dev, "Failed to get ADC channel\n");
        return PTR_ERR(adc_chan);
    }

    ret = iio_read_channel_raw(adc_chan, &val);
    if (ret >= 0)
        dev_info(&pdev->dev, "ADC Value: %d\n", val);

    return 0;
}

static int sun_remove(struct platform_device *pdev)
{
    return 0;
}
static struct platform_driver sun_adc = {
    .probe      = sun_probe,
    .remove     = sun_remove,
    .driver     = {
        .name           = "sun_sensor",
        .of_match_table = sun_match_table, // 指向修正后的数组
    },
    .id_table   = sun_idtable,
};

module_platform_driver(sun_adc);
MODULE_LICENSE("GPL");