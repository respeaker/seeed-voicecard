/*
 * ac108.c  --  ac108 ALSA SoC Audio driver
 *
 *
 * Author: Baozhu Zuo<zuobaozhu@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#define DEBUG 1
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/clk.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/initval.h>
#include <sound/tlv.h>
#include <sound/wm8960.h>

#include "ac108.h"

/**
 * TODO: 
 * 1, add PM API:  ac108_suspend,ac108_resume 
 * 2, add set_pll ,set_clkdiv 
 * 3,0x65-0x6a 
 * 4,0x76-0x79 high 4bit 
 */
struct pll_div {
	unsigned int freq_in;
	unsigned int freq_out;
	unsigned int m1;
	unsigned int m2;
	unsigned int n;
	unsigned int k1;
	unsigned int k2;
};


struct ac108_priv {
	struct i2c_client *i2c[4];
	int codec_index;
	int sysclk;
	int clk_id;
	unsigned char i2s_mode;
	unsigned char data_protocol;
};
static struct ac108_priv *ac108;

struct real_val_to_reg_val {
	unsigned int real_val;
	unsigned int reg_val;
};

static const struct real_val_to_reg_val ac108_sample_rate[] = {
	{ 8000,  0 },
	{ 11025, 1 },
	{ 12000, 2 },
	{ 16000, 3 },
	{ 22050, 4 },
	{ 24000, 5 },
	{ 32000, 6 },
	{ 44100, 7 },
	{ 48000, 8 },
	{ 96000, 9 },
};

static const struct real_val_to_reg_val ac108_sample_resolution[] = {
	{ 8,  1 },
	{ 12, 2 },
	{ 16, 3 },
	{ 20, 4 },
	{ 24, 5 },
	{ 28, 6 },
	{ 32, 7 },
};

/* FOUT =(FIN * N) / [(M1+1) * (M2+1)*(K1+1)*(K2+1)] ;	M1[0,31],  M2[0,1],  N[0,1023],  K1[0,31],  K2[0,1] */
static const struct pll_div ac108_pll_div_list[] = {
	{ 400000,   24576000, 0,  0, 614, 4, 1 },
	{ 512000,   24576000, 0,  0, 960, 9, 1 }, //24576000/48
	{ 768000,   24576000, 0,  0, 640, 9, 1 }, //24576000/32
	{ 800000,   24576000, 0,  0, 614, 9, 1 },
	{ 1024000,  24576000, 0,  0, 480, 9, 1 }, //24576000/24
	{ 1600000,  24576000, 0,  0, 307, 9, 1 },
	{ 2048000,  24576000, 0,  0, 240, 9, 1 }, //24576000/12
	{ 3072000,  24576000, 0,  0, 160, 9, 1 }, //24576000/8
	{ 4096000,  24576000, 2,  0, 360, 9, 1 }, //24576000/6
	{ 6000000,  24576000, 4,  0, 410, 9, 1 },
	{ 12000000, 24576000, 9,  0, 410, 9, 1 },
	{ 13000000, 24576000, 8,  0, 340, 9, 1 },
	{ 15360000, 24576000, 12, 0, 415, 9, 1 },
	{ 16000000, 24576000, 12, 0, 400, 9, 1 },
	{ 19200000, 24576000, 15, 0, 410, 9, 1 },
	{ 19680000, 24576000, 15, 0, 400, 9, 1 },
	{ 24000000, 24576000, 9,  0, 205, 9, 1 },

	{ 400000,   22579200, 0,  0, 566, 4, 1 },
	{ 512000,   22579200, 0,  0, 880, 9, 1 },
	{ 768000,   22579200, 0,  0, 587, 9, 1 },
	{ 800000,   22579200, 0,  0, 567, 9, 1 },
	{ 1024000,  22579200, 0,  0, 440, 9, 1 },
	{ 1600000,  22579200, 1,  0, 567, 9, 1 },
	{ 2048000,  22579200, 0,  0, 220, 9, 1 },
	{ 3072000,  22579200, 0,  0, 148, 9, 1 },
	{ 4096000,  22579200, 2,  0, 330, 9, 1 },
	{ 6000000,  22579200, 2,  0, 227, 9, 1 },
	{ 12000000, 22579200, 8,  0, 340, 9, 1 },
	{ 13000000, 22579200, 9,  0, 350, 9, 1 },
	{ 15360000, 22579200, 10, 0, 325, 9, 1 },
	{ 16000000, 22579200, 11, 0, 340, 9, 1 },
	{ 19200000, 22579200, 13, 0, 330, 9, 1 },
	{ 19680000, 22579200, 14, 0, 345, 9, 1 },
	{ 24000000, 22579200, 16, 0, 320, 9, 1 },

	{ 12288000, 24576000, 9,  0, 400, 9, 1 }, //24576000/2
	{ 11289600, 22579200, 9,  0, 400, 9, 1 }, //22579200/2

	{ 24576000 / 1,   24576000, 9,  0, 200, 9, 1 }, //24576000
	{ 24576000 / 4,   24576000, 4,  0, 400, 9, 1 }, //6144000
	{ 24576000 / 16,  24576000, 0,  0, 320, 9, 1 }, //1536000
	{ 24576000 / 64,  24576000, 0,  0, 640, 4, 1 }, //384000
	{ 24576000 / 96,  24576000, 0,  0, 960, 4, 1 }, //256000
	{ 24576000 / 128, 24576000, 0,  0, 512, 1, 1 }, //192000
	{ 24576000 / 176, 24576000, 0,  0, 880, 4, 0 }, //140000
	{ 24576000 / 192, 24576000, 0,  0, 960, 4, 0 }, //128000

	{ 22579200 / 1,   22579200, 9,  0, 200, 9, 1 }, //22579200
	{ 22579200 / 4,   22579200, 4,  0, 400, 9, 1 }, //5644800
	{ 22579200 / 16,  22579200, 0,  0, 320, 9, 1 }, //1411200
	{ 22579200 / 64,  22579200, 0,  0, 640, 4, 1 }, //352800
	{ 22579200 / 96,  22579200, 0,  0, 960, 4, 1 }, //235200
	{ 22579200 / 128, 22579200, 0,  0, 512, 1, 1 }, //176400
	{ 22579200 / 176, 22579200, 0,  0, 880, 4, 0 }, //128290
	{ 22579200 / 192, 22579200, 0,  0, 960, 4, 0 }, //117600

	{ 22579200 / 6,   22579200, 2,  0, 360, 9, 1 }, //3763200
	{ 22579200 / 8,   22579200, 0,  0, 160, 9, 1 }, //2822400
	{ 22579200 / 12,  22579200, 0,  0, 240, 9, 1 }, //1881600
	{ 22579200 / 24,  22579200, 0,  0, 480, 9, 1 }, //940800
	{ 22579200 / 32,  22579200, 0,  0, 640, 9, 1 }, //705600
	{ 22579200 / 48,  22579200, 0,  0, 960, 9, 1 }, //470400
};



//AC108 define
#define AC108_CHANNELS_MAX		16		//range[1, 16]
#define AC108_RATES 			(SNDRV_PCM_RATE_8000_96000 | SNDRV_PCM_RATE_KNOT)
#define AC108_FORMATS			(SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE | SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S32_LE)

static const DECLARE_TLV_DB_SCALE(adc1_pga_gain_tlv, 0, 100, 0);
static const DECLARE_TLV_DB_SCALE(adc2_pga_gain_tlv, 0, 100, 0);
static const DECLARE_TLV_DB_SCALE(adc3_pga_gain_tlv, 0, 100, 0);
static const DECLARE_TLV_DB_SCALE(adc4_pga_gain_tlv, 0, 100, 0);

static const DECLARE_TLV_DB_SCALE(ch1_digital_vol_tlv, -11925,75,0);
static const DECLARE_TLV_DB_SCALE(ch2_digital_vol_tlv, -11925,75,0);
static const DECLARE_TLV_DB_SCALE(ch3_digital_vol_tlv, -11925,75,0);
static const DECLARE_TLV_DB_SCALE(ch4_digital_vol_tlv, -11925,75,0);

static const DECLARE_TLV_DB_SCALE(digital_mix_vol_tlv, -600,600,0);

