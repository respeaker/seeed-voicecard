/*
 * sound\soc\codec\ac10x.c
 * (C) Copyright 2014-2017
 * Reuuimlla Technology Co., Ltd. <www.reuuimllatech.com>
 *
 * huangxin <huangxin@Reuuimllatech.com>
 * liushaohua <liushaohua@allwinnertech.com>
 *
 * some simple description for this code
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>
#include <sound/tlv.h>
#include <linux/io.h>
#include <linux/regulator/consumer.h>
#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/input.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/clk.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/regmap.h>
#include "ac101.h"


#define CONFIG_SWITCH_DETECT_EXTERNAL
#ifndef CONFIG_SWITCH_DETECT_EXTERNAL
static volatile int reset_flag = 0;
static int hook_flag1 = 0;
static int hook_flag2 = 0;
static int KEY_VOLUME_FLAG = 0;
/* 1=headphone in slot, 0 else */
static int headphone_state = 0;
static volatile int irq_flag = 0;
static struct workqueue_struct *switch_detect_queue;
static struct workqueue_struct *codec_irq_queue;
/* key define */
#define KEY_HEADSETHOOK         226
#define HEADSET_CHECKCOUNT  (10)
#define HEADSET_CHECKCOUNT_SUM  (2)
#define SWITCH_DETECT 364
#endif


//#define PA_CTL	205
#define PA_CTL		0
#define I2C_BUS 1

/*Default initialize configuration*/
#define SPEAKER_DOUBLE_USED  1
#define D_SPEAKER_VOL  0x1b
#define S_SPEAKER_VOL  0x19
#define HEADPHONE_VOL  0x3b
#define EARPIECE_VOL  0x1e
#define MAINMIC_GAIN  0x4
#define HDSETMIC_GAIN  0x4
#define DMIC_USED  0
#define ADC_DIGITAL_GAIN  0xb0b0
#define AGC_USED 0
#define DRC_USED 1

static bool speaker_double_used = false;
static int double_speaker_val = 0;
static int single_speaker_val = 0;
static int headset_val = 0;
static int earpiece_val = 0;
static int mainmic_val = 0;
static int headsetmic_val = 0;
static bool dmic_used = false;
static int adc_digital_val = 0;
static bool agc_used 		= false;
static bool drc_used 		= false;

#define ac10x_RATES  (SNDRV_PCM_RATE_8000_96000|SNDRV_PCM_RATE_KNOT)
#define ac10x_FORMATS ( SNDRV_PCM_FMTBIT_S8  |	\
			SNDRV_PCM_FMTBIT_S16_LE | \
			/* SNDRV_PCM_FMTBIT_S18_3LE |	\
			SNDRV_PCM_FMTBIT_S20_3LE |	\
			SNDRV_PCM_FMTBIT_S24_LE |	\
			SNDRV_PCM_FMTBIT_S32_LE*/	\
			0)

enum headphone_mode_u {
	HEADPHONE_IDLE,
	FOUR_HEADPHONE_PLUGIN,
	THREE_HEADPHONE_PLUGIN,
};

/*supply voltage*/
static const char *ac10x_supplies[] = {
	"vcc-avcc",
	"vcc-io1",
	"vcc-io2",
	"vcc-ldoin",
	"vcc-cpvdd",
};

/*struct for ac10x*/
struct ac10x_priv {
	struct snd_soc_codec *codec;

	unsigned sysclk;

	struct regmap* regmap;

	struct mutex dac_mutex;
	struct mutex adc_mutex;
	u8 dac_enable;
	u8 adc_enable;
	struct mutex aifclk_mutex;
	u8 aif1_clken;
	u8 aif2_clken;
	u8 aif3_clken;

	/*voltage supply*/
	int num_supplies;
	struct regulator_bulk_data *supplies;

#ifndef CONFIG_SWITCH_DETECT_EXTERNAL
	/*headset*/
	int virq; /*headset irq*/
	struct switch_dev sdev;
	int state;
	int check_count;
	int check_count_sum;
	int reset_flag;
#endif

	enum headphone_mode_u mode;
	struct work_struct work;
	struct work_struct clear_codec_irq;
	struct work_struct codec_resume;
	struct work_struct state_work;

	struct semaphore sem;

	struct timer_list timer;

	struct input_dev *key;
};

void get_configuration(void)
{
	speaker_double_used = SPEAKER_DOUBLE_USED;
	double_speaker_val = D_SPEAKER_VOL;
	single_speaker_val = S_SPEAKER_VOL;
	headset_val = HEADPHONE_VOL;
	earpiece_val = EARPIECE_VOL;
	mainmic_val = MAINMIC_GAIN;
	headsetmic_val = HDSETMIC_GAIN;
	dmic_used = DMIC_USED;
	if (dmic_used) {
		adc_digital_val = ADC_DIGITAL_GAIN;
	}
	agc_used = AGC_USED;
	drc_used = DRC_USED;

}
void agc_config(struct snd_soc_codec *codec)
{
	int reg_val;
	reg_val = snd_soc_read(codec, 0xb4);
	reg_val |= (0x3<<6);
	snd_soc_write(codec, 0xb4, reg_val);

	reg_val = snd_soc_read(codec, 0x84);
	reg_val &= ~(0x3f<<8);
	reg_val |= (0x31<<8);
	snd_soc_write(codec, 0x84, reg_val);

	reg_val = snd_soc_read(codec, 0x84);
	reg_val &= ~(0xff<<0);
	reg_val |= (0x28<<0);
	snd_soc_write(codec, 0x84, reg_val);

	reg_val = snd_soc_read(codec, 0x85);
	reg_val &= ~(0x3f<<8);
	reg_val |= (0x31<<8);
	snd_soc_write(codec, 0x85, reg_val);

	reg_val = snd_soc_read(codec, 0x85);
	reg_val &= ~(0xff<<0);
	reg_val |= (0x28<<0);
	snd_soc_write(codec, 0x85, reg_val);

	reg_val = snd_soc_read(codec, 0x8a);
	reg_val &= ~(0x7fff<<0);
	reg_val |= (0x24<<0);
	snd_soc_write(codec, 0x8a, reg_val);

	reg_val = snd_soc_read(codec, 0x8b);
	reg_val &= ~(0x7fff<<0);
	reg_val |= (0x2<<0);
	snd_soc_write(codec, 0x8b, reg_val);

	reg_val = snd_soc_read(codec, 0x8c);
	reg_val &= ~(0x7fff<<0);
	reg_val |= (0x24<<0);
	snd_soc_write(codec, 0x8c, reg_val);

	reg_val = snd_soc_read(codec, 0x8d);
	reg_val &= ~(0x7fff<<0);
	reg_val |= (0x2<<0);
	snd_soc_write(codec, 0x8d, reg_val);

	reg_val = snd_soc_read(codec, 0x8e);
	reg_val &= ~(0x1f<<8);
	reg_val |= (0xf<<8);
	reg_val &= ~(0x1f<<0);
	reg_val |= (0xf<<0);
	snd_soc_write(codec, 0x8e, reg_val);

	reg_val = snd_soc_read(codec, 0x93);
	reg_val &= ~(0x7ff<<0);
	reg_val |= (0xfc<<0);
	snd_soc_write(codec, 0x93, reg_val);
	snd_soc_write(codec, 0x94, 0xabb3);
}
void drc_config(struct snd_soc_codec *codec)
{
	int reg_val;
	reg_val = snd_soc_read(codec, 0xa3);
	reg_val &= ~(0x7ff<<0);
	reg_val |= 1<<0;
	snd_soc_write(codec, 0xa3, reg_val);
	snd_soc_write(codec, 0xa4, 0x2baf);

	reg_val = snd_soc_read(codec, 0xa5);
	reg_val &= ~(0x7ff<<0);
	reg_val |= 1<<0;
	snd_soc_write(codec, 0xa5, reg_val);
	snd_soc_write(codec, 0xa6, 0x2baf);

	reg_val = snd_soc_read(codec, 0xa7);
	reg_val &= ~(0x7ff<<0);
	snd_soc_write(codec, 0xa7, reg_val);
	snd_soc_write(codec, 0xa8, 0x44a);

	reg_val = snd_soc_read(codec, 0xa9);
	reg_val &= ~(0x7ff<<0);
	snd_soc_write(codec, 0xa9, reg_val);
	snd_soc_write(codec, 0xaa, 0x1e06);

	reg_val = snd_soc_read(codec, 0xab);
	reg_val &= ~(0x7ff<<0);
	reg_val |= (0x352<<0);
	snd_soc_write(codec, 0xab, reg_val);
	snd_soc_write(codec, 0xac, 0x6910);

	reg_val = snd_soc_read(codec, 0xad);
	reg_val &= ~(0x7ff<<0);
	reg_val |= (0x77a<<0);
	snd_soc_write(codec, 0xad, reg_val);
	snd_soc_write(codec, 0xae, 0xaaaa);

	reg_val = snd_soc_read(codec, 0xaf);
	reg_val &= ~(0x7ff<<0);
	reg_val |= (0x2de<<0);
	snd_soc_write(codec, 0xaf, reg_val);
	snd_soc_write(codec, 0xb0, 0xc982);

	snd_soc_write(codec, 0x16, 0x9f9f);

}
void agc_enable(struct snd_soc_codec *codec,bool on)
{
	int reg_val;
	if (on) {
		reg_val = snd_soc_read(codec, MOD_CLK_ENA);
		reg_val |= (0x1<<7);
		snd_soc_write(codec, MOD_CLK_ENA, reg_val);
		reg_val = snd_soc_read(codec, MOD_RST_CTRL);
		reg_val |= (0x1<<7);
		snd_soc_write(codec, MOD_RST_CTRL, reg_val);

		reg_val = snd_soc_read(codec, 0x82);
		reg_val &= ~(0xf<<0);
		reg_val |= (0x6<<0);

		reg_val &= ~(0x7<<12);
		reg_val |= (0x7<<12);
		snd_soc_write(codec, 0x82, reg_val);

		reg_val = snd_soc_read(codec, 0x83);
		reg_val &= ~(0xf<<0);
		reg_val |= (0x6<<0);

		reg_val &= ~(0x7<<12);
		reg_val |= (0x7<<12);
		snd_soc_write(codec, 0x83, reg_val);
	} else {
		reg_val = snd_soc_read(codec, MOD_CLK_ENA);
		reg_val &= ~(0x1<<7);
		snd_soc_write(codec, MOD_CLK_ENA, reg_val);
		reg_val = snd_soc_read(codec, MOD_RST_CTRL);
		reg_val &= ~(0x1<<7);
		snd_soc_write(codec, MOD_RST_CTRL, reg_val);

		reg_val = snd_soc_read(codec, 0x82);
		reg_val &= ~(0xf<<0);
		reg_val &= ~(0x7<<12);
		snd_soc_write(codec, 0x82, reg_val);

		reg_val = snd_soc_read(codec, 0x83);
		reg_val &= ~(0xf<<0);
		reg_val &= ~(0x7<<12);
		snd_soc_write(codec, 0x83, reg_val);
	}
}
void drc_enable(struct snd_soc_codec *codec,bool on)
{
	int reg_val;
	if (on) {
		snd_soc_write(codec, 0xb5, 0xA080);
		reg_val = snd_soc_read(codec, MOD_CLK_ENA);
		reg_val |= (0x1<<6);
		snd_soc_write(codec, MOD_CLK_ENA, reg_val);
		reg_val = snd_soc_read(codec, MOD_RST_CTRL);
		reg_val |= (0x1<<6);
		snd_soc_write(codec, MOD_RST_CTRL, reg_val);

		reg_val = snd_soc_read(codec, 0xa0);
		reg_val |= (0x7<<0);
		snd_soc_write(codec, 0xa0, reg_val);
	} else {
		snd_soc_write(codec, 0xb5, 0x0);
		reg_val = snd_soc_read(codec, MOD_CLK_ENA);
		reg_val &= ~(0x1<<6);
		snd_soc_write(codec, MOD_CLK_ENA, reg_val);
		reg_val = snd_soc_read(codec, MOD_RST_CTRL);
		reg_val &= ~(0x1<<6);
		snd_soc_write(codec, MOD_RST_CTRL, reg_val);

		reg_val = snd_soc_read(codec, 0xa0);
		reg_val &= ~(0x7<<0);
		snd_soc_write(codec, 0xa0, reg_val);
	}
}
void set_configuration(struct snd_soc_codec *codec)
{
	if (speaker_double_used) {
		snd_soc_update_bits(codec, SPKOUT_CTRL, (0x1f<<SPK_VOL), (double_speaker_val<<SPK_VOL));
	} else {
		snd_soc_update_bits(codec, SPKOUT_CTRL, (0x1f<<SPK_VOL), (single_speaker_val<<SPK_VOL));
	}
	snd_soc_update_bits(codec, HPOUT_CTRL, (0x3f<<HP_VOL), (headset_val<<HP_VOL));
	//snd_soc_update_bits(codec, ESPKOUT_CTRL, (0x1f<<ESP_VOL), (earpiece_val<<ESP_VOL));
	snd_soc_update_bits(codec, ADC_SRCBST_CTRL, (0x7<<ADC_MIC1G), (mainmic_val<<ADC_MIC1G));
	snd_soc_update_bits(codec, ADC_SRCBST_CTRL, (0x7<<ADC_MIC2G), (headsetmic_val<<ADC_MIC2G));
	if (dmic_used) {
		snd_soc_write(codec, ADC_VOL_CTRL, adc_digital_val);
	}
	if (agc_used) {
		agc_config(codec);
	}
	if (drc_used) {
		drc_config(codec);
	}
	/*headphone calibration clock frequency select*/
	snd_soc_update_bits(codec, SPKOUT_CTRL, (0x7<<HPCALICKS), (0x7<<HPCALICKS));

}
static int late_enable_dac(struct snd_soc_dapm_widget *w,
			  struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);
	struct ac10x_priv *ac10x = snd_soc_codec_get_drvdata(codec);
	mutex_lock(&ac10x->dac_mutex);
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		AC10X_DBG("%s,line:%d\n",__func__,__LINE__);
		if (ac10x->dac_enable == 0){
			/*enable dac module clk*/
			snd_soc_update_bits(codec, MOD_CLK_ENA, (0x1<<MOD_CLK_DAC_DIG), (0x1<<MOD_CLK_DAC_DIG));
			snd_soc_update_bits(codec, MOD_RST_CTRL, (0x1<<MOD_RESET_DAC_DIG), (0x1<<MOD_RESET_DAC_DIG));
			snd_soc_update_bits(codec, DAC_DIG_CTRL, (0x1<<ENDA), (0x1<<ENDA));
			snd_soc_update_bits(codec, DAC_DIG_CTRL, (0x1<<ENHPF),(0x1<<ENHPF));
		}
		ac10x->dac_enable++;
		break;
	case SND_SOC_DAPM_POST_PMD:
		if (ac10x->dac_enable > 0){
			ac10x->dac_enable--;
			if (ac10x->dac_enable == 0){
				snd_soc_update_bits(codec, DAC_DIG_CTRL, (0x1<<ENHPF),(0x0<<ENHPF));
				snd_soc_update_bits(codec, DAC_DIG_CTRL, (0x1<<ENDA), (0x0<<ENDA));
				/*disable dac module clk*/
				snd_soc_update_bits(codec, MOD_CLK_ENA, (0x1<<MOD_CLK_DAC_DIG), (0x0<<MOD_CLK_DAC_DIG));
				snd_soc_update_bits(codec, MOD_RST_CTRL, (0x1<<MOD_RESET_DAC_DIG), (0x0<<MOD_RESET_DAC_DIG));
			}
		}
		break;
	}
	mutex_unlock(&ac10x->dac_mutex);
	return 0;
}

