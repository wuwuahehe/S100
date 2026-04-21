#include "include.h"

#if PWM_RGB_EN

#define PWM_RGB_PR                  255*64

//LPWM0, LPWM2, LPW3对应R, G, B控制
void pwm_init(void)
{
    GPIOEDE |= (BIT(4) | BIT(6) | BIT(7));
    GPIOEDIR &= ~(BIT(4) | BIT(6) | BIT(7));
    GPIOE &= ~(BIT(4) | BIT(6) | BIT(7));
    FUNCMCON1 = LPWM0MAP_PE4 | LPWM2MAP_PE6 | LPWM3MAP_PE7;

    PWMPRCON = PWM_RGB_PR;
    PWMCON = 0;                     //初始化disable pwm
}

void pwm_rgb_write(u8 red, u8 green, u8 blue)
{
    PWM01DUTY = ((u32)red << 6);
    PWM23DUTY = ((u32)green << 6) | ((u32)blue << (6 + 16));
    if ((PWMCON & 0x0D) != 0x0D) {
        FUNCMCON1 = LPWM0MAP_PE4 | LPWM2MAP_PE6 | LPWM3MAP_PE7;
        PWMCON |= 0x0D;             //enbale LPWM0, LPWM2, LPWM3(R,G,B)
    }
}

void pwm_rgb_close(void)
{
    FUNCMCON1 = 0xff0f0000;
    PWMCON = 0;
}


/*
void pwm_test(void)
{
    printf("%s\n", __func__);
    GPIOADE |= 0x0f;
    GPIOADIR &= ~0x0f;              // PA0 ~ PA3 对应 lowpower PWM0~PWM3 Mapping G3
    GPIOA &= ~0x0f;
    FUNCMCON1 = (3 << 16) | (3 << 20) | (3 << 24) | (3 << 28);

    PWMPRCON = 0x6590;              //PWM period = (PWMPR+1)，分辨率26MHz/4    0x6590->4ms
    PWM01DUTY = 0x32c84c2c;         //DUTY: PWMIVN=0:设置高电平时间； PWMIVN=1:设置低电平时间
    PWM23DUTY = 0x19641964;

    PWMSTEP = 0x68686868;           //PWMSTEP有效位7bit，最高位为符号位 （按STEP加减调节到DUTY或0） 0x68->16us
    PWMCYCNUM = 0x01010101;         //CYCNUM当前STEP的PWM输出次数 = PWMCYCNUM + 1

    PWMCON = 0x1f;
}
*/
#endif // PWM_RGB_EN


#if ENERGY_LED_EN    //能量灯软件PWM输出.

#define  ENERGY_LED_NUM 4         //灯的个数
#define  ENERGY_PWM_MAX_DUTY  8   //一个灯分8个亮度.

#define  ENERGY_LED_INIT()     {GPIOBDE |= 0x0F; GPIOBDIR &= ~0x0F;}
#define  ENERGY_LED_0_ON()     GPIOBSET = BIT(0)
#define  ENERGY_LED_0_OFF()    GPIOBCLR = BIT(0)
#define  ENERGY_LED_1_ON()     GPIOBSET = BIT(1)
#define  ENERGY_LED_1_OFF()    GPIOBCLR = BIT(2)
#define  ENERGY_LED_2_ON()     GPIOBSET = BIT(2)
#define  ENERGY_LED_2_OFF()    GPIOBCLR = BIT(2)
#define  ENERGY_LED_3_ON()     GPIOASET = BIT(3)
#define  ENERGY_LED_3_OFF()    GPIOACLR = BIT(3)

u8 pwm_duty_buf[ENERGY_LED_NUM];

AT(.com_text.rgb.tbl)   //非线性量化表.
u8 const energy_qtz_tbl[4 * 8] = {
     1,14,18,22,26,30,35,40,
     41,43,46,48,51,53,56,60,
     61,63,66,68,71,73,76,80,
     81,83,85,88,91,93,96,100,
};

void energy_led_init(void)
{
    ENERGY_LED_INIT();
}