static const DECLARE_TLV_DB_SCALE(channel_enable_tlv, -1500, 100, 0);

/* Analog ADC */
static const char *analog_adc_mux_text[] = {
	"Analog ADC1",
	"Analog ADC2",
	"Analog ADC3",
	"Analog ADC4",
};

/* Channel Mapping */
static const char *channel_map_mux_text[] = {
	"1st adc sample",
	"2st adc sample",
	"3st adc sample",
	"4st adc sample",
};

/*Tx source select channel*/
static const char *channels_src_mux_text[] = {
	"1 channels ",
	"2 channels ",
	"3 channels ",
	"4 channels ",
	"5 channels ",
	"6 channels ",
	"7 channels ",
	"8 channels ",
	"9 channels ",
	"10 channels ",
	"11 channels ",
	"12 channels ",
	"13 channels ",
	"14 channels ",
	"15 channels ",
	"16 channels ",
};

static const  unsigned int ac108_channel_enable_values[] = {
	0x00,
	0x01,
	0x03,
	0x07,
	0x0f,
	0x1f,
	0x3f,
	0x7f,
	0xff,
};

static const char *const ac108_channel_low_enable_texts[] = {
	"disable all",
	"1-1 channels ",
	"1-2 channels ",
	"1-3 channels ",
	"1-4 channels ",
	"1-5 channels ",
	"1-6 channels ",
	"1-7 channels ",
	"1-8 channels ",
};
static const char *const ac108_channel_high_enable_texts[] = {
	"disable all",
	"8-9 channels ",
	"8-10 channels ",
	"8-11 channels ",
	"8-12 channels ",
	"8-13 channels ",
	"8-14 channels ",
	"8-15 channels ",
	"8-16 channels ",
};

static const char *const ac108_data_source_texts[] = {
	"disable all",
	"ADC1 data",
	"ADC2 data",
	"ADC3 data",
	"ADC4 data",
};
static const  unsigned int ac108_data_source_values[] = {
	0x00,
	0x01,
	0x02,
	0x04,
	0x08,
};
static SOC_VALUE_ENUM_SINGLE_DECL(ac108_tx1_channel_low_enum,
								  I2S_TX1_CTRL2, 0, 0xff,
								  ac108_channel_low_enable_texts,
								  ac108_channel_enable_values);

static SOC_VALUE_ENUM_SINGLE_DECL(ac108_tx1_channel_high_enum,
								  I2S_TX1_CTRL3, 0, 0xff,
								  ac108_channel_high_enable_texts,
								  ac108_channel_enable_values);

static SOC_VALUE_ENUM_SINGLE_DECL(ac108_tx2_channel_low_enum,
								  I2S_TX2_CTRL2, 0, 0xff,
								  ac108_channel_low_enable_texts,
								  ac108_channel_enable_values);

static SOC_VALUE_ENUM_SINGLE_DECL(ac108_tx2_channel_high_enum,
								  I2S_TX2_CTRL3, 0, 0xff,
								  ac108_channel_high_enable_texts,
								  ac108_channel_enable_values);
/*0x76: ADC1 Digital Mixer Source Control Register*/
static SOC_VALUE_ENUM_SINGLE_DECL(ac108_adc1_data_src_enum,
								  ADC1_DMIX_SRC, 0, 0x0f,
								  ac108_data_source_texts,
								  ac108_data_source_values);
static SOC_VALUE_ENUM_SINGLE_DECL(ac108_adc1_data_gc_enum,
								  ADC1_DMIX_SRC, ADC1_ADC1_DMXL_GC, 0xf0,
								  ac108_data_source_texts,
								  ac108_data_source_values);
/*0x77: ADC2 Digital Mixer Source Control Register*/
static SOC_VALUE_ENUM_SINGLE_DECL(ac108_adc2_data_src_enum,
								  ADC2_DMIX_SRC, 0, 0x0f,
								  ac108_data_source_texts,
								  ac108_data_source_values);
static SOC_VALUE_ENUM_SINGLE_DECL(ac108_adc2_data_gc_enum,
								  ADC2_DMIX_SRC, ADC2_ADC1_DMXL_GC, 0xf0,
								  ac108_data_source_texts,
								  ac108_data_source_values);
/*0x78: ADC3 Digital Mixer Source Control Register*/
static SOC_VALUE_ENUM_SINGLE_DECL(ac108_adc3_data_src_enum,
								  ADC3_DMIX_SRC, 0, 0x0f,
								  ac108_data_source_texts,
								  ac108_data_source_values);
static SOC_VALUE_ENUM_SINGLE_DECL(ac108_adc3_data_gc_enum,
								  ADC3_DMIX_SRC, ADC3_ADC1_DMXL_GC, 0xf0,
								  ac108_data_source_texts,
								  ac108_data_source_values);
/*0x79: ADC4 Digital Mixer Source Control Register*/
static SOC_VALUE_ENUM_SINGLE_DECL(ac108_adc4_data_src_enum,
								  ADC4_DMIX_SRC, 0, 0x0f,
								  ac108_data_source_texts,
								  ac108_data_source_values);
static SOC_VALUE_ENUM_SINGLE_DECL(ac108_adc4_data_gc_enum,
								  ADC4_DMIX_SRC, ADC4_ADC1_DMXL_GC, 0xf0,
								  ac108_data_source_texts,
								  ac108_data_source_values);

