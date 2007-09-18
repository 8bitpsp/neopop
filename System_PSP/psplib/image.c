/** PSP helper library ***************************************/
/**                                                         **/
/**                          image.c                        **/
/**                                                         **/
/** This file contains image manipulation routines.         **/
/** Parts of the PNG loading/saving code are based on, and  **/
/** adapted from NesterJ v1.11 code by Ruka                 **/
/**                                                         **/
/** Copyright (C) Akop Karapetyan 2007                      **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#include <malloc.h>
#include <string.h>
#include <png.h>
#include <pspgu.h>

#include "video.h"
#include "image.h"

typedef unsigned char byte;

int FindPowerOfTwoLargerThan(int n);

/* Creates an image in memory */
PspImage* pspImageCreate(int width, int height, int bpp)
{
  if (bpp != PSP_IMAGE_INDEXED && bpp != PSP_IMAGE_16BPP) return NULL;

  int size = width * height * (bpp / 8);
  void *pixels = memalign(16, size);

  if (!pixels) return NULL;

  PspImage *image = (PspImage*)malloc(sizeof(PspImage));

  if (!image)
  {
    free(pixels);
    return NULL;
  }

  memset(pixels, 0, size);

  image->Width = width;
  image->Height = height;
  image->Pixels = pixels;

  image->Viewport.X = 0;
  image->Viewport.Y = 0;
  image->Viewport.Width = width;
  image->Viewport.Height = height;

  int i;
  for (i = 1; i < width; i *= 2);
  image->PowerOfTwo = (i == width);
  image->BytesPerPixel = bpp / 8;
  image->FreeBuffer = 1;
  image->Depth = bpp;
  memset(image->Palette, 0, sizeof(image->Palette));

  switch (image->Depth)
  {
  case PSP_IMAGE_INDEXED:
    image->TextureFormat = GU_PSM_T8;
    break;
  case PSP_IMAGE_16BPP:
    image->TextureFormat = GU_PSM_5551;
    break;
  }

  return image;
}

/* Creates an image using portion of VRAM */
PspImage* pspImageCreateVram(int width, int height, int bpp)
{
  if (bpp != PSP_IMAGE_INDEXED && bpp != PSP_IMAGE_16BPP) return NULL;

  int i, size = width * height * (bpp / 8);
  void *pixels = pspVideoAllocateVramChunk(size);

  if (!pixels) return NULL;

  PspImage *image = (PspImage*)malloc(sizeof(PspImage));
  if (!image) return NULL;

  memset(pixels, 0, size);

  image->Width = width;
  image->Height = height;
  image->Pixels = pixels;

  image->Viewport.X = 0;
  image->Viewport.Y = 0;
  image->Viewport.Width = width;
  image->Viewport.Height = height;

  for (i = 1; i < width; i *= 2);
  image->PowerOfTwo = (i == width);
  image->BytesPerPixel = bpp >> 3;
  image->FreeBuffer = 0;
  image->Depth = bpp;
  memset(image->Palette, 0, sizeof(image->Palette));

  switch (image->Depth)
  {
  case PSP_IMAGE_INDEXED: image->TextureFormat = GU_PSM_T8;   break;
  case PSP_IMAGE_16BPP:   image->TextureFormat = GU_PSM_5551; break;
  }

  return image;
}

PspImage* pspImageCreateOptimized(int width, int height, int bpp)
{
  PspImage *image = pspImageCreate(FindPowerOfTwoLargerThan(width), height, bpp);
  if (image) image->Viewport.Width = width;

  return image;
}

/* Destroys image */
void pspImageDestroy(PspImage *image)
{
  if (image->FreeBuffer) free(image->Pixels);
  free(image);
}

/* Creates a half-sized thumbnail of an image */
PspImage* pspImageCreateThumbnail(const PspImage *image)
{
  PspImage *thumb;
  int i, j, p;

  if (!(thumb = pspImageCreate(image->Viewport.Width >> 1,
    image->Viewport.Height >> 1, image->Depth)))
      return NULL;

  int dy = image->Viewport.Y + image->Viewport.Height;
  int dx = image->Viewport.X + image->Viewport.Width;

  for (i = image->Viewport.Y, p = 0; i < dy; i += 2)
    for (j = image->Viewport.X; j < dx; j += 2)
      if (image->Depth == PSP_IMAGE_INDEXED)
        ((unsigned char*)thumb->Pixels)[p++]
          = ((unsigned char*)image->Pixels)[(image->Width * i) + j];
      else
        ((unsigned short*)thumb->Pixels)[p++]
          = ((unsigned short*)image->Pixels)[(image->Width * i) + j];

  if (image->Depth == PSP_IMAGE_INDEXED)
    memcpy(thumb->Palette, image->Palette, sizeof(image->Palette));

  return thumb;
}

