//https://github.com/HazouPH/android_device_motorola_smi-plus/blob/48029b4afc307c73181b108a5b0155b9f20856ca/smi-modules/alsa-lib_module_voice/pcm_voice.c
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <alsa/asoundlib.h>
#include <alsa/pcm_external.h>
#include <alsa/pcm_plugin.h>

#include <math.h>

#define ARRAY_SIZE(ary)	(sizeof(ary)/sizeof(ary[0]))
#define  AC108_FRAME_SIZE 4096
struct ac108_t {
	snd_pcm_ioplug_t io;
	snd_pcm_t *slave;
	snd_pcm_hw_params_t *hw_params;
	unsigned int last_size;
	unsigned int ptr;
	void *buf;
	unsigned int        latency;         // Delay in usec
	unsigned int        bufferSize;      // Size of sample buffer
};

/* set up the fixed parameters of slave PCM hw_parmas */
static int ac108_slave_hw_params_half(struct ac108_t *rec, unsigned int rate,snd_pcm_format_t format) {
	int err;
    snd_pcm_uframes_t bufferSize = rec->bufferSize;
    unsigned int latency = rec->latency;

    unsigned int buffer_time = 0;
    unsigned int period_time = 0;
	if ((err = snd_pcm_hw_params_malloc(&rec->hw_params)) < 0) return err;

	if ((err = snd_pcm_hw_params_any(rec->slave, rec->hw_params)) < 0) {
		SNDERR("Cannot get slave hw_params");
		goto out;
	}
	if ((err = snd_pcm_hw_params_set_access(rec->slave, rec->hw_params,
											SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
		SNDERR("Cannot set slave access RW_INTERLEAVED");
		goto out;
	}
	if ((err = snd_pcm_hw_params_set_channels(rec->slave, rec->hw_params, 2)) < 0) {
		SNDERR("Cannot set slave channels 2");
		goto out;
	}
	if ((err = snd_pcm_hw_params_set_format(rec->slave, rec->hw_params,
											format)) < 0) {
		SNDERR("Cannot set slave format");
		goto out;
	}
	if ((err = snd_pcm_hw_params_set_rate(rec->slave, rec->hw_params, rate, 0)) < 0) {
		SNDERR("Cannot set slave rate %d", rate);
		goto out;
	}

    err = snd_pcm_hw_params_get_buffer_time_max(rec->hw_params,
            &buffer_time, 0);
    if (buffer_time > 80000)
        buffer_time = 80000;
    period_time = buffer_time / 4;

    err = snd_pcm_hw_params_set_period_time_near(rec->slave, rec->hw_params,
            &period_time, 0);
    if (err < 0) {
        fprintf(stderr,"Unable to set_period_time_near");
        goto out;
    }
    err = snd_pcm_hw_params_set_buffer_time_near(rec->slave, rec->hw_params,
            &buffer_time, 0);
    if (err < 0) {
        fprintf(stderr,"Unable to set_buffer_time_near");
        goto out;
    }

    rec->bufferSize = bufferSize;
    rec->latency = latency;

	return 0;

out:
	free(rec->hw_params);
	rec->hw_params = NULL;
	return err;
}

/*
 * start and stop callbacks - just trigger slave PCM
 */
static int ac108_start(snd_pcm_ioplug_t *io) {
	struct ac108_t *rec = io->private_data;

    if(!rec->slave) {
			fprintf(stderr, "slave is lost\n");
    }

	return snd_pcm_start(rec->slave);
}

static int ac108_stop(snd_pcm_ioplug_t *io) {
	struct ac108_t *rec = io->private_data;

	return snd_pcm_drop(rec->slave);
}
/*
 * pointer callback
 *
 * Calculate the current position from the delay of slave PCM
 */
static snd_pcm_sframes_t ac108_pointer(snd_pcm_ioplug_t *io) {
	struct ac108_t *rec = io->private_data;
	int err, size;

	assert(rec);


	size = snd_pcm_avail(rec->slave);
	if (size < 0) return size;
	size = size /2;
	if (size > rec->last_size) {
			rec->ptr += size - rec->last_size;
			rec->ptr %= io->buffer_size;
	}

	rec->last_size = size;

	//fprintf(stderr, "%s :%d %d %d %d\n", __func__, rec->ptr,size, io->appl_ptr, io->hw_ptr);
	return rec->ptr;
}

/*
 * transfer callback
 */
static snd_pcm_sframes_t ac108_transfer(snd_pcm_ioplug_t *io,
										const snd_pcm_channel_area_t *areas,
										snd_pcm_uframes_t offset,
										snd_pcm_uframes_t size) {
	struct ac108_t *rec = io->private_data;
	char *buf;
	ssize_t result;
	int err;


	/* we handle only an interleaved buffer */
	buf = (char *)areas->addr + (areas->first + areas->step * offset) / 8;
	result = snd_pcm_readi(rec->slave, buf, size*2);
	if (result <= 0) {
		fprintf(stderr, "%s out error:%d %d\n", __func__, result);
		return result;
	}
	rec->last_size -= size;


	return size;

}

/*
 * poll-related callbacks - just pass to slave PCM
 */
static int ac108_poll_descriptors_count(snd_pcm_ioplug_t *io) {
	struct ac108_t *rec = io->private_data;

	//fprintf(stderr, "%s\n", __FUNCTION__);
	return snd_pcm_poll_descriptors_count(rec->slave);
}

static int ac108_poll_descriptors(snd_pcm_ioplug_t *io, struct pollfd *pfd,
								  unsigned int space) {
	struct ac108_t *rec = io->private_data;

	//fprintf(stderr, "%s\n", __FUNCTION__);
	return snd_pcm_poll_descriptors(rec->slave, pfd, space);
}

static int ac108_poll_revents(snd_pcm_ioplug_t *io, struct pollfd *pfd,
							  unsigned int nfds, unsigned short *revents) {
	struct ac108_t *rec = io->private_data;

	//fprintf(stderr, "%s\n", __FUNCTION__);
	return snd_pcm_poll_descriptors_revents(rec->slave, pfd, nfds, revents);
}

/*
 * close callback
 */
static int ac108_close(snd_pcm_ioplug_t *io) {
	struct ac108_t *rec = io->private_data;

	if (rec->slave) return snd_pcm_close(rec->slave);
	return 0;
}

static int setSoftwareParams(struct ac108_t *rec) {
	snd_pcm_sw_params_t *softwareParams;
	int err;

	snd_pcm_uframes_t bufferSize = 0;
	snd_pcm_uframes_t periodSize = 0;
	snd_pcm_uframes_t startThreshold, stopThreshold;
	snd_pcm_sw_params_alloca(&softwareParams);

	// Get the current software parameters
	err = snd_pcm_sw_params_current(rec->slave, softwareParams);
	if (err < 0) {
		fprintf(stderr, "Unable to get software parameters: %s", snd_strerror(err));
		goto done;
	}

	// Configure ALSA to start the transfer when the buffer is almost full.
	snd_pcm_get_params(rec->slave, &bufferSize, &periodSize);


	startThreshold = 1;
	stopThreshold = bufferSize;


	err = snd_pcm_sw_params_set_start_threshold(rec->slave, softwareParams,
												startThreshold);
	if (err < 0) {
		fprintf(stderr, "Unable to set start threshold to %lu frames: %s",
				startThreshold, snd_strerror(err));
		goto done;
	}

	err = snd_pcm_sw_params_set_stop_threshold(rec->slave, softwareParams,
											   stopThreshold);
	if (err < 0) {
		fprintf(stderr, "Unable to set stop threshold to %lu frames: %s",
				stopThreshold, snd_strerror(err));
		goto done;
	}
	// Allow the transfer to start when at least periodSize samples can be
	// processed.
	err = snd_pcm_sw_params_set_avail_min(rec->slave, softwareParams,
										  periodSize);
	if (err < 0) {
		fprintf(stderr, "Unable to configure available minimum to %lu: %s",
				periodSize, snd_strerror(err));
		goto done;
	}

	// Commit the software parameters back to the device.
	err = snd_pcm_sw_params(rec->slave, softwareParams);
	if (err < 0) fprintf(stderr, "Unable to configure software parameters: %s",
						 snd_strerror(err));



	return 0;
done:
	snd_pcm_sw_params_free(softwareParams);

	return err;
}
/*
 * hw_params callback
 *
 * Set up slave PCM according to the current parameters
 */
//static int ac108_hw_params(snd_pcm_ioplug_t *io,
//						   snd_pcm_hw_params_t *params ATTRIBUTE_UNUSED) {
static int ac108_hw_params(snd_pcm_ioplug_t *io) {
	struct ac108_t *rec = io->private_data;
	snd_pcm_sw_params_t *sparams;
	snd_pcm_uframes_t period_size;
	snd_pcm_uframes_t buffer_size;
	int err;
	if (!rec->hw_params) {
		err = ac108_slave_hw_params_half(rec, 2*io->rate,io->format);
		if (err < 0) {
			fprintf(stderr, "ac108_slave_hw_params_half error\n");
			return err;
		}
	}
	period_size = io->period_size;
	if ((err = snd_pcm_hw_params_set_period_size_near(rec->slave, rec->hw_params,
													  &period_size, NULL)) < 0) {
		SNDERR("Cannot set slave period size %ld", period_size);
		return err;
	}
	buffer_size = io->buffer_size;
	if ((err = snd_pcm_hw_params_set_buffer_size_near(rec->slave, rec->hw_params,
													  &buffer_size)) < 0) {
		SNDERR("Cannot set slave buffer size %ld", buffer_size);
		return err;
	}
	if ((err = snd_pcm_hw_params(rec->slave, rec->hw_params)) < 0) {
		SNDERR("Cannot set slave hw_params");
		return err;
	}

	setSoftwareParams(rec);

	return 0;
}
/*
 * hw_free callback
 */
static int ac108_hw_free(snd_pcm_ioplug_t *io) {
	struct ac108_t *rec = io->private_data;
	free(rec->hw_params);
	if (rec->buf != NULL) {
		free(rec->buf);
		rec->buf = NULL;
	}
	rec->hw_params = NULL;

	return snd_pcm_hw_free(rec->slave);

}


static int ac108_prepare(snd_pcm_ioplug_t *io) {
	struct ac108_t *rec = io->private_data;
	rec->ptr = 0;
	rec->last_size =0;
	if (rec->buf == NULL) {
		rec->buf = malloc(io->buffer_size);
	}

	return snd_pcm_prepare(rec->slave);
}
static int ac108_drain(snd_pcm_ioplug_t *io) {
	struct ac108_t *rec = io->private_data;
	return snd_pcm_drain(rec->slave);
}
static int ac108_sw_params(snd_pcm_ioplug_t *io, snd_pcm_sw_params_t *params) {
	struct ac108_t *rec = io->private_data;

	return 0;
}

static int ac108_delay(snd_pcm_ioplug_t * io, snd_pcm_sframes_t * delayp){

	return 0;
}
/*
 * callback table
 */
static snd_pcm_ioplug_callback_t a108_ops = {
	.start = ac108_start,
	.stop = ac108_stop,
	.pointer = ac108_pointer,
	.transfer = ac108_transfer,
	.poll_descriptors_count = ac108_poll_descriptors_count,
	.poll_descriptors = ac108_poll_descriptors,
	.poll_revents = ac108_poll_revents,
	.close = ac108_close,
	.hw_params = ac108_hw_params,
	.hw_free = ac108_hw_free,
//	.sw_params = ac108_sw_params,
	.prepare = ac108_prepare,
	.drain = ac108_drain,
	.delay = ac108_delay,
};


static int ac108_set_hw_constraint(struct ac108_t  *rec) {
	static unsigned int accesses[] = {
		SND_PCM_ACCESS_RW_INTERLEAVED,
		SND_PCM_ACCESS_RW_NONINTERLEAVED 
	};
	unsigned int formats[] = { SND_PCM_FORMAT_S32,
							   SND_PCM_FORMAT_S16 };

	unsigned int  rates[] = {
		8000,
		16000,
		48000
	};
	int err;
	snd_pcm_uframes_t buffer_max;
	unsigned int period_bytes, max_periods;


	err = snd_pcm_ioplug_set_param_list(&rec->io,
										SND_PCM_IOPLUG_HW_ACCESS,
										ARRAY_SIZE(accesses),
										accesses);
	if (err < 0) return err;

	if ((err = snd_pcm_ioplug_set_param_list(&rec->io, SND_PCM_IOPLUG_HW_FORMAT,
											 ARRAY_SIZE(formats), formats)) < 0 ||
		(err = snd_pcm_ioplug_set_param_minmax(&rec->io, SND_PCM_IOPLUG_HW_CHANNELS,
											   4, 4)) < 0 ||
		(err = snd_pcm_ioplug_set_param_list(&rec->io, SND_PCM_IOPLUG_HW_RATE,
												ARRAY_SIZE(rates), rates)) < 0) return err;
	err = snd_pcm_ioplug_set_param_minmax(&rec->io,
										  SND_PCM_IOPLUG_HW_BUFFER_BYTES,
										  1, 4 * 1024 * 1024);
	if (err < 0) return err;

	err = snd_pcm_ioplug_set_param_minmax(&rec->io,
										  SND_PCM_IOPLUG_HW_PERIOD_BYTES,
										  128, 2 * 1024 * 1024);
	if (err < 0) return err;

	err = snd_pcm_ioplug_set_param_minmax(&rec->io, SND_PCM_IOPLUG_HW_PERIODS,
										  3, 1024);
	return 0;
}

/*
/*
 * Main entry point
 */
SND_PCM_PLUGIN_DEFINE_FUNC(ac108) {
	snd_config_iterator_t i, next;
	int err;
	const char *card = NULL;
	const char *pcm_string = NULL;
	snd_pcm_format_t format = SND_PCM_FORMAT_S32_LE;
	char devstr[128], tmpcard[8];
	struct ac108_t *rec;
	int channels;
	struct pollfd fds;
	if (stream != SND_PCM_STREAM_CAPTURE) {
		SNDERR("a108 is only for capture");
		return -EINVAL;
	}

	snd_config_for_each(i, next, conf) {
		snd_config_t *n = snd_config_iterator_entry(i);
		const char *id;
		if (snd_config_get_id(n, &id) < 0) continue;
		if (strcmp(id, "comment") == 0 || strcmp(id, "type") == 0 || strcmp(id, "hint") == 0) continue;

		if (strcmp(id, "slavepcm") == 0) {
			if (snd_config_get_string(n, &pcm_string) < 0) {
				SNDERR("ac108 slavepcm must be a string");
				return -EINVAL;
			}
			continue;
		}

		if (strcmp(id, "channels") == 0) {
			long val;
			if (snd_config_get_integer(n, &val) < 0) {
				SNDERR("Invalid type for %s", id);
				return -EINVAL;
			}
			channels = val;
			if (channels != 2 && channels != 4 && channels != 6) {
				SNDERR("channels must be 2, 4 or 6");
				return -EINVAL;
			}
			continue;
		}
	}

	rec = calloc(1, sizeof(*rec));
	if (!rec) {
		SNDERR("cannot allocate");
		return -ENOMEM;
	}
	err = snd_pcm_open(&rec->slave, pcm_string, stream, mode);
	if (err < 0) goto error;



	//SND_PCM_NONBLOCK
	rec->io.version = SND_PCM_IOPLUG_VERSION;
	rec->io.name = "AC108 decode Plugin";
	rec->io.mmap_rw = 0;
	rec->io.callback = &a108_ops;
	rec->io.private_data = rec;

	err = snd_pcm_ioplug_create(&rec->io, name, stream, mode);
	if (err < 0) goto error;

	if ((err = ac108_set_hw_constraint(rec)) < 0) {
		snd_pcm_ioplug_delete(&rec->io);
		return err;
	}
	*pcmp = rec->io.pcm;
	return 0;
	
error:
	if (rec->slave) snd_pcm_close(rec->slave);
	free(rec);
	return err;
}

SND_PCM_PLUGIN_SYMBOL(ac108);
