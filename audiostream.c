/*==========================================================================*/
MODULE::IMPORT/*============================================================*/
/*==========================================================================*/

#include "framework.c"
#include "xmidi.c"

/*==========================================================================*/
MODULE::INTERFACE/*=========================================================*/
/*==========================================================================*/

ENUM:typedef<EAudioDataType> {
   {word, "16-bit"}, {float}
};

class CAudioBuffer : CObjPersistent {
 private:
   void new(void);
   void delete(void);

   byte *data;
   long data_size;
   long data_size_max;                        /* maximum buffer size in bytes */
   bool data_local_alloc;
   bool data_empty;

   int sample_size;
 public:
   ALIAS<"buffer">;
   ATTRIBUTE<EAudioDataType data_type>;
   ATTRIBUTE<int sampling_rate>;

   void CAudioBuffer(EAudioDataType data_type, int sampling_rate);

   bool convert(struct tag_CAudioBuffer *src);
   void copy_data(struct tag_CAudioBuffer *src);
   void add(struct tag_CAudioBuffer *src);
   void extract(struct tag_CAudioBuffer *dest,
                int sample_start, int length);
   static inline void *data_ptr(int sample);
   void clear(void);
   void gain(float gain);
   void clip(float level);
   float peak_level(void);
   void write(struct tag_CAudioBuffer *src, int samples);
   void fade(int sample_start, int sample_end, float gain_start, float gain_end, int sample);
   static inline bool empty(void);

   static inline int length(void);

   static inline void data_allocate_buffer(void *data, int size);
   static inline void data_allocate(int length);
   static inline void data_release(void);   
   
   static inline void data_length_set(int length);
   static inline void *data_pointer_get(void);
//   void data_set(void *data, int data_size, EAudioDataType data_type, int channels);
//   void data_get(ARRAY<byte> data, EAudioDataType data_type, int channels);

   EXCEPTION<Error>;
};

static inline void *CAudioBuffer::data_ptr(int sample) {
    return this->data + (sample * this->sample_size);
}

static inline bool CAudioBuffer::empty(void) {
   return this->data_empty;
}/*CAudioBuffer::empty*/

static inline int CAudioBuffer::length(void) {
   return this->data_size / this->sample_size;
}/*CAudioBuffer::length*/

static inline void CAudioBuffer::data_release(void) {
   if (this->data_local_alloc) {
      free(this->data);
   }
}/*CAudioBuffer::data_release*/

static inline void CAudioBuffer::data_allocate_buffer(void *data, int size) {
   CAudioBuffer(this).data_release();
   this->data_local_alloc = FALSE;
   this->data_size_max = size;
   this->data = data;
}/*CAudioBuffer::data_allocate_buffer*/

static inline void CAudioBuffer::data_allocate(int length) {
   if (!this->data_local_alloc) {
      this->data_local_alloc = TRUE;
      this->data_size_max = length * this->sample_size;
      this->data = malloc(this->data_size_max);
   }
}/*CAudioBuffer::data_allocate*/

static inline void CAudioBuffer::data_length_set(int length) {
   this->data_size = length * this->sample_size;
   if (this->data_size > this->data_size_max) {
      CAudioBuffer(this).data_allocate(length);
   }
}/*CAudioBuffer::data_length_set*/

static inline void *CAudioBuffer::data_pointer_get(void) {
   return this->data;
}/*CAudioBuffer::data_pointer_get*/

ARRAY:typedef<CAudioBuffer>;

class CFrame : CObject {
 private:
   int length;

   ARRAY<CAudioBuffer> audio;
   ARRAY<TMidiEventContainer> midi;

   void clear(void);

   void new(void);
   void delete(void);
 public:
   void CFrame(int channels, EAudioDataType datatype, int frame_length, int sampling_rate);
};

ENUM:typedef<EStreamMode> {
   {input},
   {output},
   {duplex},
};

class CAudioStream : CObjPersistent {
 public:
    void CAudioStream(void);

    virtual bool frame(CFrame *frame, EStreamMode mode);
};

typedef struct {
   int pan;
   int level;
   float gain[2];
} TChannelMix;

void TChannelMix_set(TChannelMix *this);

/*==========================================================================*/
MODULE::IMPLEMENTATION/*====================================================*/
/*==========================================================================*/

void CAudioBuffer::new(void) {
}/*CAudioBuffer::new*/

void CAudioBuffer::CAudioBuffer(EAudioDataType data_type, int sampling_rate) {
   this->data_type   = data_type;
   this->sampling_rate = sampling_rate;
   switch (data_type) {
   case EAudioDataType.word:
      this->sample_size = sizeof(short int);  
      break;
   default:
      this->sample_size = sizeof(float);
	  break;
   }
   this->data_empty = TRUE;
}/*CAudioBuffer::CAudioBuffer*/