AT(.com_text.rgb)
void energy_led_level_calc(void)    //约5ms调用一次.
{
    u8 level,i;
    u16 energy;
    static u8 disp_level,last_level;
    static u8 time_5ms_cnt = 0;

    time_5ms_cnt++;
    if(time_5ms_cnt >= 4){   //20ms计算一次能量
        time_5ms_cnt  = 0;

        energy = dac_pcm_pow_calc();
        //printf("[%X_%d]",rgb_led_en,energy);
        if (FUNC_MUSIC == func_cb.sta) {   //各模式能量效果可能不一样.
            energy = energy/450;
        } else if (FUNC_FMRX == func_cb.sta) {
            energy = energy/350;
        } else{
            energy = energy/300;
        }
        //非线性量化表
        for (i = 0, last_level = 0; i < 4*8; i++) {
           if (energy < energy_qtz_tbl[i]) {
               break;
           }
           last_level++;
        }
    }
    //能量相同, 不用更新
    if(disp_level == last_level){
        return;
    }

    //能量减少时,慢慢下降
    if (disp_level > last_level) {
        disp_level--;
    } else {
        disp_level = last_level;
    }

    //能量转为占空比,存放到pwm_duty_buf中.
    level = disp_level;
    for (i=0; i<ENERGY_LED_NUM; i++){
        if (level >= ENERGY_PWM_MAX_DUTY) {
            pwm_duty_buf[i] = 0;
            level -= ENERGY_PWM_MAX_DUTY;
        } else {
            pwm_duty_buf[i] = (1<<level) - 1;
            pwm_duty_buf[i] = ~pwm_duty_buf[i];
            level = 0;
        }
    }
}

AT(.com_text.rgb)
void energy_led_scan(void)  //建议1ms扫描一次.
{
    static u8 scan_bit = 0;
    if (pwm_duty_buf[0] & BIT(scan_bit)){
        ENERGY_LED_0_OFF();
    } else {
        ENERGY_LED_0_ON();
    }

    if (pwm_duty_buf[1] & BIT(scan_bit)) {
        ENERGY_LED_1_OFF();
    } else {
        ENERGY_LED_1_ON();
    }

    if (pwm_duty_buf[2] & BIT(scan_bit)) {
        ENERGY_LED_2_OFF();
    } else {
        ENERGY_LED_2_ON();
    }

    if (pwm_duty_buf[3] & BIT(scan_bit)) {
        ENERGY_LED_3_OFF();
    } else {
        ENERGY_LED_3_ON();
    }

    if (++scan_bit >= ENERGY_PWM_MAX_DUTY){ //ENERGY_PWM_MAX_DUTY= 8
        scan_bit = 0;
    }
}
#endif // ENERGY_LED_EN

#if RGB_SERIAL_EN
/**************************************************************************************************
///由于时序要求严格，建议使用120M系统时钟，否则因为系统时间更改导致SPI 波特率变化引起时序错误
***************************************************************************************************/
#define RGB_BAUD    6666666//OK
#define LOGIC_0     0xC0
#define LOGIC_1     0xFC
//逻辑0:0x80   10000000
//逻辑1:0xE0   11100000
//24bit 高位先发 数据格式：8bit G + 8bit R + 8bit B

void led_rgb_5050_data_fill(u32 grb_data,u8 led_num);
u16 timr_cnt;
u16 timr_cnt_rgb;

AT(.com_text.rgb.tbl)
u8 rgb_buf[24*LED_NUM]; //rgb数据BUF
extern u8 rgb_buf[24*LED_NUM];

u32 rgb_data;//灯颜色对应值
u32 rgb_data_1;//灯颜色对应值

//io初始化
AT(.com_text.rgb)
void rgb_spi1_init(u32 baud)
{
//    PA4 G1  PA7 G2  PB2 G3  PE7 G4  PF2 G5
 #if (RGB_SERIAL_EN == 1)
    GPIOAFEN |= BIT(4);
    GPIOADE |=  BIT(4);
    GPIOADIR &= ~(BIT(4));  //CLK,DO output
    FUNCMCON1 = (0x0F<<12);
    FUNCMCON1 = (0x01<<12);
#elif (RGB_SERIAL_EN == 2)
    GPIOAFEN |= BIT(7);
    GPIOADE |=  BIT(7);
    GPIOADIR &= ~(BIT(7));  //CLK,DO output
    FUNCMCON1 = (0x0F<<12);
    FUNCMCON1 = (0x02<<12);
#elif (RGB_SERIAL_EN == 3)
    GPIOBFEN |= BIT(2);
    GPIOBDE |=  BIT(2);
    GPIOBDIR &= ~(BIT(2));  //CLK,DO output
    FUNCMCON1 = (0x0F<<12);
    FUNCMCON1 = (0x03<<12);
#elif(RGB_SERIAL_EN == 4)
    GPIOEFEN |= BIT(7);
    GPIOEDE |=  BIT(7);
    GPIOEDIR &= ~(BIT(7));  //CLK,DO output
    FUNCMCON1 = (0x0F<<12);
    FUNCMCON1 = (0x04<<12);  //G4
#elif (RGB_SERIAL_EN == 5)
    GPIOFFEN |= BIT(2);
    GPIOFDE |=  BIT(2);
    GPIOFDIR &= ~(BIT(2));  //CLK,DO output
    FUNCMCON1 = (0x0F<<12);
    FUNCMCON1 = (0x05<<12);
#endif

    SPI1BAUD = 120000000/baud - 1;   //sysclk/baud - 1
    SPI1CON =  0x01 | (1<<2);
}