static const struct soc_enum ac108_enum[] = {
	/*0x38:TX1 Channel (slot) number Select for each output*/
	SOC_ENUM_SINGLE(I2S_TX1_CTRL1, TX1_CHSEL, 16, channels_src_mux_text),
	/*0x40:TX1 Channel (slot) number Select for each output*/
	SOC_ENUM_SINGLE(I2S_TX2_CTRL1, TX2_CHSEL, 16, channels_src_mux_text),
	/*0x3c:  TX1 Channel Mapping Control 1*/
	SOC_ENUM_SINGLE(I2S_TX1_CHMP_CTRL1, TX1_CH1_MAP, 4, channel_map_mux_text),
	SOC_ENUM_SINGLE(I2S_TX1_CHMP_CTRL1, TX1_CH2_MAP, 4, channel_map_mux_text),
	SOC_ENUM_SINGLE(I2S_TX1_CHMP_CTRL1, TX1_CH3_MAP, 4, channel_map_mux_text),
	SOC_ENUM_SINGLE(I2S_TX1_CHMP_CTRL1, TX1_CH4_MAP, 4, channel_map_mux_text),

	/*0x3d:  TX1 Channel Mapping Control 2*/
	SOC_ENUM_SINGLE(I2S_TX1_CHMP_CTRL2, TX1_CH5_MAP, 4, channel_map_mux_text),
	SOC_ENUM_SINGLE(I2S_TX1_CHMP_CTRL2, TX1_CH6_MAP, 4, channel_map_mux_text),
	SOC_ENUM_SINGLE(I2S_TX1_CHMP_CTRL2, TX1_CH7_MAP, 4, channel_map_mux_text),
	SOC_ENUM_SINGLE(I2S_TX1_CHMP_CTRL2, TX1_CH8_MAP, 4, channel_map_mux_text),

	/*0x3e:  TX1 Channel Mapping Control 3*/
	SOC_ENUM_SINGLE(I2S_TX1_CHMP_CTRL3, TX1_CH9_MAP, 4, channel_map_mux_text),
	SOC_ENUM_SINGLE(I2S_TX1_CHMP_CTRL3, TX1_CH10_MAP, 4, channel_map_mux_text),
	SOC_ENUM_SINGLE(I2S_TX1_CHMP_CTRL3, TX1_CH11_MAP, 4, channel_map_mux_text),
	SOC_ENUM_SINGLE(I2S_TX1_CHMP_CTRL3, TX1_CH12_MAP, 4, channel_map_mux_text),

	/*0x3f:  TX1 Channel Mapping Control 4*/
	SOC_ENUM_SINGLE(I2S_TX1_CHMP_CTRL4, TX1_CH13_MAP, 4, channel_map_mux_text),
	SOC_ENUM_SINGLE(I2S_TX1_CHMP_CTRL4, TX1_CH14_MAP, 4, channel_map_mux_text),
	SOC_ENUM_SINGLE(I2S_TX1_CHMP_CTRL4, TX1_CH15_MAP, 4, channel_map_mux_text),
	SOC_ENUM_SINGLE(I2S_TX1_CHMP_CTRL4, TX1_CH16_MAP, 4, channel_map_mux_text),

	/*0x44:  TX2 Channel Mapping Control 1*/
	SOC_ENUM_SINGLE(I2S_TX2_CHMP_CTRL1, TX2_CH1_MAP, 4, channel_map_mux_text),
	SOC_ENUM_SINGLE(I2S_TX2_CHMP_CTRL1, TX2_CH2_MAP, 4, channel_map_mux_text),
	SOC_ENUM_SINGLE(I2S_TX2_CHMP_CTRL1, TX2_CH3_MAP, 4, channel_map_mux_text),
	SOC_ENUM_SINGLE(I2S_TX2_CHMP_CTRL1, TX2_CH4_MAP, 4, channel_map_mux_text),

	/*0x45:  TX2 Channel Mapping Control 2*/
	SOC_ENUM_SINGLE(I2S_TX2_CHMP_CTRL2, TX2_CH5_MAP, 4, channel_map_mux_text),
	SOC_ENUM_SINGLE(I2S_TX2_CHMP_CTRL2, TX2_CH6_MAP, 4, channel_map_mux_text),
	SOC_ENUM_SINGLE(I2S_TX2_CHMP_CTRL2, TX2_CH7_MAP, 4, channel_map_mux_text),
	SOC_ENUM_SINGLE(I2S_TX2_CHMP_CTRL2, TX2_CH8_MAP, 4, channel_map_mux_text),

	/*0x46:  TX2 Channel Mapping Control 3*/
	SOC_ENUM_SINGLE(I2S_TX2_CHMP_CTRL3, TX2_CH9_MAP, 4, channel_map_mux_text),
	SOC_ENUM_SINGLE(I2S_TX2_CHMP_CTRL3, TX2_CH10_MAP, 4, channel_map_mux_text),
	SOC_ENUM_SINGLE(I2S_TX2_CHMP_CTRL3, TX2_CH11_MAP, 4, channel_map_mux_text),
	SOC_ENUM_SINGLE(I2S_TX2_CHMP_CTRL3, TX2_CH12_MAP, 4, channel_map_mux_text),

	/*0x47:  TX2 Channel Mapping Control 4*/
	SOC_ENUM_SINGLE(I2S_TX2_CHMP_CTRL4, TX2_CH13_MAP, 4, channel_map_mux_text),
	SOC_ENUM_SINGLE(I2S_TX2_CHMP_CTRL4, TX2_CH14_MAP, 4, channel_map_mux_text),
	SOC_ENUM_SINGLE(I2S_TX2_CHMP_CTRL4, TX2_CH15_MAP, 4, channel_map_mux_text),
	SOC_ENUM_SINGLE(I2S_TX2_CHMP_CTRL4, TX2_CH16_MAP, 4, channel_map_mux_text),

	/*0x63: ADC Digital Source Select Register*/
	SOC_ENUM_SINGLE(ADC_DSR, DIG_ADC4_SRS, 4, analog_adc_mux_text),
	SOC_ENUM_SINGLE(ADC_DSR, DIG_ADC3_SRS, 4, analog_adc_mux_text),
	SOC_ENUM_SINGLE(ADC_DSR, DIG_ADC2_SRS, 4, analog_adc_mux_text),
	SOC_ENUM_SINGLE(ADC_DSR, DIG_ADC1_SRS, 4, analog_adc_mux_text),

};


static const struct snd_kcontrol_new ac108_snd_controls[] = {

	SOC_SINGLE("OUT1 Mute", I2S_FMT_CTRL3, 3, 1, 0),
	SOC_SINGLE("OUT2 Mute", I2S_FMT_CTRL3, 4, 1, 0),

	/*0x39:TX1 Channel1 ~Channel8 (slot) enable*/
	SOC_ENUM("TX1 Channel1~8 enable", ac108_tx1_channel_low_enum),
	/*0x3A:TX1 Channel1 ~Channel8 (slot) enable*/
	SOC_ENUM("TX1 Channel9~16 enable", ac108_tx1_channel_high_enum),
	/*0x41:TX2 Channel1 ~Channel8 (slot) enable*/
	SOC_ENUM("TX2 Channel1~8 enable", ac108_tx2_channel_low_enum),
	/*0x42:TX2 Channel1 ~Channel8 (slot) enable*/
	SOC_ENUM("TX2 Channel9~16 enable", ac108_tx2_channel_high_enum),

	/*0x70: ADC1 Digital Channel Volume Control Register*/
	SOC_SINGLE_TLV("CH1 digital volume", ADC1_DVOL_CTRL, 0, 0xff, 0, ch1_digital_vol_tlv),
	/*0x71: ADC2 Digital Channel Volume Control Register*/
	SOC_SINGLE_TLV("CH2 digital volume", ADC2_DVOL_CTRL, 0, 0xff, 0, ch2_digital_vol_tlv),
	/*0x72: ADC3 Digital Channel Volume Control Register*/
	SOC_SINGLE_TLV("CH3 digital volume", ADC3_DVOL_CTRL, 0, 0xff, 0, ch3_digital_vol_tlv),
	/*0x73: ADC4 Digital Channel Volume Control Register*/
	SOC_SINGLE_TLV("CH4 digital volume", ADC4_DVOL_CTRL, 0, 0xff, 0, ch4_digital_vol_tlv),

	/*0x90: Analog PGA1 Control Register*/
	SOC_SINGLE_TLV("ADC1 PGA gain", ANA_PGA1_CTRL, ADC1_ANALOG_PGA, 0x1f, 0, adc1_pga_gain_tlv),
	/*0x91: Analog PGA2 Control Register*/
	SOC_SINGLE_TLV("ADC2 PGA gain", ANA_PGA2_CTRL, ADC2_ANALOG_PGA, 0x1f, 0, adc2_pga_gain_tlv),
	/*0x92: Analog PGA3 Control Register*/
	SOC_SINGLE_TLV("ADC3 PGA gain", ANA_PGA3_CTRL, ADC3_ANALOG_PGA, 0x1f, 0, adc3_pga_gain_tlv),
	/*0x93: Analog PGA4 Control Register*/
	SOC_SINGLE_TLV("ADC4 PGA gain", ANA_PGA4_CTRL, ADC4_ANALOG_PGA, 0x1f, 0, adc4_pga_gain_tlv),

	/*0x96-0x9F: use the default value*/

	SOC_ENUM("Tx1 Channels", ac108_enum[0]),
	SOC_ENUM("Tx2 Channels", ac108_enum[1]),

	SOC_ENUM("Tx1 Channels 1 MAP", ac108_enum[2]),
	SOC_ENUM("Tx1 Channels 2 MAP", ac108_enum[3]),
	SOC_ENUM("Tx1 Channels 3 MAP", ac108_enum[4]),
	SOC_ENUM("Tx1 Channels 4 MAP", ac108_enum[5]),
	SOC_ENUM("Tx1 Channels 5 MAP", ac108_enum[6]),
	SOC_ENUM("Tx1 Channels 6 MAP", ac108_enum[7]),
	SOC_ENUM("Tx1 Channels 7 MAP", ac108_enum[8]),
	SOC_ENUM("Tx1 Channels 8 MAP", ac108_enum[9]),
	SOC_ENUM("Tx1 Channels 9 MAP", ac108_enum[10]),
	SOC_ENUM("Tx1 Channels 10 MAP", ac108_enum[11]),
	SOC_ENUM("Tx1 Channels 11 MAP", ac108_enum[12]),
	SOC_ENUM("Tx1 Channels 12 MAP", ac108_enum[13]),
	SOC_ENUM("Tx1 Channels 13 MAP", ac108_enum[14]),
	SOC_ENUM("Tx1 Channels 14 MAP", ac108_enum[15]),
	SOC_ENUM("Tx1 Channels 15 MAP", ac108_enum[16]),
	SOC_ENUM("Tx1 Channels 16 MAP", ac108_enum[17]),

	SOC_ENUM("Tx2 Channels 1 MAP", ac108_enum[18]),
	SOC_ENUM("Tx2 Channels 2 MAP", ac108_enum[19]),
	SOC_ENUM("Tx2 Channels 3 MAP", ac108_enum[20]),
	SOC_ENUM("Tx2 Channels 4 MAP", ac108_enum[21]),
	SOC_ENUM("Tx2 Channels 5 MAP", ac108_enum[22]),
	SOC_ENUM("Tx2 Channels 6 MAP", ac108_enum[23]),
	SOC_ENUM("Tx2 Channels 7 MAP", ac108_enum[24]),
	SOC_ENUM("Tx2 Channels 8 MAP", ac108_enum[25]),
	SOC_ENUM("Tx2 Channels 9 MAP", ac108_enum[26]),
	SOC_ENUM("Tx2 Channels 10 MAP", ac108_enum[27]),
	SOC_ENUM("Tx2 Channels 11 MAP", ac108_enum[28]),
	SOC_ENUM("Tx2 Channels 12 MAP", ac108_enum[29]),
	SOC_ENUM("Tx2 Channels 13 MAP", ac108_enum[30]),
	SOC_ENUM("Tx2 Channels 14 MAP", ac108_enum[31]),
	SOC_ENUM("Tx2 Channels 15 MAP", ac108_enum[32]),
	SOC_ENUM("Tx2 Channels 16 MAP", ac108_enum[33]),

	SOC_ENUM("ADC4 Source", ac108_enum[34]),
	SOC_ENUM("ADC3 Source", ac108_enum[35]),
	SOC_ENUM("ADC2 Source", ac108_enum[36]),
	SOC_ENUM("ADC1 Source", ac108_enum[37]),

	SOC_ENUM("ADC1 Digital Mixer gc", ac108_adc1_data_gc_enum),
	SOC_ENUM("ADC1 Digital Mixer src", ac108_adc1_data_src_enum),

	SOC_ENUM("ADC2 Digital Mixer gc", ac108_adc2_data_gc_enum),
	SOC_ENUM("ADC2 Digital Mixer src", ac108_adc2_data_src_enum),

	SOC_ENUM("ADC3 Digital Mixer gc", ac108_adc3_data_gc_enum),
	SOC_ENUM("ADC3 Digital Mixer src", ac108_adc3_data_src_enum),

	SOC_ENUM("ADC4 Digital Mixer gc", ac108_adc4_data_gc_enum),
	SOC_ENUM("ADC4 Digital Mixer src", ac108_adc4_data_src_enum),
};


