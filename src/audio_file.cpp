#include "audio_file.hpp"

#include <stdint.h>

static const GenesisSampleFormat prioritized_sample_formats[] = {
    GenesisSampleFormatInt24,
    GenesisSampleFormatFloat,
    GenesisSampleFormatInt32,
    GenesisSampleFormatInt16,
    GenesisSampleFormatDouble,
    GenesisSampleFormatUInt8,
};

static const int prioritized_sample_rates[] = {
    48000,
    44100,
    96000,
    32000,
    16000,
};

/*
static const char *prioritized_formats = {
    "flac",
    "wav",
    "ogg",
    "opus",
    "mp3",
    "wav",
    "mp4",
    "wma",
    "wv",
};
*/

static void import_frame_uint8(const AVFrame *avframe, GenesisAudioFile *audio_file) {
    uint8_t *ptr = avframe->extended_data[0];
    double min = 0.0;
    double max = (double)UINT8_MAX;
    double half_range = max / 2.0 - min / 2.0;
    for (int frame = 0; frame < avframe->nb_samples; frame += 1) {
        for (long ch = 0; ch < audio_file->channels.length(); ch += 1, ptr += 1) {
            uint8_t sample = *ptr;
            double dbl_sample = (((double)sample) - min) / half_range - 1.0;
            audio_file->channels.at(ch).samples.append(dbl_sample);
        }
    }
}

static void import_frame_int16(const AVFrame *avframe, GenesisAudioFile *audio_file) {
    uint8_t *ptr = avframe->extended_data[0];
    double min = (double)INT16_MIN;
    double max = (double)INT16_MAX;
    double half_range = max / 2.0 - min / 2.0;
    for (int frame = 0; frame < avframe->nb_samples; frame += 1) {
        for (long ch = 0; ch < audio_file->channels.length(); ch += 1, ptr += 2) {
            int16_t *sample = reinterpret_cast<int16_t*>(ptr);
            double dbl_sample = (((double)*sample) - min) / half_range - 1.0;
            audio_file->channels.at(ch).samples.append(dbl_sample);
        }
    }
}

static void import_frame_int32(const AVFrame *avframe, GenesisAudioFile *audio_file) {
    uint8_t *ptr = avframe->extended_data[0];
    double min = (double)INT32_MIN;
    double max = (double)INT32_MAX;
    double half_range = max / 2.0 - min / 2.0;
    for (int frame = 0; frame < avframe->nb_samples; frame += 1) {
        for (long ch = 0; ch < audio_file->channels.length(); ch += 1, ptr += 4) {
            int32_t *sample = reinterpret_cast<int32_t*>(ptr);
            double dbl_sample = (((double)*sample) - min) / half_range - 1.0;
            audio_file->channels.at(ch).samples.append(dbl_sample);
        }
    }
}

static void import_frame_float(const AVFrame *avframe, GenesisAudioFile *audio_file) {
    uint8_t *ptr = avframe->extended_data[0];
    for (int frame = 0; frame < avframe->nb_samples; frame += 1) {
        for (long ch = 0; ch < audio_file->channels.length(); ch += 1, ptr += 4) {
            float *sample = reinterpret_cast<float*>(ptr);
            double dbl_sample = *sample;
            audio_file->channels.at(ch).samples.append(dbl_sample);
        }
    }
}

static void import_frame_double(const AVFrame *avframe, GenesisAudioFile *audio_file) {
    uint8_t *ptr = avframe->extended_data[0];
    for (int frame = 0; frame < avframe->nb_samples; frame += 1) {
        for (long ch = 0; ch < audio_file->channels.length(); ch += 1, ptr += 8) {
            double *sample = reinterpret_cast<double*>(ptr);
            audio_file->channels.at(ch).samples.append(*sample);
        }
    }
}

static void import_frame_uint8_planar(const AVFrame *avframe, GenesisAudioFile *audio_file) {
    double min = 0.0;
    double max = (double)UINT8_MAX;
    double half_range = max / 2.0 - min / 2.0;
    for (long ch = 0; ch < audio_file->channels.length(); ch += 1) {
        uint8_t *ptr = avframe->extended_data[ch];
        Channel *channel = &audio_file->channels.at(ch);
        for (int frame = 0; frame < avframe->nb_samples; frame += 1, ptr += 1) {
            uint8_t sample = *ptr;
            double dbl_sample = (((double)sample) - min) / half_range - 1.0;
            channel->samples.append(dbl_sample);
        }
    }
}

static void import_frame_int16_planar(const AVFrame *avframe, GenesisAudioFile *audio_file) {
    double min = (double)INT16_MIN;
    double max = (double)INT16_MAX;
    double half_range = max / 2.0 - min / 2.0;
    for (long ch = 0; ch < audio_file->channels.length(); ch += 1) {
        uint8_t *ptr = avframe->extended_data[ch];
        Channel *channel = &audio_file->channels.at(ch);
        for (int frame = 0; frame < avframe->nb_samples; frame += 1, ptr += 2) {
            int16_t *sample = reinterpret_cast<int16_t*>(ptr);
            double dbl_sample = (((double)*sample) - min) / half_range - 1.0;
            channel->samples.append(dbl_sample);
        }
    }
}

static void import_frame_int32_planar(const AVFrame *avframe, GenesisAudioFile *audio_file) {
    double min = (double)INT32_MIN;
    double max = (double)INT32_MAX;
    double half_range = max / 2.0 - min / 2.0;
    for (long ch = 0; ch < audio_file->channels.length(); ch += 1) {
        uint8_t *ptr = avframe->extended_data[ch];
        Channel *channel = &audio_file->channels.at(ch);
        for (int frame = 0; frame < avframe->nb_samples; frame += 1, ptr += 4) {
            int32_t *sample = reinterpret_cast<int32_t*>(ptr);
            double dbl_sample = (((double)*sample) - min) / half_range - 1.0;
            channel->samples.append(dbl_sample);
        }
    }
}

