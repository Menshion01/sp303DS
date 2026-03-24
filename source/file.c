#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "error.h"
#include "file.h"

#include "mp3.h"
#include "wav.h"
#include "text.h"

/**
 * Obtain file type string from file_types enum.
 *
 * \param	ft	File type enum.
 * \return		String representation of enum.
 */
const char* fileToStr(enum file_types ft) {
  static const char* file_types_str[] = {"UNKNOWN", "WAV", "MP3"};

  return file_types_str[ft];
}

/**
 * Obtains file type.
 *
 * \param	file	File location.
 * \return			file_types enum or 0 on error and errno set.
 */
enum file_types getFileType(const char* file) {
  FILE* ftest = fopen(file, "rb");
  uint32_t fileSig;
  enum file_types file_type = FILE_TYPE_ERROR;

  /* Failure opening file */
  if (ftest == NULL)
    return FILE_TYPE_ERROR;

  if (fread(&fileSig, 4, 1, ftest) == 0)
    goto err;

  switch (fileSig) {
    // "RIFF"
    case 0x46464952:
    // "riff"
    case 0x66666972:
    // "RIFX"
    case 0x58464952:
    // "RF64"
    case 0x34364652:
    // "FORM"
    case 0x4D524F46:
      file_type = FILE_TYPE_WAV;
      break;

    default:
      /*
       * MP3 without ID3 tag, ID3v1 tag is at the end of file, or MP3
       * with ID3 tag at the beginning  of the file.
       */
      if (isMp3(file) == 0) {
        file_type = FILE_TYPE_MP3;
        break;
      }

      /* TODO: Add this again at some point */
      // graphicalPrintf("Unknown magic number: %#010x .", fileSig);
      errno = FILE_NOT_SUPPORTED;
      break;
  }

err:
  fclose(ftest);
  return file_type;
}

void copyFileToDir(const char* srcPath, const char* destDir) {
  char destPath[PATH_MAX];

  graphicalPrintf("Copying from: %s To: %s ", srcPath, destDir);

  snprintf(destPath, sizeof(destPath), "%s/%s", destDir,
           strrchr(srcPath, '/') ? strrchr(srcPath, '/') + 1 : srcPath);

  FILE* src = fopen(srcPath, "rb");
  if (!src) {
    graphicalPrintf("Failed to open source: %s ", srcPath);
    return;
  }

  FILE* dest = fopen(destPath, "wb");
  if (!dest) {
    graphicalPrintf("Failed to open destination: %s ", destPath);
    fclose(src);
    return;
  }

  char buffer[4096];
  size_t bytes;
  while ((bytes = fread(buffer, 1, sizeof(buffer), src)) > 0) {
    fwrite(buffer, 1, bytes, dest);
  }

  fclose(src);
  fclose(dest);
}