static int late_enable_adc(struct snd_soc_dapm_widget *w,
			  struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);
	struct ac10x_priv *ac10x = snd_soc_codec_get_drvdata(codec);
	mutex_lock(&ac10x->adc_mutex);
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		if (ac10x->adc_enable == 0){
			/*enable adc module clk*/
			snd_soc_update_bits(codec, MOD_CLK_ENA, (0x1<<MOD_CLK_ADC_DIG), (0x1<<MOD_CLK_ADC_DIG));
			snd_soc_update_bits(codec, MOD_RST_CTRL, (0x1<<MOD_RESET_ADC_DIG), (0x1<<MOD_RESET_ADC_DIG));
			snd_soc_update_bits(codec, ADC_DIG_CTRL, (0x1<<ENAD), (0x1<<ENAD));
		}
		ac10x->adc_enable++;
		break;
	case SND_SOC_DAPM_POST_PMD:
		if (ac10x->adc_enable > 0){
			ac10x->adc_enable--;
			if (ac10x->adc_enable == 0){
				snd_soc_update_bits(codec, ADC_DIG_CTRL, (0x1<<ENAD), (0x0<<ENAD));
				/*disable adc module clk*/
				snd_soc_update_bits(codec, MOD_CLK_ENA, (0x1<<MOD_CLK_ADC_DIG), (0x0<<MOD_CLK_ADC_DIG));
				snd_soc_update_bits(codec, MOD_RST_CTRL, (0x1<<MOD_RESET_ADC_DIG), (0x0<<MOD_RESET_ADC_DIG));
			}
		}
		break;
	}
	mutex_unlock(&ac10x->adc_mutex);
	return 0;
}
static int ac10x_speaker_event(struct snd_soc_dapm_widget *w,
				struct snd_kcontrol *k,
				int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);
	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		AC10X_DBG("[speaker open ]%s,line:%d\n",__func__,__LINE__);
		if (drc_used) {
			drc_enable(codec,1);
		}
		#if PA_CTL
		msleep(30);
		gpio_set_value(PA_CTL, 1);
		#endif
		break;

	case SND_SOC_DAPM_PRE_PMD :
		AC10X_DBG("[speaker close ]%s,line:%d\n",__func__,__LINE__);
		#if PA_CTL
		gpio_set_value(PA_CTL, 0);
		#endif
		if (drc_used) {
			drc_enable(codec,0);
		}
	default:
		break;

	}
	return 0;
}
static int ac10x_headphone_event(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *k,	int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);
	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		/*open*/
		AC10X_DBG("post:open:%s,line:%d\n", __func__, __LINE__);
		snd_soc_update_bits(codec, OMIXER_DACA_CTRL, (0xf<<HPOUTPUTENABLE), (0xf<<HPOUTPUTENABLE));
		snd_soc_update_bits(codec, HPOUT_CTRL, (0x1<<HPPA_EN), (0x1<<HPPA_EN));
		msleep(10);
		snd_soc_update_bits(codec, HPOUT_CTRL, (0x3<<LHPPA_MUTE), (0x3<<LHPPA_MUTE));
		break;
	case SND_SOC_DAPM_PRE_PMD:
		/*close*/
		AC10X_DBG("pre:close:%s,line:%d\n", __func__, __LINE__);
		snd_soc_update_bits(codec, HPOUT_CTRL, (0x1<<HPPA_EN), (0x0<<HPPA_EN));
		snd_soc_update_bits(codec, OMIXER_DACA_CTRL, (0xf<<HPOUTPUTENABLE), (0x0<<HPOUTPUTENABLE));
		snd_soc_update_bits(codec, HPOUT_CTRL, (0x3<<LHPPA_MUTE), (0x0<<LHPPA_MUTE));
		break;
	}
	return 0;
}

int ac10x_aif1clk(struct snd_soc_dapm_widget *w,
		  struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);
	struct ac10x_priv *ac10x = snd_soc_codec_get_drvdata(codec);

	AC10X_DBG("%s() L%d event=%d pre_up/%d post_down/%d\n", __func__, __LINE__,
		event, SND_SOC_DAPM_PRE_PMU, SND_SOC_DAPM_POST_PMD);

	mutex_lock(&ac10x->aifclk_mutex);
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		if (ac10x->aif1_clken == 0){
			/*enable AIF1CLK*/
			snd_soc_update_bits(codec, SYSCLK_CTRL, (0x1<<AIF1CLK_ENA), (0x1<<AIF1CLK_ENA));
			snd_soc_update_bits(codec, MOD_CLK_ENA, (0x1<<MOD_CLK_AIF1), (0x1<<MOD_CLK_AIF1));
			snd_soc_update_bits(codec, MOD_RST_CTRL, (0x1<<MOD_RESET_AIF1), (0x1<<MOD_RESET_AIF1));
			/*enable systemclk*/
			if (ac10x->aif2_clken == 0 && ac10x->aif3_clken == 0)
				snd_soc_update_bits(codec, SYSCLK_CTRL, (0x1<<SYSCLK_ENA), (0x1<<SYSCLK_ENA));
		}
		ac10x->aif1_clken++;

		break;
	case SND_SOC_DAPM_POST_PMD:
		if (ac10x->aif1_clken > 0){
			ac10x->aif1_clken--;
			if (ac10x->aif1_clken == 0){
				/*disable AIF1CLK*/
				snd_soc_update_bits(codec, SYSCLK_CTRL, (0x1<<AIF1CLK_ENA), (0x0<<AIF1CLK_ENA));
				snd_soc_update_bits(codec, MOD_CLK_ENA, (0x1<<MOD_CLK_AIF1), (0x0<<MOD_CLK_AIF1));
				snd_soc_update_bits(codec, MOD_RST_CTRL, (0x1<<MOD_RESET_AIF1), (0x0<<MOD_RESET_AIF1));
				/*DISABLE systemclk*/
				if (ac10x->aif2_clken == 0 && ac10x->aif3_clken == 0)
					snd_soc_update_bits(codec, SYSCLK_CTRL, (0x1<<SYSCLK_ENA), (0x0<<SYSCLK_ENA));
			}
		}
		break;
	}
	mutex_unlock(&ac10x->aifclk_mutex);
	return 0;
}


static int dmic_mux_ev(struct snd_soc_dapm_widget *w,
		      struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);
	struct ac10x_priv *ac10x = snd_soc_codec_get_drvdata(codec);
	switch (event){
	case SND_SOC_DAPM_PRE_PMU:
		snd_soc_update_bits(codec, ADC_DIG_CTRL, (0x1<<ENDM), (0x1<<ENDM));
		break;
	case SND_SOC_DAPM_POST_PMD:
		snd_soc_update_bits(codec, ADC_DIG_CTRL, (0x1<<ENDM), (0x0<<ENDM));
		break;
	}
	mutex_lock(&ac10x->adc_mutex);
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		if (ac10x->adc_enable == 0){
			/*enable adc module clk*/
			snd_soc_update_bits(codec, MOD_CLK_ENA, (0x1<<MOD_CLK_ADC_DIG), (0x1<<MOD_CLK_ADC_DIG));
			snd_soc_update_bits(codec, MOD_RST_CTRL, (0x1<<MOD_RESET_ADC_DIG), (0x1<<MOD_RESET_ADC_DIG));
			snd_soc_update_bits(codec, ADC_DIG_CTRL, (0x1<<ENAD), (0x1<<ENAD));
		}
		ac10x->adc_enable++;
		break;
	case SND_SOC_DAPM_POST_PMD:
		if (ac10x->adc_enable > 0){
			ac10x->adc_enable--;
			if (ac10x->adc_enable == 0){
				snd_soc_update_bits(codec, ADC_DIG_CTRL, (0x1<<ENAD), (0x0<<ENAD));
				/*disable adc module clk*/
				snd_soc_update_bits(codec, MOD_CLK_ENA, (0x1<<MOD_CLK_ADC_DIG), (0x0<<MOD_CLK_ADC_DIG));
				snd_soc_update_bits(codec, MOD_RST_CTRL, (0x1<<MOD_RESET_ADC_DIG), (0x0<<MOD_RESET_ADC_DIG));
			}
		}
		break;
	}
	mutex_unlock(&ac10x->adc_mutex);
	return 0;
}
static const DECLARE_TLV_DB_SCALE(headphone_vol_tlv, -6300, 100, 0);
static const DECLARE_TLV_DB_SCALE(speaker_vol_tlv, -4800, 150, 0);

static const DECLARE_TLV_DB_SCALE(aif1_ad_slot0_vol_tlv, -11925, 75, 0);
static const DECLARE_TLV_DB_SCALE(aif1_ad_slot1_vol_tlv, -11925, 75, 0);
static const DECLARE_TLV_DB_SCALE(aif1_da_slot0_vol_tlv, -11925, 75, 0);
static const DECLARE_TLV_DB_SCALE(aif1_da_slot1_vol_tlv, -11925, 75, 0);
static const DECLARE_TLV_DB_SCALE(aif1_ad_slot0_mix_vol_tlv, -600, 600, 0);
static const DECLARE_TLV_DB_SCALE(aif1_ad_slot1_mix_vol_tlv, -600, 600, 0);
static const DECLARE_TLV_DB_SCALE(adc_vol_tlv, -11925, 75, 0);
static const DECLARE_TLV_DB_SCALE(dac_vol_tlv, -11925, 75, 0);

static const DECLARE_TLV_DB_SCALE(dig_vol_tlv, -7308, 116, 0);
static const DECLARE_TLV_DB_SCALE(dac_mix_vol_tlv, -600, 600, 0);
static const DECLARE_TLV_DB_SCALE(adc_input_vol_tlv, -450, 150, 0);

/*mic1/mic2: 0db when 000, and from 30db to 48db when 001 to 111*/
static const DECLARE_TLV_DB_SCALE(mic1_boost_vol_tlv, 0, 200, 0);
static const DECLARE_TLV_DB_SCALE(mic2_boost_vol_tlv, 0, 200, 0);

static const DECLARE_TLV_DB_SCALE(linein_amp_vol_tlv, -1200, 300, 0);

static const DECLARE_TLV_DB_SCALE(axin_to_l_r_mix_vol_tlv, -450, 150, 0);
static const DECLARE_TLV_DB_SCALE(mic1_to_l_r_mix_vol_tlv, -450, 150, 0);
static const DECLARE_TLV_DB_SCALE(mic2_to_l_r_mix_vol_tlv, -450, 150, 0);
static const DECLARE_TLV_DB_SCALE(linein_to_l_r_mix_vol_tlv, -450, 150, 0);