static void import_frame_float_planar(const AVFrame *avframe, GenesisAudioFile *audio_file) {
    for (long ch = 0; ch < audio_file->channels.length(); ch += 1) {
        uint8_t *ptr = avframe->extended_data[ch];
        Channel *channel = &audio_file->channels.at(ch);
        for (int frame = 0; frame < avframe->nb_samples; frame += 1, ptr += 4) {
            float *sample = reinterpret_cast<float*>(ptr);
            double dbl_sample = *sample;
            channel->samples.append(dbl_sample);
        }
    }
}

static void import_frame_double_planar(const AVFrame *avframe, GenesisAudioFile *audio_file) {
    for (long ch = 0; ch < audio_file->channels.length(); ch += 1) {
        uint8_t *ptr = avframe->extended_data[ch];
        Channel *channel = &audio_file->channels.at(ch);
        for (int frame = 0; frame < avframe->nb_samples; frame += 1, ptr += 8) {
            double *sample = reinterpret_cast<double*>(ptr);
            channel->samples.append(*sample);
        }
    }
}

static int decode_interrupt_cb(void *ctx) {
    return 0;
}

static int decode_frame(GenesisAudioFile *audio_file, AVPacket *pkt,
        AVCodecContext *codec_ctx, AVFrame *in_frame,
        void (*import_frame)(const AVFrame *, GenesisAudioFile *))
{
    AVPacket pkt_temp = *pkt;
    bool new_packet = true;
    int decoded_byte_count = 0;
    while (pkt_temp.size > 0 || (!pkt_temp.data && new_packet)) {
        new_packet = false;
        int got_frame;
        int len1 = avcodec_decode_audio4(codec_ctx, in_frame, &got_frame, &pkt_temp);
        if (len1 < 0) {
            char buf[256];
            av_strerror(len1, buf, sizeof(buf));
            fprintf(stderr, "error decoding audio: %s\n", buf);
            return -1;
        }
        pkt_temp.data += len1;
        pkt_temp.size -= len1;

        if (!got_frame) {
            // stop sending empty packets if the decoder is finished
            if (!pkt_temp.data && (codec_ctx->codec->capabilities & CODEC_CAP_DELAY))
                break;
            continue;
        }

        import_frame(in_frame, audio_file);
        decoded_byte_count += in_frame->nb_samples *
            av_get_bytes_per_sample((AVSampleFormat)in_frame->format) *
            av_get_channel_layout_nb_channels(in_frame->channel_layout);
    }

    return decoded_byte_count;
}

static GenesisChannelId from_libav_channel_id(uint64_t libav_channel_id) {
    switch (libav_channel_id) {
        case AV_CH_FRONT_LEFT:            return GenesisChannelIdFrontLeft;
        case AV_CH_FRONT_RIGHT:           return GenesisChannelIdFrontRight;
        case AV_CH_FRONT_CENTER:          return GenesisChannelIdFrontCenter;
        case AV_CH_LOW_FREQUENCY:         return GenesisChannelIdLowFrequency;
        case AV_CH_BACK_LEFT:             return GenesisChannelIdBackLeft;
        case AV_CH_BACK_RIGHT:            return GenesisChannelIdBackRight;
        case AV_CH_FRONT_LEFT_OF_CENTER:  return GenesisChannelIdFrontLeftOfCenter;
        case AV_CH_FRONT_RIGHT_OF_CENTER: return GenesisChannelIdFrontRightOfCenter;
        case AV_CH_BACK_CENTER:           return GenesisChannelIdBackCenter;
        case AV_CH_SIDE_LEFT:             return GenesisChannelIdSideLeft;
        case AV_CH_SIDE_RIGHT:            return GenesisChannelIdSideRight;
        case AV_CH_TOP_CENTER:            return GenesisChannelIdTopCenter;
        case AV_CH_TOP_FRONT_LEFT:        return GenesisChannelIdTopFrontLeft;
        case AV_CH_TOP_FRONT_CENTER:      return GenesisChannelIdTopFrontCenter;
        case AV_CH_TOP_FRONT_RIGHT:       return GenesisChannelIdTopFrontRight;
        case AV_CH_TOP_BACK_LEFT:         return GenesisChannelIdTopBackLeft;
        case AV_CH_TOP_BACK_CENTER:       return GenesisChannelIdTopBackCenter;
        case AV_CH_TOP_BACK_RIGHT:        return GenesisChannelIdTopBackRight;

        default: return GenesisChannelIdInvalid;
    }
}

static enum GenesisError channel_layout_init_from_libav(uint64_t libav_channel_layout,
        GenesisChannelLayout *layout)
{
    int channel_count = av_get_channel_layout_nb_channels(libav_channel_layout);
    if (layout->channel_count > GENESIS_MAX_CHANNELS)
        return GenesisErrorMaxChannelsExceeded;

    layout->channel_count = channel_count;
    for (int i = 0; i < layout->channel_count; i += 1) {
        uint64_t libav_channel_id = av_channel_layout_extract_channel(libav_channel_layout, i);
        GenesisChannelId channel_id = from_libav_channel_id(libav_channel_id);
        layout->channels[i] = channel_id;
    }

    int builtin_layout_count = genesis_channel_layout_builtin_count();
    for (int i = 0; i < builtin_layout_count; i += 1) {
        const GenesisChannelLayout *builtin_layout = genesis_channel_layout_get_builtin(i);
        if (genesis_channel_layout_equal(builtin_layout, layout)) {
            layout->name = builtin_layout->name;
            return GenesisErrorNone;
        }
    }

    layout->name = nullptr;
    return GenesisErrorNone;
}