int pspImageDiscardColors(const PspImage *original)
{
  if (original->Depth != PSP_IMAGE_16BPP) return 0;

  int y, x, gray;
  unsigned short *p;

  for (y = 0, p = (unsigned short*)original->Pixels; y < original->Height; y++)
    for (x = 0; x < original->Width; x++, p++)
    {
      gray = (RED(*p) * 3 + GREEN(*p) * 4 + BLUE(*p) * 2) / 9;
      *p = RGB(gray, gray, gray);
    }

  return 1;
}

int pspImageBlur(const PspImage *original, PspImage *blurred)
{
  if (original->Width != blurred->Width
    || original->Height != blurred->Height
    || original->Depth != blurred->Depth
    || original->Depth != PSP_IMAGE_16BPP) return 0;

  int r, g, b, n, i, y, x, dy, dx;
  unsigned short p;

  for (y = 0, i = 0; y < original->Height; y++)
  {
    for (x = 0; x < original->Width; x++, i++)
    {
      r = g = b = n = 0;
      for (dy = y - 1; dy <= y + 1; dy++)
      {
        if (dy < 0 || dy >= original->Height) continue;

        for (dx = x - 1; dx <= x + 1; dx++)
        {
          if (dx < 0 || dx >= original->Width) continue;

          p = ((unsigned short*)original->Pixels)[dx + dy * original->Width];
          r += RED(p);
          g += GREEN(p);
          b += BLUE(p);
          n++;
        }

        r /= n;
        g /= n;
        b /= n;
        ((unsigned short*)blurred->Pixels)[i] = RGB(r, g, b);
      }
    }
  }

  return 1;
}

/* Creates an exact copy of the image */
PspImage* pspImageCreateCopy(const PspImage *image)
{
  PspImage *copy;

  /* Create image */
  if (!(copy = pspImageCreate(image->Width, image->Height, image->Depth)))
    return NULL;

  /* Copy pixels */
  int size = image->Width * image->Height * image->BytesPerPixel;
  memcpy(copy->Pixels, image->Pixels, size);
  memcpy(&copy->Viewport, &image->Viewport, sizeof(PspViewport));
  memcpy(copy->Palette, image->Palette, sizeof(image->Palette));

  return copy;
}

/* Clears an image */
void pspImageClear(PspImage *image, unsigned int color)
{
  if (image->Depth == PSP_IMAGE_INDEXED)
  {
    memset(image->Pixels, color & 0xff, image->Width * image->Height);
  }
  else if (image->Depth == PSP_IMAGE_16BPP)
  {
    int i;
    unsigned short *pixel = image->Pixels;
    for (i = image->Width * image->Height - 1; i >= 0; i--, pixel++)
      *pixel = color & 0xffff;
  }
}

/* Loads an image from a file */
PspImage* pspImageLoadPng(const char *path)
{
  FILE *fp = fopen(path,"rb");
  if(!fp) return NULL;

  PspImage *image = pspImageLoadPngFd(fp);
  fclose(fp);

  return image;
}

/* Saves an image to a file */
int pspImageSavePng(const char *path, const PspImage* image)
{
  FILE *fp = fopen( path, "wb" );
	if (!fp) return 0;

  int stat = pspImageSavePngFd(fp, image);
  fclose(fp);

  return stat;
}

/* Loads an image from an open file descriptor (16-bit PNG)*/
PspImage* pspImageLoadPngFd(FILE *fp)
{
  const size_t nSigSize = 8;
  byte signature[nSigSize];
  if (fread(signature, sizeof(byte), nSigSize, fp) != nSigSize)
    return 0;

  if (!png_check_sig(signature, nSigSize))
    return 0;

  png_struct *pPngStruct = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if(!pPngStruct)
    return 0;

  png_info *pPngInfo = png_create_info_struct(pPngStruct);
  if(!pPngInfo)
  {
    png_destroy_read_struct(&pPngStruct, NULL, NULL);
    return 0;
  }

  if (setjmp(pPngStruct->jmpbuf))
  {
    png_destroy_read_struct(&pPngStruct, NULL, NULL);
    return 0;
  }

  png_init_io(pPngStruct, fp);
  png_set_sig_bytes(pPngStruct, nSigSize);
  png_read_png(pPngStruct, pPngInfo,
    PNG_TRANSFORM_STRIP_16 | PNG_TRANSFORM_PACKING |
    PNG_TRANSFORM_EXPAND | PNG_TRANSFORM_BGR , NULL);

  png_uint_32 width = pPngInfo->width;
  png_uint_32 height = pPngInfo->height;
  int color_type = pPngInfo->color_type;

  PspImage *image;

  int mod_width = FindPowerOfTwoLargerThan(width);
  if (!(image = pspImageCreate(mod_width, height, PSP_IMAGE_16BPP)))
  {
    png_destroy_read_struct(&pPngStruct, &pPngInfo, NULL);
    return 0;
  }

  image->Viewport.Width = width;

  png_byte **pRowTable = pPngInfo->row_pointers;
  unsigned int x, y;
  byte r, g, b;
  unsigned short *out = image->Pixels;

  for (y=0; y<height; y++)
  {
    png_byte *pRow = pRowTable[y];

    for (x=0; x<width; x++)
    {
      switch(color_type)
      {
        case PNG_COLOR_TYPE_GRAY:
          r = g = b = *pRow++;
          break;
        case PNG_COLOR_TYPE_GRAY_ALPHA:
          r = g = b = pRow[0];
          pRow += 2;
          break;
        case PNG_COLOR_TYPE_RGB:
          b = pRow[0];
          g = pRow[1];
          r = pRow[2];
          pRow += 3;
          break;
        case PNG_COLOR_TYPE_RGB_ALPHA:
          b = pRow[0];
          g = pRow[1];
          r = pRow[2];
          pRow += 4;
          break;
        default:
          r = g = b = 0;
          break;
      }

      *out++ = RGB(r,g,b);
    }

    out += (mod_width - width);
  }

  png_destroy_read_struct(&pPngStruct, &pPngInfo, NULL);

  return image;
}

