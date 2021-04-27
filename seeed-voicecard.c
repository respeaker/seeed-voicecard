/*
 * SEEED voice card
 *
 * (C) Copyright 2017-2018
 * Seeed Technology Co., Ltd. <www.seeedstudio.com>
 *
 * base on ASoC simple sound card support
 *
 * Copyright (C) 2012 Renesas Solutions Corp.
 * Kuninori Morimoto <kuninori.morimoto.gx@renesas.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
/* #undef DEBUG */
#include <linux/version.h>
#include <linux/clk.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/string.h>
#include <sound/soc.h>
#include <sound/soc-dai.h>
#include <sound/simple_card_utils.h>
#include "ac10x.h"

#define LINUX_VERSION_IS_GEQ(x1,x2,x3)	(LINUX_VERSION_CODE >= KERNEL_VERSION(x1,x2,x3))

/*
 * single codec:
 *	0 - allow multi codec
 *	1 - yes
 */
#define _SINGLE_CODEC		1

struct seeed_card_data {
	struct snd_soc_card snd_card;
	struct seeed_dai_props {
		struct asoc_simple_dai cpu_dai;
		struct asoc_simple_dai codec_dai;
		struct snd_soc_dai_link_component cpus;   /* single cpu */
		struct snd_soc_dai_link_component codecs; /* single codec */
		struct snd_soc_dai_link_component platforms;
		unsigned int mclk_fs;
	} *dai_props;
	unsigned int mclk_fs;
	unsigned channels_playback_default;
	unsigned channels_playback_override;
	unsigned channels_capture_default;
	unsigned channels_capture_override;
	struct snd_soc_dai_link *dai_link;
	#if CONFIG_AC10X_TRIG_LOCK
	spinlock_t lock;
	#endif
	struct work_struct work_codec_clk;
	#define TRY_STOP_MAX	3
	int try_stop;
};

struct seeed_card_info {
	const char *name;
	const char *card;
	const char *codec;
	const char *platform;

	unsigned int daifmt;
	struct asoc_simple_dai cpu_dai;
	struct asoc_simple_dai codec_dai;
};

#define seeed_priv_to_card(priv) (&(priv)->snd_card)
#define seeed_priv_to_dev(priv) ((priv)->snd_card.dev)
#define seeed_priv_to_link(priv, i) ((priv)->snd_card.dai_link + (i))
#define seeed_priv_to_props(priv, i) ((priv)->dai_props + (i))

#define DAI	"sound-dai"
#define CELL	"#sound-dai-cells"
#define PREFIX	"seeed-voice-card,"

static int seeed_voice_card_startup(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct seeed_card_data *priv =	snd_soc_card_get_drvdata(rtd->card);
	struct seeed_dai_props *dai_props =
		seeed_priv_to_props(priv, rtd->num);
	int ret;

	ret = clk_prepare_enable(dai_props->cpu_dai.clk);
	if (ret)
		return ret;

	ret = clk_prepare_enable(dai_props->codec_dai.clk);
	if (ret)
		clk_disable_unprepare(dai_props->cpu_dai.clk);

	if (asoc_rtd_to_cpu(rtd, 0)->driver->playback.channels_min) {
		priv->channels_playback_default = asoc_rtd_to_cpu(rtd, 0)->driver->playback.channels_min;
	}
	if (asoc_rtd_to_cpu(rtd, 0)->driver->capture.channels_min) {
		priv->channels_capture_default = asoc_rtd_to_cpu(rtd, 0)->driver->capture.channels_min;
	}
	asoc_rtd_to_cpu(rtd, 0)->driver->playback.channels_min = priv->channels_playback_override;
	asoc_rtd_to_cpu(rtd, 0)->driver->playback.channels_max = priv->channels_playback_override;
	asoc_rtd_to_cpu(rtd, 0)->driver->capture.channels_min = priv->channels_capture_override;
	asoc_rtd_to_cpu(rtd, 0)->driver->capture.channels_max = priv->channels_capture_override;

	return ret;
}