static const struct snd_kcontrol_new ac10x_controls[] = {
	/*AIF1*/
	SOC_DOUBLE_TLV("AIF1 ADC timeslot 0 volume", AIF1_VOL_CTRL1, AIF1_AD0L_VOL, AIF1_AD0R_VOL, 0xff, 0, aif1_ad_slot0_vol_tlv),
	SOC_DOUBLE_TLV("AIF1 ADC timeslot 1 volume", AIF1_VOL_CTRL2, AIF1_AD1L_VOL, AIF1_AD1R_VOL, 0xff, 0, aif1_ad_slot1_vol_tlv),
	SOC_DOUBLE_TLV("AIF1 DAC timeslot 0 volume", AIF1_VOL_CTRL3, AIF1_DA0L_VOL, AIF1_DA0R_VOL, 0xff, 0, aif1_da_slot0_vol_tlv),
	SOC_DOUBLE_TLV("AIF1 DAC timeslot 1 volume", AIF1_VOL_CTRL4, AIF1_DA1L_VOL, AIF1_DA1R_VOL, 0xff, 0, aif1_da_slot1_vol_tlv),
	SOC_DOUBLE_TLV("AIF1 ADC timeslot 0 mixer gain", AIF1_MXR_GAIN, AIF1_AD0L_MXR_GAIN, AIF1_AD0R_MXR_GAIN, 0xf, 0, aif1_ad_slot0_mix_vol_tlv),
	SOC_DOUBLE_TLV("AIF1 ADC timeslot 1 mixer gain", AIF1_MXR_GAIN, AIF1_AD1L_MXR_GAIN, AIF1_AD1R_MXR_GAIN, 0x3, 0, aif1_ad_slot1_mix_vol_tlv),

	/*ADC*/
	SOC_DOUBLE_TLV("ADC volume", ADC_VOL_CTRL, ADC_VOL_L, ADC_VOL_R, 0xff, 0, adc_vol_tlv),
	/*DAC*/
	SOC_DOUBLE_TLV("DAC volume", DAC_VOL_CTRL, DAC_VOL_L, DAC_VOL_R, 0xff, 0, dac_vol_tlv),
	SOC_DOUBLE_TLV("DAC mixer gain", DAC_MXR_GAIN, DACL_MXR_GAIN, DACR_MXR_GAIN, 0xf, 0, dac_mix_vol_tlv),

	SOC_SINGLE_TLV("digital volume", DAC_DBG_CTRL, DVC, 0x3f, 0, dig_vol_tlv),

	/*ADC*/
	SOC_DOUBLE_TLV("ADC input gain", ADC_APC_CTRL, ADCLG, ADCRG, 0x7, 0, adc_input_vol_tlv),

	SOC_SINGLE_TLV("MIC1 boost amplifier gain", ADC_SRCBST_CTRL, ADC_MIC1G, 0x7, 0, mic1_boost_vol_tlv),
	SOC_SINGLE_TLV("MIC2 boost amplifier gain", ADC_SRCBST_CTRL, ADC_MIC2G, 0x7, 0, mic2_boost_vol_tlv),
	SOC_SINGLE_TLV("LINEINL-LINEINR pre-amplifier gain", ADC_SRCBST_CTRL, LINEIN_PREG, 0x7, 0, linein_amp_vol_tlv),

	SOC_SINGLE_TLV("MIC1 BST stage to L_R outp mixer gain", OMIXER_BST1_CTRL, OMIXER_MIC1G, 0x7, 0, mic1_to_l_r_mix_vol_tlv),
	SOC_SINGLE_TLV("MIC2 BST stage to L_R outp mixer gain", OMIXER_BST1_CTRL, OMIXER_MIC2G, 0x7, 0, mic2_to_l_r_mix_vol_tlv),
	SOC_SINGLE_TLV("LINEINL/R to L_R output mixer gain", OMIXER_BST1_CTRL, LINEING, 0x7, 0, linein_to_l_r_mix_vol_tlv),

	SOC_SINGLE_TLV("speaker volume", SPKOUT_CTRL, SPK_VOL, 0x1f, 0, speaker_vol_tlv),
	SOC_SINGLE_TLV("headphone volume", HPOUT_CTRL, HP_VOL, 0x3f, 0, headphone_vol_tlv),
};
/*AIF1 AD0 OUT */
static const char *aif1out0l_text[] = {
	"AIF1_AD0L", "AIF1_AD0R","SUM_AIF1AD0L_AIF1AD0R", "AVE_AIF1AD0L_AIF1AD0R"
};
static const char *aif1out0r_text[] = {
	"AIF1_AD0R", "AIF1_AD0L","SUM_AIF1AD0L_AIF1AD0R", "AVE_AIF1AD0L_AIF1AD0R"
};

static const struct soc_enum aif1out0l_enum =
	SOC_ENUM_SINGLE(AIF1_ADCDAT_CTRL, 10, 4, aif1out0l_text);

static const struct snd_kcontrol_new aif1out0l_mux =
	SOC_DAPM_ENUM("AIF1OUT0L Mux", aif1out0l_enum);

static const struct soc_enum aif1out0r_enum =
	SOC_ENUM_SINGLE(AIF1_ADCDAT_CTRL, 8, 4, aif1out0r_text);

static const struct snd_kcontrol_new aif1out0r_mux =
	SOC_DAPM_ENUM("AIF1OUT0R Mux", aif1out0r_enum);


/*AIF1 AD1 OUT */
static const char *aif1out1l_text[] = {
	"AIF1_AD1L", "AIF1_AD1R","SUM_AIF1ADC1L_AIF1ADC1R", "AVE_AIF1ADC1L_AIF1ADC1R"
};
static const char *aif1out1r_text[] = {
	"AIF1_AD1R", "AIF1_AD1L","SUM_AIF1ADC1L_AIF1ADC1R", "AVE_AIF1ADC1L_AIF1ADC1R"
};

static const struct soc_enum aif1out1l_enum =
	SOC_ENUM_SINGLE(AIF1_ADCDAT_CTRL, 6, 4, aif1out1l_text);

static const struct snd_kcontrol_new aif1out1l_mux =
	SOC_DAPM_ENUM("AIF1OUT1L Mux", aif1out1l_enum);

static const struct soc_enum aif1out1r_enum =
	SOC_ENUM_SINGLE(AIF1_ADCDAT_CTRL, 4, 4, aif1out1r_text);

static const struct snd_kcontrol_new aif1out1r_mux =
	SOC_DAPM_ENUM("AIF1OUT1R Mux", aif1out1r_enum);


/*AIF1 DA0 IN*/
static const char *aif1in0l_text[] = {
	"AIF1_DA0L", "AIF1_DA0R", "SUM_AIF1DA0L_AIF1DA0R", "AVE_AIF1DA0L_AIF1DA0R"
};
static const char *aif1in0r_text[] = {
	"AIF1_DA0R", "AIF1_DA0L", "SUM_AIF1DA0L_AIF1DA0R", "AVE_AIF1DA0L_AIF1DA0R"
};

static const struct soc_enum aif1in0l_enum =
	SOC_ENUM_SINGLE(AIF1_DACDAT_CTRL, 10, 4, aif1in0l_text);

static const struct snd_kcontrol_new aif1in0l_mux =
	SOC_DAPM_ENUM("AIF1IN0L Mux", aif1in0l_enum);

static const struct soc_enum aif1in0r_enum =
	SOC_ENUM_SINGLE(AIF1_DACDAT_CTRL, 8, 4, aif1in0r_text);

static const struct snd_kcontrol_new aif1in0r_mux =
	SOC_DAPM_ENUM("AIF1IN0R Mux", aif1in0r_enum);


/*AIF1 DA1 IN*/
static const char *aif1in1l_text[] = {
	"AIF1_DA1L", "AIF1_DA1R","SUM_AIF1DA1L_AIF1DA1R", "AVE_AIF1DA1L_AIF1DA1R"
};
static const char *aif1in1r_text[] = {
	"AIF1_DA1R", "AIF1_DA1L","SUM_AIF1DA1L_AIF1DA1R", "AVE_AIF1DA1L_AIF1DA1R"
};

static const struct soc_enum aif1in1l_enum =
	SOC_ENUM_SINGLE(AIF1_DACDAT_CTRL, 6, 4, aif1in1l_text);

static const struct snd_kcontrol_new aif1in1l_mux =
	SOC_DAPM_ENUM("AIF1IN1L Mux", aif1in1l_enum);

static const struct soc_enum aif1in1r_enum =
	SOC_ENUM_SINGLE(AIF1_DACDAT_CTRL, 4, 4, aif1in1r_text);

static const struct snd_kcontrol_new aif1in1r_mux =
	SOC_DAPM_ENUM("AIF1IN1R Mux", aif1in1r_enum);


/*0x13register*/
/*AIF1 ADC0 MIXER SOURCE*/
static const struct snd_kcontrol_new aif1_ad0l_mxr_src_ctl[] = {
	SOC_DAPM_SINGLE("AIF1 DA0L Switch", 	AIF1_MXR_SRC,  	AIF1_AD0L_AIF1_DA0L_MXR, 1, 0),
	SOC_DAPM_SINGLE("ADCL Switch", 		AIF1_MXR_SRC, 	AIF1_AD0L_ADCL_MXR, 1, 0),
};
static const struct snd_kcontrol_new aif1_ad0r_mxr_src_ctl[] = {
	SOC_DAPM_SINGLE("AIF1 DA0R Switch", 	AIF1_MXR_SRC,  	AIF1_AD0R_AIF1_DA0R_MXR, 1, 0),
	SOC_DAPM_SINGLE("ADCR Switch", 		AIF1_MXR_SRC, 	AIF1_AD0R_ADCR_MXR, 1, 0),
};


/*AIF1 ADC1 MIXER SOURCE*/
static const struct snd_kcontrol_new aif1_ad1l_mxr_src_ctl[] = {
	SOC_DAPM_SINGLE("ADCL Switch", 	AIF1_MXR_SRC, 	AIF1_AD1L_ADCL_MXR, 1, 0),
};
static const struct snd_kcontrol_new aif1_ad1r_mxr_src_ctl[] = {
	SOC_DAPM_SINGLE("ADCR Switch", 	AIF1_MXR_SRC, 	AIF1_AD1R_ADCR_MXR, 1, 0),
};


/*4C register*/
static const struct snd_kcontrol_new dacl_mxr_src_controls[] = {
	SOC_DAPM_SINGLE("ADCL Switch", 			DAC_MXR_SRC,  	DACL_MXR_ADCL, 1, 0),
	SOC_DAPM_SINGLE("AIF1DA1L Switch", 		DAC_MXR_SRC, 	DACL_MXR_AIF1_DA1L, 1, 0),
	SOC_DAPM_SINGLE("AIF1DA0L Switch", 		DAC_MXR_SRC, 	DACL_MXR_AIF1_DA0L, 1, 0),
};
static const struct snd_kcontrol_new dacr_mxr_src_controls[] = {
	SOC_DAPM_SINGLE("ADCR Switch", 			DAC_MXR_SRC,  	DACR_MXR_ADCR, 1, 0),
	SOC_DAPM_SINGLE("AIF1DA1R Switch", 		DAC_MXR_SRC, 	DACR_MXR_AIF1_DA1R, 1, 0),
	SOC_DAPM_SINGLE("AIF1DA0R Switch", 		DAC_MXR_SRC, 	DACR_MXR_AIF1_DA0R, 1, 0),
};


/*output mixer source select*/

/*defined left output mixer*/
static const struct snd_kcontrol_new ac10x_loutmix_controls[] = {
	SOC_DAPM_SINGLE("DACR Switch", OMIXER_SR, LMIXMUTEDACR, 1, 0),
	SOC_DAPM_SINGLE("DACL Switch", OMIXER_SR, LMIXMUTEDACL, 1, 0),
	SOC_DAPM_SINGLE("LINEINL Switch", OMIXER_SR, LMIXMUTELINEINL, 1, 0),
	SOC_DAPM_SINGLE("LINEINL-LINEINR Switch", OMIXER_SR, LMIXMUTELINEINLR, 1, 0),
	SOC_DAPM_SINGLE("MIC2Booststage Switch", OMIXER_SR, LMIXMUTEMIC2BOOST, 1, 0),
	SOC_DAPM_SINGLE("MIC1Booststage Switch", OMIXER_SR, LMIXMUTEMIC1BOOST, 1, 0),
};

/*defined right output mixer*/
static const struct snd_kcontrol_new ac10x_routmix_controls[] = {
	SOC_DAPM_SINGLE("DACL Switch", OMIXER_SR, RMIXMUTEDACL, 1, 0),
	SOC_DAPM_SINGLE("DACR Switch", OMIXER_SR, RMIXMUTEDACR, 1, 0),
	SOC_DAPM_SINGLE("LINEINR Switch", OMIXER_SR, RMIXMUTELINEINR, 1, 0),
	SOC_DAPM_SINGLE("LINEINL-LINEINR Switch", OMIXER_SR, RMIXMUTELINEINLR, 1, 0),
	SOC_DAPM_SINGLE("MIC2Booststage Switch", OMIXER_SR, RMIXMUTEMIC2BOOST, 1, 0),
	SOC_DAPM_SINGLE("MIC1Booststage Switch", OMIXER_SR, RMIXMUTEMIC1BOOST, 1, 0),
};


/*hp source select*/

/*headphone input source*/
static const char *ac10x_hp_r_func_sel[] = {
	"DACR HPR Switch", "Right Analog Mixer HPR Switch"};
static const struct soc_enum ac10x_hp_r_func_enum =
	SOC_ENUM_SINGLE(HPOUT_CTRL, RHPS, 2, ac10x_hp_r_func_sel);

static const struct snd_kcontrol_new ac10x_hp_r_func_controls =
	SOC_DAPM_ENUM("HP_R Mux", ac10x_hp_r_func_enum);

static const char *ac10x_hp_l_func_sel[] = {
	"DACL HPL Switch", "Left Analog Mixer HPL Switch"};
static const struct soc_enum ac10x_hp_l_func_enum =
	SOC_ENUM_SINGLE(HPOUT_CTRL, LHPS, 2, ac10x_hp_l_func_sel);

static const struct snd_kcontrol_new ac10x_hp_l_func_controls =
	SOC_DAPM_ENUM("HP_L Mux", ac10x_hp_l_func_enum);


/*spk source select*/
static const char *ac10x_rspks_func_sel[] = {
	"MIXER Switch", "MIXR MIXL Switch"};
static const struct soc_enum ac10x_rspks_func_enum =
	SOC_ENUM_SINGLE(SPKOUT_CTRL, RSPKS, 2, ac10x_rspks_func_sel);

static const struct snd_kcontrol_new ac10x_rspks_func_controls =
	SOC_DAPM_ENUM("SPK_R Mux", ac10x_rspks_func_enum);

static const char *ac10x_lspks_l_func_sel[] = {
	"MIXEL Switch", "MIXL MIXR  Switch"};
static const struct soc_enum ac10x_lspks_func_enum =
	SOC_ENUM_SINGLE(SPKOUT_CTRL, LSPKS, 2, ac10x_lspks_l_func_sel);

static const struct snd_kcontrol_new ac10x_lspks_func_controls =
	SOC_DAPM_ENUM("SPK_L Mux", ac10x_lspks_func_enum);