static const struct snd_soc_dapm_widget ac108_dapm_widgets[] = {
	//input widgets
	SND_SOC_DAPM_INPUT("MIC1P"),
	SND_SOC_DAPM_INPUT("MIC1N"),

	SND_SOC_DAPM_INPUT("MIC2P"),
	SND_SOC_DAPM_INPUT("MIC2N"),

	SND_SOC_DAPM_INPUT("MIC3P"),
	SND_SOC_DAPM_INPUT("MIC3N"),

	SND_SOC_DAPM_INPUT("MIC4P"),
	SND_SOC_DAPM_INPUT("MIC4N"),

	SND_SOC_DAPM_INPUT("DMIC1"),
	SND_SOC_DAPM_INPUT("DMIC2"),

	/*0xa0: ADC1 Analog Control 1 Register*/
	/*0xa1-0xa6:use the defualt value*/
	SND_SOC_DAPM_AIF_IN("Channel 1 AAF", "Capture", 0, ANA_ADC1_CTRL1, ADC1_DSM_ENABLE, 1),
	SND_SOC_DAPM_SUPPLY("Channel 1 EN", ANA_ADC1_CTRL1, ADC1_PGA_ENABLE, 1, NULL, 0),
	SND_SOC_DAPM_MICBIAS("MIC1BIAS", ANA_ADC1_CTRL1, ADC1_MICBIAS_EN, 1),

	/*0xa7: ADC2 Analog Control 1 Register*/
	/*0xa8-0xad:use the defualt value*/
	SND_SOC_DAPM_AIF_IN("Channel 2 AAF", "Capture", 0, ANA_ADC2_CTRL1, ADC2_DSM_ENABLE, 1),
	SND_SOC_DAPM_SUPPLY("Channel 2 EN", ANA_ADC2_CTRL1, ADC2_PGA_ENABLE, 1, NULL, 0),
	SND_SOC_DAPM_MICBIAS("MIC2BIAS", ANA_ADC2_CTRL1, ADC2_MICBIAS_EN, 1),

	/*0xae: ADC3 Analog Control 1 Register*/
	/*0xaf-0xb4:use the defualt value*/
	SND_SOC_DAPM_AIF_IN("Channel 3 AAF", "Capture", 0, ANA_ADC3_CTRL1, ADC3_DSM_ENABLE, 1),
	SND_SOC_DAPM_SUPPLY("Channel 3 EN", ANA_ADC3_CTRL1, ADC3_PGA_ENABLE, 1, NULL, 0),
	SND_SOC_DAPM_MICBIAS("MIC3BIAS", ANA_ADC3_CTRL1, ADC3_MICBIAS_EN, 1),

	/*0xb5: ADC4 Analog Control 1 Register*/
	/*0xb6-0xbb:use the defualt value*/
	SND_SOC_DAPM_AIF_IN("Channel 4 AAF", "Capture", 0, ANA_ADC4_CTRL1, ADC4_DSM_ENABLE, 1),
	SND_SOC_DAPM_SUPPLY("Channel 4 EN", ANA_ADC4_CTRL1, ADC4_PGA_ENABLE, 1, NULL, 0),
	SND_SOC_DAPM_MICBIAS("MIC4BIAS", ANA_ADC4_CTRL1, ADC4_MICBIAS_EN, 1),


	/*0x61: ADC Digital Part Enable Register*/
	SND_SOC_DAPM_SUPPLY("ADC EN", ADC_DIG_EN, 4,  1, NULL, 0),
	SND_SOC_DAPM_ADC("ADC1", "Capture", ADC_DIG_EN, 0,  1),
	SND_SOC_DAPM_ADC("ADC2", "Capture", ADC_DIG_EN, 1,  1),
	SND_SOC_DAPM_ADC("ADC3", "Capture", ADC_DIG_EN, 2,  1),
	SND_SOC_DAPM_ADC("ADC4", "Capture", ADC_DIG_EN, 3,  1),

	SND_SOC_DAPM_SUPPLY("ADC1 CLK", ANA_ADC4_CTRL7, ADC1_CLK_GATING, 1, NULL, 0),
	SND_SOC_DAPM_SUPPLY("ADC2 CLK", ANA_ADC4_CTRL7, ADC2_CLK_GATING, 1, NULL, 0),
	SND_SOC_DAPM_SUPPLY("ADC3 CLK", ANA_ADC4_CTRL7, ADC3_CLK_GATING, 1, NULL, 0),
	SND_SOC_DAPM_SUPPLY("ADC4 CLK", ANA_ADC4_CTRL7, ADC4_CLK_GATING, 1, NULL, 0),

	SND_SOC_DAPM_SUPPLY("DSM EN", ANA_ADC4_CTRL6, DSM_DEMOFF, 1, NULL, 0),

	/*0x62:Digital MIC Enable Register*/
	SND_SOC_DAPM_MICBIAS("DMIC1 enable", DMIC_EN, 0, 0),
	SND_SOC_DAPM_MICBIAS("DMIC2 enable", DMIC_EN, 1, 0),

	/**/
};

