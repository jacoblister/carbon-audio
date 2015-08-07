/*==========================================================================*/
MODULE::IMPORT/*============================================================*/
/*==========================================================================*/

#include "framework.c"
//#include "xmidi.c"
#include "audiostream.c"

/*==========================================================================*/
MODULE::INTERFACE/*=========================================================*/
/*==========================================================================*/

#if OS_Linux
#include <sys/types.h>
#include <dirent.h>
#endif
#if OS_Win32
/* Opendir for Win32, should move into framework */
#include <sys/types.h>
#include <io.h>
#include <errno.h>
//#define _MAX_FNAME 128
 
 typedef struct dirent {
         long d_ino;                                     /* inode (always 1 in WIN32) */
         off_t d_off;                            /* offset to this dirent */
         unsigned short d_reclen;        /* length of d_name */
         char d_name[_MAX_FNAME + 1];    /* filename (null terminated) */
 }dirent;
 
#define NAMLEN(dirent) strlen((dirent)->d_name)
 
/* typedef DIR - not the same as Unix */
typedef struct {
         long handle;                            /* _findfirst/_findnext handle */
         short offset;                           /* offset into directory */
         short finished;                         /* 1 if there are not more files */
         struct _finddata_t fileinfo;    /* from _findfirst/_findnext */
         char *dir;                                      /* the dir we are reading */
         struct dirent dent;                     /* the dirent to return */
} DIR;

DIR *opendir(const char *dir);
struct dirent *readdir(DIR * dp);
int closedir(DIR * dp);
void rewinddir(DIR *dir_Info);
#endif

class CAudioFile : CObject {
 private:
   CFile file;
   CAudioBuffer *buffer;
 public:
   void CAudioFile(CAudioBuffer *buffer);
 
   virtual bool file_load(const char *filename);
   virtual bool file_save(const char *filename);
};

class CAudioFileWave : CAudioFile {
   void CAudioFileWave(CAudioBuffer *buffer);
 
   virtual bool file_load(const char *filename);
   virtual bool file_save(const char *filename);
};

/*==========================================================================*/
MODULE::IMPLEMENTATION/*====================================================*/
/*==========================================================================*/

#if OS_Win32
/* Opendir for Win32, should move into framework */
DIR *opendir(const char *dir) {
         DIR *dp;
         char *filespec;
         long handle;
         int index;
 
         filespec = (char *)malloc(strlen(dir) + 2 + 1);
         strcpy(filespec, dir);
         index = strlen(filespec) - 1;
         if (index >= 0 && (filespec[index] == '/' || filespec[index] == '\\'))
                 filespec[index] = '\0';
         strcat(filespec, "/""*");
 
         dp = (DIR *) malloc(sizeof(DIR));
         dp->offset = 0;
         dp->finished = 0;
         dp->dir = (char *)strdup(dir);
 
         if ((handle = _findfirst(filespec, &(dp->fileinfo))) < 0) {
            free(filespec); free(dp);
            return NULL;
         }
         dp->handle = handle;
         free(filespec);
 
         return dp;
 }
 
struct dirent *readdir(DIR * dp) {
         if (!dp || dp->finished)
                 return NULL;
 
         if (dp->offset != 0) {
                 if (_findnext(dp->handle, &(dp->fileinfo)) < 0) {
                         dp->finished = 1;
       if (ENOENT == errno)
         /* Clear error set to mean no more files else that breaks things */
         errno = 0;
                         return NULL;
                 }
         }
         dp->offset++;
 
         strncpy(dp->dent.d_name, dp->fileinfo.name, _MAX_FNAME);
         dp->dent.d_ino = 1;
   /* reclen is used as meaning the length of the whole record */
         dp->dent.d_reclen = strlen(dp->dent.d_name) + sizeof(char) + sizeof(dp->dent.d_ino) + sizeof(dp->dent.d_reclen) + sizeof(dp->dent.d_off);
         dp->dent.d_off = dp->offset;
 
         return &(dp->dent);
 }
 
int closedir(DIR * dp) {
         if (!dp)
                 return 0;
         _findclose(dp->handle);
         if (dp->dir)
                free(dp->dir);
         if (dp)
                 free(dp);
 
         return 0;
     }

void rewinddir(DIR *dir_Info) {
         /* Re-set to the beginning */
         char *filespec;
         long handle;
         int index;
 
         dir_Info->handle = 0;
         dir_Info->offset = 0;
         dir_Info->finished = 0;
 
         filespec = (char *)malloc(strlen(dir_Info->dir) + 2 + 1);
         strcpy(filespec, dir_Info->dir);
         index = strlen(filespec) - 1;
         if (index >= 0 && (filespec[index] == '/' || filespec[index] == '\\'))
                 filespec[index] = '\0';
         strcat(filespec, "/""*");
 
         if ((handle = _findfirst(filespec, &(dir_Info->fileinfo))) < 0) {
                 if (errno == ENOENT) {
                         dir_Info->finished = 1;
                 }
         }
         dir_Info->handle = handle;
         free(filespec);
}
#endif

#define WAVE_RIFF "RIFF"
#define WAVE_WAVE "WAVE"
#define WAVE_FMT  "fmt "
#define WAVE_DATA "data"

/* Deals only with the simple case format/wave type .wav format */
typedef struct {
    /* RIFF header */
   char chunk_id[4];
   int chunk_data_size;
   char riff_type[4];
   /* Fmt Chunk */
   char fmt_chunk_id[4];
   word compression_code;
   word number_of_channels;
   int sampling_rate;
   int average_bytes_per_second;
   word block_align;
   word bits_per_sample;
   word extra_format_bytes;
} TWaveHeader;