void CAudioBuffer::delete(void) {
   if (this->data_local_alloc) {
      free(this->data);
      }
}/*CAudioBuffer::delete*/

bool CAudioBuffer::convert(struct tag_CAudioBuffer *src) {
   int i;
   if (src->data_empty) {
      this->data_empty = TRUE;
      return TRUE;
   }
   this->data_empty = FALSE;

   if (src->data_type == EAudioDataType.float && 
       this->data_type == EAudioDataType.word) {
      CAudioBuffer(this).data_length_set(CAudioBuffer(src).length());
      for (i = 0; i < CAudioBuffer(this).length(); i++) {
         ((signed short *)this->data)[i] = ((float *)src->data)[i] * 32767;
      }
      return TRUE;
   }

   if (src->data_type == EAudioDataType.word && 
       this->data_type == EAudioDataType.float) {
      CAudioBuffer(this).data_length_set(CAudioBuffer(src).length());
      for (i = 0; i < CAudioBuffer(this).length(); i++) {
         ((float *)this->data)[i] = ((float)(((short int *)src->data)[i])) / 32767;
//printf("converted sample %f\n", ((float *)this->data)[i]);
      }
      return TRUE;
   }

   if (src->data_type != this->data_type) {
      ASSERT("CAudioBuffer::convert - data type mismatch");
      return FALSE;
   }
   if (src->data_size != this->data_size) {
      ASSERT("CAudioBuffer::convert - data length mismatch");
      return FALSE;
   }

   CAudioBuffer(this).copy_data(src);
   return TRUE;
}/*CAudioBuffer::convert*/

/*>>>minimial implementations at present*/
void CAudioBuffer::copy_data(struct tag_CAudioBuffer *src) {
   if (src->data_empty) {
      CAudioBuffer(this).clear();
   }
   else {
      this->data_empty = FALSE;
      this->data_size = src->data_size;
      memcpy(this->data, src->data, src->data_size);
   }
}/*CAudioBuffer::copy_data*/

void CAudioBuffer::add(struct tag_CAudioBuffer *src) {
   int i;
   if (src->data_empty)
      return;

   if (this->data_empty) {
      memset(this->data, 0, this->data_size);
      this->data_empty = FALSE;
   }

   if (this->data_size != src->data_size) {
      throw(CObject(this), EXCEPTION<CAudioBuffer,Error>, "buffers not equal size");
   }

   switch (this->data_type) {
   case EAudioDataType.word:
      for (i = 0; i < this->data_size; i += this->sample_size) {
         *((signed short *)&this->data[i]) += *((signed short *)&src->data[i]);
      }
      break;
   case EAudioDataType.float:
      for (i = 0; i < this->data_size; i += this->sample_size) {
         *((float *)&this->data[i]) += *((float *)&src->data[i]);
      }
      break;
   }
}/*CAudioBuffer::add*/

void CAudioBuffer::extract(struct tag_CAudioBuffer *dest,
                           int sample_start, int length) {
   int offset = 0;
   if (this->data_empty) {
      dest->data_empty = TRUE;
   }
   else {
      dest->data_empty = FALSE;
      dest->data_size = length * (this->sample_size);
      memset(dest->data, 0, dest->data_size);
      if (sample_start < 0) {
         offset = -sample_start;
         sample_start = 0;
         length -= offset;
      }
      if (sample_start + length > CAudioBuffer(this).length()) {
         length = CAudioBuffer(this).length() - sample_start;
         length = length > 0 ? length : 0;
      }
      memcpy(dest->data + (offset * this->sample_size), 
         this->data + sample_start * (this->sample_size),
         length * this->sample_size);
   }
}/*CAudioBuffer::extract*/

void CAudioBuffer::clear(void) {
   memset(this->data, 0, this->data_size);
   this->data_empty = TRUE;
}/*CAudioBuffer::clear*/

float CAudioBuffer::peak_level(void) {
   return 0;
}/*CAudioBuffer::peak_level*/

void CAudioBuffer::gain(float gain) {
   int i;
   float *float_data;

   if (this->data_empty)
      return;

   switch (this->data_type) {
   case EAudioDataType.word:
      /*broken*/
      break;
   case EAudioDataType.float:
      float_data = (float *)this->data;
      for (i = 0; i < this->data_size; i += this->sample_size) {
         *float_data = *float_data * gain;
         float_data++;
      }
      break;
   }
}/*CAudioBuffer::gain*/