/*直接调用此函数更新数据即可*/
AT(.com_text.rgb)
void spi1_senddata(void)
{
    SPI1DMAADR = DMA_ADR(rgb_buf);
    SPI1DMACNT = sizeof(rgb_buf);
    while(!(SPI1CON & BIT(16))){  //wait send finish
        WDT_CLR();
    }
}

//关灯模式
AT(.com_text.rgb)
void rgb_spi_off(void)
{
    WDT_CLR();
    memset(rgb_buf, 0xC0, sizeof(rgb_buf));
    spi1_senddata();
}

AT(.com_text.rgb)
void led_rgb_5050_data_fill(u32 grb_data,u8 led_num)
{
     grb_data <<=8;//左移8bit，高位对齐
     for(u8 i=0;i<24;i++)
     {
        if(grb_data&BIT(31))
        {
            rgb_buf[led_num*24+i] |=LOGIC_1;
        }
        else
        {
            rgb_buf[led_num*24+i] |=LOGIC_0;
        }
        grb_data <<=1;
     }
}


//根据颜色来选择颜色对应值并返回
AT(.com_text.rgb)
u32 led_color_select(u8 num)
{
    u32 re_color = 0;
    if     (num == 0)re_color = bright_Red;
    else if(num == 1)re_color = bright_Green;
    else if(num == 2)re_color = bright_Blue;
    else if(num == 3)re_color = bright_Yellow;
    else if(num == 4)re_color = bright_Cyan;
    else if(num == 5)re_color = bright_Magenta;
    else if(num == 6)re_color = single_led_off;
    else re_color = single_led_off;

    return re_color;
}
AT(.com_text.rgb.table)
u32 data_fill[LED_NUM]={};//GRB

//AT(.com_text.rgb)
//void rgb_spi_single_color(void)//单色模式
//{
//    if(timr_cnt++ < 30){ //调节速度
//        return;
//    }
//    timr_cnt = 0;
//    WDT_CLR();
//    rgb_data = led_color_select(sys_cb.rgb_flag);//红色
//    for(u8 i=0;i<LED_NUM;i++)
//    {
//        data_fill[i] = rgb_data;
//    }
//    memset(rgb_buf,0xc0,sizeof(rgb_buf));//清除rgb数据BUF
//    for(u8 i=0;i<LED_NUM;i++)
//    {
//      led_rgb_5050_data_fill(data_fill[i],i);//填充rgb数据BUF
//    }
//    spi1_senddata();
//}