static const struct snd_soc_dapm_route ac108_dapm_routes[] = {

	{ "ADC1", NULL, "Channel 1 AAF" },
	{ "ADC2", NULL, "Channel 2 AAF" },
	{ "ADC3", NULL, "Channel 3 AAF" },
	{ "ADC4", NULL, "Channel 4 AAF" },

	{ "Channel 1 AAF", NULL, "MIC1BIAS" },
	{ "Channel 2 AAF", NULL, "MIC2BIAS" },
	{ "Channel 3 AAF", NULL, "MIC3BIAS" },
	{ "Channel 4 AAF", NULL, "MIC4BIAS" },

	{ "MIC1BIAS", NULL, "ADC1 CLK" },
	{ "MIC2BIAS", NULL, "ADC2 CLK" },
	{ "MIC3BIAS", NULL, "ADC3 CLK" },
	{ "MIC4BIAS", NULL, "ADC4 CLK" },


	{ "ADC1 CLK", NULL, "DSM EN" },
	{ "ADC2 CLK", NULL, "DSM EN" },
	{ "ADC3 CLK", NULL, "DSM EN" },
	{ "ADC4 CLK", NULL, "DSM EN" },


	{ "DSM EN", NULL, "ADC EN" },

	{ "Channel 1 EN", NULL, "DSM EN" },
	{ "Channel 2 EN", NULL, "DSM EN" },
	{ "Channel 3 EN", NULL, "DSM EN" },
	{ "Channel 4 EN", NULL, "DSM EN" },


	{ "MIC1P", NULL, "Channel 1 EN" },
	{ "MIC1N", NULL, "Channel 1 EN" },

	{ "MIC2P", NULL, "Channel 2 EN" },
	{ "MIC2N", NULL, "Channel 2 EN" },

	{ "MIC3P", NULL, "Channel 3 EN" },
	{ "MIC3N", NULL, "Channel 3 EN" },

	{ "MIC4P", NULL, "Channel 4 EN" },
	{ "MIC4N", NULL, "Channel 4 EN" },

};
static int ac108_read(u8 reg, u8 *rt_value, struct i2c_client *client) {
	int ret;
	u8 read_cmd[3] = { reg, 0, 0 };
	u8 cmd_len = 1;

	ret = i2c_master_send(client, read_cmd, cmd_len);
	if (ret != cmd_len) {
		pr_err("ac108_read error1\n");
		return -1;
	}
	ret = i2c_master_recv(client, rt_value, 1);
	if (ret != 1) {
		pr_err("ac108_read error2, ret = %d.\n", ret);
		return -1;
	}

	return 0;
}

static int ac108_write(u8 reg, unsigned char val, struct i2c_client *client) {
	int ret = 0;
	u8 write_cmd[2] = { reg, val };

	ret = i2c_master_send(client, write_cmd, 2);
	if (ret != 2) {
		pr_err("ac108_write error->[REG-0x%02x,val-0x%02x]\n", reg, val);
		return -1;
	}
	return 0;
}

static int ac108_update_bits(u8 reg, u8 mask, u8 val, struct i2c_client *client) {
	u8 val_old, val_new;

	ac108_read(reg, &val_old, client);
	val_new = (val_old & ~mask) | (val & mask);
	if (val_new != val_old) {
		ac108_write(reg, val_new, client);
	}

	return 0;
}

static int ac108_multi_chips_read(u8 reg, u8 *rt_value, struct ac108_priv *ac108) {
	u8 i;
	for (i = 0; i < ac108->codec_index; i++) {
		ac108_read(reg, rt_value++, ac108->i2c[i]);
	}

	return 0;
}


static int ac108_multi_chips_write(u8 reg, u8 val, struct ac108_priv *ac108) {
	u8 i;
	for (i = 0; i < ac108->codec_index; i++) {
		ac108_write(reg, val, ac108->i2c[i]);
	}
	return 0;
}

static int ac108_multi_chips_update_bits(u8 reg, u8 mask, u8 val, struct ac108_priv *ac108) {
	u8 i;
	for (i = 0; i < ac108->codec_index; i++) {
		ac108_update_bits(reg, mask, val, ac108->i2c[i]);
	}
	return 0;
}

static unsigned int ac108_codec_read(struct snd_soc_codec *codec, unsigned int reg) {
	unsigned char val_r;
	struct ac108_priv *ac108 = dev_get_drvdata(codec->dev);
	/*read one chip is fine*/
	ac108_read(reg, &val_r, ac108->i2c[0]);
	return val_r;
}

static int ac108_codec_write(struct snd_soc_codec *codec, unsigned int reg, unsigned int val) {
	struct ac108_priv *ac108 = dev_get_drvdata(codec->dev);
	ac108_multi_chips_write(reg, val, ac108);
	return 0;
}

/**
 * The Power management related registers are Reg01h~Reg09h
 * 0x01-0x05,0x08,use the default value
 * @author baozhu (17-6-21)
 * 
 * @param ac108 
 */
static void ac108_configure_power(struct ac108_priv *ac108) {
	/**
	 * 0x06:Enable Analog LDO
	 */
	ac108_multi_chips_update_bits(PWR_CTRL6, 0x01 << LDO33ANA_ENABLE, 0x01 << LDO33ANA_ENABLE, ac108);
	/**
	 * 0x07: 
	 * Control VREF output and micbias voltage ? 
	 * REF faststart disable, enable Enable VREF (needed for Analog 
	 * LDO and MICBIAS) 
	 */
	ac108_multi_chips_update_bits(PWR_CTRL7, 0x1f << VREF_SEL | 0x01 << VREF_FASTSTART_ENABLE | 0x01 << VREF_ENABLE,
								  0x13 << VREF_SEL | 0x00 << VREF_FASTSTART_ENABLE | 0x01 << VREF_ENABLE, ac108);
	/**
	 * 0x09: 
	 * Disable fast-start circuit on VREFP 
	 * VREFP_RESCTRL=00=1 MOhm 
	 * IGEN_TRIM=100=+25% 
	 * Enable VREFP (needed by all audio input channels) 
	 */
	ac108_multi_chips_update_bits(PWR_CTRL9, 0x01 << VREFP_FASTSTART_ENABLE | 0x03 << VREFP_RESCTRL |
								  0x07 << IGEN_TRIM | 0x01 << VREFP_ENABLE,
								  0x00 << VREFP_FASTSTART_ENABLE | 0x00 << VREFP_RESCTRL |
								  0x04 << IGEN_TRIM | 0x01 << VREFP_ENABLE, ac108);
}

/**
 * The clock management related registers are Reg20h~Reg25h
 * The PLL management related registers are Reg10h~Reg18h.
 * @author baozhu (17-6-20)
 * 
 * @param ac108 
 * @param rate : sample rate
 * 
 * @return int : fail or success
 */
static int ac108_configure_clocking(struct ac108_priv *ac108, unsigned int rate) {
	unsigned int i = 0;
	struct pll_div ac108_pll_div = { 0 };
	if (ac108->clk_id == SYSCLK_SRC_PLL) {
		/* FOUT =(FIN * N) / [(M1+1) * (M2+1)*(K1+1)*(K2+1)] */
		for (i = 0; i < ARRAY_SIZE(ac108_pll_div_list); i++) {
			if (ac108_pll_div_list[i].freq_in == ac108->sysclk && ac108_pll_div_list[i].freq_out % rate == 0) {
				ac108_pll_div = ac108_pll_div_list[i];
				pr_err("AC108 PLL freq_in match:%u, freq_out:%u\n\n", ac108_pll_div.freq_in, ac108_pll_div.freq_out);
				break;
			}
		}
		/* 0x11,0x12,0x13,0x14: Config PLL DIV param M1/M2/N/K1/K2 */
		ac108_multi_chips_update_bits(PLL_CTRL5, 0x1f << PLL_POSTDIV1 | 0x01 << PLL_POSTDIV2, ac108_pll_div.k1 << PLL_POSTDIV1 |
									  ac108_pll_div.k2 << PLL_POSTDIV2, ac108);
		ac108_multi_chips_update_bits(PLL_CTRL4, 0xff << PLL_LOOPDIV_LSB, (unsigned char)ac108_pll_div.n << PLL_LOOPDIV_LSB, ac108);
		ac108_multi_chips_update_bits(PLL_CTRL3, 0x03 << PLL_LOOPDIV_MSB, (ac108_pll_div.n >> 8) << PLL_LOOPDIV_MSB, ac108);
		ac108_multi_chips_update_bits(PLL_CTRL2, 0x1f << PLL_PREDIV1 | 0x01 << PLL_PREDIV2,
									  ac108_pll_div.m1 << PLL_PREDIV1 | ac108_pll_div.m2 << PLL_PREDIV2, ac108);

		/*0x18: PLL clk lock enable*/
		ac108_multi_chips_update_bits(PLL_LOCK_CTRL, 0x1 << PLL_LOCK_EN, 0x1 << PLL_LOCK_EN, ac108);
		/*0x10: PLL Common voltage Enable, PLL Enable,PLL loop divider factor detection enable*/
		ac108_multi_chips_update_bits(PLL_CTRL1, 0x01 << PLL_EN | 0x01 << PLL_COM_EN | 0x01 << PLL_NDET,
									  0x1 << PLL_EN | 0x1 << PLL_COM_EN |  0x01 << PLL_NDET, ac108);

		/**
		 * 0x20: enable pll,pll source from mclk, sysclk source from 
		 * pll,enable sysclk 
		 */
		ac108_multi_chips_update_bits(SYSCLK_CTRL, 0x01 << PLLCLK_EN | 0x03 << PLLCLK_SRC | 0x01 << SYSCLK_SRC | 0x01 << SYSCLK_EN,
									  0x01 << PLLCLK_EN | 0x00 << PLLCLK_SRC | 0x01 << SYSCLK_SRC | 0x01 << SYSCLK_EN, ac108);
	}
	if (ac108->clk_id == SYSCLK_SRC_MCLK) {
		/**
		 *0x20: sysclk source from  mclk,enable sysclk 
		 */
		ac108_multi_chips_update_bits(SYSCLK_CTRL, 0x01 << PLLCLK_EN | 0x01 << SYSCLK_SRC | 0x01 << SYSCLK_EN,
									  0x00 << PLLCLK_EN | 0x00 << SYSCLK_SRC | 0x01 << SYSCLK_EN, ac108);
	}
	/*0x21: Module clock enable<I2S, ADC digital, MIC offset Calibration, ADC analog>*/
	ac108_multi_chips_write(MOD_CLK_EN, 1 << I2S | 1 << ADC_DIGITAL | 1 << MIC_OFFSET_CALIBRATION | 1 << ADC_ANALOG, ac108);
	/*0x22: Module reset de-asserted<I2S, ADC digital, MIC offset Calibration, ADC analog>*/
	ac108_multi_chips_write(MOD_RST_CTRL, 1 << I2S | 1 << ADC_DIGITAL | 1 << MIC_OFFSET_CALIBRATION | 1 << ADC_ANALOG, ac108);
	return 0;
}