enum GenesisError genesis_audio_file_load(struct GenesisContext *context,
        const char *input_filename, struct GenesisAudioFile **out_audio_file)
{
    *out_audio_file = nullptr;
    GenesisAudioFile *audio_file = create_zero<GenesisAudioFile>();
    if (!audio_file) {
        genesis_audio_file_destroy(audio_file);
        return GenesisErrorNoMem;
    }

    AVFormatContext *ic = avformat_alloc_context();
    if (!ic) {
        genesis_audio_file_destroy(audio_file);
        return GenesisErrorNoMem;
    }

    ic->interrupt_callback.callback = decode_interrupt_cb;
    ic->interrupt_callback.opaque = NULL;

    int err = avformat_open_input(&ic, input_filename, NULL, NULL);
    if (err < 0) {
        char buf[256];
        av_strerror(err, buf, sizeof(buf));
        panic("unable to open %s: %s", input_filename, buf);
    }

    err = avformat_find_stream_info(ic, NULL);
    if (err < 0) {
        char buf[256];
        av_strerror(err, buf, sizeof(buf));
        panic("unable to find stream in %s: %s", input_filename, buf);
    }

    // set all streams to discard. in a few lines here we will find the audio
    // stream and cancel discarding it
    for (long i = 0; i < ic->nb_streams; i += 1)
        ic->streams[i]->discard = AVDISCARD_ALL;

    AVCodec *decoder = NULL;
    int audio_stream_index = av_find_best_stream(ic, AVMEDIA_TYPE_AUDIO,
            -1, -1, &decoder, 0);
    if (audio_stream_index < 0)
        panic("no audio stream found");
    if (!decoder)
        panic("no decoder found");

    AVStream *audio_st = ic->streams[audio_stream_index];
    audio_st->discard = AVDISCARD_DEFAULT;

    AVCodecContext *codec_ctx = audio_st->codec;
    err = avcodec_open2(codec_ctx, decoder, NULL);
    if (err < 0) {
        char buf[256];
        av_strerror(err, buf, sizeof(buf));
        panic("unable to open decoder: %s", buf);
    }

    if (!codec_ctx->channel_layout)
        codec_ctx->channel_layout = av_get_default_channel_layout(codec_ctx->channels);
    if (!codec_ctx->channel_layout)
        panic("unable to guess channel layout");

    // copy the audio stream metadata to the context metadata
    av_dict_copy(&ic->metadata, audio_st->metadata, 0);

    AVDictionaryEntry *tag = NULL;
    audio_file->tags.clear();
    while ((tag = av_dict_get(ic->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))) {
        audio_file->tags.put(tag->key, tag->value);
    }

    GenesisError genesis_err = channel_layout_init_from_libav(codec_ctx->channel_layout,
           &audio_file->channel_layout);
    if (genesis_err)
        panic("unable to determine channel layout: %s", genesis_error_string(genesis_err));
    audio_file->sample_rate = codec_ctx->sample_rate;
    long channel_count = audio_file->channel_layout.channel_count;

    void (*import_frame)(const AVFrame *, GenesisAudioFile *);
    switch (codec_ctx->sample_fmt) {
        default:
            panic("unrecognized sample format");
            break;
        case AV_SAMPLE_FMT_U8:
            import_frame = import_frame_uint8;
            break;
        case AV_SAMPLE_FMT_S16:
            import_frame = import_frame_int16;
            break;
        case AV_SAMPLE_FMT_S32:
            import_frame = import_frame_int32;
            break;
        case AV_SAMPLE_FMT_FLT:
            import_frame = import_frame_float;
            break;
        case AV_SAMPLE_FMT_DBL:
            import_frame = import_frame_double;
            break;

        case AV_SAMPLE_FMT_U8P:
            import_frame = import_frame_uint8_planar;
            break;
        case AV_SAMPLE_FMT_S16P:
            import_frame = import_frame_int16_planar;
            break;
        case AV_SAMPLE_FMT_S32P:
            import_frame = import_frame_int32_planar;
            break;
        case AV_SAMPLE_FMT_FLTP:
            import_frame = import_frame_float_planar;
            break;
        case AV_SAMPLE_FMT_DBLP:
            import_frame = import_frame_double_planar;
            break;
    }

    audio_file->channels.resize(channel_count);
    for (long i = 0; i < audio_file->channels.length(); i += 1) {
        audio_file->channels.at(i).samples.clear();
    }

    AVFrame *in_frame = av_frame_alloc();
    if (!in_frame)
        panic("unable to allocate frame");

    AVPacket pkt;
    memset(&pkt, 0, sizeof(AVPacket));

    for (;;) {
        err = av_read_frame(ic, &pkt);
        if (err == AVERROR_EOF) {
            break;
        } else if (err < 0) {
            char buf[256];
            av_strerror(err, buf, sizeof(buf));
            panic("unable to read frame: %s", buf);
        }
        decode_frame(audio_file, &pkt, codec_ctx, in_frame, import_frame);
        av_free_packet(&pkt);
    }

    // flush
    for (;;) {
        av_init_packet(&pkt);
        pkt.data = NULL;
        pkt.size = 0;
        pkt.stream_index = audio_stream_index;
        if (decode_frame(audio_file, &pkt, codec_ctx, in_frame, import_frame) <= 0)
            break;
    }

    av_frame_free(&in_frame);
    avcodec_close(codec_ctx);
    avformat_close_input(&ic);

    *out_audio_file = audio_file;
    return GenesisErrorNone;
}

