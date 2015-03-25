#ifndef AUDIO_FILE_HPP
#define AUDIO_FILE_HPP

#include "genesis.h"
#include "list.hpp"
#include "hash_map.hpp"
#include "byte_buffer.hpp"
#include "libav.hpp"

struct Channel {
    List<float> samples;
};

struct GenesisAudioFile {
    List<Channel> channels;
    GenesisChannelLayout channel_layout;
    int sample_rate;
    HashMap<ByteBuffer, ByteBuffer, ByteBuffer::hash> tags;
};

struct GenesisAudioFileCodec {
    GenesisAudioFileFormat *format;
    AVCodec *codec;
    List<GenesisSampleFormat> prioritized_sample_formats;
    List<int> prioritized_sample_rates;
};

struct GenesisAudioFileFormat {
    List<GenesisAudioFileCodec> codecs;
    AVOutputFormat *oformat;
    AVInputFormat *iformat;
};

void audio_file_init(void);
int __attribute__((warn_unused_result)) audio_file_get_out_formats(List<GenesisAudioFileFormat*> &formats);
int __attribute__((warn_unused_result)) audio_file_get_in_formats(List<GenesisAudioFileFormat*> &formats);
GenesisAudioFileCodec *audio_file_guess_audio_file_codec(
        List<GenesisAudioFileFormat*> &out_formats, const char *filename_hint,
        const char *format_name, const char *codec_name);

#endif