static int ac108_hw_params(struct snd_pcm_substream *substream, struct snd_pcm_hw_params *params, struct snd_soc_dai *dai) {
	unsigned int i, channels, sample_resolution, rate;
	struct snd_soc_codec *codec = dai->codec;
	struct ac108_priv *ac108 = snd_soc_codec_get_drvdata(codec);
	rate = 99;
	dev_dbg(dai->dev, "%s\n", __FUNCTION__);

	channels = params_channels(params);

	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S8:
		sample_resolution = 0;
		break;
	case SNDRV_PCM_FORMAT_S16_LE:
		sample_resolution = 2;
		break;
	case SNDRV_PCM_FORMAT_S20_3LE:
		sample_resolution = 3;
		break;
	case SNDRV_PCM_FORMAT_S24_LE:
		sample_resolution = 4;
		break;
	case SNDRV_PCM_FORMAT_S32_LE:
		sample_resolution = 6;
		break;
	default:
		pr_err("AC108 don't supported the sample resolution: %u\n", params_format(params));
		return -EINVAL;
	}

	dev_dbg(dai->dev,"rate:%d \n", params_rate(params));
	for (i = 0; i < ARRAY_SIZE(ac108_sample_rate); i++) {
		if (ac108_sample_rate[i].real_val == params_rate(params)) {
			rate = i;
			break;
		}
	}
	if (rate == 99) return -EINVAL;


	dev_dbg(dai->dev, "rate: %d , channels: %d , sample_resolution: %d",
			ac108_sample_rate[rate].real_val,
			channels,
			ac108_sample_resolution[sample_resolution].real_val);

	/**
	 * 0x33: 
	 *  The 8-Low bit of LRCK period value. It is used to program
	 *  the number of BCLKs per channel of sample frame. This value
	 *  is interpreted as follow:
	 *  The 8-Low bit of LRCK period value. It is used to program
	 *  the number of BCLKs per channel of sample frame. This value
	 *  is interpreted as follow: PCM mode: Number of BCLKs within
	 *  (Left + Right) channel width I2S / Left-Justified /
	 *  Right-Justified mode: Number of BCLKs within each individual
	 *  channel width (Left or Right) N+1
	 *  For example:
	 *  n = 7: 8 BCLK width
	 *  …
	 *  n = 1023: 1024 BCLKs width
	 *  0X32[0:1]:
	 *  The 2-High bit of LRCK period value. 
	 */
	if (ac108->i2s_mode != PCM_FORMAT) {
		if (ac108->data_protocol) {
			ac108_multi_chips_write(I2S_LRCK_CTRL2, ac108_sample_resolution[sample_resolution].real_val - 1, ac108);
			/*encoding mode, the max LRCK period value < 32,so the 2-High bit is zero*/
			ac108_multi_chips_update_bits(I2S_LRCK_CTRL1, 0x03 << 0, 0x00, ac108);
		} else {
			/*TDM mode or normal mode*/
			/**
			 * TODO: need test.
			 */
		}

	} else {
		/**
		 * TODO: need test.
		 */
	}

	/**
	 * 0x35: 
	 * TX Encoding mode will add  4bits to mark channel number 
	 * TODO: need a chat to explain this 
	 */
	ac108_multi_chips_update_bits(I2S_FMT_CTRL2, 0x07 << SAMPLE_RESOLUTION | 0x07 << SLOT_WIDTH_SEL,
								  ac108_sample_resolution[sample_resolution].reg_val << SAMPLE_RESOLUTION
								  | ac108_sample_resolution[sample_resolution].reg_val << SLOT_WIDTH_SEL, ac108);

	/**
	 * 0x60: 
	 * ADC Sample Rate synchronised with I2S1 clock zone 
	 */
	ac108_multi_chips_update_bits(ADC_SPRC, 0x0f << ADC_FS_I2S1, ac108_sample_rate[rate].reg_val << ADC_FS_I2S1, ac108);

	ac108_configure_clocking(ac108, rate);
	return 0;
}

static int ac108_set_sysclk(struct snd_soc_dai *dai, int clk_id, unsigned int freq, int dir) {

	struct ac108_priv *ac108 = snd_soc_dai_get_drvdata(dai);

	pr_info("%s  :%d\n", __FUNCTION__, freq);
	switch (clk_id) {
	case SYSCLK_SRC_MCLK:
		ac108_multi_chips_update_bits(SYSCLK_CTRL, 0x1 << SYSCLK_SRC, SYSCLK_SRC_MCLK << SYSCLK_SRC, ac108);
		break;
	case SYSCLK_SRC_PLL:
		ac108_multi_chips_update_bits(SYSCLK_CTRL, 0x1 << SYSCLK_SRC, SYSCLK_SRC_PLL << SYSCLK_SRC, ac108);
		break;
	default:
		return -EINVAL;
	}
	ac108->sysclk = freq;
	ac108->clk_id = clk_id;

	return 0;
}

/**
 *  The i2s format management related registers are Reg
 *  30h~Reg36h
 *  33h,35h will be set in ac108_hw_params, It's BCLK width and
 *  Sample Resolution.
 * @author baozhu (17-6-20)
 * 
 * @param dai 
 * @param fmt 
 * 
 * @return int 
 */