/*defined left input adc mixer*/
static const struct snd_kcontrol_new ac10x_ladcmix_controls[] = {
	SOC_DAPM_SINGLE("MIC1 boost Switch", ADC_SRC, LADCMIXMUTEMIC1BOOST, 1, 0),
	SOC_DAPM_SINGLE("MIC2 boost Switch", ADC_SRC, LADCMIXMUTEMIC2BOOST, 1, 0),
	SOC_DAPM_SINGLE("LININL-R Switch", ADC_SRC, LADCMIXMUTELINEINLR, 1, 0),
	SOC_DAPM_SINGLE("LINEINL Switch", ADC_SRC, LADCMIXMUTELINEINL, 1, 0),
	SOC_DAPM_SINGLE("Lout_Mixer_Switch", ADC_SRC, LADCMIXMUTELOUTPUT, 1, 0),
	SOC_DAPM_SINGLE("Rout_Mixer_Switch", ADC_SRC, LADCMIXMUTEROUTPUT, 1, 0),
};

/*defined right input adc mixer*/
static const struct snd_kcontrol_new ac10x_radcmix_controls[] = {
	SOC_DAPM_SINGLE("MIC1 boost Switch", ADC_SRC, RADCMIXMUTEMIC1BOOST, 1, 0),
	SOC_DAPM_SINGLE("MIC2 boost Switch", ADC_SRC, RADCMIXMUTEMIC2BOOST, 1, 0),
	SOC_DAPM_SINGLE("LINEINL-R Switch", ADC_SRC, RADCMIXMUTELINEINLR, 1, 0),
	SOC_DAPM_SINGLE("LINEINR Switch", ADC_SRC, RADCMIXMUTELINEINR, 1, 0),
	SOC_DAPM_SINGLE("Rout_Mixer_Switch", ADC_SRC, RADCMIXMUTEROUTPUT, 1, 0),
	SOC_DAPM_SINGLE("Lout_Mixer_Switch", ADC_SRC, RADCMIXMUTELOUTPUT, 1, 0),
};

/*mic2 source select*/
static const char *mic2src_text[] = {
	"none","MIC2"};

static const struct soc_enum mic2src_enum =
	SOC_ENUM_SINGLE(ADC_SRCBST_CTRL, 7, 2, mic2src_text);

static const struct snd_kcontrol_new mic2src_mux =
	SOC_DAPM_ENUM("MIC2 SRC", mic2src_enum);
/*DMIC*/
static const char *adc_mux_text[] = {
	"ADC",
	"DMIC",
};
static SOC_ENUM_SINGLE_VIRT_DECL(adc_enum, adc_mux_text);
static const struct snd_kcontrol_new adcl_mux =
	SOC_DAPM_ENUM("ADCL Mux", adc_enum);
static const struct snd_kcontrol_new adcr_mux =
	SOC_DAPM_ENUM("ADCR Mux", adc_enum);

