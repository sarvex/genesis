#ifndef AUDIO_FILE_HPP
#define AUDIO_FILE_HPP

#include "genesis.h"
#include "list.hpp"
#include "hash_map.hpp"
#include "byte_buffer.hpp"
#include "ffmpeg.hpp"

struct Channel {
    List<float> samples;
};

struct GenesisAudioFile {
    List<Channel> channels;
    SoundIoChannelLayout channel_layout;
    int sample_rate;
    HashMap<ByteBuffer, ByteBuffer, ByteBuffer::hash> tags;

    // private
    AVFormatContext *ic;
    AVCodecContext *codec_ctx;
    AVFrame *in_frame;
};

struct GenesisAudioFileCodec {
    GenesisAudioFileFormat *audio_file_format;
    GenesisRenderFormat *render_format;
    AVCodec *codec;
    List<SoundIoFormat> sample_format_list;
    List<int> sample_rate_list;
    List<int> bit_rate_list;
    bool has_bit_rate;
};

// When modifying, see prioritized_render_formats in audio_file.cpp
enum RenderFormatType {
    RenderFormatTypeInvalid = -1,

    RenderFormatTypeFlac = 0,
    RenderFormatTypeVorbis,
    RenderFormatTypeOpus,
    RenderFormatTypeWav,
    RenderFormatTypeMp3,
    RenderFormatTypeAac,

    RenderFormatTypeCount,
};

struct GenesisAudioFileFormat {
    List<GenesisAudioFileCodec> codecs;
    AVInputFormat *iformat;
};

struct GenesisRenderFormat {
    GenesisAudioFileCodec codec;
    AVOutputFormat *oformat;
    const char *description;
    const char *extension;
    RenderFormatType render_format_type;
};


int audio_file_init(void);
int __attribute__((warn_unused_result)) audio_file_get_out_formats(List<GenesisRenderFormat*> &formats);
int __attribute__((warn_unused_result)) audio_file_get_in_formats(List<GenesisAudioFileFormat*> &formats);
GenesisAudioFileCodec *audio_file_guess_audio_file_codec(
        List<GenesisRenderFormat*> &out_formats, const char *filename_hint,
        const char *format_name, const char *codec_name);

uint64_t channel_layout_to_libav(const SoundIoChannelLayout *channel_layout);

int audio_file_sample_rate_count(void);
int audio_file_sample_rate_index(int index);

#endif
