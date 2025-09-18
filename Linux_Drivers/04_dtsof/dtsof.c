#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/of.h>
#include <linux/of_address.h>


#if 0
    aliases {
		serial0 = &uart4;
	};

    reserved-memory {
        gpu_reserved: gpu@f6000000 {
            reg = <0xf6000000 0x8000000>;
            no-map;
        };

        optee_memory: optee@fe000000 {
            reg = <0xfe000000 0x02000000>;
            no-map;
        };
    };

    cpus {
		#address-cells = <1>;
		#size-cells = <0>;

		cpu0: cpu@0 {
			compatible = "arm,cortex-a7";
			device_type = "cpu";
			reg = <0>;
			clocks = <&scmi0_clk CK_SCMI0_MPU>;
			clock-names = "cpu";
			operating-points-v2 = <&cpu0_opp_table>;
			nvmem-cells = <&part_number_otp>;
			nvmem-cell-names = "part_number";
			#cooling-cells = <2>;
		};
	};
#endif

/*
 *   模块入口
 */
static int __init dtsof_init(void)
{
    int ret = 0;
    struct device_node *ali_nd = NULL; //节点
    struct property *alipro = NULL;
    const char *str;

    struct device_node *cpus_nd = NULL; //节点
    u32 addr_value = 0;

    struct device_node *res_nd = NULL, *ret_nd = NULL; //节点
    u32 elemsize = 0;
    u32 *regval;
    u8 i;
    

    
    /* 1、找到aliases节点，路径是：/aliases */
    ali_nd = of_find_node_by_path("/aliases");
    if(ali_nd == NULL){ // 失败
        ret = -EINVAL;
        goto fail_findnd;
    }

    /* 1、找到cpu节点，路径是：/cpu */
    cpus_nd = of_find_node_by_path("/cpus");
    if(cpus_nd == NULL){ // 失败
        ret = -EINVAL;
        goto fail_findnd;
    }

    /* 1、找到reserved-memory节点，路径是：/reserved-memory */
    res_nd = of_find_node_by_path("/reserved-memory");
    if(res_nd == NULL){ // 失败
        printk("get reserved-memory failed");
        ret = -EINVAL;
        goto fail_findnd;
    }

    /* 2、调用of_find_property获取属性 */
    alipro = of_find_property(ali_nd, "serial0", NULL);
    if(alipro == NULL){ // 失败
        ret = -EINVAL;
        goto fail_findpro;
    }else{  // 成功
        printk("aliases=%s\r\n", (char*)alipro->value);
    }

    /* 2、调用of_property_read_string获取属性 */
    ret = of_property_read_string(ali_nd, "serial0", &str);   
    if(ret < 0){
        goto fail_rs;
    }else{
        printk("of_property_read_string aliases=%s\r\n", str);
    }

    /* 3、获取数字属性值 */
    ret = of_property_read_u32(cpus_nd, "#address-cells", &addr_value);
    if(ret < 0){
        goto fail_read32;
    }else{
        printk("of_property_read_u32 address-cell=%d\r\n", addr_value);
    }

    /* 4、获取数组类型的大小 */
    

    /* 查找子节点 */
    ret_nd = of_get_next_child(res_nd, NULL);
    if(ret_nd == NULL){
        ret = -EINVAL;
        printk("get child gpu_reserved failed");
        goto fail_child;
    }else{
        printk("get child success");
        /* 获取子节点下的数组类型大小 */
        elemsize = of_property_count_elems_of_size(ret_nd, "reg", sizeof(u32));
        if(elemsize < 0){
            ret = -EINVAL;
            printk("get reg failed");
            goto fail_readele;
        }else{
            printk("reserved-memory/gpu_reserved/reg = %d\r\n", elemsize);
        }
    }

    /* 申请内存 */
    regval = kmalloc(elemsize * sizeof(u32), GFP_KERNEL);
    if(!regval){
        ret = -EINVAL;
        goto fail_mem;
    }
    /* 获取数组 */
    ret = of_property_read_u32_array(ret_nd, "reg", regval, elemsize);
    if(ret < 0){
        goto fail_read32array;
    }else{
        for(i=0; i < elemsize; i++){
            printk("reg[%d] = %x\r\n", i, regval[i]);
        }
    }
    kfree(regval);

    return 0;

fail_read32array:
    kfree(regval);  // 释放内存
fail_mem:
fail_readele:
fail_child:
fail_read32:
fail_rs:
fail_findpro:
fail_findnd:
    return ret;
}

static void __exit dtsof_exit(void)
{

}

/*
 *   注册模块入口与出口
 */

module_init(dtsof_init);   /* 入口 */
module_exit(dtsof_exit);   /* 出口 */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("ALIENTEK");
MODULE_INFO(intree, "Y");