/*built widget*/
static const struct snd_soc_dapm_widget ac10x_dapm_widgets[] = {

	SND_SOC_DAPM_MUX("AIF1OUT0L Mux", AIF1_ADCDAT_CTRL, 15, 0, &aif1out0l_mux),
	SND_SOC_DAPM_MUX("AIF1OUT0R Mux", AIF1_ADCDAT_CTRL, 14, 0, &aif1out0r_mux),

	SND_SOC_DAPM_MUX("AIF1OUT1L Mux", AIF1_ADCDAT_CTRL, 13, 0, &aif1out1l_mux),
	SND_SOC_DAPM_MUX("AIF1OUT1R Mux", AIF1_ADCDAT_CTRL, 12, 0, &aif1out1r_mux),

	SND_SOC_DAPM_MUX("AIF1IN0L Mux", AIF1_DACDAT_CTRL, 15, 0, &aif1in0l_mux),
	SND_SOC_DAPM_MUX("AIF1IN0R Mux", AIF1_DACDAT_CTRL, 14, 0, &aif1in0r_mux),

	SND_SOC_DAPM_MUX("AIF1IN1L Mux", AIF1_DACDAT_CTRL, 13, 0, &aif1in1l_mux),
	SND_SOC_DAPM_MUX("AIF1IN1R Mux", AIF1_DACDAT_CTRL, 12, 0, &aif1in1r_mux),

	SND_SOC_DAPM_MIXER("AIF1 AD0L Mixer", SND_SOC_NOPM, 0, 0, aif1_ad0l_mxr_src_ctl, ARRAY_SIZE(aif1_ad0l_mxr_src_ctl)),
	SND_SOC_DAPM_MIXER("AIF1 AD0R Mixer", SND_SOC_NOPM, 0, 0, aif1_ad0r_mxr_src_ctl, ARRAY_SIZE(aif1_ad0r_mxr_src_ctl)),

	SND_SOC_DAPM_MIXER("AIF1 AD1L Mixer", SND_SOC_NOPM, 0, 0, aif1_ad1l_mxr_src_ctl, ARRAY_SIZE(aif1_ad1l_mxr_src_ctl)),
	SND_SOC_DAPM_MIXER("AIF1 AD1R Mixer", SND_SOC_NOPM, 0, 0, aif1_ad1r_mxr_src_ctl, ARRAY_SIZE(aif1_ad1r_mxr_src_ctl)),

	SND_SOC_DAPM_MIXER_E("DACL Mixer", OMIXER_DACA_CTRL, DACALEN, 0, dacl_mxr_src_controls, ARRAY_SIZE(dacl_mxr_src_controls),
		     	late_enable_dac, SND_SOC_DAPM_PRE_PMU|SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_MIXER_E("DACR Mixer", OMIXER_DACA_CTRL, DACAREN, 0, dacr_mxr_src_controls, ARRAY_SIZE(dacr_mxr_src_controls),
		     	late_enable_dac, SND_SOC_DAPM_PRE_PMU|SND_SOC_DAPM_POST_PMD),

	/*dac digital enble*/
	SND_SOC_DAPM_DAC("DAC En", NULL, DAC_DIG_CTRL, ENDA, 0),

	/*ADC digital enble*/
	SND_SOC_DAPM_ADC("ADC En", NULL, ADC_DIG_CTRL, ENAD, 0),

	SND_SOC_DAPM_MIXER("Left Output Mixer", OMIXER_DACA_CTRL, LMIXEN, 0,
			ac10x_loutmix_controls, ARRAY_SIZE(ac10x_loutmix_controls)),
	SND_SOC_DAPM_MIXER("Right Output Mixer", OMIXER_DACA_CTRL, RMIXEN, 0,
			ac10x_routmix_controls, ARRAY_SIZE(ac10x_routmix_controls)),

	SND_SOC_DAPM_MUX("HP_R Mux", SND_SOC_NOPM, 0, 0,	&ac10x_hp_r_func_controls),
	SND_SOC_DAPM_MUX("HP_L Mux", SND_SOC_NOPM, 0, 0,	&ac10x_hp_l_func_controls),

	SND_SOC_DAPM_MUX("SPK_R Mux", SPKOUT_CTRL, RSPK_EN, 0,	&ac10x_rspks_func_controls),
	SND_SOC_DAPM_MUX("SPK_L Mux", SPKOUT_CTRL, LSPK_EN, 0,	&ac10x_lspks_func_controls),

	SND_SOC_DAPM_PGA("SPK_LR Adder", SND_SOC_NOPM, 0, 0, NULL, 0),

	/*output widget*/
	SND_SOC_DAPM_OUTPUT("HPOUTL"),
	SND_SOC_DAPM_OUTPUT("HPOUTR"),
	SND_SOC_DAPM_OUTPUT("SPK1P"),
	SND_SOC_DAPM_OUTPUT("SPK2P"),
	SND_SOC_DAPM_OUTPUT("SPK1N"),
	SND_SOC_DAPM_OUTPUT("SPK2N"),

	SND_SOC_DAPM_MIXER_E("LEFT ADC input Mixer", ADC_APC_CTRL, ADCLEN, 0,
		ac10x_ladcmix_controls, ARRAY_SIZE(ac10x_ladcmix_controls),late_enable_adc, SND_SOC_DAPM_PRE_PMU|SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_MIXER_E("RIGHT ADC input Mixer", ADC_APC_CTRL, ADCREN, 0,
		ac10x_radcmix_controls, ARRAY_SIZE(ac10x_radcmix_controls),late_enable_adc, SND_SOC_DAPM_PRE_PMU|SND_SOC_DAPM_POST_PMD),

	/*mic reference*/
	SND_SOC_DAPM_PGA("MIC1 PGA", ADC_SRCBST_CTRL, MIC1AMPEN, 0, NULL, 0),
	SND_SOC_DAPM_PGA("MIC2 PGA", ADC_SRCBST_CTRL, MIC2AMPEN, 0, NULL, 0),

	SND_SOC_DAPM_PGA("LINEIN PGA", SND_SOC_NOPM, 0, 0, NULL, 0),

	SND_SOC_DAPM_MUX("MIC2 SRC", SND_SOC_NOPM, 0, 0, &mic2src_mux),

	/*INPUT widget*/
	SND_SOC_DAPM_INPUT("MIC1P"),
	SND_SOC_DAPM_INPUT("MIC1N"),

	SND_SOC_DAPM_MICBIAS("MainMic Bias", ADC_APC_CTRL, MBIASEN, 0),
	SND_SOC_DAPM_MICBIAS("HMic Bias", SND_SOC_NOPM, 0, 0),
	//SND_SOC_DAPM_MICBIAS("HMic Bias", ADC_APC_CTRL, HBIASEN, 0),
	SND_SOC_DAPM_INPUT("MIC2"),

	SND_SOC_DAPM_INPUT("LINEINP"),
	SND_SOC_DAPM_INPUT("LINEINN"),

	SND_SOC_DAPM_INPUT("D_MIC"),
	/*aif1 interface*/
	SND_SOC_DAPM_AIF_IN_E("AIF1DACL", "AIF1 Playback", 0, SND_SOC_NOPM, 0, 0,ac10x_aif1clk,
		   SND_SOC_DAPM_PRE_PMU|SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_AIF_IN_E("AIF1DACR", "AIF1 Playback", 0, SND_SOC_NOPM, 0, 0,ac10x_aif1clk,
		   SND_SOC_DAPM_PRE_PMU|SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_AIF_OUT_E("AIF1ADCL", "AIF1 Capture", 0, SND_SOC_NOPM, 0, 0,ac10x_aif1clk,
		   SND_SOC_DAPM_PRE_PMU|SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_AIF_OUT_E("AIF1ADCR", "AIF1 Capture", 0, SND_SOC_NOPM, 0, 0,ac10x_aif1clk,
		   SND_SOC_DAPM_PRE_PMU|SND_SOC_DAPM_POST_PMD),

	/*headphone*/
	SND_SOC_DAPM_HP("Headphone", ac10x_headphone_event),
	/*speaker*/
	SND_SOC_DAPM_SPK("External Speaker", ac10x_speaker_event),

	/*DMIC*/
	SND_SOC_DAPM_MUX("ADCL Mux", SND_SOC_NOPM, 0, 0, &adcl_mux),
	SND_SOC_DAPM_MUX("ADCR Mux", SND_SOC_NOPM, 0, 0, &adcr_mux),

	SND_SOC_DAPM_PGA_E("DMICL VIR", SND_SOC_NOPM, 0, 0, NULL, 0,
				dmic_mux_ev, SND_SOC_DAPM_PRE_PMU|SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_PGA_E("DMICR VIR", SND_SOC_NOPM, 0, 0, NULL, 0,
				dmic_mux_ev, SND_SOC_DAPM_PRE_PMU|SND_SOC_DAPM_POST_PMD),
};

static const struct snd_soc_dapm_route ac10x_dapm_routes[] = {

	{"AIF1ADCL", NULL, "AIF1OUT0L Mux"},
	{"AIF1ADCR", NULL, "AIF1OUT0R Mux"},

	{"AIF1ADCL", NULL, "AIF1OUT1L Mux"},
	{"AIF1ADCR", NULL, "AIF1OUT1R Mux"},

	/* aif1out0 mux 11---13*/
	{"AIF1OUT0L Mux", "AIF1_AD0L", "AIF1 AD0L Mixer"},
	{"AIF1OUT0L Mux", "AIF1_AD0R", "AIF1 AD0R Mixer"},

	{"AIF1OUT0R Mux", "AIF1_AD0R", "AIF1 AD0R Mixer"},
	{"AIF1OUT0R Mux", "AIF1_AD0L", "AIF1 AD0L Mixer"},

	/*AIF1OUT1 mux 11--13 */
	{"AIF1OUT1L Mux", "AIF1_AD1L", "AIF1 AD1L Mixer"},
	{"AIF1OUT1L Mux", "AIF1_AD1R", "AIF1 AD1R Mixer"},

	{"AIF1OUT1R Mux", "AIF1_AD1R", "AIF1 AD1R Mixer"},
	{"AIF1OUT1R Mux", "AIF1_AD1L", "AIF1 AD1L Mixer"},

	/*AIF1 AD0L Mixer*/
	{"AIF1 AD0L Mixer", "AIF1 DA0L Switch",		"AIF1IN0L Mux"},
	{"AIF1 AD0L Mixer", "ADCL Switch",		"ADCL Mux"},
	/*AIF1 AD0R Mixer*/
	{"AIF1 AD0R Mixer", "AIF1 DA0R Switch",		"AIF1IN0R Mux"},
	{"AIF1 AD0R Mixer", "ADCR Switch",		"ADCR Mux"},

	/*AIF1 AD1L Mixer*/
	{"AIF1 AD1L Mixer", "ADCL Switch",		"ADCL Mux"},
	/*AIF1 AD1R Mixer*/
	{"AIF1 AD1R Mixer", "ADCR Switch",		"ADCR Mux"},
	/*AIF1 DA0 IN 12h*/
	{"AIF1IN0L Mux", "AIF1_DA0L",		"AIF1DACL"},
	{"AIF1IN0L Mux", "AIF1_DA0R",		"AIF1DACR"},

	{"AIF1IN0R Mux", "AIF1_DA0R",		"AIF1DACR"},
	{"AIF1IN0R Mux", "AIF1_DA0L",		"AIF1DACL"},

	/*AIF1 DA1 IN 12h*/
	{"AIF1IN1L Mux", "AIF1_DA1L",		"AIF1DACL"},
	{"AIF1IN1L Mux", "AIF1_DA1R",		"AIF1DACR"},

	{"AIF1IN1R Mux", "AIF1_DA1R",		"AIF1DACR"},
	{"AIF1IN1R Mux", "AIF1_DA1L",		"AIF1DACL"},

	/*4c*/
	{"DACL Mixer", "AIF1DA0L Switch",		"AIF1IN0L Mux"},
	{"DACL Mixer", "AIF1DA1L Switch",		"AIF1IN1L Mux"},

	{"DACL Mixer", "ADCL Switch",		"ADCL Mux"},
	{"DACR Mixer", "AIF1DA0R Switch",		"AIF1IN0R Mux"},
	{"DACR Mixer", "AIF1DA1R Switch",		"AIF1IN1R Mux"},

	{"DACR Mixer", "ADCR Switch",		"ADCR Mux"},

	{"Right Output Mixer", "DACR Switch",		"DACR Mixer"},
	{"Right Output Mixer", "DACL Switch",		"DACL Mixer"},

	{"Right Output Mixer", "LINEINR Switch",		"LINEINN"},
	{"Right Output Mixer", "LINEINL-LINEINR Switch",		"LINEIN PGA"},
	{"Right Output Mixer", "MIC2Booststage Switch",		"MIC2 PGA"},
	{"Right Output Mixer", "MIC1Booststage Switch",		"MIC1 PGA"},


	{"Left Output Mixer", "DACL Switch",		"DACL Mixer"},
	{"Left Output Mixer", "DACR Switch",		"DACR Mixer"},

	{"Left Output Mixer", "LINEINL Switch",		"LINEINP"},
	{"Left Output Mixer", "LINEINL-LINEINR Switch",		"LINEIN PGA"},
	{"Left Output Mixer", "MIC2Booststage Switch",		"MIC2 PGA"},
	{"Left Output Mixer", "MIC1Booststage Switch",		"MIC1 PGA"},

	/*hp mux*/
	{"HP_R Mux", "DACR HPR Switch",		"DACR Mixer"},
	{"HP_R Mux", "Right Analog Mixer HPR Switch",		"Right Output Mixer"},


	{"HP_L Mux", "DACL HPL Switch",		"DACL Mixer"},
	{"HP_L Mux", "Left Analog Mixer HPL Switch",		"Left Output Mixer"},

	/*hp endpoint*/
	{"HPOUTR", NULL,				"HP_R Mux"},
	{"HPOUTL", NULL,				"HP_L Mux"},

	{"Headphone", NULL,				"HPOUTR"},
	{"Headphone", NULL,				"HPOUTL"},

	/*External Speaker*/
	{"External Speaker", NULL, "SPK1P"},
	{"External Speaker", NULL, "SPK1N"},

	{"External Speaker", NULL, "SPK2P"},
	{"External Speaker", NULL, "SPK2N"},

	/*spk mux*/
	{"SPK_LR Adder", NULL,				"Right Output Mixer"},
	{"SPK_LR Adder", NULL,				"Left Output Mixer"},

	{"SPK_L Mux", "MIXL MIXR  Switch",			"SPK_LR Adder"},
	{"SPK_L Mux", "MIXEL Switch",				"Left Output Mixer"},

	{"SPK_R Mux", "MIXR MIXL Switch",			"SPK_LR Adder"},
	{"SPK_R Mux", "MIXER Switch",				"Right Output Mixer"},

	{"SPK1P", NULL,				"SPK_R Mux"},
	{"SPK1N", NULL,				"SPK_R Mux"},

	{"SPK2P", NULL,				"SPK_L Mux"},
	{"SPK2N", NULL,				"SPK_L Mux"},

	/*LADC SOURCE mixer*/
	{"LEFT ADC input Mixer", "MIC1 boost Switch",				"MIC1 PGA"},
	{"LEFT ADC input Mixer", "MIC2 boost Switch",				"MIC2 PGA"},
	{"LEFT ADC input Mixer", "LININL-R Switch",				"LINEIN PGA"},
	{"LEFT ADC input Mixer", "LINEINL Switch",				"LINEINN"},
	{"LEFT ADC input Mixer", "Lout_Mixer_Switch",				"Left Output Mixer"},
	{"LEFT ADC input Mixer", "Rout_Mixer_Switch",				"Right Output Mixer"},

	/*RADC SOURCE mixer*/
	{"RIGHT ADC input Mixer", "MIC1 boost Switch",				"MIC1 PGA"},
	{"RIGHT ADC input Mixer", "MIC2 boost Switch",				"MIC2 PGA"},
	{"RIGHT ADC input Mixer", "LINEINL-R Switch",				"LINEIN PGA"},
	{"RIGHT ADC input Mixer", "LINEINR Switch",				"LINEINP"},
	{"RIGHT ADC input Mixer", "Rout_Mixer_Switch",				"Right Output Mixer"},
	{"RIGHT ADC input Mixer", "Lout_Mixer_Switch",				"Left Output Mixer"},

	{"MIC1 PGA", NULL,				"MIC1P"},
	{"MIC1 PGA", NULL,				"MIC1N"},

	{"MIC2 PGA", NULL,				"MIC2 SRC"},

	{"MIC2 SRC", "MIC2",				"MIC2"},

	{"LINEIN PGA", NULL,				"LINEINP"},
	{"LINEIN PGA", NULL,				"LINEINN"},

	/*ADC--ADCMUX*/
	{"ADCR Mux", "ADC", "RIGHT ADC input Mixer"},
	{"ADCL Mux", "ADC", "LEFT ADC input Mixer"},

	/*DMIC*/
	{"ADCR Mux", "DMIC", "DMICR VIR"},
	{"ADCL Mux", "DMIC", "DMICL VIR"},

	{"DMICL VIR", NULL, "D_MIC"},
	{"DMICR VIR", NULL, "D_MIC"},
};

/* PLL divisors */
struct pll_div {
	unsigned int pll_in;
	unsigned int pll_out;
	int m;
	int n_i;
	int n_f;
};

struct aif1_fs {
	unsigned samp_rate;
	int bclk_div;
	int srbit;
	#define _SERIES_24_576K		0
	#define _SERIES_22_579K		1
	int series;
};

struct aif1_lrck {
	int div;
	int bit;
};

struct aif1_word_size {
	int val;
	int bit;
};

/*
*	Note : pll code from original tdm/i2s driver.
* 	freq_out = freq_in * N/(m*(2k+1)) , k=1,N=N_i+N_f,N_f=factor*0.2;
*/
static const struct pll_div codec_pll_div[] = {
	{128000, 22579200, 1, 529, 1},
	{192000, 22579200, 1, 352, 4},
	{256000, 22579200, 1, 264, 3},
	{384000, 22579200, 1, 176, 2},/*((176+2*0.2)*6000000)/(38*(2*1+1))*/
	{6000000, 22579200, 38, 429, 0},/*((429+0*0.2)*6000000)/(38*(2*1+1))*/
	{13000000, 22579200, 19, 99, 0},
	{19200000, 22579200, 25, 88, 1},
	{24000000, 22579200, 63, 177, 4},/*((177 + 4 * 0.2) * 24000000) / (63 * (2 * 1 + 1)) */
	{128000, 24576000, 1, 576, 0},
	{192000, 24576000, 1, 384, 0},
	{256000, 24576000, 1, 288, 0},
	{384000, 24576000, 1, 192, 0},
	{1411200, 22579200, 1, 48, 0},
	{2048000, 24576000, 1, 36, 0},
	{6000000, 24576000, 25, 307, 1},
	{13000000, 24576000, 42, 238, 1},
	{19200000, 24576000, 25, 96, 0},
	{24000000, 24576000, 39, 119, 4},/*((119 + 4 * 0.2) * 24000000) / (39 * (2 * 1 + 1)) */
	{11289600, 22579200, 1, 6, 0},
	{12288000, 24576000, 1, 6, 0},
};

static const struct aif1_fs codec_aif1_fs[] = {
	{44100, 2, 7, _SERIES_22_579K},
	{48000, 2, 8},
	{8000, 12, 0},
	{11025, 8, 1, _SERIES_22_579K},
	{12000, 8, 2},
	{16000, 6, 3},
	{22050, 4, 4, _SERIES_22_579K},
	{24000, 4, 5},
	/* {32000, 3, 6}, not support */
	{96000, 1, 9},
};

static const struct aif1_lrck codec_aif1_lrck[] = {
	{16, 0},
	{32, 1},
	{64, 2},
	{128, 3},
	{256, 4},
};

static const struct aif1_word_size codec_aif1_wsize[] = {
	{8, 0},
	{16, 1},
	{20, 2},
	{24, 3},
	{32, 3},
};

static const unsigned ac10x_bclkdivs[] = {
	  1,   2,   4,   6,
	  8,  12,  16,  24,
	 32,  48,  64,  96,
	128, 192,   0,   0,
};

static int ac10x_aif_mute(struct snd_soc_dai *codec_dai, int mute)
{
	struct snd_soc_codec *codec = codec_dai->codec;

	AC10X_DBG("%s() L%d mute=%d\n", __func__, __LINE__, mute);

	snd_soc_write(codec, DAC_VOL_CTRL, mute? 0: 0xA0A0);
	return 0;
}

static void ac10x_aif_shutdown(struct snd_pcm_substream *substream,
	struct snd_soc_dai *codec_dai)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	int reg_val;

	AC10X_DBG("%s,line:%d\n", __func__, __LINE__);

	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
		if(agc_used){
			agc_enable(codec, 0);
		}
		reg_val = (snd_soc_read(codec, AIF_SR_CTRL) >> 12);
		reg_val &= 0xf;
		if (codec_dai->playback_active && dmic_used && reg_val == 0x4) {
			snd_soc_update_bits(codec, AIF_SR_CTRL, (0xf<<AIF1_FS), (0x7<<AIF1_FS));
		}
	}
}


static int ac10x_set_pll(struct snd_soc_dai *codec_dai, int pll_id, int source,
			unsigned int freq_in, unsigned int freq_out);

static int ac10x_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params,
	struct snd_soc_dai *codec_dai)
{
	int i = 0;
	int AIF_CLK_CTRL = 0;
	int aif1_word_size = 16;
	int aif1_lrck_div = 64;
	struct snd_soc_codec *codec = codec_dai->codec;
	struct ac10x_priv *ac10x = snd_soc_codec_get_drvdata(codec);
	int reg_val, freq_out;
	unsigned channels;

	AC10X_DBG("%s() L%d +++\n", __func__, __LINE__);

	channels = params_channels(params);

	switch (codec_dai->id) {
	case AIF1_CLK:
		AIF_CLK_CTRL = AIF1_CLK_CTRL;
		aif1_lrck_div = 16 * channels; 
		break;
	default:
		return -EINVAL;
	}

	for (i = 0; i < ARRAY_SIZE(codec_aif1_lrck); i++) {
		if (codec_aif1_lrck[i].div == aif1_lrck_div) {
			snd_soc_update_bits(codec, AIF_CLK_CTRL, (0x7<<AIF1_LRCK_DIV), ((codec_aif1_lrck[i].bit)<<AIF1_LRCK_DIV));
			break;
		}
	}

	freq_out = 24576000;
	for (i = 0; i < ARRAY_SIZE(codec_aif1_fs); i++) {
		if (codec_aif1_fs[i].samp_rate ==  params_rate(params)) {
			if (codec_dai->capture_active && dmic_used && codec_aif1_fs[i].samp_rate == 44100) {
				snd_soc_update_bits(codec, AIF_SR_CTRL, (0xf<<AIF1_FS), (0x4<<AIF1_FS));
			} else {
				snd_soc_update_bits(codec, AIF_SR_CTRL, (0xf<<AIF1_FS), ((codec_aif1_fs[i].srbit)<<AIF1_FS));
			}
			if (codec_aif1_fs[i].series == _SERIES_22_579K)
				freq_out = 22579200;
			break;
		}
	}

	/* set I2S word size */
	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S24_LE:
	case SNDRV_PCM_FORMAT_S32_LE:
		aif1_word_size = 24;
		break;
	case SNDRV_PCM_FORMAT_S16_LE:
	default:
		aif1_word_size = 16;
		break;
	}
	for (i = 0; i < ARRAY_SIZE(codec_aif1_wsize); i++) {
		if (codec_aif1_wsize[i].val == aif1_word_size) {
			break;
		}
	}
	snd_soc_update_bits(codec, AIF_CLK_CTRL, (0x3<<AIF1_WORK_SIZ), ((codec_aif1_wsize[i].bit)<<AIF1_WORK_SIZ));

	/* set TDM slot size, fixed 32 bits */
	#if 0
	if ((i = codec_aif1_wsize[i].bit) > 2)
	#endif
		i = 2;
	snd_soc_update_bits(codec, AIF1_ADCDAT_CTRL, 0x3 << AIF1_SLOT_SIZ, i << AIF1_SLOT_SIZ);

	/* setting pll if it's master mode */
	reg_val = snd_soc_read(codec, AIF_CLK_CTRL);
	if ((reg_val & (0x1 << AIF1_MSTR_MOD)) == 0) {
		unsigned bclkdiv;


		ac10x_set_pll(codec_dai, AC10X_MCLK1, 0, ac10x->sysclk, freq_out);

		bclkdiv = freq_out / (aif1_lrck_div * params_rate(params));

		for (i = 0; i < ARRAY_SIZE(ac10x_bclkdivs) - 1; i++) {
			if (ac10x_bclkdivs[i] >= bclkdiv) {
				break;
			}
		}

		snd_soc_update_bits(codec, AIF_CLK_CTRL, (0xf<<AIF1_BCLK_DIV), i<<AIF1_BCLK_DIV);
	}

	AC10X_DBG("%s() L%d ---\n", __func__, __LINE__);
	return 0;
}

static int ac10x_set_dai_sysclk(struct snd_soc_dai *codec_dai,
				  int clk_id, unsigned int freq, int dir)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	struct ac10x_priv *ac10x = snd_soc_codec_get_drvdata(codec);

	AC10X_DBG("%s,line:%d, id=%d freq=%d, dir=%d\n", __func__, __LINE__,
		clk_id, freq, dir);
#if 0
	struct snd_soc_codec *codec = codec_dai->codec;

	switch (clk_id) {
	case AIF1_CLK:
		AC10X_DBG("%s,line:%d,snd_soc_read(codec, SYSCLK_CTRL):%x\n",
			__func__, __LINE__,
			snd_soc_read(codec, SYSCLK_CTRL));
		/*system clk from aif1*/
		snd_soc_update_bits(codec, SYSCLK_CTRL, (0x1<<SYSCLK_SRC), (0x0<<SYSCLK_SRC));
		break;
	case AIF2_CLK:
		/*system clk from aif2*/
		snd_soc_update_bits(codec, SYSCLK_CTRL, (0x1<<SYSCLK_SRC), (0x1<<SYSCLK_SRC));
		break;
	default:
		return -EINVAL;
	}
#else
	ac10x->sysclk = freq;
#endif
	return 0;
}

static int ac10x_set_dai_fmt(struct snd_soc_dai *codec_dai,
			       unsigned int fmt)
{
	int reg_val;
	int AIF_CLK_CTRL = 0;
	struct snd_soc_codec *codec = codec_dai->codec;

	AC10X_DBG("%s() L%d\n", __func__, __LINE__);

	switch (codec_dai->id) {
	case AIF1_CLK:
		AIF_CLK_CTRL = AIF1_CLK_CTRL;
		break;
	default:
		return -EINVAL;
	}

	/*
	 * 	master or slave selection
	 *	0 = Master mode
	 *	1 = Slave mode
	 */
	reg_val = snd_soc_read(codec, AIF_CLK_CTRL);
	reg_val &= ~(0x1<<AIF1_MSTR_MOD);
	switch(fmt & SND_SOC_DAIFMT_MASTER_MASK) {
		case SND_SOC_DAIFMT_CBM_CFM:   /* codec clk & frm master, ap is slave*/
			reg_val |= (0x0<<AIF1_MSTR_MOD);
			break;
		case SND_SOC_DAIFMT_CBS_CFS:   /* codec clk & frm slave, ap is master*/
			reg_val |= (0x1<<AIF1_MSTR_MOD);
			break;
		default:
			pr_err("unknwon master/slave format\n");
			return -EINVAL;
	}

	/*
	 * Enable TDM mode
	 */
	// reg_val |=  (0x1 << AIF1_TDMM_ENA);
	reg_val &= ~(0x1 << AIF1_TDMM_ENA);
	snd_soc_write(codec, AIF_CLK_CTRL, reg_val);

	/* i2s mode selection */
	reg_val = snd_soc_read(codec, AIF_CLK_CTRL);
	reg_val&=~(3<<AIF1_DATA_FMT);
	switch(fmt & SND_SOC_DAIFMT_FORMAT_MASK){
		case SND_SOC_DAIFMT_I2S:        /* I2S1 mode */
			reg_val |= (0x0<<AIF1_DATA_FMT);
			break;
		case SND_SOC_DAIFMT_RIGHT_J:    /* Right Justified mode */
			reg_val |= (0x2<<AIF1_DATA_FMT);
			break;
		case SND_SOC_DAIFMT_LEFT_J:     /* Left Justified mode */
			reg_val |= (0x1<<AIF1_DATA_FMT);
			break;
		case SND_SOC_DAIFMT_DSP_A:      /* L reg_val msb after FRM LRC */
			reg_val |= (0x3<<AIF1_DATA_FMT);
			break;
		default:
			pr_err("%s, line:%d\n", __func__, __LINE__);
			return -EINVAL;
	}
	snd_soc_write(codec, AIF_CLK_CTRL, reg_val);

	/* DAI signal inversions */
	reg_val = snd_soc_read(codec, AIF_CLK_CTRL);
	switch(fmt & SND_SOC_DAIFMT_INV_MASK){
		case SND_SOC_DAIFMT_NB_NF:     /* normal bit clock + nor frame */
			reg_val &= ~(0x1<<AIF1_LRCK_INV);
			reg_val &= ~(0x1<<AIF1_BCLK_INV);
			break;
		case SND_SOC_DAIFMT_NB_IF:     /* normal bclk + inv frm */
			reg_val |= (0x1<<AIF1_LRCK_INV);
			reg_val &= ~(0x1<<AIF1_BCLK_INV);
			break;
		case SND_SOC_DAIFMT_IB_NF:     /* invert bclk + nor frm */
			reg_val &= ~(0x1<<AIF1_LRCK_INV);
			reg_val |= (0x1<<AIF1_BCLK_INV);
			break;
		case SND_SOC_DAIFMT_IB_IF:     /* invert bclk + inv frm */
			reg_val |= (0x1<<AIF1_LRCK_INV);
			reg_val |= (0x1<<AIF1_BCLK_INV);
			break;
	}
	snd_soc_write(codec, AIF_CLK_CTRL, reg_val);

	return 0;
}

static int ac10x_set_pll(struct snd_soc_dai *codec_dai, int pll_id, int source,
			unsigned int freq_in, unsigned int freq_out)
{
	int i = 0;
	int m 	= 0;
	int n_i = 0;
	int n_f = 0;
	struct snd_soc_codec *codec = codec_dai->codec;

	AC10X_DBG("%s, line:%d, pll_id:%d\n", __func__, __LINE__, pll_id);

	if (!freq_out)
		return 0;
	if ((freq_in < 128000) || (freq_in > 24576000)) {
		return -EINVAL;
	} else if ((freq_in == 24576000) || (freq_in == 22579200)) {
		switch (pll_id) {
		case AC10X_MCLK1:
			/*select aif1 clk source from mclk1*/
			snd_soc_update_bits(codec, SYSCLK_CTRL, (0x3<<AIF1CLK_SRC), (0x0<<AIF1CLK_SRC));
			break;
		default:
			return -EINVAL;
		}
		return 0;
	}
	switch (pll_id) {
	case AC10X_MCLK1:
		/*pll source from MCLK1*/
		snd_soc_update_bits(codec, SYSCLK_CTRL, (0x3<<PLLCLK_SRC), (0x0<<PLLCLK_SRC));
		break;
	case AC10X_BCLK1:
		/*pll source from BCLK1*/
		snd_soc_update_bits(codec, SYSCLK_CTRL, (0x3<<PLLCLK_SRC), (0x2<<PLLCLK_SRC));
		break;
	default:
		return -EINVAL;
	}
	/* freq_out = freq_in * n/(m*(2k+1)) , k=1,N=N_i+N_f */
	for (i = 0; i < ARRAY_SIZE(codec_pll_div); i++) {
		if ((codec_pll_div[i].pll_in == freq_in) && (codec_pll_div[i].pll_out == freq_out)) {
			m   = codec_pll_div[i].m;
			n_i = codec_pll_div[i].n_i;
			n_f = codec_pll_div[i].n_f;
			break;
		}
	}
	/*config pll m*/
	snd_soc_update_bits(codec, PLL_CTRL1, (0x3f<<PLL_POSTDIV_M), (m<<PLL_POSTDIV_M));
	/*config pll n*/
	snd_soc_update_bits(codec, PLL_CTRL2, (0x3ff<<PLL_PREDIV_NI), (n_i<<PLL_PREDIV_NI));
	snd_soc_update_bits(codec, PLL_CTRL2, (0x7<<PLL_POSTDIV_NF), (n_f<<PLL_POSTDIV_NF));
	snd_soc_update_bits(codec, PLL_CTRL2, (0x1<<PLL_EN), (1<<PLL_EN));
	/*enable pll_enable*/
	snd_soc_update_bits(codec, SYSCLK_CTRL, (0x1<<PLLCLK_ENA),  (0x1<<PLLCLK_ENA));
	snd_soc_update_bits(codec, SYSCLK_CTRL, (0x3<<AIF1CLK_SRC), (0x3<<AIF1CLK_SRC));

	return 0;
}

static int ac10x_audio_startup(struct snd_pcm_substream *substream,
	struct snd_soc_dai *codec_dai)
{
	struct snd_soc_codec *codec = codec_dai->codec;

	AC10X_DBG("%s,line:%d\n", __func__, __LINE__);

	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
		if(agc_used){
			agc_enable(codec, 1);
		}
	}
	return 0;
}