static void seeed_voice_card_shutdown(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct seeed_card_data *priv =	snd_soc_card_get_drvdata(rtd->card);
	struct seeed_dai_props *dai_props =
		seeed_priv_to_props(priv, rtd->num);

	asoc_rtd_to_cpu(rtd, 0)->driver->playback.channels_min = priv->channels_playback_default;
	asoc_rtd_to_cpu(rtd, 0)->driver->playback.channels_max = priv->channels_playback_default;
	asoc_rtd_to_cpu(rtd, 0)->driver->capture.channels_min = priv->channels_capture_default;
	asoc_rtd_to_cpu(rtd, 0)->driver->capture.channels_max = priv->channels_capture_default;

	clk_disable_unprepare(dai_props->cpu_dai.clk);

	clk_disable_unprepare(dai_props->codec_dai.clk);
}

static int seeed_voice_card_hw_params(struct snd_pcm_substream *substream,
				      struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = asoc_rtd_to_codec(rtd, 0);
	struct snd_soc_dai *cpu_dai = asoc_rtd_to_cpu(rtd, 0);
	struct seeed_card_data *priv = snd_soc_card_get_drvdata(rtd->card);
	struct seeed_dai_props *dai_props =
		seeed_priv_to_props(priv, rtd->num);
	unsigned int mclk, mclk_fs = 0;
	int ret = 0;

	if (priv->mclk_fs)
		mclk_fs = priv->mclk_fs;
	else if (dai_props->mclk_fs)
		mclk_fs = dai_props->mclk_fs;

	if (mclk_fs) {
		mclk = params_rate(params) * mclk_fs;
		ret = snd_soc_dai_set_sysclk(codec_dai, 0, mclk,
					     SND_SOC_CLOCK_IN);
		if (ret && ret != -ENOTSUPP)
			goto err;

		ret = snd_soc_dai_set_sysclk(cpu_dai, 0, mclk,
					     SND_SOC_CLOCK_OUT);
		if (ret && ret != -ENOTSUPP)
			goto err;
	}
	return 0;
err:
	return ret;
}

#define _SET_CLOCK_CNT		2
static int (* _set_clock[_SET_CLOCK_CNT])(int y_start_n_stop, struct snd_pcm_substream *substream, int cmd, struct snd_soc_dai *dai);

int seeed_voice_card_register_set_clock(int stream, int (*set_clock)(int, struct snd_pcm_substream *, int, struct snd_soc_dai *)) {
	if (! _set_clock[stream]) {
		_set_clock[stream] = set_clock;
	}
	return 0;
}
EXPORT_SYMBOL(seeed_voice_card_register_set_clock);

/*
 * work_cb_codec_clk: clear audio codec inner clock.
 */
static void work_cb_codec_clk(struct work_struct *work)
{
	struct seeed_card_data *priv = container_of(work, struct seeed_card_data, work_codec_clk);
	int r = 0;

	if (_set_clock[SNDRV_PCM_STREAM_CAPTURE]) {
		r = r || _set_clock[SNDRV_PCM_STREAM_CAPTURE](0, NULL, 0, NULL); /* not using 2nd to 4th arg if 1st == 0 */
	}
	if (_set_clock[SNDRV_PCM_STREAM_PLAYBACK]) {
		r = r || _set_clock[SNDRV_PCM_STREAM_PLAYBACK](0, NULL, 0, NULL); /* not using 2nd to 4th arg if 1st == 0 */
	}

	if (r && priv->try_stop++ < TRY_STOP_MAX) {
		if (0 != schedule_work(&priv->work_codec_clk)) {}
	}
	return;
}

