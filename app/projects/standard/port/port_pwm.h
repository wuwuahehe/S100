#ifndef __PORT_PWM_H__
#define __PORT_PWM_H__

void energy_music_vol_level(void);
void ledseg_7p7s_update_energy_buf(u8 buf);
void ledseg_7p7s_update_energy_buf_do(u8 buf);

void pwm_init(void);
void pwm_rgb_write(u8 red, u8 green, u8 blue);
void pwm_rgb_close(void);
void pwm_rgb_write_new(void);//5ms-PWM调用呼吸灯。
void pwm_rgb_run(void);

//根据需求放在对应的中断(5ms)
//#if PWM_RGB_EN
//    pwm_rgb_write_new();
//#endif

//注意硬件电路设计，如果硬件处理的不好，会有明显的噪声
//13x的pwm寄存器不一样 16代表定时器4 12代表定时器3 8代表定时器2


/*****************************************************************************
 * Module    : SPI-幻彩
 *****************************************************************************/


void rgb_spi1_init(u32 baud);
void spi1_senddata(void);
void rgb_spi_run(void);
void rgb_spi_off(void);
void led_rgb_5050_mode_one(void);       //移动-顺时针-逆时针
void led_rgb_5050_data_fill(u32 grb_data,u8 led_num);//数据填充 0码1码

//以下5个模式，基本上够用，还有一些对称的效果，参考陆工A8
void rgb_spi_single_color(void);///单色模式(r-g-b-rg-rb-gb-rgb)
void rgb_spi_rgb_rotating(void);///旋转模式
void rgb_spi_pause(void);       ///呼吸渐变灯
void rgb_spi_rgb_music_playing(void);///频谱灯，根据节奏决定亮灯数量
void rgb_spi_rgb_six_color(void);///单色呼吸R-G-B-RG-RB-GB

void rgb_spi_run_2(void);

//模式切换
//#if RGB_SERIAL_EN
//        case KD_MODE:
//        case KU_LIGHT_LED:
//            sys_cb.rgb_on_off = 0;//关灯模式
//            sys_cb.rgb_mode++;
//            if(sys_cb.rgb_mode>LED_MODE_NUM)
//            {
//                sys_cb.rgb_mode = 0;
//            }
//            printf("sys_cb.rgb_mode = %d\n",sys_cb.rgb_mode);
//            break;
//        case KL_MODE:
//        case KL_LIGHT_LED:
//            sys_cb.rgb_on_off ^= 1;
//            printf("sys_cb.rgb_on_off = %d\n",sys_cb.rgb_on_off);
//            break;
//#endif // ifRGB_SERIAL_EN

//能量灯 能量 dac_pcm_pow_calc(); 随机数get_random();
//#if RGB_SERIAL_EN
//    else if(tmr5ms_cnt % 20) {
//        //dac_pcm_pow_calc 需要避免和 dac_dnr_detect 在同一时间调用 (两个函数用到同一硬件模块)，否则能量值不准
//        //dac_pcm_pow_calc 可以在这里调用，硬件计算能量,很快,us级别
//        u16 dac_pwr = dac_pcm_pow_calc(); //调用示例
//        sys_cb.rgb_number = dac_pwr / 3000;
//        if(sys_cb.rgb_number >=LED_NUM) sys_cb.rgb_number =LED_NUM;
//    }
//#endif // RGB_SERIAL_EN

//步骤1 初始化 标志位需要在bsp_sys.h初始化
//#if RGB_SERIAL_EN
//    u8  rgb_on_off;              //RGB开关
//    u8  rgb_flag;                //颜色转换
//    u8  rgb_mode;                //模式转换
//    u8  rgb_number;              //闪灯的数量
//#endif // RGB_SERIAL_EN

//步骤2 打开宏并且把rgb_spi_run放在5MS中断
//如果存在其他异常，karaok模式出现喇叭异响-空间不够，放在大循环使用+播报提示音。
//#if RGB_SERIAL_EN
//    static u8 flag;
//    if(sys_cb.rgb_on_off == 0){
//        rgb_spi_run();
//        flag = 0;
//    }else{
//        if(flag<=20)
//        {
//            flag++;
//            rgb_spi_off();//关灯模式
//        }
//    }
//#endif // RGB_SERIAL_EN

//#if RGB_SERIAL_EN
//    if(sys_cb.rgb_on_off == 0&&(SPI1CON & BIT(16))){//中断执行代码，不需要等待。
//        rgb_spi_run();
//    }else if(sys_cb.rgb_on_off == 1){
//        rgb_spi_off();//关灯模式
//    }
//#endif // RGB_SERIAL_EN

//步骤3 时钟需要120M，如果RGB不正常，排查一下时钟。
//#define SYS_CLK_SEL                     SYS_120M                 //选择系统时钟

//步骤4 收音模式会切换时钟 需要改两点 1 fmrx_sysclk_config()    2 void fmrx_auto_sysclk_switch(u16 freq){}; 但是会影响收音效果，560X音响时钟必须120M,不然工作异常。
//步骤5 收音模式优化 会导致spi功能关闭 FMRX_OPTIMIZE_TRY
//步骤6 注意事项新增：1关机需要关灯    2关灯函数需要增加时间50Ms执行 3引脚初始化放在IO_INIT U盘sd卡升级需要关灯
//步骤7 注意程序执行速度，过快会影响其他异常

//注意硬件电路设计，如果硬件处理的不好，会出现信号互相干扰

//注意 这个需要给公共区，否则会线程丢失 //AT(.com_text.rgb)

//注意 由于程序空间大小以及芯片spi的性能，这个灯数量尽量小于等于16个 没有用到的模式可以注释掉，省空间。



//#define LED_NUM         16
#define LED_NUM         4
#define LED_MODE_NUM    13

//颜色定义 (r-g-b-rg-rb-gb-rgb)
#define bright_Green    0x00ff0000 //GRB
#define bright_Red      0x0000ff00
#define bright_Blue     0x000000ff
#define bright_Yellow   0x00ffff00//黄色(bright_Green | bright_Red)
#define bright_Cyan     0x00ff00ff//青色(bright_Green | bright_Blue)
#define bright_Magenta  0x0000ffff//品红色(bright_Red | bright_Blue)
#define bright_White    0X00FFFFFF//白色(bright_Green | bright_Red | bright_Blue)//
#define dim_White       0x007f7f7f                  //暗淡白
#define single_led_off  0x00000000                  //关灯
#endif //__PORT_PWM_H__