#ifndef CONFIG_SWITCH_DETECT_EXTERNAL
/*
**switch_hw_config:config the 53 codec register
*/
static void switch_hw_config(struct snd_soc_codec *codec)
{
	/*HMIC/MMIC BIAS voltage level select:2.5v*/
	snd_soc_update_bits(codec, OMIXER_BST1_CTRL, (0xf<<BIASVOLTAGE), (0xf<<BIASVOLTAGE));
	/*debounce when Key down or keyup*/
	snd_soc_update_bits(codec, HMIC_CTRL1, (0xf<<HMIC_M), (0x0<<HMIC_M));
	/*debounce when earphone plugin or pullout*/
	snd_soc_update_bits(codec, HMIC_CTRL1, (0xf<<HMIC_N), (0x0<<HMIC_N));
	/*Down Sample Setting Select/11:Downby 8,16Hz*/
	snd_soc_update_bits(codec, HMIC_CTRL2, (0x3<<HMIC_SAMPLE_SELECT), (0x0<<HMIC_SAMPLE_SELECT));
	/*Hmic_th2 for detecting Keydown or Keyup.*/
	snd_soc_update_bits(codec, HMIC_CTRL2, (0x1f<<HMIC_TH2), (0x8<<HMIC_TH2));
	/*Hmic_th1[4:0],detecting eraphone plugin or pullout*/
	snd_soc_update_bits(codec, HMIC_CTRL2, (0x1f<<HMIC_TH1), (0x1<<HMIC_TH1));
	/*Headset microphone BIAS Enable*/
	snd_soc_update_bits(codec, ADC_APC_CTRL, (0x1<<HBIASEN), (0x1<<HBIASEN));
	/*Headset microphone BIAS Current sensor & ADC Enable*/
	snd_soc_update_bits(codec, ADC_APC_CTRL, (0x1<<HBIASADCEN), (0x1<<HBIASADCEN));
	/*Earphone Plugin/out Irq Enable*/
	snd_soc_update_bits(codec, HMIC_CTRL1, (0x1<<HMIC_PULLOUT_IRQ), (0x1<<HMIC_PULLOUT_IRQ));
	snd_soc_update_bits(codec, HMIC_CTRL1, (0x1<<HMIC_PLUGIN_IRQ), (0x1<<HMIC_PLUGIN_IRQ));

	/*Hmic KeyUp/key down Irq Enable*/
	snd_soc_update_bits(codec, HMIC_CTRL1, (0x1<<HMIC_KEYDOWN_IRQ), (0x1<<HMIC_KEYDOWN_IRQ));
	snd_soc_update_bits(codec, HMIC_CTRL1, (0x1<<HMIC_KEYUP_IRQ), (0x1<<HMIC_KEYUP_IRQ));

	/*headphone calibration clock frequency select*/
	snd_soc_update_bits(codec, SPKOUT_CTRL, (0x7<<HPCALICKS), (0x7<<HPCALICKS));
}
#endif
static int ac10x_set_bias_level(struct snd_soc_codec *codec,
				      enum snd_soc_bias_level level)
{
	switch (level) {
	case SND_SOC_BIAS_ON:
		AC10X_DBG("%s,line:%d, SND_SOC_BIAS_ON\n", __func__, __LINE__);
		break;
	case SND_SOC_BIAS_PREPARE:
		AC10X_DBG("%s,line:%d, SND_SOC_BIAS_PREPARE\n", __func__, __LINE__);
		break;
	case SND_SOC_BIAS_STANDBY:
		#ifndef CONFIG_SWITCH_DETECT_EXTERNAL
		switch_hw_config(codec);
		#endif

		AC10X_DBG("%s,line:%d, SND_SOC_BIAS_STANDBY\n", __func__, __LINE__);
		break;
	case SND_SOC_BIAS_OFF:
		#ifndef CONFIG_SWITCH_DETECT_EXTERNAL
		snd_soc_update_bits(codec, ADC_APC_CTRL, (0x1<<HBIASEN), (0<<HBIASEN));
		snd_soc_update_bits(codec, ADC_APC_CTRL, (0x1<<HBIASADCEN), (0<<HBIASADCEN));
		#endif
		snd_soc_update_bits(codec, OMIXER_DACA_CTRL, (0xf<<HPOUTPUTENABLE), (0<<HPOUTPUTENABLE));
		snd_soc_update_bits(codec, ADDA_TUNE3, (0x1<<OSCEN), (0<<OSCEN));
		AC10X_DBG("%s,line:%d, SND_SOC_BIAS_OFF\n", __func__, __LINE__);
		break;
	}
	snd_soc_codec_get_dapm(codec)->bias_level = level;
	return 0;
}
static const struct snd_soc_dai_ops ac10x_aif1_dai_ops = {
	.set_sysclk	= ac10x_set_dai_sysclk,
	.set_fmt	= ac10x_set_dai_fmt,
	.hw_params	= ac10x_hw_params,
	.shutdown	= ac10x_aif_shutdown,
	.digital_mute	= ac10x_aif_mute,
	.set_pll	= ac10x_set_pll,
	.startup	= ac10x_audio_startup,
};


static struct snd_soc_dai_driver ac10x_dai[] = {
	{
		.name = "ac10x-aif1",
		.id = AIF1_CLK,
		.playback = {
			.stream_name = "AIF1 Playback",
			.channels_min = 1,
			.channels_max = 8,
			.rates = ac10x_RATES,
			.formats = ac10x_FORMATS,
		},
		#if 0
		.capture = {
			.stream_name = "AIF1 Capture",
			.channels_min = 1,
			.channels_max = 8,
			.rates = ac10x_RATES,
			.formats = ac10x_FORMATS,
		},
		#endif
		.ops = &ac10x_aif1_dai_ops,
	}
};
#ifndef CONFIG_SWITCH_DETECT_EXTERNAL
static ssize_t switch_gpio_print_state(struct switch_dev *sdev, char *buf)
{
	struct ac10x_priv	*ac10x =
		container_of(sdev, struct ac10x_priv, sdev);
	return sprintf(buf, "%d\n", ac10x->state);
}