static int ac108_set_fmt(struct snd_soc_dai *dai, unsigned int fmt) {

	unsigned char tx_offset, lrck_polarity, brck_polarity;
	struct ac108_priv *ac108 = dev_get_drvdata(dai->dev);
	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBM_CFM:    /*AC108 Master*/
		dev_dbg(dai->dev, "AC108 set to work as Master\n");
		/**
		 * 0x30:chip is master mode ,BCLK & LRCK output
		 */
		ac108_multi_chips_update_bits(I2S_CTRL, 0x03 << LRCK_IOEN, 0x03 << LRCK_IOEN, ac108);
		/* multi_chips: only one chip set as Master, and the others also need to set as Slave */
		if (ac108->codec_index > 1) ac108_update_bits(I2S_CTRL, 0x3 << LRCK_IOEN, 0x0 << LRCK_IOEN, ac108->i2c[1]);

		break;
	case SND_SOC_DAIFMT_CBS_CFS:    /*AC108 Slave*/
		dev_dbg(dai->dev, "AC108 set to work as Slave\n");
		/**
		 * 0x30:chip is slave mode, BCLK & LRCK input,enable SDO1_EN and 
		 *  SDO2_EN, Transmitter Block Enable, Globe Enable
		 */
		ac108_multi_chips_update_bits(I2S_CTRL, 0x03 << LRCK_IOEN | 0x03 << SDO1_EN | 0x1 << TXEN | 0x1 << GEN,
									  0x00 << LRCK_IOEN | 0x03 << SDO1_EN | 0x1 << TXEN | 0x1 << GEN, ac108);
		break;
	default:
		pr_err("AC108 Master/Slave mode config error:%u\n\n", (fmt & SND_SOC_DAIFMT_MASTER_MASK) >> 12);
		return -EINVAL;
	}

	/*AC108 config I2S/LJ/RJ/PCM format*/
	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		dev_dbg(dai->dev, "AC108 config I2S format\n");
		ac108->i2s_mode = LEFT_JUSTIFIED_FORMAT;
		tx_offset = 1;
		break;
	case SND_SOC_DAIFMT_RIGHT_J:
		dev_dbg(dai->dev, "AC108 config RIGHT-JUSTIFIED format\n");
		ac108->i2s_mode = RIGHT_JUSTIFIED_FORMAT;
		tx_offset = 0;
		break;
	case SND_SOC_DAIFMT_LEFT_J:
		dev_dbg(dai->dev, "AC108 config LEFT-JUSTIFIED format\n");
		ac108->i2s_mode = LEFT_JUSTIFIED_FORMAT;
		tx_offset = 0;
		break;
	case SND_SOC_DAIFMT_DSP_A:
		dev_dbg(dai->dev, "AC108 config PCM-A format\n");
		ac108->i2s_mode = PCM_FORMAT;
		tx_offset = 1;
		break;
	case SND_SOC_DAIFMT_DSP_B:
		dev_dbg(dai->dev, "AC108 config PCM-B format\n");
		ac108->i2s_mode = PCM_FORMAT;
		tx_offset = 0;
		break;
	default:
		ac108->i2s_mode = LEFT_JUSTIFIED_FORMAT;
		tx_offset = 1;
		return -EINVAL;
		pr_err("AC108 I2S format config error:%u\n\n", fmt & SND_SOC_DAIFMT_FORMAT_MASK);
	}
	/*AC108 config BCLK&LRCK polarity*/
	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
		dev_dbg(dai->dev, "AC108 config BCLK&LRCK polarity: BCLK_normal,LRCK_normal\n");
		brck_polarity = BCLK_NORMAL_DRIVE_N_SAMPLE_P;
		lrck_polarity = LRCK_LEFT_LOW_RIGHT_HIGH;
		break;
	case SND_SOC_DAIFMT_NB_IF:
		dev_dbg(dai->dev, "AC108 config BCLK&LRCK polarity: BCLK_normal,LRCK_invert\n");
		brck_polarity = BCLK_NORMAL_DRIVE_N_SAMPLE_P;
		lrck_polarity = LRCK_LEFT_HIGH_RIGHT_LOW;
		break;
	case SND_SOC_DAIFMT_IB_NF:
		dev_dbg(dai->dev, "AC108 config BCLK&LRCK polarity: BCLK_invert,LRCK_normal\n");
		brck_polarity = BCLK_INVERT_DRIVE_P_SAMPLE_N;
		lrck_polarity = LRCK_LEFT_LOW_RIGHT_HIGH;
		break;
	case SND_SOC_DAIFMT_IB_IF:
		dev_dbg(dai->dev, "AC108 config BCLK&LRCK polarity: BCLK_invert,LRCK_invert\n");
		brck_polarity = BCLK_INVERT_DRIVE_P_SAMPLE_N;
		lrck_polarity = LRCK_LEFT_HIGH_RIGHT_LOW;
		break;
	default:
		pr_err("AC108 config BCLK/LRCLK polarity error:%u\n\n", (fmt & SND_SOC_DAIFMT_INV_MASK) >> 8);
		return -EINVAL;
	}
	ac108_configure_power(ac108);

	/**
	 *0x31: 0: normal mode, negative edge drive and positive edge sample
			1: invert mode, positive edge drive and negative edge sample
	 */
	ac108_multi_chips_update_bits(I2S_BCLK_CTRL,  0x01 << BCLK_POLARITY, brck_polarity << BCLK_POLARITY, ac108);
	/**
	 * 0x32: same as 0x31
	 */
	ac108_multi_chips_update_bits(I2S_LRCK_CTRL1, 0x01 << LRCK_POLARITY, lrck_polarity << LRCK_POLARITY, ac108);
	/**
	 * 0x34:Encoding Mode Selection,Mode 
	 * Selection,data is offset by 1 BCLKs to LRCK 
	 * normal mode for the last half cycle of BCLK in the slot ?
	 * turn to hi-z state (TDM) when not transferring slot ?
	 */
	ac108_multi_chips_update_bits(I2S_FMT_CTRL1, 0x01 << ENCD_SEL | 0x03 << MODE_SEL | 0x01 << TX2_OFFSET |
								  0x01 << TX1_OFFSET | 0x01 << TX_SLOT_HIZ | 0x01 << TX_STATE,
								  ac108->data_protocol << ENCD_SEL 	|
								  ac108->i2s_mode << MODE_SEL 		|
								  tx_offset << TX2_OFFSET 			|
								  tx_offset << TX1_OFFSET 			|
								  0x00 << TX_SLOT_HIZ 				|
								  0x01 << TX_STATE, ac108);

	/**
	 * 0x60: 
	 * MSB / LSB First Select: This driver only support MSB First 
	 * Select . 
	 * OUT2_MUTE,OUT1_MUTE shoule be set in widget. 
	 * LRCK = 1 BCLK width 
	 * Linear PCM 
	 *  
	 * TODO:pcm mode, bit[0:1] and bit[2] is special
	 */
	ac108_multi_chips_update_bits(I2S_FMT_CTRL3, 0x01 << TX_MLS | 0x03 << SEXT  | 0x01 << LRCK_WIDTH | 0x03 << TX_PDM,
								  0x00 << TX_MLS | 0x03 << SEXT  | 0x00 << LRCK_WIDTH | 0x00 << TX_PDM, ac108);

	/**???*/
	ac108_multi_chips_write(HPF_EN,0x00,ac108);
	return 0;
}

static const struct snd_soc_dai_ops ac108_dai_ops = {
	/*DAI clocking configuration*/
	.set_sysclk = ac108_set_sysclk,


	/*ALSA PCM audio operations*/
	.hw_params = ac108_hw_params,
//	.trigger = ac108_trigger,
//	.hw_free = ac108_hw_free,
//
//	/*DAI format configuration*/
	.set_fmt = ac108_set_fmt,
};


static const struct regmap_config ac108_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.max_register = PRNG_CLK_CTRL,
	.cache_type = REGCACHE_RBTREE,
};
static  struct snd_soc_dai_driver ac108_dai0 = {
	.name = "ac108-codec0",
	.capture = {
		.stream_name = "Capture",
		.channels_min = 1,
		.channels_max = AC108_CHANNELS_MAX,
		.rates = AC108_RATES,
		.formats = AC108_FORMATS,
	},
	.ops = &ac108_dai_ops,
};


static  struct snd_soc_dai_driver ac108_dai1 = {
	.name = "ac108-codec1",
	.capture = {
		.stream_name = "Capture",
		.channels_min = 1,
		.channels_max = AC108_CHANNELS_MAX,
		.rates = AC108_RATES,
		.formats = AC108_FORMATS,
	},
	.ops = &ac108_dai_ops,
};

static  struct snd_soc_dai_driver ac108_dai2 = {
	.name = "ac108-codec2",
	.capture = {
		.stream_name = "Capture",
		.channels_min = 1,
		.channels_max = AC108_CHANNELS_MAX,
		.rates = AC108_RATES,
		.formats = AC108_FORMATS,
	},
	.ops = &ac108_dai_ops,
};

static  struct snd_soc_dai_driver ac108_dai3 = {
	.name = "ac108-codec3",
	.capture = {
		.stream_name = "Capture",
		.channels_min = 1,
		.channels_max = AC108_CHANNELS_MAX,
		.rates = AC108_RATES,
		.formats = AC108_FORMATS,
	},
	.ops = &ac108_dai_ops,
};