GenesisAudioFileCodec *audio_file_guess_audio_file_codec(
        List<GenesisAudioFileFormat*> &out_formats, const char *filename_hint,
        const char *format_name, const char *codec_name)
{
    AVOutputFormat *oformat = av_guess_format(format_name, filename_hint, nullptr);
    if (!oformat)
        return nullptr;

    for (int i = 0; i < out_formats.length(); i += 1) {
        GenesisAudioFileFormat *format = out_formats.at(i);
        if (format->oformat == oformat) {
            // there is always guaranteed to be the main codec, and we always
            // choose it here
            return &format->codecs.at(0);
        }
    }
    return nullptr;
}

static uint64_t closest_supported_channel_layout(AVCodec *codec, uint64_t target) {
    // if we can, return exact match. otherwise, return the layout with the
    // minimum number of channels >= target

    if (!codec->channel_layouts)
        return target;

    int target_count = av_get_channel_layout_nb_channels(target);
    const uint64_t *p = codec->channel_layouts;
    uint64_t best = *p;
    int best_count = av_get_channel_layout_nb_channels(best);
    while (*p) {
        if (*p == target)
            return target;

        int count = av_get_channel_layout_nb_channels(*p);
        if ((best_count < target_count && count > best_count) ||
            (count >= target_count &&
             abs_diff(target_count, count) < abs_diff(target_count, best_count)))
        {
            best_count = count;
            best = *p;
        }

        p += 1;
    }

    return best;
}

static bool get_is_planar(const AVCodec *codec, GenesisSampleFormat sample_format) {
    if (!codec->sample_fmts)
        return false;

    const enum AVSampleFormat *p = (enum AVSampleFormat*) codec->sample_fmts;
    while (*p != AV_SAMPLE_FMT_NONE) {
        if (sample_format == GenesisSampleFormatUInt8 && *p == AV_SAMPLE_FMT_U8)
            return false;
        if (sample_format == GenesisSampleFormatInt16 && *p == AV_SAMPLE_FMT_S16)
            return false;
        if (sample_format == GenesisSampleFormatInt24 && *p == AV_SAMPLE_FMT_S32)
            return false;
        if (sample_format == GenesisSampleFormatInt32 && *p == AV_SAMPLE_FMT_S32)
            return false;
        if (sample_format == GenesisSampleFormatFloat && *p == AV_SAMPLE_FMT_FLT)
            return false;
        if (sample_format == GenesisSampleFormatDouble && *p == AV_SAMPLE_FMT_DBL)
            return false;

        p += 1;
    }

    p = (enum AVSampleFormat*) codec->sample_fmts;
    while (*p != AV_SAMPLE_FMT_NONE) {
        if (sample_format == GenesisSampleFormatUInt8 && *p == AV_SAMPLE_FMT_U8P)
            return true;
        if (sample_format == GenesisSampleFormatInt16 && *p == AV_SAMPLE_FMT_S16P)
            return true;
        if (sample_format == GenesisSampleFormatInt24 && *p == AV_SAMPLE_FMT_S32P)
            return true;
        if (sample_format == GenesisSampleFormatInt32 && *p == AV_SAMPLE_FMT_S32P)
            return true;
        if (sample_format == GenesisSampleFormatFloat && *p == AV_SAMPLE_FMT_FLTP)
            return true;
        if (sample_format == GenesisSampleFormatDouble && *p == AV_SAMPLE_FMT_DBLP)
            return true;

        p += 1;
    }

    panic("format not supported planar or non planar");
}

static void set_codec_ctx_format(AVCodecContext *codec_ctx, GenesisExportFormat *format,
        bool *is_planar)
{
    *is_planar = get_is_planar(codec_ctx->codec, format->sample_format);
    switch (format->sample_format) {
        case GenesisSampleFormatUInt8:
            codec_ctx->bits_per_raw_sample = 8;
            codec_ctx->sample_fmt = *is_planar ? AV_SAMPLE_FMT_U8P : AV_SAMPLE_FMT_U8;
            return;
        case GenesisSampleFormatInt16:
            codec_ctx->bits_per_raw_sample = 16;
            codec_ctx->sample_fmt = *is_planar ? AV_SAMPLE_FMT_S16P : AV_SAMPLE_FMT_S16;
            return;
        case GenesisSampleFormatInt24:
            codec_ctx->bits_per_raw_sample = 24;
            codec_ctx->sample_fmt = *is_planar ? AV_SAMPLE_FMT_S32P : AV_SAMPLE_FMT_S32;
            return;
        case GenesisSampleFormatInt32:
            codec_ctx->bits_per_raw_sample = 32;
            codec_ctx->sample_fmt = *is_planar ? AV_SAMPLE_FMT_S32P : AV_SAMPLE_FMT_S32;
            return;
        case GenesisSampleFormatFloat:
            codec_ctx->bits_per_raw_sample = 32;
            codec_ctx->sample_fmt = *is_planar ? AV_SAMPLE_FMT_FLTP : AV_SAMPLE_FMT_FLT;
            return;
        case GenesisSampleFormatDouble:
            codec_ctx->bits_per_raw_sample = 64;
            codec_ctx->sample_fmt = *is_planar ? AV_SAMPLE_FMT_DBLP : AV_SAMPLE_FMT_DBL;
            return;
        case GenesisSampleFormatInvalid:
            panic("invalid sample format");
    }
    panic("invalid sample format");
}

static void write_frames_uint8_planar(const GenesisAudioFile *audio_file,
        long start, long end, uint8_t *buffer, AVFrame *frame)
{
    for (long ch = 0; ch < audio_file->channels.length(); ch += 1) {
        uint8_t *ch_buf = frame->extended_data[ch];
        for (long i = start; i < end; i += 1) {
            float sample = audio_file->channels.at(ch).samples.at(i);
            *ch_buf = (uint8_t)((sample * 127.5) + 127.5);
            ch_buf += 1;
        }
    }
}