//u32 data_fill_2[LED_NUM]={0xFF00,0xFF00,0xFF00,0xFF00,0xFF00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF0000,0xFF0000,0xFF0000,0xFF0000,0xFF0000};//旋转专用 //GRB
//AT(.com_text.rgb)
//void led_rgb_5050_mode_one(void)
//{
///***************************滚动代码**************************************/ //往前
////    u32 overflow_buf;
////    for(u8 i=0;i<1;i++)
////    {
////        overflow_buf =data_fill_1[i];//保存第一个灯24bit数据
////    }
////    for(u8 i=0;i<LED_NUM-1;i++)
////    {
////        data_fill_1[i] =data_fill_1[(i+1)];//左移一个灯数据24byte
////
////    }
////    for(u8 i=LED_NUM-1;i < LED_NUM;i++)
////    {
////        data_fill_1[LED_NUM-1] =overflow_buf;//保存第一个灯24bit数据
////    }
//
///***************************滚动代码**************************************/ //往后
//    u32 overflow_buf;
//    for(u8 i=LED_NUM-1;i < LED_NUM;i++)
//    {
//        overflow_buf =data_fill_2[LED_NUM-1];//保存最后一个灯24bit数据
//    }
//    for(u8 i=LED_NUM-1;i>0;i--)
//    {
//        data_fill_2[i] =data_fill_2[(i-1)];//左移一个灯数据24byte
//
//    }
//    for(u8 i=0;i<1;i++)
//    {
//        data_fill_2[0] =overflow_buf;//保存第一个灯24bit数据
//    }
//}
//
AT(.com_text.rgb)
void rgb_spi_pause(void)
{
    if(timr_cnt++ < 10){ //调节速度
        return;
    }
    timr_cnt = 0;

    WDT_CLR();

    static u16 r_timr_cnt = 255;//0~255
    static u16 g_timr_cnt = 0;//0~255
    static u16 b_timr_cnt = 0;//0~255
    static u8 rgb_cnt = 0;//0~2

    if(rgb_cnt == 0){
        r_timr_cnt--;g_timr_cnt++;b_timr_cnt = 0;
        if(r_timr_cnt == 0){rgb_cnt = 1;}
    }else if(rgb_cnt == 1){
        r_timr_cnt = 0;g_timr_cnt--;b_timr_cnt++;
        if(g_timr_cnt == 0){rgb_cnt = 2;}
    }else if(rgb_cnt == 2){
        r_timr_cnt++;g_timr_cnt = 0;b_timr_cnt--;
        if(b_timr_cnt == 0){rgb_cnt = 0;}
    }

    rgb_data = (r_timr_cnt << 8)|(g_timr_cnt << 16)|(b_timr_cnt << 0);

    for(u8 i = 0;i < LED_NUM;i++){        //亮灯
        data_fill[i] = rgb_data;
    }

    memset(rgb_buf,0xc0,sizeof(rgb_buf));//清除rgb数据BUF
    for(u8 i=0;i<LED_NUM;i++)
    {
      led_rgb_5050_data_fill(data_fill[i],i);//填充rgb数据BUF
    }
    spi1_senddata();

}
//AT(.com_text.rgb)
//void rgb_spi_rgb_rotating(void)////旋转模式
//{
//    if(timr_cnt++ < 60){ //调节速度
//        return;
//    }
//    timr_cnt = 0;
//    WDT_CLR();
//
//    led_rgb_5050_mode_one();
//    memset(rgb_buf,0xc0,sizeof(rgb_buf));//清除rgb数据BUF
//    for(u8 i=0;i<LED_NUM;i++)
//    {
//      led_rgb_5050_data_fill(data_fill_2[i],i);//填充rgb数据BUF
//    }
//    spi1_senddata();
//}
//
//
//AT(.com_text.rgb)
//void rgb_spi_rgb_music_playing(void)
//{
//    WDT_CLR();
//    if(timr_cnt++ < 500){
//
//    }
//    else{
//        timr_cnt = 0;
//        sys_cb.rgb_flag++;
//    }
//    if(sys_cb.rgb_flag == 6)
//    {
//        sys_cb.rgb_flag = 0;
//    }
//
//    rgb_data = led_color_select(sys_cb.rgb_flag);//更换RGB数据
//
//    for(u8 i=0;i<sys_cb.rgb_number;i++)      //填充数据
//    {
//        data_fill[i] = rgb_data;
//    }
//    memset(rgb_buf,0xc0,sizeof(rgb_buf));//清除rgb数据BUF
//    for(u8 i=0;i<sys_cb.rgb_number;i++)
//    {
//      led_rgb_5050_data_fill(data_fill[i],i);//填充rgb数据BUF
//    }
//    spi1_senddata();
//
//
//}
//
u8 led_dir; //加减的方向
AT(.com_text.rgb)
void rgb_spi_rgb_six_color(void)
{
    WDT_CLR();

    static u8 r_timr_cnt = 0;//0~255
    static u8 g_timr_cnt = 0;//0~255
    static u8 b_timr_cnt = 0;//0~255
    static u8 rgb_cnt = 0;//0~5

    if(rgb_cnt == 0){//r
        if(led_dir == 0)
        {
            r_timr_cnt++;
            if(r_timr_cnt == 255) led_dir = 1;
        }else{
            r_timr_cnt--;
            if(r_timr_cnt == 0){rgb_cnt = 1;led_dir = 0;}
        }
    }else if(rgb_cnt == 1){//g
        if(led_dir == 0)
        {
            g_timr_cnt++;
            if(g_timr_cnt == 255) led_dir = 1;
        }else{
            g_timr_cnt--;
            if(g_timr_cnt == 0){rgb_cnt = 2;led_dir = 0;}
        }
    }else if(rgb_cnt == 2){//b
        if(led_dir == 0)
        {
            b_timr_cnt++;
            if(b_timr_cnt == 255) led_dir = 1;
        }else{
            b_timr_cnt--;
            if(b_timr_cnt == 0){rgb_cnt = 3;led_dir = 0;}
        }
    }
    else if(rgb_cnt == 3){//rg
        if(led_dir == 0)
        {
            r_timr_cnt++;g_timr_cnt++;
            if(r_timr_cnt == 255) led_dir = 1;
        }else{
            r_timr_cnt--;g_timr_cnt--;
            if(r_timr_cnt == 0){rgb_cnt = 4;led_dir = 0;}
        }
    }
    else if(rgb_cnt == 4){//rg
        if(led_dir == 0)
        {
            b_timr_cnt++;g_timr_cnt++;
            if(b_timr_cnt == 255) led_dir = 1;
        }else{
            b_timr_cnt--;g_timr_cnt--;
            if(b_timr_cnt == 0){rgb_cnt = 5;led_dir = 0;}
        }
    }
    else if(rgb_cnt == 5){//rg
        if(led_dir == 0)
        {
            b_timr_cnt++;r_timr_cnt++;
            if(b_timr_cnt == 255) led_dir = 1;
        }else{
            b_timr_cnt--;r_timr_cnt--;
            if(b_timr_cnt == 0){rgb_cnt = 0;led_dir = 0;}
        }

    }
    rgb_data = (r_timr_cnt << 8)|(g_timr_cnt << 16)|(b_timr_cnt << 0);

    for(u8 i = 0;i < LED_NUM;i++){        //亮灯
        data_fill[i] = rgb_data;
    }

    memset(rgb_buf,0xc0,sizeof(rgb_buf));//清除rgb数据BUF
    for(u8 i=0;i<LED_NUM;i++)
    {
      led_rgb_5050_data_fill(data_fill[i],i);//填充rgb数据BUF
    }
    spi1_senddata();

}