void CAudioFile::CAudioFile(CAudioBuffer *buffer) {
    this->buffer = buffer;
};

bool CAudioFile::file_load(const char *filename) {return FALSE; }
bool CAudioFile::file_save(const char *filename) {return FALSE; }

void CAudioFileWave::CAudioFileWave(CAudioBuffer *buffer) {     
    CAudioFile(this).CAudioFile(buffer);
};

bool CAudioFileWave::file_load(const char *filename) {
   char buffer[8];
   int value;
   bool result = TRUE;
   short wav_channels;
   int wav_sample_rate;
   bool done = FALSE;
   
   CAudioBuffer abuffer;
	
   /* convert input buffer to 16 bit signed integers */
   new(&abuffer).CAudioBuffer(EAudioDataType.word, CAudioFile(this)->buffer->sampling_rate);
    
   memset(buffer, 0, sizeof(buffer));
    
   try {
      new(&CAudioFile(this)->file).CFile();
      CIOObject(&CAudioFile(this)->file).open(filename, O_RDONLY | O_BINARY);

      CIOObject(&CAudioFile(this)->file).read(buffer, 4);
      if (strncmp(buffer, WAVE_RIFF, 4) != 0) 
         throw(CObject(this), EXCEPTION<CObject,InvalidCast>, NULL);
      
      CIOObject(&CAudioFile(this)->file).read(&value, 4);
      CIOObject(&CAudioFile(this)->file).read(buffer, 4);      
      if (strncmp(buffer, WAVE_WAVE, 4) != 0)
         throw(CObject(this), EXCEPTION<CObject,InvalidCast>, NULL);

      while (!done) {
         CIOObject(&CAudioFile(this)->file).read(buffer, 4);
         CIOObject(&CAudioFile(this)->file).read(&value, 4);          
         if (strncmp(buffer, WAVE_FMT, 4) == 0) {
            /* better handling of the unexpected */
            CIOObject(&CAudioFile(this)->file).read(&value, 2);
            CIOObject(&CAudioFile(this)->file).read(&wav_channels, 2);
            CIOObject(&CAudioFile(this)->file).read(&wav_sample_rate, 4);    
            CIOObject(&CAudioFile(this)->file).read(&value, 4);    
            CIOObject(&CAudioFile(this)->file).read(&value, 2);        
            CIOObject(&CAudioFile(this)->file).read(&value, 2);
            CIOObject(&CAudioFile(this)->file).read(&value, 2);        
         }
         else if (strncmp(buffer, WAVE_DATA, 4) == 0) {
             CAudioBuffer(&abuffer).data_length_set(value / sizeof(short int));
//abuffer.data_size = value;
             CIOObject(&CAudioFile(this)->file).read(abuffer.data, abuffer.data_size);
			 CAudioBuffer(&abuffer)->data_empty = FALSE;
             done = TRUE;
         }
         else {
            CIOObject(&CAudioFile(this)->file).lseek(value, SEEK_CUR);
         }
      }
   }
   catch(NULL, EXCEPTION<>) {
      result = FALSE;
   }
   finally {
      CIOObject(&CAudioFile(this)->file).close();
      delete(&CAudioFile(this)->file);
   }
   
   CAudioBuffer(CAudioFile(this)->buffer).convert(&abuffer);
   delete(&abuffer);
   
   return result;
}

bool CAudioFileWave::file_save(const char *filename) {
    int value;
	CAudioBuffer abuffer;
	
	/* convert input buffer to 16 bit signed integers */
	new(&abuffer).CAudioBuffer(EAudioDataType.word, CAudioFile(this)->buffer->sampling_rate);
	CAudioBuffer(&abuffer).convert(CAudioFile(this)->buffer);
	
    /*>>>don't allow unhandled sample formats (16 bit mono only at present) */
    new(&CAudioFile(this)->file).CFile();
    CIOObject(&CAudioFile(this)->file).open(filename, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY);
    CIOObject(&CAudioFile(this)->file).write(WAVE_RIFF, 4);
    value = 4 + (18 + 8) + 8 + CAudioFile(this)->buffer->data_size; CIOObject(&CAudioFile(this)->file).write(&value, 4);
    CIOObject(&CAudioFile(this)->file).write(WAVE_WAVE, 4);
    
    /* FMT chunk */
    CIOObject(&CAudioFile(this)->file).write(WAVE_FMT, 4);
    value = 18; CIOObject(&CAudioFile(this)->file).write(&value, 4);
    value = 1; CIOObject(&CAudioFile(this)->file).write(&value, 2);
    value = 1; CIOObject(&CAudioFile(this)->file).write(&value, 2);
    value = abuffer.sampling_rate; CIOObject(&CAudioFile(this)->file).write(&value, 4);    
    value = abuffer.sampling_rate * 2; CIOObject(&CAudioFile(this)->file).write(&value, 4);    
    value = 2; CIOObject(&CAudioFile(this)->file).write(&value, 2);        
    value = 16; CIOObject(&CAudioFile(this)->file).write(&value, 2);
    value = 0; CIOObject(&CAudioFile(this)->file).write(&value, 2);        
    
    /* Wave chunk */
    CIOObject(&CAudioFile(this)->file).write(WAVE_DATA, 4);    
    value = abuffer.data_size; CIOObject(&CAudioFile(this)->file).write(&value, 4);
    CIOObject(&CAudioFile(this)->file).write(abuffer.data, abuffer.data_size);
    
    CIOObject(&CAudioFile(this)->file).close();
    delete(&CAudioFile(this)->file);
	
	delete(&abuffer);
	
    return TRUE;
};

/*==========================================================================*/
MODULE::END/*===============================================================*/
/*==========================================================================*/
