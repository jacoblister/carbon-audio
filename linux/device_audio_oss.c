/*==========================================================================*/
MODULE::IMPORT/*============================================================*/
/*==========================================================================*/

#include "framework.c"
#include "../devices.c"
#include "../xmidi.c"
#include "../mixer.c"

/*==========================================================================*/
MODULE::INTERFACE/*=========================================================*/
/*==========================================================================*/

/*==========================================================================*/
MODULE::IMPLEMENTATION/*====================================================*/
/*==========================================================================*/

#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/soundcard.h>

class CDeviceAudioOSS : CDeviceAudio {
 private:
   int handle;
   CAudioBuffer buffer;
 
   int frame_length; 
 public:
   ALIAS<"deviceAudioOSS">;    
   void CDeviceAudioOSS(void);    
    
   virtual EDeviceError open(void);
   virtual EDeviceError close(void);
   virtual bool frame(CFrame *frame, EStreamMode mode); 
};

EDeviceError CDeviceAudioOSS::open(TFrameInfo *info) {
   int open_mode, caps, enable_bits, tmp, scratch_size, i;
   int dev_format, stereo, samp_rate, frag_size, frag_arg;
   int block_size_2;
   jmp_buf env;

   if (setjmp(env)) {
      printf("error initilizing sound device\n");
      exit(0);
   }
   
   class:base.open(info);   
   
   this->frame_length = info->frame_length;

   block_size_2 = 0;
   while (!(info->frame_length & (1 << block_size_2))) {   
      block_size_2++;
   }

   scratch_size = 0;
   enable_bits = 0;

   open_mode = O_RDWR;
   /*>>> expand this to take unsigned foramts */
   //dev_format = AF_WIDTH(PVALINT(&dev->std.dev.ptable, PTD_AUD_FORMAT))
   //  == AF_16Bit ? AFMT_S16_LE : AFMT_S8;
   dev_format = AFMT_S16_LE;
   //>>>stereo = dev->std.channels == 2 ? TRUE : FALSE;
   stereo = TRUE;

   /* Open the device, set sample format parameters */
   if ((this->handle = open(CString(&CDeviceHardware(this)->device).string(),
                               open_mode, 0)) == -1)
     longjmp(env, 1);

   if ((ioctl(this->handle, SNDCTL_DSP_SETDUPLEX, 0)) == -1 )
     longjmp(env, 1);
   
   if ((ioctl(this->handle, SNDCTL_DSP_GETCAPS, &caps)) == -1 )
     longjmp(env, 1);

   if (!(caps & (DSP_CAP_TRIGGER | DSP_CAP_MMAP)))
     longjmp(env, 1);

   samp_rate = info->sampling_rate;

   /*>>> check sample parameters are accepted */
   ioctl(this->handle, SNDCTL_DSP_SETFMT, &dev_format);
   ioctl(this->handle, SNDCTL_DSP_STEREO, &stereo);
   ioctl(this->handle, SNDCTL_DSP_SPEED, &samp_rate);

#if 0
   ioctl(this->handle, SNDCTL_DSP_CHANNELS, 4);
#endif

   /* set fragment size & allocate buffers */
   frag_size = block_size_2;
   if (stereo)
     frag_size += 1;
   if (dev_format == AFMT_S16_LE)
     frag_size += 1;

   /* set fragment size, request 2 fragments */
   frag_arg = frag_size | 0x00010000L;
   ioctl(this->handle, SNDCTL_DSP_SETFRAGMENT, &frag_arg);

   /* Allocate buffer(s) and configure sets for select() */
   for (i = 0; i <= 1; i++) {
      enable_bits |= i ? PCM_ENABLE_OUTPUT : PCM_ENABLE_INPUT;
//      ioctl(this->handle, i ? SNDCTL_DSP_GETOSPACE : SNDCTL_DSP_GETISPACE, &buf_info);
//      tmp = buf_info.fragstotal * buf_info.fragsize;

//      dev->block_size[i] = 1 << frag_size;
//      dev->buffer_size[i] = tmp;
//      dev->buffer[i].v = malloc(dev->block_size[i]);
   }

   new(&this->buffer).CAudioBuffer(CDeviceAudio(this)->data_format, CDeviceAudio(this)->sampling_rate);
   CAudioBuffer(&this->buffer).data_length_set(info->frame_length);

   /* Start the device rolling */
   tmp = 0;
   ioctl(this->handle, SNDCTL_DSP_SETTRIGGER, &tmp);
   ioctl(this->handle, SNDCTL_DSP_SETTRIGGER, &enable_bits);

   return EDeviceError.noError;
}/*CDeviceAudioOSS::open*/

EDeviceError CDeviceAudioOSS::close(void) {
   close(this->handle);
	
   delete(&this->buffer);
	
   class:base.close();   
    
   return EDeviceError.noError;    
}/*CDeviceAudioOSS::close*/

bool CDeviceAudioOSS::frame(CFrame *frame, EStreamMode mode) {
   int bytes;

   /*>>>oss support is mono only at present */           	      
   switch (mode) {
   case EStreamMode.input:	   
      do {	   
         bytes = read(this->handle, CAudioBuffer(&this->buffer).data_pointer_get(), this->frame_length * 4);
         CAudioBuffer(&ARRAY(&frame->audio).data()[0]).convert(&this->buffer);
      } while (bytes == -1);
      class:base.frame(frame, mode);         
      break;
   case EStreamMode.output:
      class:base.frame(frame, mode);
      do {
         CAudioBuffer(&this->buffer).convert(&ARRAY(&frame->audio).data()[0]);
         bytes = write(this->handle, CAudioBuffer(&this->buffer).data_pointer_get(), this->frame_length * 4);
      } while (bytes == -1);
      
#if 0
      ioctl(this->handle, SNDCTL_DSP_GETOSPACE, &oinfo);      
      if (oinfo.bytes > 256) {
         printf("sync\n");
         write(this->handle, buf/*CAudioBuffer(&this->buffer).data_pointer_get()*/, this->frame_length * 4);
      }
#endif      
      break;
   default:
      return FALSE;
   }
//   read(this->handle, CAudioBuffer(&ARRAY(&frame->audio).data()[this->active_channel]).data_pointer_get(), this->frame_length * 4);
//   write(this->handle, CAudioBuffer(&ARRAY(&frame->audio).data()[0]).data_pointer_get(), this->frame_length * 4);
   return TRUE;
}/*CDeviceAudioOSS::frame*/

/*==========================================================================*/
MODULE::END/*===============================================================*/
/*==========================================================================*/