void CAudioBuffer::clip(float level) {
   int i;
   float *float_data;

   if (this->data_empty)
      return;

   switch (this->data_type) {
   case EAudioDataType.word:
      /*broken*/
      break;
   case EAudioDataType.float:
      float_data = (float *)this->data;
      for (i = 0; i < this->data_size; i += this->sample_size) {
         *float_data = *float_data > level ? level : *float_data;
         *float_data = *float_data < -level ? -level : *float_data;
         float_data++;
      }
      break;
   }
}/*CAudioBuffer::gain*/


void CAudioBuffer::write(struct tag_CAudioBuffer *src, int samples) {
   long size = this->data_size;
//printf("buffer write this=%d src=%d\n", this, src);
//printf("buffer write data=%d size=%d, src=%d, length=%d\n", this->data, this->data_size, src->data, samples * this->sample_size);
   this->data_empty &= src->data_empty;

   if (size + (samples * this->sample_size) <= this->data_size_max) {
      if (src->data_empty) {
         memset(this->data + this->data_size, 0, samples * this->sample_size);
      }
      else {
         memcpy(this->data + this->data_size, src->data,
               samples * this->sample_size);
      }
      this->data_size += (samples * this->sample_size);
   }
}/*CAudioBuffer::write*/

void CAudioBuffer::fade(int sample_start, int sample_end, float gain_start, float gain_end, int sample) {
   int i;
   int start, end;
   float slope, gain;
   float *float_data;
   
   if (this->data_type != EAudioDataType.float)
      return;
   
   if (sample < sample_start || sample + CAudioBuffer(this).length() > sample_end)
      return;

#if 0   
   slope = (gain_end - gain_start) / (sample_end - sample_start);

   float_data = &((float *)this->data)[start];
   for (i = start; i <= end; i++) {
      *float_data = *float_data * gain;
      gain += slope;
      float_data++;
   }
#endif

#if 0
   slope = (gain_end - gain_start) / (sample_end - sample_start);
   float_data = (float *)this->data;
   for (i = 0; i < CAudioBuffer(this).length(); i++) {
      if (i + sample >= sample_start && i + sample <= sample_end) {
         gain = (i + sample - sample_start) * slope;
         *float_data = *float_data * gain;
      }
      float_data++;
   }
#endif   
}

void CFrame::new(void) {
   ARRAY(&this->audio).new();
   ARRAY(&this->midi).new();
};

void CFrame::CFrame(int channels, EAudioDataType datatype, int frame_length, int sampling_rate) {
   int i;

   this->length = frame_length;
   ARRAY(&this->audio).used_set(channels);
   ARRAY(&this->midi).used_set(512);
   ARRAY(&this->midi).used_set(0);

   for (i = 0; i < channels; i++) {
      new(&ARRAY(&this->audio).data()[i]).CAudioBuffer(datatype, sampling_rate);
      CAudioBuffer(&ARRAY(&this->audio).data()[i]).data_length_set(frame_length);
   }
};

void CFrame::delete(void) {
   int i;

   ARRAY(&this->midi).delete();

   for (i = 0; i < ARRAY(&this->audio).count(); i++) {
      delete(&ARRAY(&this->audio).data()[i]);
   }
   ARRAY(&this->audio).delete();
};

void CFrame::clear(void) {
   int i;
   TMidiEventContainer *event;

   for (i = 0; i < ARRAY(&this->midi).count(); i++) {
       event = &ARRAY(&this->midi).data()[i];
       /*>>>cheating! should delete midi event object */
       memset(event, 0, sizeof(TMidiEventContainer));
   }
   ARRAY(&this->midi).used_set(0);

   for (i = 0; i < ARRAY(&this->audio).count(); i++) {
      CAudioBuffer(&ARRAY(&this->audio).data()[i]).clear();
   }
}/*CFrame::clear*/

void CAudioStream::CAudioStream(void) {
}/*CAudioStream::CAudioStream*/

bool CAudioStream::frame(CFrame *frame, EStreamMode mode) {
   return 1;
}/*CAudioStream::frame*/

void TChannelMix_set(TChannelMix *this) {
   if (this->pan < 64) {
      this->gain[0] = ((float)this->level / 127);
      this->gain[1] = (((float)this->level / 127) * ((float)this->pan / 64));
   }
   else {
      this->gain[0] = (((float)this->level / 127) * (127 - (float)this->pan) / 64);
      this->gain[1] = ((float)this->level / 127);
   }
};

/*==========================================================================*/
MODULE::END/*===============================================================*/
/*==========================================================================*/