static  struct snd_soc_dai_driver *ac108_dai[] = {
	&ac108_dai0,

	&ac108_dai1,

	&ac108_dai2,

	&ac108_dai3,
};

static int ac108_add_widgets(struct snd_soc_codec *codec) {
	struct snd_soc_dapm_context *dapm = snd_soc_codec_get_dapm(codec);

	snd_soc_add_codec_controls(codec, ac108_snd_controls,
							   ARRAY_SIZE(ac108_snd_controls));


	snd_soc_dapm_new_controls(dapm, ac108_dapm_widgets,
							  ARRAY_SIZE(ac108_dapm_widgets));

	snd_soc_dapm_add_routes(dapm, ac108_dapm_routes, ARRAY_SIZE(ac108_dapm_routes));

	return 0;
}
static int ac108_probe(struct snd_soc_codec *codec) {

	dev_set_drvdata(codec->dev, ac108);
	ac108_add_widgets(codec);

	return 0;
}


static int ac108_set_bias_level(struct snd_soc_codec *codec,
								enum snd_soc_bias_level level) {
	struct ac108_priv *ac108 = snd_soc_codec_get_drvdata(codec);
	dev_dbg(codec->dev, "AC108 level:%d\n", level);
	switch (level) {
	case SND_SOC_BIAS_ON:
		ac108_multi_chips_update_bits(ANA_ADC1_CTRL1, 0x01 << ADC1_MICBIAS_EN,  0x01 << ADC1_MICBIAS_EN, ac108);
		ac108_multi_chips_update_bits(ANA_ADC2_CTRL1, 0x01 << ADC2_MICBIAS_EN,  0x01 << ADC2_MICBIAS_EN, ac108);
		ac108_multi_chips_update_bits(ANA_ADC3_CTRL1, 0x01 << ADC3_MICBIAS_EN,  0x01 << ADC3_MICBIAS_EN, ac108);
		ac108_multi_chips_update_bits(ANA_ADC4_CTRL1, 0x01 << ADC4_MICBIAS_EN,  0x01 << ADC4_MICBIAS_EN, ac108);
		break;
	case SND_SOC_BIAS_PREPARE:
		/* Put the MICBIASes into regulating mode */
		break;

	case SND_SOC_BIAS_STANDBY:

		break;

	case SND_SOC_BIAS_OFF:
		ac108_multi_chips_update_bits(ANA_ADC1_CTRL1, 0x01 << ADC1_MICBIAS_EN,  0x00 << ADC1_MICBIAS_EN, ac108);
		ac108_multi_chips_update_bits(ANA_ADC2_CTRL1, 0x01 << ADC2_MICBIAS_EN,  0x00 << ADC2_MICBIAS_EN, ac108);
		ac108_multi_chips_update_bits(ANA_ADC3_CTRL1, 0x01 << ADC3_MICBIAS_EN,  0x00 << ADC3_MICBIAS_EN, ac108);
		ac108_multi_chips_update_bits(ANA_ADC4_CTRL1, 0x01 << ADC4_MICBIAS_EN,  0x00 << ADC4_MICBIAS_EN, ac108);
		break;
	}

	return 0;
}


static const struct snd_soc_codec_driver ac108_soc_codec_driver = {
	.probe = ac108_probe,
	.set_bias_level = ac108_set_bias_level,
	.read = ac108_codec_read,
	.write = ac108_codec_write,
};


static ssize_t ac108_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {
	int val = 0, flag = 0;
	u8 i = 0, reg, num, value_w, value_r;

	val = simple_strtol(buf, NULL, 16);
	flag = (val >> 16) & 0xF;

	if (flag) {
		reg = (val >> 8) & 0xFF;
		value_w = val & 0xFF;
		ac108_multi_chips_write(reg, value_w, ac108);
		printk("Write 0x%02x to REG:0x%02x\n", value_w, reg);
	} else {
		reg = (val >> 8) & 0xFF;
		num = val & 0xff;
		printk("\nRead: start REG:0x%02x,count:0x%02x\n", reg, num);

		do {
			value_r = 0;
			ac108_multi_chips_read(reg, &value_r, ac108);
			printk("REG[0x%02x]: 0x%02x;  ", reg, value_r);
			reg++;
			i++;
			if ((i == num) || (i % 4 == 0))	printk("\n");
		} while (i < num);
	}

	return count;
}

static ssize_t ac108_show(struct device *dev, struct device_attribute *attr, char *buf) {
#if 1
	printk("echo flag|reg|val > ac108\n");
	printk("eg read star addres=0x06,count 0x10:echo 0610 >ac108\n");
	printk("eg write value:0xfe to address:0x06 :echo 106fe > ac108\n");
	return 0;
#else
	return snprintf(buf, PAGE_SIZE,
					"echo flag|reg|val > ac108\n"
					"eg read star addres=0x06,count 0x10:echo 0610 >ac108\n"
					"eg write value:0xfe to address:0x06 :echo 106fe > ac108\n");
#endif
}

static DEVICE_ATTR(ac108, 0644, ac108_show, ac108_store);

static struct attribute *ac108_debug_attrs[] = {
	&dev_attr_ac108.attr,
	NULL,
};

static struct attribute_group ac108_debug_attr_group = {
	.name   = "ac108_debug",
	.attrs  = ac108_debug_attrs,
};




static int ac108_i2c_probe(struct i2c_client *i2c,
						   const struct i2c_device_id *i2c_id) {
	int ret = 0;
	struct device_node *np = i2c->dev.of_node;
	unsigned int val = 0;

	if (ac108 == NULL) {
		ac108 = devm_kzalloc(&i2c->dev, sizeof(struct ac108_priv), GFP_KERNEL);
		if (ac108 == NULL) {
			dev_err(&i2c->dev, "Unable to allocate ac108 private data\n");
			return -ENOMEM;
		}
	}
	ret = of_property_read_u32(np, "data-protocol", &val);
	if (ret) {
		pr_err("Please set data-protocol.\n");
		return -EINVAL;
	}
	ac108->data_protocol = val;


	pr_err(" i2c_id number :%d\n", (int)(i2c_id->driver_data));
	pr_err(" ac108  codec_index :%d\n", ac108->codec_index);
	pr_err(" ac108  I2S data protocol type :%d\n", ac108->data_protocol);

	ac108->i2c[i2c_id->driver_data] = i2c;
	if (ac108->codec_index == 0) {
		ret = snd_soc_register_codec(&i2c->dev, &ac108_soc_codec_driver, ac108_dai[i2c_id->driver_data], 1);
		if (ret < 0) {
			dev_err(&i2c->dev, "Failed to register ac108 codec: %d\n", ret);
		}
	}

	ac108->codec_index++;

	/*Writing this register 0x12 resets all register to their default state.*/
	ac108_write(CHIP_RST, CHIP_RST_VAL, i2c);

	ret = sysfs_create_group(&i2c->dev.kobj, &ac108_debug_attr_group);
	if (ret) {
		pr_err("failed to create attr group\n");
	}

	return ret;
}

static int ac108_i2c_remove(struct i2c_client *client) {
	snd_soc_unregister_codec(&client->dev);
	return 0;
}

static const struct i2c_device_id ac108_i2c_id[] = {
	{ "ac108_0", 0 },
	{ "ac108_1", 1 },
	{ "ac108_2", 2 },
	{ "ac108_3", 3 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ac108_i2c_id);

static const struct of_device_id ac108_of_match[] = {
	{ .compatible = "x-power,ac108_0", },
	{ .compatible = "x-power,ac108_1", },
	{ .compatible = "x-power,ac108_2", },
	{ .compatible = "x-power,ac108_3", },
	{ }
};
MODULE_DEVICE_TABLE(of, ac108_of_match);

static struct i2c_driver ac108_i2c_driver = {
	.driver = {
		.name = "ac108-codec",
		.of_match_table = ac108_of_match,
	},
	.probe =    ac108_i2c_probe,
	.remove =   ac108_i2c_remove,
	.id_table = ac108_i2c_id,
};

module_i2c_driver(ac108_i2c_driver);

MODULE_DESCRIPTION("ASoC AC108 driver");
MODULE_AUTHOR("Baozhu Zuo<zuobaozhu@gmail.com>");
MODULE_LICENSE("GPL");