static int seeed_voice_card_trigger(struct snd_pcm_substream *substream, int cmd)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *dai = asoc_rtd_to_codec(rtd, 0);
	struct seeed_card_data *priv = snd_soc_card_get_drvdata(rtd->card);
	#if CONFIG_AC10X_TRIG_LOCK
	unsigned long flags;
	#endif
	int ret = 0;

	dev_dbg(rtd->card->dev, "%s() stream=%s  cmd=%d play:%d, capt:%d\n",
		__FUNCTION__, snd_pcm_stream_str(substream), cmd,
		dai->stream_active[SNDRV_PCM_STREAM_PLAYBACK], dai->stream_active[SNDRV_PCM_STREAM_CAPTURE]);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		if (cancel_work_sync(&priv->work_codec_clk) != 0) {}
		#if CONFIG_AC10X_TRIG_LOCK
		/* I know it will degrades performance, but I have no choice */
		spin_lock_irqsave(&priv->lock, flags);
		#endif
		if (_set_clock[SNDRV_PCM_STREAM_CAPTURE]) _set_clock[SNDRV_PCM_STREAM_CAPTURE](1, substream, cmd, dai);
		if (_set_clock[SNDRV_PCM_STREAM_PLAYBACK]) _set_clock[SNDRV_PCM_STREAM_PLAYBACK](1, substream, cmd, dai);
		#if CONFIG_AC10X_TRIG_LOCK
		spin_unlock_irqrestore(&priv->lock, flags);
		#endif
		break;

	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		/* capture channel resync, if overrun */
		if (dai->stream_active[SNDRV_PCM_STREAM_CAPTURE] && substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			break;
		}

		/* interrupt environment */
		if (in_irq() || in_nmi() || in_serving_softirq()) {
			priv->try_stop = 0;
			if (0 != schedule_work(&priv->work_codec_clk)) {
			}
		} else {
			if (_set_clock[SNDRV_PCM_STREAM_CAPTURE]) _set_clock[SNDRV_PCM_STREAM_CAPTURE](0, NULL, 0, NULL); /* not using 2nd to 4th arg if 1st == 0 */
			if (_set_clock[SNDRV_PCM_STREAM_PLAYBACK]) _set_clock[SNDRV_PCM_STREAM_PLAYBACK](0, NULL, 0, NULL); /* not using 2nd to 4th arg if 1st == 0 */
		}
		break;
	default:
		ret = -EINVAL;
	}

	dev_dbg(rtd->card->dev, "%s() stream=%s  cmd=%d play:%d, capt:%d;finished %d\n",
		__FUNCTION__, snd_pcm_stream_str(substream), cmd,
		dai->stream_active[SNDRV_PCM_STREAM_PLAYBACK], dai->stream_active[SNDRV_PCM_STREAM_CAPTURE], ret);

	return ret;
}

static struct snd_soc_ops seeed_voice_card_ops = {
	.startup = seeed_voice_card_startup,
	.shutdown = seeed_voice_card_shutdown,
	.hw_params = seeed_voice_card_hw_params,
	.trigger = seeed_voice_card_trigger,
};

static int asoc_simple_parse_dai(struct device_node *node,
				 struct snd_soc_dai_link_component *dlc,
				 int *is_single_link)
{
	struct of_phandle_args args;
	int ret;

	if (!node)
		return 0;

	/*
	 * Get node via "sound-dai = <&phandle port>"
	 * it will be used as xxx_of_node on soc_bind_dai_link()
	 */
	ret = of_parse_phandle_with_args(node, DAI, CELL, 0, &args);
	if (ret)
		return ret;

	/*
	 * FIXME
	 *
	 * Here, dlc->dai_name is pointer to CPU/Codec DAI name.
	 * If user unbinded CPU or Codec driver, but not for Sound Card,
	 * dlc->dai_name is keeping unbinded CPU or Codec
	 * driver's pointer.
	 *
	 * If user re-bind CPU or Codec driver again, ALSA SoC will try
	 * to rebind Card via snd_soc_try_rebind_card(), but because of
	 * above reason, it might can't bind Sound Card.
	 * Because Sound Card is pointing to released dai_name pointer.
	 *
	 * To avoid this rebind Card issue,
	 * 1) It needs to alloc memory to keep dai_name eventhough
	 *    CPU or Codec driver was unbinded, or
	 * 2) user need to rebind Sound Card everytime
	 *    if he unbinded CPU or Codec.
	 */
	ret = snd_soc_of_get_dai_name(node, &dlc->dai_name);
	if (ret < 0)
		return ret;

	dlc->of_node = args.np;

	if (is_single_link)
		*is_single_link = !args.args_count;

	return 0;
}