/* Saves an image to an open file descriptor (16-bit PNG)*/
int pspImageSavePngFd(FILE *fp, const PspImage* image)
{
  unsigned char *bitmap;
  int i, j, width, height;

  width = image->Viewport.Width;
  height = image->Viewport.Height;

  if (!(bitmap = (u8*)malloc(sizeof(u8) * width * height * 3)))
    return 0;

  if (image->Depth == PSP_IMAGE_INDEXED)
  {
    const unsigned char *pixel;
    pixel = image->Pixels + (image->Viewport.Y * image->Width);

    for (i = 0; i < height; i++)
    {
      /* Skip to the start of the viewport */
      pixel += image->Viewport.X;
      for (j = 0; j < width; j++, pixel++)
      {
        bitmap[i * width * 3 + j * 3 + 0] = RED(image->Palette[*pixel]);
        bitmap[i * width * 3 + j * 3 + 1] = GREEN(image->Palette[*pixel]);
        bitmap[i * width * 3 + j * 3 + 2] = BLUE(image->Palette[*pixel]);
      }
      /* Skip to the end of the line */
      pixel += image->Width - (image->Viewport.X + width);
    }
  }
  else
  {
    const unsigned short *pixel;
    pixel = image->Pixels + (image->Viewport.Y * image->Width);

    for (i = 0; i < height; i++)
    {
      /* Skip to the start of the viewport */
      pixel += image->Viewport.X;
      for (j = 0; j < width; j++, pixel++)
      {
        bitmap[i * width * 3 + j * 3 + 0] = RED(*pixel);
        bitmap[i * width * 3 + j * 3 + 1] = GREEN(*pixel);
        bitmap[i * width * 3 + j * 3 + 2] = BLUE(*pixel);
      }
      /* Skip to the end of the line */
      pixel += image->Width - (image->Viewport.X + width);
    }
  }

  png_struct *pPngStruct = png_create_write_struct( PNG_LIBPNG_VER_STRING,
    NULL, NULL, NULL );

  if (!pPngStruct)
  {
    free(bitmap);
    return 0;
  }

  png_info *pPngInfo = png_create_info_struct( pPngStruct );
  if (!pPngInfo)
  {
    png_destroy_write_struct( &pPngStruct, NULL );
    free(bitmap);
    return 0;
  }

  png_byte **buf = (png_byte**)malloc(height * sizeof(png_byte*));
  if (!buf)
  {
    png_destroy_write_struct( &pPngStruct, &pPngInfo );
    free(bitmap);
    return 0;
  }

  unsigned int y;
  for (y = 0; y < height; y++)
    buf[y] = (byte*)&bitmap[y * width * 3];

  if (setjmp( pPngStruct->jmpbuf ))
  {
    free(buf);
    free(bitmap);
    png_destroy_write_struct( &pPngStruct, &pPngInfo );
    return 0;
  }

  png_init_io( pPngStruct, fp );
  png_set_IHDR( pPngStruct, pPngInfo, width, height, 8,
    PNG_COLOR_TYPE_RGB,
    PNG_INTERLACE_NONE,
    PNG_COMPRESSION_TYPE_DEFAULT,
    PNG_FILTER_TYPE_DEFAULT);
  png_write_info( pPngStruct, pPngInfo );
  png_write_image( pPngStruct, buf );
  png_write_end( pPngStruct, pPngInfo );

  png_destroy_write_struct( &pPngStruct, &pPngInfo );
  free(buf);
  free(bitmap);

  return 1;
}

int FindPowerOfTwoLargerThan(int n)
{
  int i;
  for (i = n; i < n; i *= 2);
  return i;
}