static ssize_t print_headset_name(struct switch_dev *sdev, char *buf)
{
	struct ac10x_priv	*ac10x =
		container_of(sdev, struct ac10x_priv, sdev);
	return sprintf(buf, "%s\n", ac10x->sdev.name);
}

/*
**switch_status_update: update the switch state.
*/
static void switch_status_update(struct ac10x_priv *para)
{
	struct ac10x_priv *ac10x = para;
	AC10X_DBG("%s,line:%d,ac10x->state:%d\n",__func__, __LINE__, ac10x->state);
	down(&ac10x->sem);
	switch_set_state(&ac10x->sdev, ac10x->state);
	up(&ac10x->sem);
}

/*
**clear_codec_irq_work: clear audiocodec pending and Record the interrupt.
*/
static void clear_codec_irq_work(struct work_struct *work)
{
	int reg_val = 0;
	struct ac10x_priv *ac10x = container_of(work, struct ac10x_priv, clear_codec_irq);
	struct snd_soc_codec *codec = ac10x->codec;
	irq_flag = irq_flag+1;
	reg_val = snd_soc_read(codec, HMIC_STS);
	if ((0x1<<4)&reg_val) {
		reset_flag++;
		AC10X_DBG("reset_flag:%d\n",reset_flag);
	}
	reg_val |= (0x1f<<0);
	snd_soc_write(codec, HMIC_STS, reg_val);
	reg_val = snd_soc_read(codec, HMIC_STS);
	if((reg_val&0x1f) != 0){
		reg_val |= (0x1f<<0);
		snd_soc_write(codec, HMIC_STS, reg_val);
	}

	if (cancel_work_sync(&ac10x->work) != 0) {
			irq_flag--;
	}

	if (0 == queue_work(switch_detect_queue, &ac10x->work)) {
		irq_flag--;
		pr_err("[clear_codec_irq_work]add work struct failed!\n");
	}
}

/*
**earphone_switch_work: judge the status of the headphone
*/
static void earphone_switch_work(struct work_struct *work)
{
	int reg_val = 0;
	int tmp  = 0;
	unsigned int temp_value[11];
	struct ac10x_priv *ac10x = container_of(work, struct ac10x_priv, work);
	struct snd_soc_codec *codec = ac10x->codec;
	irq_flag--;
	ac10x->check_count = 0;
	ac10x->check_count_sum = 0;
	/*read HMIC_DATA */
	tmp = snd_soc_read(codec, HMIC_STS);
	reg_val = tmp;
	tmp = (tmp>>HMIC_DATA);
	tmp &= 0x1f;

	if ((tmp>=0xb) && (ac10x->mode== FOUR_HEADPHONE_PLUGIN)) {
		tmp = snd_soc_read(codec, HMIC_STS);
		tmp = (tmp>>HMIC_DATA);
		tmp &= 0x1f;
		if(tmp>=0x19){
			msleep(150);
			tmp = snd_soc_read(codec, HMIC_STS);
			tmp = (tmp>>HMIC_DATA);
			tmp &= 0x1f;
			if(((tmp<0xb && tmp>=0x1) || tmp>=0x19)&&(reset_flag == 0)){
				input_report_key(ac10x->key, KEY_HEADSETHOOK, 1);
				input_sync(ac10x->key);
				AC10X_DBG("%s,line:%d,KEY_HEADSETHOOK1\n",__func__,__LINE__);
				if(hook_flag1 != hook_flag2){
					hook_flag1 = hook_flag2 = 0;
				}
				hook_flag1++;
			}
			if(reset_flag)
				reset_flag--;
		}else if(tmp<0x19 && tmp>=0x17){
			msleep(80);
			tmp = snd_soc_read(codec, HMIC_STS);
			tmp = (tmp>>HMIC_DATA);
			tmp &= 0x1f;
			if(tmp<0x19 && tmp>=0x17 &&(reset_flag == 0)) {
				KEY_VOLUME_FLAG = 1;
				input_report_key(ac10x->key, KEY_VOLUMEUP, 1);
				input_sync(ac10x->key);
				input_report_key(ac10x->key, KEY_VOLUMEUP, 0);
				input_sync(ac10x->key);
				AC10X_DBG("%s,line:%d,tmp:%d,KEY_VOLUMEUP\n",__func__,__LINE__,tmp);
			}
			if(reset_flag)
				reset_flag--;
		}else if(tmp<0x17 && tmp>=0x13){
			msleep(80);
			tmp = snd_soc_read(codec, HMIC_STS);
			tmp = (tmp>>HMIC_DATA);
			tmp &= 0x1f;
			if(tmp<0x17 && tmp>=0x13  && (reset_flag == 0)){
				KEY_VOLUME_FLAG = 1;
				input_report_key(ac10x->key, KEY_VOLUMEDOWN, 1);
				input_sync(ac10x->key);
				input_report_key(ac10x->key, KEY_VOLUMEDOWN, 0);
				input_sync(ac10x->key);
				AC10X_DBG("%s,line:%d,KEY_VOLUMEDOWN\n",__func__,__LINE__);
			}
			if(reset_flag)
				reset_flag--;
		}
	} else if ((tmp<0xb && tmp>=0x2) && (ac10x->mode== FOUR_HEADPHONE_PLUGIN)) {
		/*read HMIC_DATA */
		tmp = snd_soc_read(codec, HMIC_STS);
		tmp = (tmp>>HMIC_DATA);
		tmp &= 0x1f;
		if (tmp<0xb && tmp>=0x2) {
			if(KEY_VOLUME_FLAG) {
				KEY_VOLUME_FLAG = 0;
			}
			if(hook_flag1 == (++hook_flag2)) {
				hook_flag1 = hook_flag2 = 0;
				input_report_key(ac10x->key, KEY_HEADSETHOOK, 0);
				input_sync(ac10x->key);
				AC10X_DBG("%s,line:%d,KEY_HEADSETHOOK0\n",__func__,__LINE__);
			}
		}
	} else {
		while (irq_flag == 0) {
			msleep(20);
			/*read HMIC_DATA */
			tmp = snd_soc_read(codec, HMIC_STS);
			tmp = (tmp>>HMIC_DATA);
			tmp &= 0x1f;
			if(ac10x->check_count_sum <= HEADSET_CHECKCOUNT_SUM){
				if (ac10x->check_count <= HEADSET_CHECKCOUNT){
					temp_value[ac10x->check_count] = tmp;
					ac10x->check_count++;
					if(ac10x->check_count >= 2){
						if( !(temp_value[ac10x->check_count - 1] == temp_value[(ac10x->check_count) - 2])){
							ac10x->check_count = 0;
							ac10x->check_count_sum = 0;
						}
					}

				}else{
					ac10x->check_count_sum++;
				}
			}else{
				if (temp_value[ac10x->check_count -2] >= 0xb) {

					ac10x->state		= 2;
					ac10x->mode = THREE_HEADPHONE_PLUGIN;
					switch_status_update(ac10x);
					ac10x->check_count = 0;
					ac10x->check_count_sum = 0;
					reset_flag = 0;
					break;
				} else if(temp_value[ac10x->check_count - 2]>=0x1 && temp_value[ac10x->check_count -2]<0xb) {
					ac10x->mode = FOUR_HEADPHONE_PLUGIN;
					ac10x->state		= 1;
					switch_status_update(ac10x);
					ac10x->check_count = 0;
					ac10x->check_count_sum = 0;
					reset_flag = 0;
					break;

				} else {
					ac10x->mode = HEADPHONE_IDLE;
					ac10x->state = 0;
					switch_status_update(ac10x);
					ac10x->check_count = 0;
					ac10x->check_count_sum = 0;
					reset_flag = 0;
					break;
				}
			}
		}
	}
}

/*
**audio_hmic_irq:  the interrupt handlers
*/
static irqreturn_t audio_hmic_irq(int irq, void *para)
{
	struct ac10x_priv *ac10x = (struct ac10x_priv *)para;
	if (ac10x == NULL) {
		return -EINVAL;
	}
	if(codec_irq_queue == NULL)
		pr_err("------------codec_irq_queue is null!!----------");
	if(&ac10x->clear_codec_irq == NULL)
		pr_err("------------ac10x->clear_codec_irq is null!!----------");

	if(0 == queue_work(codec_irq_queue, &ac10x->clear_codec_irq)){
		pr_err("[audio_hmic_irq]add work struct failed!\n");
	}
	return 0;
}
#endif
static void codec_resume_work(struct work_struct *work)
{
	struct ac10x_priv *ac10x = container_of(work, struct ac10x_priv, codec_resume);
	struct snd_soc_codec *codec = ac10x->codec;
	int i, ret = 0;

	AC10X_DBG("%s() L%d +++\n", __func__, __LINE__);

#ifndef CONFIG_SWITCH_DETECT_EXTERNAL
	ac10x->virq = gpio_to_irq(SWITCH_DETECT);
	if (IS_ERR_VALUE(ac10x->virq)) {
		pr_warn("[ac10x] map gpio to virq failed, errno = %d\n",ac10x->virq);
		//return -EINVAL;
	}
	/* request virq, set virq type to high level trigger */
	ret = devm_request_irq(codec->dev, ac10x->virq, audio_hmic_irq, IRQF_TRIGGER_FALLING, "SWTICH_EINT", ac10x);
	if (IS_ERR_VALUE(ret)) {
		pr_warn("[ac10x] request virq %d failed, errno = %d\n", ac10x->virq, ret);
        	//return -EINVAL;
	}
#endif
	for (i = 0; i < ARRAY_SIZE(ac10x_supplies); i++){
		ret = regulator_enable(ac10x->supplies[i].consumer);

		if (0 != ret) {
			pr_err("[ac10x] %s: some error happen, fail to enable regulator!\n", __func__);
		}
	}
	msleep(50);
	set_configuration(codec);
	if (agc_used) {
		agc_config(codec);
	}
	if (drc_used) {
		drc_config(codec);
	}
	/*enable this bit to prevent leakage from ldoin*/
	snd_soc_update_bits(codec, ADDA_TUNE3, (0x1<<OSCEN), (0x1<<OSCEN));
	
	#ifndef CONFIG_SWITCH_DETECT_EXTERNAL
	gpio_direction_output(PA_CTL, 1);
	gpio_set_value(PA_CTL, 0);
	msleep(200);
	ret = snd_soc_read(codec, HMIC_STS);
	ret = (ret>>HMIC_DATA);
	ret &= 0x1f;
	if (ret < 1) {
		ac10x->state = 0;
		switch_status_update(ac10x);
	}
	#endif

	AC10X_DBG("%s() L%d +++\n", __func__, __LINE__);
	return;
}


/***************************************************************************/
static ssize_t ac10x_debug_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	static int val = 0, flag = 0;
	u8 reg,num,i=0;
	u16 value_w,value_r[128];
	struct ac10x_priv *ac10x = dev_get_drvdata(dev);
	val = simple_strtol(buf, NULL, 16);
	flag = (val >> 24) & 0xF;
	if(flag) {//write
		reg = (val >> 16) & 0xFF;
		value_w =  val & 0xFFFF;
		snd_soc_write(ac10x->codec, reg, value_w);
		printk("write 0x%x to reg:0x%x\n",value_w,reg);
	} else {
		reg =(val>>8)& 0xFF;
		num=val&0xff;
		printk("\n");
		printk("read:start add:0x%x,count:0x%x\n",reg,num);
		do{
			value_r[i] = snd_soc_read(ac10x->codec, reg);
			printk("0x%x: 0x%04x ",reg,value_r[i]);
			reg+=1;
			i++;
			if(i == num)
				printk("\n");
			if(i%4==0)
				printk("\n");
		}while(i<num);
	}
	return count;
}
static ssize_t ac10x_debug_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	printk("echo flag|reg|val > ac10x\n");
	printk("eg read star addres=0x06,count 0x10:echo 0610 >ac10x\n");
	printk("eg write value:0x13fe to address:0x06 :echo 10613fe > ac10x\n");
	return 0;
}
static DEVICE_ATTR(ac10x, 0644, ac10x_debug_show, ac10x_debug_store);

static struct attribute *audio_debug_attrs[] = {
	&dev_attr_ac10x.attr,
	NULL,
};

static struct attribute_group audio_debug_attr_group = {
	.name   = "ac10x_debug",
	.attrs  = audio_debug_attrs,
};
/************************************************************/
static const struct regmap_config ac101_regmap = {
	.reg_bits = 8,
	.val_bits = 16,
	.reg_stride = 1,

	.max_register = 0xB5,
	.cache_type = REGCACHE_RBTREE,
};

