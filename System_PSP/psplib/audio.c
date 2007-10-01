/** PSP helper library ***************************************/
/**                                                         **/
/**                          audio.c                        **/
/**                                                         **/
/** This file contains the audio rendering library. It is   **/
/** based almost entirely on the pspaudio library by Adresd **/
/** and Marcus R. Brown, 2005.                              **/
/**                                                         **/
/** Akop Karapetyan 2007                                    **/
/*************************************************************/
#include "audio.h"

#include <stdio.h>
#include <pspaudio.h>
#include <pspthreadman.h>
#include <string.h>
#include <malloc.h>

#define AUDIO_CHANNELS       1
#define DEFAULT_SAMPLE_COUNT 512

static int AudioReady;
static volatile int StopAudio;
static int SampleCount;

typedef struct {
  int ThreadHandle;
  int Handle;
  int LeftVolume;
  int RightVolume;
  pspAudioCallback Callback;
  void *Userdata;
} ChannelInfo;

static int DirectHandle;
static ChannelInfo AudioStatus[AUDIO_CHANNELS];
static short *AudioBuffer[AUDIO_CHANNELS][2];

static int AudioChannelThread(int args, void *argp);
static void FreeBuffers();
static int OutputBlocking(unsigned int channel, 
  unsigned int vol1, unsigned int vol2, void *buf, int length);

int pspAudioInit(int sample_count)
{
  int i, j, failed;

  StopAudio = 0;
  AudioReady = 0;
  DirectHandle = -1;

  for (i = 0; i < AUDIO_CHANNELS; i++)
  {
    AudioStatus[i].Handle = -1;
    AudioStatus[i].ThreadHandle = -1;
    AudioStatus[i].LeftVolume = PSP_AUDIO_MAX_VOLUME;
    AudioStatus[i].RightVolume = PSP_AUDIO_MAX_VOLUME;
    AudioStatus[i].Callback = NULL;
    AudioStatus[i].Userdata = NULL;

    for (j = 0; j < 2; j++)
      AudioBuffer[i][j] = NULL;
  }

  DirectHandle = sceAudioChReserve(PSP_AUDIO_NEXT_CHANNEL, 
    DEFAULT_SAMPLE_COUNT, PSP_AUDIO_FORMAT_STEREO);

  /* No callback */
  if ((SampleCount = sample_count) == 0)
    return 1;

  /* Check sample count */
  if ((SampleCount = sample_count) < 0)
    SampleCount = DEFAULT_SAMPLE_COUNT;
  SampleCount = PSP_AUDIO_SAMPLE_ALIGN(SampleCount);

  /* Initialize buffers */
  for (i = 0; i < AUDIO_CHANNELS; i++)
  {
    for (j = 0; j < 2; j++)
    {
      if (!(AudioBuffer[i][j] = (short*)malloc(SampleCount * 2 * sizeof(short))))
      {
        FreeBuffers();
        return 0;
      }
    }
  }

  /* Initialize channels */
  for (i = 0, failed = 0; i < AUDIO_CHANNELS; i++)
  {
    AudioStatus[i].Handle = sceAudioChReserve(PSP_AUDIO_NEXT_CHANNEL, 
      SampleCount, PSP_AUDIO_FORMAT_STEREO);

    if (AudioStatus[i].Handle < 0)
    { 
      failed = 1;
      break;
    }
  }

  if (!failed)
	  DirectHandle = sceAudioChReserve(PSP_AUDIO_NEXT_CHANNEL, 
	    SampleCount, PSP_AUDIO_FORMAT_STEREO);

  if (failed)
  {
    for (i = 0; i < AUDIO_CHANNELS; i++)
    {
      if (AudioStatus[i].Handle != -1)
      {
        sceAudioChRelease(AudioStatus[i].Handle);
        AudioStatus[i].Handle = -1;
      }
    }

    if (DirectHandle != -1)
      sceAudioChRelease(DirectHandle);

    FreeBuffers();
    return 0;
  }

  AudioReady = 1;

  char label[16];
  strcpy(label, "audiotX");

  for (i = 0; i < AUDIO_CHANNELS; i++)
  {
    label[6] = '0' + i;
    AudioStatus[i].ThreadHandle = 
      sceKernelCreateThread(label, (void*)&AudioChannelThread, 0x12, 0x10000, 
        0, NULL);

    if (AudioStatus[i].ThreadHandle < 0)
    {
      AudioStatus[i].ThreadHandle = -1;
      failed = 1;
      break;
    }

    if (sceKernelStartThread(AudioStatus[i].ThreadHandle, sizeof(i), &i) != 0)
    {
      failed = 1;
      break;
    }
  }

  if (failed)
  {
    StopAudio = 1;
    for (i = 0; i < AUDIO_CHANNELS; i++)
    {
      if (AudioStatus[i].ThreadHandle != -1)
      {
        //sceKernelWaitThreadEnd(AudioStatus[i].threadhandle,NULL);
        sceKernelDeleteThread(AudioStatus[i].ThreadHandle);
      }

      AudioStatus[i].ThreadHandle = -1;
    }

    AudioReady = 0;
    FreeBuffers();
    return 0;
  }

  return SampleCount;
}

