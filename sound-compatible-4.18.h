/*
 * (C) Copyright 2017-2018
 * Seeed Technology Co., Ltd. <www.seeedstudio.com>
 *
 * PeterYang <linsheng.yang@seeed.cc>
 */
#ifndef __SOUND_COMPATIBLE_4_18_H__
#define __SOUND_COMPATIBLE_4_18_H__

#include <linux/version.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,18,0)
#define __NO_SND_SOC_CODEC_DRV     1
#else
#define __NO_SND_SOC_CODEC_DRV     0
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(5,4,0)
#if __has_attribute(__fallthrough__)
# define fallthrough                    __attribute__((__fallthrough__))
#else
# define fallthrough                    do {} while (0)  /* fallthrough */
#endif
#endif

#if __NO_SND_SOC_CODEC_DRV
#define codec                      component
#define snd_soc_codec              snd_soc_component
#define snd_soc_codec_driver       snd_soc_component_driver
#define snd_soc_codec_get_drvdata  snd_soc_component_get_drvdata
#define snd_soc_codec_get_dapm     snd_soc_component_get_dapm
#define snd_soc_codec_get_bias_level snd_soc_component_get_bias_level
#define snd_soc_kcontrol_codec     snd_soc_kcontrol_component
#define snd_soc_read               snd_soc_component_read32
#define snd_soc_register_codec     snd_soc_register_component
#define snd_soc_unregister_codec   snd_soc_unregister_component
#define snd_soc_update_bits        snd_soc_component_update_bits
#define snd_soc_write              snd_soc_component_write
#define snd_soc_add_codec_controls snd_soc_add_component_controls
#endif

#endif//__SOUND_COMPATIBLE_4_18_H__