static int asoc_simple_init_dai(struct snd_soc_dai *dai,
				     struct asoc_simple_dai *simple_dai)
{
	int ret;

	if (!simple_dai)
		return 0;

	if (simple_dai->sysclk) {
		ret = snd_soc_dai_set_sysclk(dai, 0, simple_dai->sysclk,
					     simple_dai->clk_direction);
		if (ret && ret != -ENOTSUPP) {
			dev_err(dai->dev, "simple-card: set_sysclk error\n");
			return ret;
		}
	}

	if (simple_dai->slots) {
		ret = snd_soc_dai_set_bclk_ratio(dai,
					       simple_dai->slots *
					       simple_dai->slot_width);
		if (ret && ret != -ENOTSUPP) {
			dev_err(dai->dev, "simple-card: set_tdm_slot error\n");
			return ret;
		}
	}

	return 0;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,7,0)
static int asoc_simple_init_dai_link_params(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_soc_dai_link *dai_link = rtd->dai_link;
	struct snd_soc_component *component;
	struct snd_soc_pcm_stream *params;
	struct snd_pcm_hardware hw;
	int i, ret, stream;

	/* Only codecs should have non_legacy_dai_naming set. */
	for_each_rtd_components(rtd, i, component) {
		if (!component->driver->non_legacy_dai_naming)
			return 0;
	}

	/* Assumes the capabilities are the same for all supported streams */
	for (stream = 0; stream < 2; stream++) {
		ret = snd_soc_runtime_calc_hw(rtd, &hw, stream);
		if (ret == 0)
			break;
	}

	if (ret < 0) {
		dev_err(rtd->dev, "simple-card: no valid dai_link params\n");
		return ret;
	}

	params = devm_kzalloc(rtd->dev, sizeof(*params), GFP_KERNEL);
	if (!params)
		return -ENOMEM;

	params->formats = hw.formats;
	params->rates = hw.rates;
	params->rate_min = hw.rate_min;
	params->rate_max = hw.rate_max;
	params->channels_min = hw.channels_min;
	params->channels_max = hw.channels_max;

	dai_link->params = params;
	dai_link->num_params = 1;

	return 0;
}
#endif

static int seeed_voice_card_dai_init(struct snd_soc_pcm_runtime *rtd)
{
	struct seeed_card_data *priv =	snd_soc_card_get_drvdata(rtd->card);
	struct snd_soc_dai *codec = asoc_rtd_to_codec(rtd, 0);
	struct snd_soc_dai *cpu = asoc_rtd_to_cpu(rtd, 0);
	struct seeed_dai_props *dai_props =
		seeed_priv_to_props(priv, rtd->num);
	int ret;

	ret = asoc_simple_init_dai(codec, &dai_props->codec_dai);
	if (ret < 0)
		return ret;

	ret = asoc_simple_init_dai(cpu, &dai_props->cpu_dai);
	if (ret < 0)
		return ret;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,7,0)
	ret = asoc_simple_init_dai_link_params(rtd);
	if (ret < 0)
		return ret;
#endif

	dev_dbg(rtd->card->dev, "codec \"%s\" mapping to cpu \"%s\"\n", codec->name, cpu->name);
	return 0;
}

