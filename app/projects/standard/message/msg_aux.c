#include "include.h"
#include "func.h"
#include "bsp_karaok.h"

#if FUNC_AUX_EN
AT(.text.func.aux.msg)
void func_aux_message(u16 msg)
{
    switch (msg) {
        case KU_PLAY:
        case KU_PLAY_POWER:
        case KU_PLAY_MODE:
        case KU_PLAY_HSF:
            bsp_clr_mute_sta();
            func_aux_pause_play();
            break;

#if SYS_KARAOK_EN
        case KU_PREV:
        case KH_PREV:
            bsp_karaok_music_vol_adjust(0);
            break;

        case KU_NEXT:
        case KH_NEXT:
            bsp_karaok_music_vol_adjust(1);
            break;

        case KU_PREV_VOL_DOWN:
        case KL_PREV_VOL_DOWN:
        case KH_PREV_VOL_DOWN:
            bsp_karaok_mic_vol_adjust(0);
            break;

        case KU_NEXT_VOL_UP:
        case KL_NEXT_VOL_UP:
        case KH_NEXT_VOL_UP:
            bsp_karaok_mic_vol_adjust(1);
            break;
#endif

        case EVT_LINEIN_REMOVE:
            sys_cb.aux_sd_flag = 0;
            printf("1111111144EVT_LINEIN_REMOVE\n");
            if(sys_cb.aux_sd_detect_flag == 1) {
                // AUX拔出了，但如果ADC检测到SD还在，直接切回 SD音乐模式
                func_cb.sta = FUNC_MUSIC;
                sys_cb.cur_dev = DEV_SDCARD; // 同步指针
                sys_cb.aux_sd_flag2 = 0;
            } else {
                // 全空了，强制回蓝牙
                func_cb.sta = FUNC_BT;
                sys_cb.aux_sd_flag = 0;
                sys_cb.aux_sd_flag2 = 0;
            }
            break;

        case KU_REC:
#if AUX_REC_EN || REC_WHITOUT_KARAOK
            if ((!dev_is_online(DEV_SDCARD)) && (!dev_is_online(DEV_UDISK) && (!dev_is_online(DEV_SDCARD1)))) {
                break;
            }
            if (f_aux.pause) {
                func_aux_pause_play();
            }
            func_mp3_res_play(RES_BUF_REC_START_MP3, RES_LEN_REC_START_MP3);
            sfunc_record();
    #if REC_AUTO_PLAY
            if (rec_cb.flag_play) {
                rec_cb.flag_play = 0;
                func_aux_stop();
                sfunc_record_play();
                func_aux_start();
            }
    #endif

#endif // AUX_REC_EN
            break;

        default:
            func_message(msg);
            break;
    }
}
#endif // FUNC_AUX_EN