static void write_frames_int16_planar(const GenesisAudioFile *audio_file,
        long start, long end, uint8_t *buffer, AVFrame *frame)
{
    for (long ch = 0; ch < audio_file->channels.length(); ch += 1) {
        int16_t *ch_buf = reinterpret_cast<int16_t*>(frame->extended_data[ch]);
        for (long i = start; i < end; i += 1) {
            float sample = audio_file->channels.at(ch).samples.at(i);
            *ch_buf = (int16_t)(sample * 32767.0);
            ch_buf += 1;
        }
    }
}

static void write_frames_int32_planar(const GenesisAudioFile *audio_file,
        long start, long end, uint8_t *buffer, AVFrame *frame)
{
    for (long ch = 0; ch < audio_file->channels.length(); ch += 1) {
        int32_t *ch_buf = reinterpret_cast<int32_t*>(frame->extended_data[ch]);
        for (long i = start; i < end; i += 1) {
            float sample = audio_file->channels.at(ch).samples.at(i);
            *ch_buf = (int32_t)(sample * 2147483647.0);
            ch_buf += 1;
        }
    }
}

static void write_frames_int24_planar(const GenesisAudioFile *audio_file,
        long start, long end, uint8_t *buffer, AVFrame *frame)
{
    for (long ch = 0; ch < audio_file->channels.length(); ch += 1) {
        int32_t *ch_buf = reinterpret_cast<int32_t*>(frame->extended_data[ch]);
        for (long i = start; i < end; i += 1) {
            float sample = audio_file->channels.at(ch).samples.at(i);
            // libav looks at the most significant bytes
            *ch_buf = (int32_t)(sample * 8388607.0);
            ch_buf += 1;
        }
    }
}

static void write_frames_float_planar(const GenesisAudioFile *audio_file,
        long start, long end, uint8_t *buffer, AVFrame *frame)
{
    for (long ch = 0; ch < audio_file->channels.length(); ch += 1) {
        float *ch_buf = reinterpret_cast<float*>(frame->extended_data[ch]);
        for (long i = start; i < end; i += 1) {
            float sample = audio_file->channels.at(ch).samples.at(i);
            *ch_buf = (float)sample;
            ch_buf += 1;
        }
    }
}

static void write_frames_double_planar(const GenesisAudioFile *audio_file,
        long start, long end, uint8_t *buffer, AVFrame *frame)
{
    for (long ch = 0; ch < audio_file->channels.length(); ch += 1) {
        double *ch_buf = reinterpret_cast<double*>(frame->extended_data[ch]);
        for (long i = start; i < end; i += 1) {
            float sample = audio_file->channels.at(ch).samples.at(i);
            *ch_buf = sample;
            ch_buf += 1;
        }
    }
}

static void write_frames_uint8(const GenesisAudioFile *audio_file,
        long start, long end, uint8_t *buffer, AVFrame *)
{
    for (long i = start; i < end; i += 1) {
        for (long ch = 0; ch < audio_file->channels.length(); ch += 1) {
            float sample = audio_file->channels.at(ch).samples.at(i);

            *buffer = (uint8_t)((sample * 127.5) + 127.5);

            buffer += 1;
        }
    }
}

static void write_frames_int16(const GenesisAudioFile *audio_file,
        long start, long end, uint8_t *buffer, AVFrame *)
{
    for (long i = start; i < end; i += 1) {
        for (long ch = 0; ch < audio_file->channels.length(); ch += 1) {
            float sample = audio_file->channels.at(ch).samples.at(i);

            int16_t *int_ptr = reinterpret_cast<int16_t*>(buffer);
            *int_ptr = (int16_t)(sample * 32767.0);

            buffer += 2;
        }
    }
}

static void write_frames_int32(const GenesisAudioFile *audio_file,
        long start, long end, uint8_t *buffer, AVFrame *)
{
    for (long i = start; i < end; i += 1) {
        for (long ch = 0; ch < audio_file->channels.length(); ch += 1) {
            float sample = audio_file->channels.at(ch).samples.at(i);

            int32_t *int_ptr = reinterpret_cast<int32_t*>(buffer);
            *int_ptr = (int32_t)(sample * 2147483647.0);

            buffer += 4;
        }
    }
}

static void write_frames_int24(const GenesisAudioFile *audio_file,
        long start, long end, uint8_t *buffer, AVFrame *)
{
    for (long i = start; i < end; i += 1) {
        for (long ch = 0; ch < audio_file->channels.length(); ch += 1) {
            float sample = audio_file->channels.at(ch).samples.at(i);

            int32_t *int_ptr = reinterpret_cast<int32_t*>(buffer);
            // libav looks at the most significant bytes
            *int_ptr = (int32_t)(sample * 8388607.0);

            buffer += 4;
        }
    }
}

static void write_frames_float(const GenesisAudioFile *audio_file,
        long start, long end, uint8_t *buffer, AVFrame *)
{
    for (long i = start; i < end; i += 1) {
        for (long ch = 0; ch < audio_file->channels.length(); ch += 1) {
            float sample = audio_file->channels.at(ch).samples.at(i);

            float *float_ptr = reinterpret_cast<float*>(buffer);
            *float_ptr = (float)sample;

            buffer += 4;
        }
    }
}

static void write_frames_double(const GenesisAudioFile *audio_file,
        long start, long end, uint8_t *buffer, AVFrame *)
{
    for (long i = start; i < end; i += 1) {
        for (long ch = 0; ch < audio_file->channels.length(); ch += 1) {
            float sample = audio_file->channels.at(ch).samples.at(i);
            double *double_ptr = reinterpret_cast<double*>(buffer);
            *double_ptr = sample;
            buffer += 8;
        }
    }
}