AT(.com_text.rgb)
void  rgb_spi_single_color_two_bright_two_extinguish()//亮两灯，熄灭两灯
{
    if(timr_cnt++ < 100){ //调节速度
        return;
    }
    timr_cnt = 0;
    WDT_CLR();
    static u8 mode_color= 0;
    mode_color++;
    if(mode_color>=12){
        mode_color = 0;
    }
    rgb_data = led_color_select(mode_color/2);//红色

    if(mode_color%2){
        data_fill[0] = rgb_data;
        data_fill[1] = 0X00;
        data_fill[2] = 0X00;
        data_fill[3] = rgb_data;
    }else{
        data_fill[0] = 0X00;
        data_fill[1] = rgb_data;
        data_fill[2] = rgb_data;
        data_fill[3] = 0X00;
    }


    memset(rgb_buf,0xc0,sizeof(rgb_buf));//清除rgb数据BUF
    for(u8 i=0;i<LED_NUM;i++)
    {
      led_rgb_5050_data_fill(data_fill[i],i);//填充rgb数据BUF
    }
    spi1_senddata();
}


//
//u32 data_fill_1[LED_NUM]={0x00C80000,0x00A02800,0x00785000,0x00507800,0x0028A000,
//                            0x0100C800,0x0100A028,0x01007850,0x01005078,0x010028A0,
//                            0x020000C8,0x022800A0,0x02500078,0x02780050,0x02A00028,0x02A00028};//   16个rgb使用
//
//#define RGB_SPEED           4          // 1或者2  只能固定
//AT(.com_text.rgb)
//u32 rgb_spi_breathing_one(u32 date)
//{
//    //RGB_CNT - R - G - B   U32 date 0XFFFFFF
//    u8 rgb_cnt    = date >> 24;
//    u8 r_timr_cnt = date >> 16;
//    u8 g_timr_cnt = date >> 8;
//    u8 b_timr_cnt = date;
//
//    if(rgb_cnt == 0){
//        r_timr_cnt-=RGB_SPEED;g_timr_cnt+=RGB_SPEED;b_timr_cnt = 0;
//        if(r_timr_cnt == 0){rgb_cnt = 1;}
//    }else if(rgb_cnt == 1){
//        r_timr_cnt = 0;g_timr_cnt-=RGB_SPEED;b_timr_cnt+=RGB_SPEED;
//        if(g_timr_cnt == 0){rgb_cnt = 2;}
//    }else if(rgb_cnt == 2){
//        r_timr_cnt+=RGB_SPEED;g_timr_cnt = 0;b_timr_cnt-=RGB_SPEED;
//        if(b_timr_cnt == 0){rgb_cnt = 0;}
//    }
//
//    rgb_data = (r_timr_cnt << 16)|(g_timr_cnt << 8)|(rgb_cnt << 24)|(b_timr_cnt << 0);//返回值
//    return rgb_data;
//}
//
//
//AT(.com_text.rgb)
//void rgb_spi_breathing_color(u8 speed,u8 speed_rgb){//每个灯不同颜色移动
//    WDT_CLR();
//
//    if(timr_cnt++ < speed){             //控制发送数据变化----调节速度
//        return;
//    }
//    timr_cnt = 0;
//
//    for(u8 i=0;i<LED_NUM;i++)
//    {
//        data_fill_1[i] = rgb_spi_breathing_one(data_fill_1[i]);//全部呼吸效果
//    }
//
//    if(timr_cnt_rgb++ < speed_rgb){ //控制颜色变化----调节速度
//        return;
//    }
//    timr_cnt_rgb = 0;
//
//
//    memset(rgb_buf,0xc0,sizeof(rgb_buf));//清除rgb数据BUF
//    for(u8 i=0;i<LED_NUM;i++)
//    {
//      led_rgb_5050_data_fill(data_fill_1[i],i);//填充rgb数据BUF
//    }
//    spi1_senddata();
//}
//*******************彩虹色往前移动，不常用*******************
AT(.com_text.rgb)
u32 WS281x_Color(u8 red, u8 green, u8 blue) { return green << 16 | red << 8 | blue; }