static int seeed_voice_card_dai_link_of(struct device_node *node,
					struct seeed_card_data *priv,
					int idx,
					bool is_top_level_node)
{
	struct device *dev = seeed_priv_to_dev(priv);
	struct snd_soc_dai_link *dai_link = seeed_priv_to_link(priv, idx);
	struct seeed_dai_props *dai_props = seeed_priv_to_props(priv, idx);
	struct asoc_simple_dai *cpu_dai = &dai_props->cpu_dai;
	struct asoc_simple_dai *codec_dai = &dai_props->codec_dai;
	struct device_node *cpu = NULL;
	struct device_node *plat = NULL;
	struct device_node *codec = NULL;
	char prop[128];
	char *prefix = "";
	int ret, single_cpu;

	/* For single DAI link & old style of DT node */
	if (is_top_level_node)
		prefix = PREFIX;

	snprintf(prop, sizeof(prop), "%scpu", prefix);
	cpu = of_get_child_by_name(node, prop);

	if (!cpu) {
		ret = -EINVAL;
		dev_err(dev, "%s: Can't find %s DT node\n", __func__, prop);
		goto dai_link_of_err;
	}

	snprintf(prop, sizeof(prop), "%splat", prefix);
	plat = of_get_child_by_name(node, prop);

	snprintf(prop, sizeof(prop), "%scodec", prefix);
	codec = of_get_child_by_name(node, prop);

	if (!codec) {
		ret = -EINVAL;
		dev_err(dev, "%s: Can't find %s DT node\n", __func__, prop);
		goto dai_link_of_err;
	}

	ret = asoc_simple_parse_daifmt(dev, node, codec,
					    prefix, &dai_link->dai_fmt);
	if (ret < 0)
		goto dai_link_of_err;

	of_property_read_u32(node, "mclk-fs", &dai_props->mclk_fs);

	ret = asoc_simple_parse_cpu(cpu, dai_link, &single_cpu);
	if (ret < 0)
		goto dai_link_of_err;

	#if _SINGLE_CODEC
	ret = asoc_simple_parse_codec(codec, dai_link);
	if (ret < 0)
		goto dai_link_of_err;
	#else
	ret = snd_soc_of_get_dai_link_codecs(dev, codec, dai_link);
	if (ret < 0) {
		dev_err(dev, "parse codec info error %d\n", ret);
		goto dai_link_of_err;
	}
	dev_dbg(dev, "dai_link num_codecs = %d\n", dai_link->num_codecs);
	#endif

	ret = asoc_simple_parse_platform(plat, dai_link);
	if (ret < 0)
		goto dai_link_of_err;

	ret = snd_soc_of_parse_tdm_slot(cpu,	&cpu_dai->tx_slot_mask,
						&cpu_dai->rx_slot_mask,
						&cpu_dai->slots,
						&cpu_dai->slot_width);
	dev_dbg(dev, "cpu_dai : slot,width,tx,rx = %d,%d,%d,%d\n", 
			cpu_dai->slots, cpu_dai->slot_width,
			cpu_dai->tx_slot_mask, cpu_dai->rx_slot_mask
			);
	if (ret < 0)
		goto dai_link_of_err;

	ret = snd_soc_of_parse_tdm_slot(codec,	&codec_dai->tx_slot_mask,
						&codec_dai->rx_slot_mask,
						&codec_dai->slots,
						&codec_dai->slot_width);
	if (ret < 0)
		goto dai_link_of_err;

	#if LINUX_VERSION_CODE <= KERNEL_VERSION(4,10,0)
	ret = asoc_simple_card_parse_clk_cpu(cpu, dai_link, cpu_dai);
	#else
	ret = asoc_simple_parse_clk_cpu(dev, cpu, dai_link, cpu_dai);
	#endif
	if (ret < 0)
		goto dai_link_of_err;

	#if LINUX_VERSION_CODE <= KERNEL_VERSION(4,10,0)
	ret = asoc_simple_card_parse_clk_codec(codec, dai_link, codec_dai);
	#else
	ret = asoc_simple_parse_clk_codec(dev, codec, dai_link, codec_dai);
	#endif
	if (ret < 0)
		goto dai_link_of_err;

	ret = asoc_simple_set_dailink_name(dev, dai_link,
						"%s-%s",
						dai_link->cpus->dai_name,
						#if _SINGLE_CODEC
						dai_link->codecs->dai_name
						#else
						dai_link->codecs[0].dai_name
						#endif
	);
	if (ret < 0)
		goto dai_link_of_err;

	dai_link->ops = &seeed_voice_card_ops;
	dai_link->init = seeed_voice_card_dai_init;

	dev_dbg(dev, "\tname : %s\n", dai_link->stream_name);
	dev_dbg(dev, "\tformat : %04x\n", dai_link->dai_fmt);
	dev_dbg(dev, "\tcpu : %s / %d\n",
		dai_link->cpus->dai_name,
		dai_props->cpu_dai.sysclk);
	dev_dbg(dev, "\tcodec : %s / %d\n",
		#if _SINGLE_CODEC
		dai_link->codecs->dai_name,
		#else
		dai_link->codecs[0].dai_name,
		#endif
		dai_props->codec_dai.sysclk);

	asoc_simple_canonicalize_cpu(dai_link, single_cpu);
	#if _SINGLE_CODEC
	asoc_simple_canonicalize_platform(dai_link);
	#endif

dai_link_of_err:
	of_node_put(cpu);
	of_node_put(codec);

	return ret;
}