static int ac10x_codec_probe(struct snd_soc_codec *codec)
{
	int ret = 0;
	int i = 0;
	struct ac10x_priv *ac10x;
	struct snd_soc_dapm_context *dapm = snd_soc_codec_get_dapm(codec);

	ac10x = dev_get_drvdata(codec->dev);
	if (ac10x == NULL) {
		AC10X_DBG("not set client data %s() L%d\n", __func__, __LINE__);
		return -ENOMEM;
	}
	ac10x->codec = codec;

#ifndef CONFIG_SWITCH_DETECT_EXTERNAL
	ac10x->sdev.state 	= 0;
	ac10x->state		= -1;
	ac10x->check_count	= 0;
	ac10x->check_count_sum	= 0;
	ac10x->sdev.name 	= "h2w";
	ac10x->sdev.print_name 	= print_headset_name;
	ac10x->sdev.print_state = switch_gpio_print_state;

	ret = switch_dev_register(&ac10x->sdev);
	if (ret < 0) {
		goto err_switch_dev_register;
	}

	/*use for judge the state of switch*/
	INIT_WORK(&ac10x->work, earphone_switch_work);
	INIT_WORK(&ac10x->clear_codec_irq,clear_codec_irq_work);

	/********************create input device************************/
	ac10x->key = input_allocate_device();
	if (!ac10x->key) {
	     pr_err("[ac10x] input_allocate_device: not enough memory for input device\n");
	     ret = -ENOMEM;
	     goto err_input_allocate_device;
	}

	ac10x->key->name          = "headset";
	ac10x->key->phys          = "headset/input0";
	ac10x->key->id.bustype    = BUS_HOST;
	ac10x->key->id.vendor     = 0x0001;
	ac10x->key->id.product    = 0xffff;
	ac10x->key->id.version    = 0x0100;

	ac10x->key->evbit[0] = BIT_MASK(EV_KEY);

	set_bit(KEY_HEADSETHOOK, ac10x->key->keybit);
	set_bit(KEY_VOLUMEUP, ac10x->key->keybit);
	set_bit(KEY_VOLUMEDOWN, ac10x->key->keybit);

	ret = input_register_device(ac10x->key);
	if (ret) {
	    pr_err("[ac10x] input_register_device: input_register_device failed\n");
	    goto err_input_register_device;
	}
	headphone_state = 0;
	ac10x->mode = HEADPHONE_IDLE;
	sema_init(&ac10x->sem, 1);
	codec_irq_queue = create_singlethread_workqueue("codec_irq");
	switch_detect_queue = create_singlethread_workqueue("codec_resume");
	if (switch_detect_queue == NULL || codec_irq_queue == NULL) {
		pr_err("[ac10x] try to create workqueue for codec failed!\n");
		ret = -ENOMEM;
		goto err_switch_work_queue;
	}
#endif
	INIT_WORK(&ac10x->codec_resume, codec_resume_work);
	ac10x->dac_enable = 0;
	ac10x->adc_enable = 0;
	ac10x->aif1_clken = 0;
	ac10x->aif2_clken = 0;
	ac10x->aif3_clken = 0;
	mutex_init(&ac10x->dac_mutex);
	mutex_init(&ac10x->adc_mutex);
	mutex_init(&ac10x->aifclk_mutex);

#ifndef CONFIG_SWITCH_DETECT_EXTERNAL
	/*request pa gpio*/
	ret = gpio_request(PA_CTL, NULL);
	if (0 != ret) {
		pr_err("request gpio failed!\n");
	} else {
		/*
		* config gpio info of audio_pa_ctrl, the default pa config is close(check pa sys_config1.fex)
		*/
		gpio_direction_output(PA_CTL, 1);
		gpio_set_value(PA_CTL, 0);
	}

	ac10x->virq = gpio_to_irq(SWITCH_DETECT);
	if (IS_ERR_VALUE(ac10x->virq)) {
		pr_err("[ac10x] map gpio to virq failed, errno = %d\n",ac10x->virq);
	}
	/* request virq, set virq type to high level trigger */
	ret = devm_request_irq(codec->dev, ac10x->virq, audio_hmic_irq, IRQF_TRIGGER_FALLING, "SWTICH_EINT", ac10x);
	if (IS_ERR_VALUE(ret)) {
		pr_err("[ac10x] request virq %d failed, errno = %d\n", ac10x->virq, ret);
	}
#endif
	ac10x->num_supplies = ARRAY_SIZE(ac10x_supplies);
	ac10x->supplies = devm_kzalloc(ac10x->codec->dev,
						sizeof(struct regulator_bulk_data) *
						ac10x->num_supplies, GFP_KERNEL);
	if (!ac10x->supplies) {
		pr_err("[ac10x] Failed to get mem.\n");
		return -ENOMEM;
	}
	for (i = 0; i < ARRAY_SIZE(ac10x_supplies); i++)
		ac10x->supplies[i].supply = ac10x_supplies[i];

	ret = regulator_bulk_get(NULL, ac10x->num_supplies, ac10x->supplies);
	if (ret != 0) {
		pr_err("[ac10x] Failed to get supplies: %d\n", ret);
	}

	for (i = 0; i < ARRAY_SIZE(ac10x_supplies); i++){
		ret = regulator_enable(ac10x->supplies[i].consumer);

		if (0 != ret) {
			pr_err("[ac10x] %s: some error happen, fail to enable regulator!\n", __func__);
		}
	}
	get_configuration();
	set_configuration(ac10x->codec);

	/*enable this bit to prevent leakage from ldoin*/
	snd_soc_update_bits(codec, ADDA_TUNE3, (0x1<<OSCEN), (0x1<<OSCEN));
	snd_soc_write(codec, DAC_VOL_CTRL, 0);
	ret = snd_soc_add_codec_controls(codec, ac10x_controls,
					ARRAY_SIZE(ac10x_controls));
	if (ret) {
		pr_err("[ac10x] Failed to register audio mode control, "
				"will continue without it.\n");
	}

	snd_soc_dapm_new_controls(dapm, ac10x_dapm_widgets, ARRAY_SIZE(ac10x_dapm_widgets));
 	snd_soc_dapm_add_routes(dapm, ac10x_dapm_routes, ARRAY_SIZE(ac10x_dapm_routes));
	return 0;

#ifndef CONFIG_SWITCH_DETECT_EXTERNAL
err_switch_work_queue:
	devm_free_irq(codec->dev,ac10x->virq,NULL);

err_input_register_device:
	if(ac10x->key){
		input_free_device(ac10x->key);
	}

err_input_allocate_device:
	switch_dev_unregister(&ac10x->sdev);

err_switch_dev_register:
#endif
	return ret;
}

/* power down chip */
static int ac10x_codec_remove(struct snd_soc_codec *codec)
{
	struct ac10x_priv *ac10x = snd_soc_codec_get_drvdata(codec);
	int i = 0;
	int ret = 0;

#ifndef CONFIG_SWITCH_DETECT_EXTERNAL
	devm_free_irq(codec->dev,ac10x->virq,NULL);
	if (ac10x->key) {
		input_unregister_device(ac10x->key);
		input_free_device(ac10x->key);
   	}
 	switch_dev_unregister(&ac10x->sdev);
#endif
	for (i = 0; i < ARRAY_SIZE(ac10x_supplies); i++){
		ret = regulator_disable(ac10x->supplies[i].consumer);

		if (0 != ret) {
		pr_err("[ac10x] %s: some error happen, fail to disable regulator!\n", __func__);
		}
		regulator_put(ac10x->supplies[i].consumer);
	}

	return 0;
}

static int ac10x_codec_suspend(struct snd_soc_codec *codec)
{
	int i ,ret =0;
	struct ac10x_priv *ac10x = snd_soc_codec_get_drvdata(codec);

	AC10X_DBG("[codec]:suspend\n");
	ac10x_set_bias_level(codec, SND_SOC_BIAS_OFF);
	for (i = 0; i < ARRAY_SIZE(ac10x_supplies); i++){
		ret = regulator_disable(ac10x->supplies[i].consumer);

		if (0 != ret) {
			pr_err("[ac10x] %s: some error happen, fail to disable regulator!\n", __func__);
		}
	}

	regcache_cache_only(ac10x->regmap, true);
	return 0;
}

static int ac10x_codec_resume(struct snd_soc_codec *codec)
{
	struct ac10x_priv *ac10x = snd_soc_codec_get_drvdata(codec);
	int ret;

	AC10X_DBG("[codec]:resume");

	#ifndef CONFIG_SWITCH_DETECT_EXTERNAL
	ac10x->mode = HEADPHONE_IDLE;
	headphone_state = 0;
	ac10x->state	= -1;
	#endif

	/* Sync reg_cache with the hardware */
	regcache_cache_only(ac10x->regmap, false);
	ret = regcache_sync(ac10x->regmap);
	if (ret != 0) {
		dev_err(codec->dev, "Failed to sync register cache: %d\n", ret);
		regcache_cache_only(ac10x->regmap, true);
		return ret;
	}


	ac10x_set_bias_level(codec, SND_SOC_BIAS_STANDBY);
	schedule_work(&ac10x->codec_resume);
	return 0;
}
static struct snd_soc_codec_driver soc_codec_dev_sndvir_audio = {
	.probe 		= ac10x_codec_probe,
	.remove 	= ac10x_codec_remove,
	.suspend 	= ac10x_codec_suspend,
	.resume 	= ac10x_codec_resume,
	.set_bias_level = ac10x_set_bias_level,
	.ignore_pmdown_time = 1,
};

static int ac10x_probe(struct i2c_client *i2c,
				      const struct i2c_device_id *id)
{
	int ret = 0;
	struct ac10x_priv *ac10x;
	unsigned v;

	AC10X_DBG("%s,line:%d\n", __func__, __LINE__);
	ac10x = devm_kzalloc(&i2c->dev, sizeof(struct ac10x_priv), GFP_KERNEL);
	if (ac10x == NULL) {
		AC10X_DBG("no memory %s() L%d\n", __func__, __LINE__);
		return -ENOMEM;
	}
	i2c_set_clientdata(i2c, ac10x);

	ac10x->regmap = devm_regmap_init_i2c(i2c, &ac101_regmap);
	if (IS_ERR(ac10x->regmap)) {
		ret = PTR_ERR(ac10x->regmap);
		dev_err(&i2c->dev, "Fail to initialize I/O: %d\n", ret);
		return ret;
	}

	ret = regmap_read(ac10x->regmap, CHIP_AUDIO_RST, &v);
	if (ret < 0) {
		dev_err(&i2c->dev, "failed to read vendor ID: %d\n", ret);
		return ret;
	}

	if (v != AC101_CHIP_ID) {
		dev_err(&i2c->dev, "chip is not AC101\n");
		dev_err(&i2c->dev, "Expected %X\n", AC101_CHIP_ID);
		return -ENODEV;
	}

	ret = snd_soc_register_codec(&i2c->dev, &soc_codec_dev_sndvir_audio, ac10x_dai, ARRAY_SIZE(ac10x_dai));
	if (ret < 0) {
		dev_err(&i2c->dev, "Failed to register ac10x: %d\n", ret);
	}

	ret = sysfs_create_group(&i2c->dev.kobj, &audio_debug_attr_group);
	if (ret) {
		pr_err("failed to create attr group\n");
	}
	return 0;
}
static void ac10x_shutdown(struct i2c_client *i2c)
{
	int reg_val;
	struct snd_soc_codec *codec = NULL;
	struct ac10x_priv *ac10x = i2c_get_clientdata(i2c);
	if (ac10x->codec != NULL)
		codec = ac10x->codec;
	else{
		pr_err("no sound card.\n");
		return ;
	}
	/*set headphone volume to 0*/
	reg_val = snd_soc_read(codec, HPOUT_CTRL);
	reg_val &= ~(0x3f<<HP_VOL);
	snd_soc_write(codec, HPOUT_CTRL, reg_val);

	/*disable pa*/
	reg_val = snd_soc_read(codec, HPOUT_CTRL);
	reg_val &= ~(0x1<<HPPA_EN);
	snd_soc_write(codec, HPOUT_CTRL, reg_val);

	/*hardware xzh support*/
	reg_val = snd_soc_read(codec, OMIXER_DACA_CTRL);
	reg_val &= ~(0xf<<HPOUTPUTENABLE);
	snd_soc_write(codec, OMIXER_DACA_CTRL, reg_val);

	/*unmute l/r headphone pa*/
	reg_val = snd_soc_read(codec, HPOUT_CTRL);
	reg_val &= ~((0x1<<RHPPA_MUTE)|(0x1<<LHPPA_MUTE));
	snd_soc_write(codec, HPOUT_CTRL, reg_val);

	#if PA_CTL
	/*disable pa_ctrl*/
	gpio_set_value(PA_CTL, 0);
	#endif

}
static int ac10x_remove(struct i2c_client *i2c)
{
	struct ac10x_priv *ac10x = i2c_get_clientdata(i2c);
	sysfs_remove_group(&i2c->dev.kobj, &audio_debug_attr_group);
	snd_soc_unregister_codec(&i2c->dev);
	if (ac10x) {
		kfree(ac10x);
	}
	return 0;
}

int ac10x_driver_detect(struct i2c_client *client, struct i2c_board_info *info)
{
	struct i2c_adapter *adapter = client->adapter;

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
		AC10X_DBG("no device found %s() L%d\n", __func__, __LINE__);
		return -ENODEV;
	}
	if (I2C_BUS == adapter->nr) {
		strlcpy(info->type, "ac10x-codec", I2C_NAME_SIZE);
		return 0;
	} else {
		AC10X_DBG("no device found %s() L%d\n", __func__, __LINE__);
		return -ENODEV;
	}
}
static unsigned short normal_i2c[] = {0x1a, I2C_CLIENT_END };
static const struct i2c_device_id ac10x_id[] = {
	{"ac10x-codec", 0},
	{},
};
static const struct of_device_id ac101_of_match[] = {
	{ .compatible = "x-power,ac101", },
	{ }
};
MODULE_DEVICE_TABLE(of, ac101_of_match);

static struct i2c_driver ac10x_codec_driver = {
	.class = I2C_CLASS_HWMON,
	.id_table = ac10x_id,
	.probe = ac10x_probe,
	.remove = ac10x_remove,
	.driver = {
		.name = "ac10x-codec",
		.owner = THIS_MODULE,
		.of_match_table = ac101_of_match,
	},
	.address_list = normal_i2c,
	.detect = ac10x_driver_detect,
	.shutdown = ac10x_shutdown,
};
static int __init ac10x_init(void){
	int ret;
	pr_info("%s(%d): ac100 driver register!\n", __func__, __LINE__);
	ret = i2c_add_driver(&ac10x_codec_driver);
	return ret;
}

module_init(ac10x_init);
static void __exit ac10x_exit(void)
{
	pr_info("%s(%d): ac100 device unregister!\n", __func__, __LINE__);
	i2c_del_driver(&ac10x_codec_driver);
}
module_exit(ac10x_exit);


MODULE_DESCRIPTION("ASoC ac10x driver");
MODULE_AUTHOR("huangxin,liushaohua");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:ac10x-codec");