void pspAudioShutdown()
{
  int i;
  AudioReady = 0;
  StopAudio = 1;

  for (i = 0; i < AUDIO_CHANNELS; i++)
  {
    if (AudioStatus[i].ThreadHandle != -1)
    {
      //sceKernelWaitThreadEnd(AudioStatus[i].threadhandle,NULL);
      sceKernelDeleteThread(AudioStatus[i].ThreadHandle);
    }

    AudioStatus[i].ThreadHandle = -1;
  }

  for (i = 0; i < AUDIO_CHANNELS; i++)
  {
    if (AudioStatus[i].Handle != -1)
    {
      sceAudioChRelease(AudioStatus[i].Handle);
      AudioStatus[i].Handle = -1;
    }
  }

  if (DirectHandle != -1)
  {
    sceAudioChRelease(DirectHandle);
    DirectHandle = -1;
  }

  FreeBuffers();
}

void pspAudioSetVolume(int channel, int left, int right)
{
  AudioStatus[channel].LeftVolume = left;
  AudioStatus[channel].RightVolume = right;
}

static int AudioChannelThread(int args, void *argp)
{
  volatile int bufidx = 0;
  int channel = *(int*)argp;
  int i;
  unsigned int *ptr, length;
  void *bufptr;
  pspAudioCallback callback;

  for (i = 0; i < 2; i++)
    memset(AudioBuffer[channel][bufidx], 
      0, sizeof(short) * SampleCount * 2);

  while (!StopAudio)
  {
    callback = AudioStatus[channel].Callback;
    bufptr = AudioBuffer[channel][bufidx];
    length = SampleCount;

    if (callback) callback(bufptr, &length, AudioStatus[channel].Userdata);
    else for (i = 0, ptr = bufptr; i < SampleCount; i++) *(ptr++) = 0;

	  OutputBlocking(channel, AudioStatus[channel].LeftVolume, 
      AudioStatus[channel].RightVolume, bufptr, length);

    bufidx = (bufidx ? 0 : 1);
  }

  sceKernelExitThread(0);
  return 0;
}

int pspAudioOutputBlocking(void *buf, unsigned int length)
{
  if (DirectHandle < 0) return -1;

  sceAudioSetChannelDataLen(DirectHandle, length);
  return sceAudioOutputPannedBlocking(DirectHandle, PSP_AUDIO_MAX_VOLUME, 
    PSP_AUDIO_MAX_VOLUME, buf);
}

int pspAudioOutput(void *buf, unsigned int length)
{
  if (DirectHandle < 0) return -1;

  sceAudioSetChannelDataLen(DirectHandle, length);
  return sceAudioOutputPanned(DirectHandle, PSP_AUDIO_MAX_VOLUME, 
    PSP_AUDIO_MAX_VOLUME, buf);
}

static int OutputBlocking(unsigned int channel,
  unsigned int vol1, unsigned int vol2, void *buf, int length)
{
  if (!AudioReady) return -1;
  if (channel >= AUDIO_CHANNELS) return -1;
  if (vol1 > PSP_AUDIO_MAX_VOLUME) vol1 = PSP_AUDIO_MAX_VOLUME;
  if (vol2 > PSP_AUDIO_MAX_VOLUME) vol2 = PSP_AUDIO_MAX_VOLUME;

  sceAudioSetChannelDataLen(AudioStatus[channel].Handle, length);
  return sceAudioOutputPannedBlocking(AudioStatus[channel].Handle,
    vol1, vol2, buf);
}

static void FreeBuffers()
{
  int i, j;

  for (i = 0; i < AUDIO_CHANNELS; i++)
  {
    for (j = 0; j < 2; j++)
    {
      if (AudioBuffer[i][j])
      {
        free(AudioBuffer[i][j]);
        AudioBuffer[i][j] = NULL;
      }
    }
  }
}

void pspAudioSetChannelCallback(int channel, pspAudioCallback callback, void *userdata)
{
  volatile ChannelInfo *pci = &AudioStatus[channel];
  pci->Callback = NULL;
  pci->Userdata = userdata;
  pci->Callback = callback;
}
int pspAudioGetSampleCount()
{
  return SampleCount;
}

