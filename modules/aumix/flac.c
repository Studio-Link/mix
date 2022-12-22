#include <FLAC/metadata.h>
#include <FLAC/stream_encoder.h>
#include <string.h>

#include <re.h>
#include <rem.h>
#include <baresip.h>
#include "aumix.h"

struct flac {
	FLAC__StreamEncoder *enc;
	FLAC__StreamMetadata *m[2];
	FLAC__int32 *pcm;
};


static void flac_destruct(void *arg)
{
	struct flac *flac = arg;

	mem_deref(flac->pcm);
	FLAC__stream_encoder_finish(flac->enc);
	FLAC__stream_encoder_delete(flac->enc);
	FLAC__metadata_object_delete(flac->m[0]);
	FLAC__metadata_object_delete(flac->m[1]);
}


int flac_init(struct flac **flacp, struct auframe *af, char *file)
{

	struct flac *flac;
	FLAC__bool ret;
	FLAC__StreamEncoderInitStatus init;
	FLAC__StreamMetadata_VorbisComment_Entry entry;


	if (!flacp || !af || !file)
		return EINVAL;

	flac = mem_zalloc(sizeof(struct flac), flac_destruct);
	if (!flac)
		return ENOMEM;

	flac->pcm = mem_zalloc(af->sampc * sizeof(FLAC__int32), NULL);


	flac->enc = FLAC__stream_encoder_new();
	if (!flac->enc)
		return ENOMEM;

	ret = FLAC__stream_encoder_set_verify(flac->enc, true);
	ret &= FLAC__stream_encoder_set_compression_level(flac->enc, 5);
	ret &= FLAC__stream_encoder_set_channels(flac->enc, af->ch);
	ret &= FLAC__stream_encoder_set_bits_per_sample(flac->enc, 16);
	ret &= FLAC__stream_encoder_set_sample_rate(flac->enc, af->srate);
	ret &= FLAC__stream_encoder_set_total_samples_estimate(flac->enc, 0);

	if (!ret) {
		warning("record: FLAC__stream_encoder_set\n");
		return EINVAL;
	}

	/* METADATA */
	flac->m[0] =
		FLAC__metadata_object_new(FLAC__METADATA_TYPE_VORBIS_COMMENT);
	flac->m[1] = FLAC__metadata_object_new(FLAC__METADATA_TYPE_PADDING);

	ret = FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(
		&entry, "ENCODED_BY", "STUDIO LINK MIX");

	ret &= FLAC__metadata_object_vorbiscomment_append_comment(
		flac->m[0], entry, /*copy=*/false);

	if (!ret) {
		warning("record: FLAC METADATA ERROR: out of memory or tag "
			"error\n");
		return ENOMEM;
	}

	flac->m[1]->length = 1234; /* padding length */

	ret = FLAC__stream_encoder_set_metadata(flac->enc, flac->m, 2);

	if (!ret) {
		warning("record: FLAC__stream_encoder_set_metadata\n");
		return ENOMEM;
	}

	init = FLAC__stream_encoder_init_file(flac->enc, file, NULL, NULL);

	if (init != FLAC__STREAM_ENCODER_INIT_STATUS_OK) {
		warning("record: FLAC ERROR: initializing encoder: %s\n",
			FLAC__StreamEncoderInitStatusString[init]);
		return ENOMEM;
	}

	*flacp = flac;

	return 0;
}


int flac_record(struct flac *flac, struct auframe *af, uint64_t offset)
{
	FLAC__StreamEncoderState state;
	FLAC__bool ret;

	if (!flac || !af || !af->ch)
		return EINVAL;

	if (offset > 24 * 3600 * 1000) {
		warning("flac_record: ignoring high >24h offset (%llu)\n",
			offset);
		offset = 0;
	}

	if (offset < 2 * PTIME) /* FIXME */
		offset = 0;

	if (offset) {
		warning("flac_record: offset %llu\n", offset);
		memset(flac->pcm, 0, af->sampc * sizeof(FLAC__int32));
		uint64_t offsampc = af->srate * af->ch * offset / 1000;

		while (offsampc) {
			ret = FLAC__stream_encoder_process_interleaved(
				flac->enc, flac->pcm,
				(uint32_t)af->sampc / af->ch);
			if (!ret)
				goto err;

			if (offsampc >= af->sampc)
				offsampc -= af->sampc;
			else {
				ret = FLAC__stream_encoder_process_interleaved(
					flac->enc, flac->pcm,
					(uint32_t)offsampc / af->ch);
				if (!ret)
					goto err;
				offsampc = 0;
			}
		}
	}

	int16_t *sampv = (int16_t *)af->sampv;

	for (size_t i = 0; i < af->sampc; i++) {
		flac->pcm[i] = sampv[i];
	}

	ret = FLAC__stream_encoder_process_interleaved(
		flac->enc, flac->pcm, (uint32_t)af->sampc / af->ch);
	if (ret)
		return 0;


err:
	state = FLAC__stream_encoder_get_state(flac->enc);
	warning("record: FLAC ENCODE ERROR: %s\n",
		FLAC__StreamEncoderStateString[state]);

	return EBADFD;
}