static int seeed_voice_card_parse_aux_devs(struct device_node *node,
					   struct seeed_card_data *priv)
{
	struct device *dev = seeed_priv_to_dev(priv);
	struct device_node *aux_node;
	int i, n, len;

	if (!of_find_property(node, PREFIX "aux-devs", &len))
		return 0;		/* Ok to have no aux-devs */

	n = len / sizeof(__be32);
	if (n <= 0)
		return -EINVAL;

	priv->snd_card.aux_dev = devm_kzalloc(dev,
			n * sizeof(*priv->snd_card.aux_dev), GFP_KERNEL);
	if (!priv->snd_card.aux_dev)
		return -ENOMEM;

	for (i = 0; i < n; i++) {
		aux_node = of_parse_phandle(node, PREFIX "aux-devs", i);
		if (!aux_node)
			return -EINVAL;
		priv->snd_card.aux_dev[i].dlc.of_node = aux_node;
	}

	priv->snd_card.num_aux_devs = n;
	return 0;
}

static int seeed_voice_card_parse_of(struct device_node *node,
				     struct seeed_card_data *priv)
{
	struct device *dev = seeed_priv_to_dev(priv);
	struct device_node *dai_link;
	int ret;

	if (!node)
		return -EINVAL;

	dai_link = of_get_child_by_name(node, PREFIX "dai-link");

	/* The off-codec widgets */
	if (of_property_read_bool(node, PREFIX "widgets")) {
		ret = snd_soc_of_parse_audio_simple_widgets(&priv->snd_card,
					PREFIX "widgets");
		if (ret)
			goto card_parse_end;
	}

	/* DAPM routes */
	if (of_property_read_bool(node, PREFIX "routing")) {
		ret = snd_soc_of_parse_audio_routing(&priv->snd_card,
					PREFIX "routing");
		if (ret)
			goto card_parse_end;
	}

	/* Factor to mclk, used in hw_params() */
	of_property_read_u32(node, PREFIX "mclk-fs", &priv->mclk_fs);

	/* Single/Muti DAI link(s) & New style of DT node */
	if (dai_link) {
		struct device_node *np = NULL;
		int i = 0;

		for_each_child_of_node(node, np) {
			dev_dbg(dev, "\tlink %d:\n", i);
			ret = seeed_voice_card_dai_link_of(np, priv,
							   i, false);
			if (ret < 0) {
				of_node_put(np);
				goto card_parse_end;
			}
			i++;
		}
	} else {
		/* For single DAI link & old style of DT node */
		ret = seeed_voice_card_dai_link_of(node, priv, 0, true);
		if (ret < 0)
			goto card_parse_end;
	}

	ret = asoc_simple_parse_card_name(&priv->snd_card, PREFIX);
	if (ret < 0)
		goto card_parse_end;

	ret = seeed_voice_card_parse_aux_devs(node, priv);

	priv->channels_playback_default  = 0;
	priv->channels_playback_override = 2;
	priv->channels_capture_default   = 0;
	priv->channels_capture_override  = 2;
	of_property_read_u32(node, PREFIX "channels-playback-default",
				    &priv->channels_playback_default);
	of_property_read_u32(node, PREFIX "channels-playback-override",
				    &priv->channels_playback_override);
	of_property_read_u32(node, PREFIX "channels-capture-default",
				    &priv->channels_capture_default);
	of_property_read_u32(node, PREFIX "channels-capture-override",
				    &priv->channels_capture_override);

card_parse_end:
	of_node_put(dai_link);