AT(.com_text.rgb)
u32 Wheel(u8 WheelPos)
{
	WheelPos = 255 - WheelPos;
	if (WheelPos < 85){
		return WS281x_Color(255 - WheelPos * 3, 0, WheelPos * 3);
	}
	if (WheelPos < 170){
		WheelPos -= 85;
		return WS281x_Color(0, WheelPos * 3, 255 - WheelPos * 3);
	}
	WheelPos -= 170;
	return WS281x_Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

AT(.com_text.rgb)
void rgb_spi_rainbow(void)
{
	static u8 j;
	u32 color;
	WDT_CLR();

	j++;//左右移动速度

	for (u8 i = 0; i < LED_NUM; i++){
		color=Wheel(((i * 256 / (LED_NUM)) + j) & 255);
		data_fill[i] = color;
	}

    memset(rgb_buf,0xc0,sizeof(rgb_buf));//清除rgb数据BUF
    for(u8 i=0;i<LED_NUM;i++){
      led_rgb_5050_data_fill(data_fill[i],i);//填充rgb数据BUF
    }
    spi1_senddata();
}

u8 mode_change_flag;
AT(.com_text.rgb)
void  rgb_spi_single_color_18()
{
    static u8 led_color;
    static u16 timr_cnt;

    if(timr_cnt++ < 20 && (!mode_change_flag)){ //调节速度
        return;
    }
    mode_change_flag = 0;
    timr_cnt = 0;
    WDT_CLR();

    static u8 led_cnt =0;


    led_cnt++;

    if(led_cnt>=18){
        led_cnt = 0;
        led_color++;
        if(led_color>=6){
            led_color = 0;
        }
    }

    rgb_data = led_color_select(led_color);//红色
    for(u8 i=0;i<LED_NUM;i++)
    {
        data_fill[i] = 0x00;
    }

    if(led_cnt<12){
        data_fill[led_cnt%4] = rgb_data;
    }
    else if(led_cnt == 12){
        data_fill[0] = rgb_data;
        data_fill[3] = rgb_data;
    }
    else if(led_cnt == 13){
        data_fill[1] = rgb_data;
        data_fill[3] = rgb_data;
    }
    else if(led_cnt == 14){
        data_fill[2] = rgb_data;
        data_fill[3] = rgb_data;
    }
    else if(led_cnt == 15){
        data_fill[0] = rgb_data;
        data_fill[2] = rgb_data;
        data_fill[3] = rgb_data;
    }
    else if(led_cnt == 16){
        data_fill[1] = rgb_data;
        data_fill[2] = rgb_data;
        data_fill[3] = rgb_data;
    }
    else if(led_cnt == 17){
        data_fill[0] = rgb_data;
        data_fill[1] = rgb_data;
        data_fill[2] = rgb_data;
        data_fill[3] = rgb_data;
    }
    memset(rgb_buf,0xc0,sizeof(rgb_buf));//清除rgb数据BUF
    for(u8 i=0;i<LED_NUM;i++)
    {
      led_rgb_5050_data_fill(data_fill[i],i);//填充rgb数据BUF
    }
    spi1_senddata();

}

AT(.com_text.rgb)
void  rgb_spi_single_color_four_bright()//两两成对，显示不同颜色
{
    if(timr_cnt++ < 40){ //调节速度
        return;
    }
    timr_cnt = 0;
    WDT_CLR();
    static u8 led_color_1=1;
    static u8 led_color=0;

    led_color++;
    if(led_color>=6){
        led_color = 0;
    }

    led_color_1++;
    if(led_color_1>=6){
        led_color_1 = 0;
    }
    rgb_data = led_color_select(led_color);//红色
    rgb_data_1 = led_color_select(led_color_1);//绿色

     static u8 rgb_flag =0;
    if(rgb_flag ==0){
        data_fill[0] = rgb_data;
        data_fill[1] = rgb_data;

        data_fill[2] = rgb_data_1;
        data_fill[3] = rgb_data_1;
        rgb_flag =1;
    }else{
        data_fill[0] = rgb_data_1;
        data_fill[1] = rgb_data_1;

        data_fill[2] = rgb_data;
        data_fill[3] = rgb_data;
        rgb_flag =0;
    }
    memset(rgb_buf,0xc0,sizeof(rgb_buf));//清除rgb数据BUF
    for(u8 i=0;i<LED_NUM;i++)
    {
      led_rgb_5050_data_fill(data_fill[i],i);//填充rgb数据BUF
    }
    spi1_senddata();
}

AT(.com_text.rgb)
void  rgb_spi_single_color_gradient_fast()
{
    if(timr_cnt++ < 60){ //调节速度
        return;
    }
    timr_cnt = 0;
    WDT_CLR();
    static u8 led_color = 0;
    led_color++;
    if(led_color>=6){
        led_color = 0;
    }
    rgb_data = led_color_select(led_color);//红色
    for(u8 i=0;i<LED_NUM;i++)
    {
        data_fill[i] = rgb_data;
    }
    memset(rgb_buf,0xc0,sizeof(rgb_buf));//清除rgb数据BUF
    for(u8 i=0;i<LED_NUM;i++)
    {
      led_rgb_5050_data_fill(data_fill[i],i);//填充rgb数据BUF
    }
    spi1_senddata();
}

//AT(.com_text.rgb)
void rgb_spi_single_color_mode(u8 mode_color)
{
    if(timr_cnt++ < 30){ //调节速度
        return;
    }
    timr_cnt = 0;
    WDT_CLR();
    rgb_data = led_color_select(mode_color);
    for(u8 i=0;i<LED_NUM;i++)
    {
        data_fill[i] = rgb_data;
    }
    memset(rgb_buf,0xc0,sizeof(rgb_buf));//清除rgb数据BUF
    for(u8 i=0;i<LED_NUM;i++)
    {
      led_rgb_5050_data_fill(data_fill[i],i);//填充rgb数据BUF
    }
    spi1_senddata();
}


AT(.com_text.rgb)
void rgb_spi_run(void)
{

        if(sys_cb.rgb_mode == 0)
        {
            rgb_spi_rainbow();       //彩虹色往前移动
        }
        else if(sys_cb.rgb_mode == 1)
        {
            rgb_spi_rgb_six_color();//单色呼吸,模式二
        }
        else if(sys_cb.rgb_mode == 2)
        {
            rgb_spi_single_color_two_bright_two_extinguish(); //模式三
        }
        else if(sys_cb.rgb_mode == 3)
        {
            rgb_spi_pause();//全部呼吸效果,模式四
        }
        else if(sys_cb.rgb_mode == 4)
        {
            rgb_spi_single_color_18();        //模式五
        }
        else if(sys_cb.rgb_mode == 5)
        {
            rgb_spi_single_color_four_bright();  //模式六
        }
        else if(sys_cb.rgb_mode == 6)
        {
            rgb_spi_single_color_gradient_fast();  //模式七
        }
//        else if(sys_cb.rgb_mode == 7)
//        {
//            rgb_spi_single_color_mode(0); //红色
//        }
//         else if(sys_cb.rgb_mode == 8)
//        {
//            rgb_spi_single_color_mode(1); //绿色
//        }
//        else if(sys_cb.rgb_mode == 9)
//        {
//            rgb_spi_single_color_mode(2); //蓝色
//        }
//         else if(sys_cb.rgb_mode == 10)
//        {
//            rgb_spi_single_color_mode(3); //黄色
//        }
//         else if(sys_cb.rgb_mode == 11)
//        {
//            rgb_spi_single_color_mode(4); //青色
//        }
//         else if(sys_cb.rgb_mode == 12)
//        {
//            rgb_spi_single_color_mode(5); //紫色
//        }
         else if(sys_cb.rgb_mode == 13)
        {
            rgb_spi_off();//关灯模式
        }
}

AT(.com_text.rgb)
void rgb_spi_run_2(void)
{

        if(sys_cb.rgb_mode == 7)
        {
            rgb_spi_single_color_mode(0); //红色
        }
         else if(sys_cb.rgb_mode == 8)
        {
            rgb_spi_single_color_mode(1); //绿色
        }
        else if(sys_cb.rgb_mode == 9)
        {
            rgb_spi_single_color_mode(2); //蓝色
        }
         else if(sys_cb.rgb_mode == 10)
        {
            rgb_spi_single_color_mode(3); //黄色
        }
         else if(sys_cb.rgb_mode == 11)
        {
            rgb_spi_single_color_mode(4); //青色
        }
         else if(sys_cb.rgb_mode == 12)
        {
            rgb_spi_single_color_mode(5); //紫色
        }

}

#endif // RGB_SERIAL_EN



#if 0  //SYSCLK_OUT_TEST
#define PLL0DIV         SFR_RW (SFR3_BASE + 0x23*4)
#define PLL1DIV         SFR_RW (SFR3_BASE + 0x24*4)
const char strp[] = "\nPWRCON0 = 0x%X,PLL0CON = 0x%X, PLL0DIV=0x%X\n";
const char strp1[] = "CLKCON0_1= 0x%X,0X%X\n";
const char strp2[] = "CLKCON2_3= 0x%X,0X%X\n";
void print_sysclk_info(void)
{
    static u32 ticks = 0;
    if (tick_check_expire(ticks,1000)) {
        ticks = tick_get();
        printf(strp,PWRCON0,PLL0CON,PLL0DIV);
        printf(strp1,CLKCON0,CLKCON1);
        printf(strp2,CLKCON2,CLKCON3);
    }
}

u8 test_buf[360];   //rgb数据BUF
void test_sysclk_out(void)  //default PA3输出，显示需要选择为GUI_NO
{
    static bool init = false;
    if (!init) {
        init = true;
        //SPI1_G1: CLK_PA3
        GPIOAFEN |= BIT(3);
        GPIOADE |=  BIT(3);
        GPIOADIR &= ~BIT(3);    //CLK output
        FUNCMCON1 = (0x0F<<12);
        FUNCMCON1 = (0x01<<12);  //G1

        SPI1BAUD = 9;   //sysclk/baud - 1
        SPI1CON =  0x01 | (1<<2);
        SPI1DMAADR = DMA_ADR(test_buf);
        SPI1DMACNT = sizeof(test_buf);
    }
    WDT_CLR();
    print_sysclk_info();
    if (SPI1CON & BIT(16)) {
        SPI1DMAADR = DMA_ADR(test_buf);
        SPI1DMACNT = sizeof(test_buf);
    }
}
#endif    //SYSCLK_OUT_TEST

#if PWM_OUT_EN
//PA5 PWM0-T5-G1 PA6 PWM1-T5-G1  PA7 PWM2-T5-G1
AT(.text.bsp.pwm)
void timer5_pwm_init(void)
{
    GPIOADIR &= ~BIT(5);
    GPIOADE |= BIT(5);
    GPIOAFEN |= BIT(5);
    //GPIOAPU |= BIT(5);

    GPIOADIR &= ~BIT(6);
    GPIOADE |= BIT(6);
    GPIOAFEN |= BIT(6);
    //GPIOAPU |= BIT(6);

    GPIOADIR &= ~BIT(7);
    GPIOADE |= BIT(7);
    GPIOAFEN |= BIT(7);
    //GPIOAPU |= BIT(7);

    FUNCMCON2 = (0x0f << 16);
    FUNCMCON2 = (1 << 16);                    //timer5 PWM G1

    CLKGAT0 |= BIT(28);                       //pwm_clk
    CLKGAT1 |= BIT(10);                       //tmr5_clk
    asm("nop");asm("nop");
    asm("nop");asm("nop");

    TMR5CON = 0x00;
    TMR5CNT = 1;
    TMR5PR = 26000000/500-1;                  //例如为26M时, 频率为 26M/(13000*4) = 500HZ, 即周期为0.002S.
    TMR5DUTY0 = 13000-1;                      //timer5 pwm0 duty  PA5  25% 负占空比
    TMR5DUTY1 = 13000*2-1;                    //timer5 pwm1 duty  PA6  50% 负占空比
    TMR5DUTY2 = 13000*3-1;                    //timer5 pwm2 duty  PA7  75% 负占空比

    TMR5CON |= (BIT(9) | BIT(10) | BIT(11));  //timer5 pwm0 pwm1 pwm2 enable
    TMR5CON |= BIT(2) | BIT(0);               //timer5 enable
}
#endif //PWM_OUT_EN