static uint64_t to_libav_channel_id(enum GenesisChannelId channel_id) {
    switch (channel_id) {
    case GenesisChannelIdInvalid: panic("invalid channel id");
    case GenesisChannelIdFrontLeft: return AV_CH_FRONT_LEFT;
    case GenesisChannelIdFrontRight: return AV_CH_FRONT_RIGHT;
    case GenesisChannelIdFrontCenter: return AV_CH_FRONT_CENTER;
    case GenesisChannelIdLowFrequency: return AV_CH_LOW_FREQUENCY;
    case GenesisChannelIdBackLeft: return AV_CH_BACK_LEFT;
    case GenesisChannelIdBackRight: return AV_CH_BACK_RIGHT;
    case GenesisChannelIdFrontLeftOfCenter: return AV_CH_FRONT_LEFT_OF_CENTER;
    case GenesisChannelIdFrontRightOfCenter: return AV_CH_FRONT_RIGHT_OF_CENTER;
    case GenesisChannelIdBackCenter: return AV_CH_BACK_CENTER;
    case GenesisChannelIdSideLeft: return AV_CH_SIDE_LEFT;
    case GenesisChannelIdSideRight: return AV_CH_SIDE_RIGHT;
    case GenesisChannelIdTopCenter: return AV_CH_TOP_CENTER;
    case GenesisChannelIdTopFrontLeft: return AV_CH_TOP_FRONT_LEFT;
    case GenesisChannelIdTopFrontCenter: return AV_CH_TOP_FRONT_CENTER;
    case GenesisChannelIdTopFrontRight: return AV_CH_TOP_FRONT_RIGHT;
    case GenesisChannelIdTopBackLeft: return AV_CH_TOP_BACK_LEFT;
    case GenesisChannelIdTopBackCenter: return AV_CH_TOP_BACK_CENTER;
    case GenesisChannelIdTopBackRight: return AV_CH_TOP_BACK_RIGHT;
    }
    panic("invalid channel id");
}


static uint64_t channel_layout_to_libav(const GenesisChannelLayout *channel_layout) {
    uint64_t result = 0;
    for (int i = 0; i < channel_layout->channel_count; i += 1) {
        GenesisChannelId channel_id = channel_layout->channels[i];
        result |= to_libav_channel_id(channel_id);
    }
    return result;
}