	return ret;
}

#ifdef DEBUG
inline void seeed_debug_dai(struct seeed_card_data *priv,
				  char *name,
				  struct asoc_simple_dai *dai)
{
	struct device *dev = seeed_priv_to_dev(priv);

	if (dai->name)
		dev_dbg(dev, "%s dai name = %s\n",
			name, dai->name);
	if (dai->sysclk)
		dev_dbg(dev, "%s sysclk = %d\n",
			name, dai->sysclk);

	dev_dbg(dev, "%s direction = %s\n",
		name, dai->clk_direction ? "OUT" : "IN");

	if (dai->slots)
		dev_dbg(dev, "%s slots = %d\n", name, dai->slots);
	if (dai->slot_width)
		dev_dbg(dev, "%s slot width = %d\n", name, dai->slot_width);
	if (dai->tx_slot_mask)
		dev_dbg(dev, "%s tx slot mask = %d\n", name, dai->tx_slot_mask);
	if (dai->rx_slot_mask)
		dev_dbg(dev, "%s rx slot mask = %d\n", name, dai->rx_slot_mask);
	if (dai->clk)
		dev_dbg(dev, "%s clk %luHz\n", name, clk_get_rate(dai->clk));
}

inline void seeed_debug_info(struct seeed_card_data *priv)
{
	struct snd_soc_card *card = seeed_priv_to_card(priv);
	struct device *dev = seeed_priv_to_dev(priv);

	int i;

	if (card->name)
		dev_dbg(dev, "Card Name: %s\n", card->name);

	for (i = 0; i < card->num_links; i++) {
		struct seeed_dai_props *props = seeed_priv_to_props(priv, i);
		struct snd_soc_dai_link *link = seeed_priv_to_link(priv, i);

		dev_dbg(dev, "DAI%d\n", i);

		seeed_debug_dai(priv, "cpu", &props->cpu_dai);
		seeed_debug_dai(priv, "codec", &props->codec_dai);

		if (link->name)
			dev_dbg(dev, "dai name = %s\n", link->name);

		dev_dbg(dev, "dai format = %04x\n", link->dai_fmt);

		/*
		if (props->adata.convert_rate)
			dev_dbg(dev, "convert_rate = %d\n",
				props->adata.convert_rate);
		if (props->adata.convert_channels)
			dev_dbg(dev, "convert_channels = %d\n",
				props->adata.convert_channels);
		if (props->codec_conf && props->codec_conf->name_prefix)
			dev_dbg(dev, "name prefix = %s\n",
				props->codec_conf->name_prefix);
		*/
		if (props->mclk_fs)
			dev_dbg(dev, "mclk-fs = %d\n",
				props->mclk_fs);
	}
}
#else
#define  seeed_debug_info(priv)
#endif /* DEBUG */

