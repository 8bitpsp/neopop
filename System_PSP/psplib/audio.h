/** PSP helper library ***************************************/
/**                                                         **/
/**                          audio.h                        **/
/**                                                         **/
/** This file contains definitions for the audio rendering  **/
/** library. It is based on the pspaudio library by Adresd  **/
/** and Marcus R. Brown, 2005.                              **/
/**                                                         **/
/** Akop Karapetyan 2007                                    **/
/*************************************************************/
#ifndef _PSP_AUDIO_H
#define _PSP_AUDIO_H

#ifdef __cplusplus
extern "C" {
#endif

#define PSP_AUDIO_SAMPLE_ALIGN(s) (((s) + 63) & ~63)
#define PSP_AUDIO_MAX_VOLUME      0x8000

typedef struct 
{
  short Left;
  short Right;
} PspSample;

typedef void (*pspAudioCallback)(void *buf, unsigned int *length, void *userdata);

int  pspAudioInit(int sample_count);
void pspAudioSetVolume(int channel, int left, int right);
void pspAudioSetChannelCallback(int channel, pspAudioCallback callback, void *userdata);
void pspAudioShutdown();
int  pspAudioOutputBlocking(unsigned int channel, void *buf);
int  pspAudioGetSampleCount();

#ifdef __cplusplus
}
#endif

#endif // _PSP_AUDIO_H