enum GenesisError genesis_audio_file_export(struct GenesisAudioFile *audio_file,
        const char *output_filename, struct GenesisExportFormat *export_format)
{
    AVCodec *codec = export_format->codec->codec;
    AVOutputFormat *oformat = export_format->codec->format->oformat;

    if (!av_codec_is_encoder(codec))
        panic("not encoder: %s\n", codec->name);

    uint64_t target_channel_layout = channel_layout_to_libav(&audio_file->channel_layout);
    uint64_t out_channel_layout = closest_supported_channel_layout(codec, target_channel_layout);

    AVFormatContext *fmt_ctx = avformat_alloc_context();
    if (!fmt_ctx)
        return GenesisErrorNoMem;

    int err = avio_open(&fmt_ctx->pb, output_filename, AVIO_FLAG_WRITE);
    if (err < 0) {
        char buf[256];
        av_strerror(err, buf, sizeof(buf));
        panic("could not open %s: %s", output_filename, buf);
    }

    fmt_ctx->oformat = oformat;

    AVStream *stream = avformat_new_stream(fmt_ctx, codec);
    if (!stream)
        panic("unable to create output stream");

    stream->time_base.den = export_format->sample_rate;
    stream->time_base.num = 1;

    AVCodecContext *codec_ctx = stream->codec;
    codec_ctx->bit_rate = export_format->bit_rate;
    bool is_planar;
    set_codec_ctx_format(codec_ctx, export_format, &is_planar);
    codec_ctx->sample_rate = export_format->sample_rate;
    codec_ctx->channel_layout = out_channel_layout;
    codec_ctx->channels = audio_file->channels.length();
    codec_ctx->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;

    if (fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
        codec_ctx->flags |= CODEC_FLAG_GLOBAL_HEADER;

    err = avcodec_open2(codec_ctx, codec, NULL);
    if (err < 0) {
        char buf[256];
        av_strerror(err, buf, sizeof(buf));
        panic("unable to open codec: %s", buf);
    }

    // copy metadata to format context
    av_dict_free(&fmt_ctx->metadata);
    auto it = audio_file->tags.entry_iterator();
    for (;;) {
        auto *entry = it.next();
        if (!entry)
            break;
        av_dict_set(&fmt_ctx->metadata, entry->key.raw(), entry->value.raw(),
                AV_DICT_IGNORE_SUFFIX);
    }

    err = avformat_write_header(fmt_ctx, NULL);
    if (err < 0) {
        char buf[256];
        av_strerror(err, buf, sizeof(buf));
        panic("error writing header: %s", buf);
    }


    AVFrame *frame = av_frame_alloc();
    if (!frame)
        panic("error allocating frame");

    bool var_frame_size_1 = (codec->capabilities & CODEC_CAP_VARIABLE_FRAME_SIZE);
    bool var_frame_size_2 = (codec_ctx->frame_size == 0);

    if (var_frame_size_1 != var_frame_size_2)
        panic("inconsistent variable frame size");

    int buffer_size;
    if (codec_ctx->frame_size) {
        buffer_size = av_samples_get_buffer_size(NULL, codec_ctx->channels,
            codec_ctx->frame_size, codec_ctx->sample_fmt, 0);
        if (buffer_size < 0) {
            char buf[256];
            av_strerror(err, buf, sizeof(buf));
            panic("error determining buffer size: %s", buf);
        }
    } else {
        buffer_size = 16 * 1024;
    }
    int bytes_per_sample = av_get_bytes_per_sample(codec_ctx->sample_fmt);
    int frame_count = buffer_size / bytes_per_sample / codec_ctx->channels;

    frame->pts = 0;
    frame->nb_samples = frame_count;
    frame->format = codec_ctx->sample_fmt;
    frame->channel_layout = codec_ctx->channel_layout;

    uint8_t *buffer = allocate<uint8_t>(buffer_size);

    err = avcodec_fill_audio_frame(frame, codec_ctx->channels, codec_ctx->sample_fmt,
            buffer, buffer_size, 0);
    if (err < 0) {
        char buf[256];
        av_strerror(err, buf, sizeof(buf));
        panic("error setting up audio frame: %s", buf);
    }

    void (*write_frames)(const GenesisAudioFile *, long, long, uint8_t *, AVFrame *);

    if (is_planar) {
        switch (export_format->sample_format) {
            case GenesisSampleFormatInvalid:
                panic("invalid sample format");
            case GenesisSampleFormatUInt8:
                write_frames = write_frames_uint8_planar;
                break;
            case GenesisSampleFormatInt16:
                write_frames = write_frames_int16_planar;
                break;
            case GenesisSampleFormatInt24:
                write_frames = write_frames_int24_planar;
                break;
            case GenesisSampleFormatInt32:
                write_frames = write_frames_int32_planar;
                break;
            case GenesisSampleFormatFloat:
                write_frames = write_frames_float_planar;
                break;
            case GenesisSampleFormatDouble:
                write_frames = write_frames_double_planar;
                break;
        }
    } else {
        switch (export_format->sample_format) {
            case GenesisSampleFormatInvalid:
                panic("invalid sample format");
            case GenesisSampleFormatUInt8:
                write_frames = write_frames_uint8;
                break;
            case GenesisSampleFormatInt16:
                write_frames = write_frames_int16;
                break;
            case GenesisSampleFormatInt24:
                write_frames = write_frames_int24;
                break;
            case GenesisSampleFormatInt32:
                write_frames = write_frames_int32;
                break;
            case GenesisSampleFormatFloat:
                write_frames = write_frames_float;
                break;
            case GenesisSampleFormatDouble:
                write_frames = write_frames_double;
                break;
        }
    }

    long source_frame_count = audio_file->channels.at(0).samples.length();

    AVPacket pkt;
    long start = 0;
    for (;;) {
        av_init_packet(&pkt);
        pkt.data = NULL; // packet data will be allocated by the encoder
        pkt.size = 0;

        long end = min(start + frame_count, source_frame_count);
        write_frames(audio_file, start, end, buffer, frame);
        start = end;

        int got_packet = 0;
        err = avcodec_encode_audio2(codec_ctx, &pkt, frame, &got_packet);
        if (err < 0) {
            char buf[256];
            av_strerror(err, buf, sizeof(buf));
            panic("error encoding audio frame: %s", buf);
        }
        if (got_packet) {
            err = av_write_frame(fmt_ctx, &pkt);
            if (err < 0)
                panic("error writing frame");
            av_free_packet(&pkt);
        }
        if (end == source_frame_count)
            break;

        frame->pts += frame_count;
    }
    for (;;) {
        int got_packet = 0;
        err = avcodec_encode_audio2(codec_ctx, &pkt, NULL, &got_packet);
        if (err < 0) {
            char buf[256];
            av_strerror(err, buf, sizeof(buf));
            panic("error encoding audio frame: %s", buf);
        }
        if (got_packet) {
            err = av_write_frame(fmt_ctx, &pkt);
            if (err < 0)
                panic("error writing frame");
            av_free_packet(&pkt);
        } else {
            break;
        }
    }
    for (;;) {
        err = av_write_frame(fmt_ctx, NULL);
        if (err < 0) {
            panic("error flushing format");
        } else if (err == 1) {
            break;
        }
    }

    destroy(buffer, buffer_size);
    av_frame_free(&frame);

    err = av_write_trailer(fmt_ctx);
    if (err < 0)
        panic("error writing trailer");

    avio_closep(&fmt_ctx->pb);
    avcodec_close(codec_ctx);
    avformat_free_context(fmt_ctx);

    return GenesisErrorNone;
}

void audio_file_init(void) {
    avcodec_register_all();
    av_register_all();
}

void genesis_audio_file_destroy(struct GenesisAudioFile *audio_file) {
    if (audio_file) {
        destroy(audio_file, 1);
    }
}

static void add_audio_file_codec(GenesisAudioFileFormat *fmt, AVCodec *codec) {
    if (!codec)
        return;
    GenesisAudioFileCodec *audio_file_codec = &fmt->codecs.add_one();
    audio_file_codec->codec = codec;
    audio_file_codec->format = fmt;
    for (int j = 0; j < array_length(prioritized_sample_formats); j += 1) {
        if (genesis_audio_file_codec_supports_sample_format(audio_file_codec,
                    prioritized_sample_formats[j]))
        {
            audio_file_codec->prioritized_sample_formats.append(prioritized_sample_formats[j]);
        }
    }
    for (int j = 0; j < array_length(prioritized_sample_rates); j += 1) {
        if (genesis_audio_file_codec_supports_sample_rate(audio_file_codec,
                    prioritized_sample_rates[j]))
        {
            audio_file_codec->prioritized_sample_rates.append(prioritized_sample_rates[j]);
        }
    }
}

void audio_file_get_out_formats(List<GenesisAudioFileFormat*> &formats) {
    formats.clear();
    AVOutputFormat *oformat = nullptr;
    while ((oformat = av_oformat_next(oformat))) {
        if (oformat->audio_codec == AV_CODEC_ID_NONE)
            continue;
        AVCodec *main_codec = avcodec_find_encoder(oformat->audio_codec);
        if (!main_codec)
            continue;

        GenesisAudioFileFormat *fmt = create<GenesisAudioFileFormat>();
        formats.append(fmt);

        fmt->iformat = nullptr;
        fmt->oformat = oformat;

        fmt->codecs.clear();
        add_audio_file_codec(fmt, main_codec);
        for (int i = 0;; i += 1) {
            AVCodecID codec_id = av_codec_get_id(oformat->codec_tag, i);
            if (codec_id == AV_CODEC_ID_NONE)
                break;
            if (codec_id != oformat->audio_codec)
                add_audio_file_codec(fmt, avcodec_find_encoder(codec_id));
        }
    }
}

void audio_file_get_in_formats(List<GenesisAudioFileFormat*> &formats) {
    formats.clear();
    AVInputFormat *iformat = nullptr;
    while ((iformat = av_iformat_next(iformat))) {
        GenesisAudioFileFormat *fmt = create<GenesisAudioFileFormat>();
        formats.append(fmt);
        fmt->iformat = iformat;
        fmt->oformat = nullptr;

        fmt->codecs.clear();
        for (int i = 0;; i += 1) {
            AVCodecID codec_id = av_codec_get_id(iformat->codec_tag, i);
            if (codec_id == AV_CODEC_ID_NONE)
                break;

            GenesisAudioFileCodec *audio_file_codec = &fmt->codecs.add_one();
            audio_file_codec->codec = avcodec_find_decoder(codec_id);
            audio_file_codec->format = fmt;
        }
    }
}

const char *genesis_audio_file_format_name(const struct GenesisAudioFileFormat *format) {
    if (format->oformat) {
        return format->oformat->name;
    } else if (format->iformat) {
        return format->iformat->name;
    }
    panic("oformat or iformat must be set");
}

const char *genesis_audio_file_format_description(const struct GenesisAudioFileFormat *format) {
    if (format->oformat) {
        return format->oformat->long_name;
    } else if (format->iformat) {
        return format->iformat->long_name;
    }
    panic("oformat or iformat must be set");
}

int genesis_audio_file_format_codec_count(const struct GenesisAudioFileFormat *format) {
    return format->codecs.length();
}

struct GenesisAudioFileCodec * genesis_audio_file_format_codec_index(
        struct GenesisAudioFileFormat *format, int codec_index)
{
    return &format->codecs.at(codec_index);
}

const char *genesis_audio_file_codec_name(const struct GenesisAudioFileCodec *audio_file_codec) {
    return audio_file_codec->codec->name;
}

const char *genesis_audio_file_codec_description(const struct GenesisAudioFileCodec *audio_file_codec) {
    return audio_file_codec->codec->long_name;
}

struct GenesisChannelLayout genesis_audio_file_channel_layout(const struct GenesisAudioFile *audio_file) {
    return audio_file->channel_layout;
}

long genesis_audio_file_frame_count(const struct GenesisAudioFile *audio_file) {
    return audio_file->channels.at(0).samples.length();
}

int genesis_audio_file_sample_rate(const struct GenesisAudioFile *audio_file) {
    return audio_file->sample_rate;
}

struct GenesisAudioFileIterator genesis_audio_file_iterator(
        struct GenesisAudioFile *audio_file, int channel_index, long start_frame_index)
{
    long frame_count = genesis_audio_file_frame_count(audio_file);
    return {
        audio_file,
        start_frame_index,
        frame_count,
        audio_file->channels.at(channel_index).samples.raw(),
    };
}

void genesis_audio_file_iterator_next(struct GenesisAudioFileIterator *it) {
    long frame_count = genesis_audio_file_frame_count(it->audio_file);
    it->start = frame_count;
    it->end = frame_count;
    it->ptr = nullptr;
}

bool genesis_audio_file_codec_supports_sample_format(
        const struct GenesisAudioFileCodec *audio_file_codec,
        enum GenesisSampleFormat sample_format)
{
    AVCodec *codec = audio_file_codec->codec;

    if (!codec->sample_fmts)
        return true;

    const enum AVSampleFormat *p = (enum AVSampleFormat*) codec->sample_fmts;
    while (*p != AV_SAMPLE_FMT_NONE) {
        if (sample_format == GenesisSampleFormatUInt8 && (*p == AV_SAMPLE_FMT_U8P || *p == AV_SAMPLE_FMT_U8))
            return true;
        if (sample_format == GenesisSampleFormatInt16 && (*p == AV_SAMPLE_FMT_S16P || *p == AV_SAMPLE_FMT_S16))
            return true;
        if (sample_format == GenesisSampleFormatInt24 && (*p == AV_SAMPLE_FMT_S32P || *p == AV_SAMPLE_FMT_S32))
            return true;
        if (sample_format == GenesisSampleFormatInt32 && (*p == AV_SAMPLE_FMT_S32P || *p == AV_SAMPLE_FMT_S32))
            return true;
        if (sample_format == GenesisSampleFormatFloat && (*p == AV_SAMPLE_FMT_FLTP || *p == AV_SAMPLE_FMT_FLT))
            return true;
        if (sample_format == GenesisSampleFormatDouble && (*p == AV_SAMPLE_FMT_DBLP || *p == AV_SAMPLE_FMT_DBL))
            return true;

        p += 1;
    }

    return false;
}

bool genesis_audio_file_codec_supports_sample_rate(
        const struct GenesisAudioFileCodec *audio_file_codec, int sample_rate)
{
    AVCodec *codec = audio_file_codec->codec;
    if (!codec->supported_samplerates)
        return true;

    const int *p = codec->supported_samplerates;
    while (*p) {
        if (*p == sample_rate)
            return true;

        p += 1;
    }

    return false;
}