static int seeed_voice_card_probe(struct platform_device *pdev)
{
	struct seeed_card_data *priv;
	struct snd_soc_dai_link *dai_link;
	struct seeed_dai_props *dai_props;
	struct device_node *np = pdev->dev.of_node;
	struct device *dev = &pdev->dev;
	int num, ret, i;

	/* Get the number of DAI links */
	if (np && of_get_child_by_name(np, PREFIX "dai-link"))
		num = of_get_child_count(np);
	else
		num = 1;

	/* Allocate the private data and the DAI link array */
	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	dai_props = devm_kzalloc(dev, sizeof(*dai_props) * num, GFP_KERNEL);
	dai_link  = devm_kzalloc(dev, sizeof(*dai_link)  * num, GFP_KERNEL);
	if (!dai_props || !dai_link)
		return -ENOMEM;

	/*
	 * Use snd_soc_dai_link_component instead of legacy style
	 * It is codec only. but cpu/platform will be supported in the future.
	 * see
	 *      soc-core.c :: snd_soc_init_multicodec()
	 *
	 * "platform" might be removed
	 * see
	 *      simple-card-utils.c :: asoc_simple_canonicalize_platform()
	 */
	for (i = 0; i < num; i++) {
		dai_link[i].cpus		= &dai_props[i].cpus;
		dai_link[i].num_cpus		= 1;
		dai_link[i].codecs		= &dai_props[i].codecs;
		dai_link[i].num_codecs		= 1;
		dai_link[i].platforms		= &dai_props[i].platforms;
		dai_link[i].num_platforms	= 1;
	}

	priv->dai_props			= dai_props;
	priv->dai_link			= dai_link;

	/* Init snd_soc_card */
	priv->snd_card.owner		= THIS_MODULE;
	priv->snd_card.dev		= dev;
	priv->snd_card.dai_link		= priv->dai_link;
	priv->snd_card.num_links	= num;

	if (np && of_device_is_available(np)) {
		ret = seeed_voice_card_parse_of(np, priv);
		if (ret < 0) {
			if (ret != -EPROBE_DEFER)
				dev_err(dev, "parse error %d\n", ret);
			goto err;
		}
	} else {
		struct seeed_card_info *cinfo;
		struct snd_soc_dai_link_component *cpus;
		struct snd_soc_dai_link_component *codecs;
		struct snd_soc_dai_link_component *platform;

		cinfo = dev->platform_data;
		if (!cinfo) {
			dev_err(dev, "no info for seeed-voice-card\n");
			return -EINVAL;
		}

		if (!cinfo->name ||
		    !cinfo->codec_dai.name ||
		    !cinfo->codec ||
		    !cinfo->platform ||
		    !cinfo->cpu_dai.name) {
			dev_err(dev, "insufficient seeed_voice_card_info settings\n");
			return -EINVAL;
		}

		cpus			= dai_link->cpus;
		cpus->dai_name		= cinfo->cpu_dai.name;

		codecs			= dai_link->codecs;
		codecs->name		= cinfo->codec;
		codecs->dai_name	= cinfo->codec_dai.name;

		platform		= dai_link->platforms;
		platform->name		= cinfo->platform;

		priv->snd_card.name	= (cinfo->card) ? cinfo->card : cinfo->name;
		dai_link->name		= cinfo->name;
		dai_link->stream_name	= cinfo->name;
		dai_link->dai_fmt	= cinfo->daifmt;
		dai_link->init		= seeed_voice_card_dai_init;
		memcpy(&priv->dai_props->cpu_dai, &cinfo->cpu_dai,
					sizeof(priv->dai_props->cpu_dai));
		memcpy(&priv->dai_props->codec_dai, &cinfo->codec_dai,
					sizeof(priv->dai_props->codec_dai));
	}

	snd_soc_card_set_drvdata(&priv->snd_card, priv);

	#if CONFIG_AC10X_TRIG_LOCK
	spin_lock_init(&priv->lock);
	#endif

	INIT_WORK(&priv->work_codec_clk, work_cb_codec_clk);

	seeed_debug_info(priv);

	ret = devm_snd_soc_register_card(&pdev->dev, &priv->snd_card);
	if (ret >= 0)
		return ret;

err:
	asoc_simple_clean_reference(&priv->snd_card);

	return ret;
}

static int seeed_voice_card_remove(struct platform_device *pdev)
{
	struct snd_soc_card *card = platform_get_drvdata(pdev);
	struct seeed_card_data *priv = snd_soc_card_get_drvdata(card);

	if (cancel_work_sync(&priv->work_codec_clk) != 0) {
	}
	return asoc_simple_clean_reference(card);
}

static const struct of_device_id seeed_voice_of_match[] = {
	{ .compatible = "seeed-voicecard", },
	{},
};
MODULE_DEVICE_TABLE(of, seeed_voice_of_match);

static struct platform_driver seeed_voice_card = {
	.driver = {
		.name = "seeed-voicecard",
		.pm = &snd_soc_pm_ops,
		.of_match_table = seeed_voice_of_match,
	},
	.probe = seeed_voice_card_probe,
	.remove = seeed_voice_card_remove,
};

module_platform_driver(seeed_voice_card);

MODULE_ALIAS("platform:seeed-voice-card");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("ASoC SEEED Voice Card");
MODULE_AUTHOR("PeterYang<linsheng.yang@seeed.cc>");
