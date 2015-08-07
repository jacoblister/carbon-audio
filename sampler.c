/*==========================================================================*/
MODULE::IMPORT/*============================================================*/
/*==========================================================================*/

#include "framework.c"
#include "xmidi.c"
#include "audiostream.c"
#include "audiofile.c"
#include "devices.c"

/*==========================================================================*/
MODULE::INTERFACE/*=========================================================*/
/*==========================================================================*/

#define SAMPLER_CHANNELS 16
#define SAMPLER_BANKS    10

#define SAMPLER_FADETIME 44100

class CSampler : CDevice {
 private:
   void new(void);
   void delete(void);
   TFrameInfo frame_info;
   CAudioBuffer mix_buffer;
   
   bool recording;
   byte *mem_buf;
   int mem_buf_size;
   int mem_ptr;

   void diag_view(void);
   bool sample_filename_decode(const char *filename, int *channel, int *bank);
   void mem_allocate(void);
   void mem_release(void);
 public:
   ALIAS<"sampler">;

   CObjServer *server;

   void CSampler(CObjServer *server);

   virtual EDeviceError open(TFrameInfo *info);
   virtual EDeviceError close();
   virtual bool frame(CFrame *frame);

   void clear(void);
   bool sample_save(const char *dirname);
   bool sample_load(const char *dirname);
};

class CSamplerBank : CAudioBuffer {
 private:
   void new(void);
   void delete(void);
   ARRAY<TMidiEventContainer> midi;
 public:
   ALIAS<"samplerBank">;
   ATTRIBUTE<bool active> {
      this->active = FALSE;             /*>>>test kludge*/
   };
   ATTRIBUTE<bool stop> {
//      CSamplerBank(this).command(EMESampleType.stop);
   };
   ATTRIBUTE<bool record> {
//      CSamplerBank(this).command(EMESampleType.record);
   };
   ATTRIBUTE<bool play> {
//      CSamplerBank(this).command(EMESampleType.play);
   };

   void CSamplerBank(void);
};

typedef struct {
   EMESampleType mode;
   CSamplerBank *bank;
   int sample;
   int midi_index;

   EMESampleType pending_mode;
   int pending_sample;
   CSamplerBank *pending_bank;
} TSamplePtr;

class CSamplerChannel : CAudioStream {
 private:
   void command(EMESampleType command, int bank, int sample);

   int index;
   TSamplePtr play_ptr;
   TSamplePtr record_ptr;

//   void active_set(CSamplerBank *bank);
 public:
   ALIAS<"sampChannel">;

   void CSamplerChannel(int index);
};

/*==========================================================================*/
MODULE::IMPLEMENTATION/*====================================================*/
/*==========================================================================*/

#include <sys/mman.h>
#define SAMPLE_MEM 100000000

void CSampler::new(void) {
}/*CSampler::new*/

void CSampler::CSampler(CObjServer *server) {
   int i;
   CSamplerChannel *channel;

   this->server = server;

   for (i = 0; i < SAMPLER_CHANNELS; i++) {
      channel = new.CSamplerChannel(i);
      CObject(this).child_add(CObject(channel));
   }

//   CSampler(this).diag_view();
}/*CSampler::CSampler*/

void CSampler::delete(void) {
}/*CSampler::delete*/

void CSampler::diag_view(void) {
   CGLayout *layout;
//   CObjPersistent *device_tree;
   CGTree *tree;
   CGWindow *window;

   layout = new.CGLayout(0, 0, this->server, CObjPersistent(this));
   CGCanvas(layout).colour_background_set(GCOL_NONE);
   CGLayout(layout).render_set(EGLayoutRender.none);

   tree = new.CGTree(this->server, CObjPersistent(this), 0, 0, 0, 0);
   CObject(layout).child_add_front(CObject(tree));

   window = new.CGWindow("Sampler Tree", CGCanvas(layout), NULL);
   CGWindow(window).show(TRUE);
}/*CSampler::diag_view*/

void CSampler::mem_allocate(void) {
   this->mem_ptr = 0;
}/*CSampler::mem_alloc*/

void CSampler::mem_release(void) {
}/*CSampler::mem_release*/

EDeviceError CSampler::open(TFrameInfo *info) {
   this->frame_info = *info;
   
   new(&this->mix_buffer).CAudioBuffer(info->data_format, info->sampling_rate);
   CAudioBuffer(&this->mix_buffer).data_allocate(info->frame_length);
   CAudioBuffer(&this->mix_buffer).data_length_set(0);
   
   this->mem_buf = malloc(SAMPLE_MEM);
   mlock(this->mem_buf, SAMPLE_MEM);

   return EDeviceError.noError;
}/*CSampler::open*/

EDeviceError CSampler::close(void) {
   delete(&this->mix_buffer);
   
   munlock(this->mem_buf, SAMPLE_MEM);
   free(this->mem_buf);
   
   return EDeviceError.noError;
}/*CSampler::close*/

bool CSampler::frame(CFrame *frame, EStreamMode mode) {
   CSamplerChannel *channel;
   int count, i, j;
   CMidiEvent *event;
   TMidiEventContainer new_event;
   int sample;
   EMESampleType command;

   
   for (i = 0; i < ARRAY(&frame->midi).count(); i++) {
      event = CMidiEvent(&ARRAY(&frame->midi).data()[i]);
      if (CObject(event).obj_class() == &class(CMEStart) && CMEStart(event)->recording) {
         CSampler(this).mem_release();
         CSampler(this).mem_allocate();
      }
      if (CObject(event).obj_class() == &class(CMEStart)) {
         this->recording = CMEStart(event)->recording;
      }
      if (CObject(event).obj_class() == &class(CMESample)) {
         command = CMESample(event)->type;
         switch(command) {
         case EMESampleType.stopRecord:
            command = this->recording ? EMESampleType.stopRecord : EMESampleType.stopPlay;
            /* Fallthrough */
         case EMESampleType.stop:
         case EMESampleType.stopPlay:
            channel = CSamplerChannel(CObject(this).child_n(event->channel - 1));
            if (channel) {
               CSamplerChannel(channel).command(command, CMESample(event)->bank, CMidiEvent(event)->time);
            }
            
            if (command == EMESampleType.stop || command == EMESampleType.stopPlay) {
printf("send notes off %d\n", event->channel);
               new(&new_event).CMEControlChange(event->channel, frame->length - 1, MIDI_CONTROL_ALLNOTESOFF, 0);
               ARRAY(&frame->midi).item_add(new_event);
            }
            break;
         default:
            break;
         }
      }
   }

   for (i = 0; i < ARRAY(&frame->midi).count(); i++) {
      event = CMidiEvent(&ARRAY(&frame->midi).data()[i]);
      if (CObject(event).obj_class() == &class(CMESample)) {
         command = CMESample(event)->type;
         switch(command) {
         case EMESampleType.record:
            command = this->recording ? EMESampleType.record : EMESampleType.play;
            /* Fallthrough */
         case EMESampleType.play:
         case EMESampleType.playHalf:
         case EMESampleType.playReverse:
            channel = CSamplerChannel(CObject(this).child_n(event->channel - 1));
            if (channel) {
               CSamplerChannel(channel).command(command, CMESample(event)->bank, CMidiEvent(event)->time);
            }
            break;
         default:
            break;
         }
      }
   }

   i = 0;
   channel = CSamplerChannel(CObject(this).child_first());
   while (channel) {
      /* recording */
      sample = 0;
      if (channel->record_ptr.pending_mode != EMESampleType.none) {
         if (channel->record_ptr.pending_sample >= frame->length) {
            channel->record_ptr.pending_sample -= frame->length;
         }
         else {
            if (channel->record_ptr.bank) {
               /* Write last contents of currently recording bank */
               CAudioBuffer(channel->record_ptr.bank).write(&ARRAY(&frame->audio).data()[i], 
                                                            channel->record_ptr.pending_sample);
            }
            channel->record_ptr.mode = channel->record_ptr.pending_mode;
            channel->record_ptr.bank = channel->record_ptr.pending_bank;
            channel->record_ptr.sample = -channel->record_ptr.pending_sample;
            channel->record_ptr.pending_mode = EMESampleType.none;
            sample = channel->record_ptr.pending_sample;
         }
      }
      if (channel->record_ptr.bank) {
         CAudioBuffer(channel->record_ptr.bank)->sampling_rate = this->frame_info.sampling_rate;
         CAudioBuffer(&ARRAY(&frame->audio).data()[i]).extract(&this->mix_buffer,
                                                               sample, 
                                                               frame->length - sample);
         CAudioBuffer(channel->record_ptr.bank).write(&this->mix_buffer, 
                                                      frame->length - sample);
                                                       
         // now record MIDI    
         count = ARRAY(&frame->midi).count();
         for (j = 0; j < count; j++) {
            event = CMidiEvent(&ARRAY(&frame->midi).data()[j]);
            if (event->interface == IFACE_INPUT_INSTRUMENT && event->channel == i + 1) {
               new_event = *(TMidiEventContainer *)event;
               CMidiEvent(&new_event)->time += channel->record_ptr.sample;
               ARRAY(&channel->record_ptr.bank->midi).item_add(new_event);
//               printf("record midi event time=%d\n", CMidiEvent(&new_event)->time);
            }
         }
                                                     
         channel->record_ptr.sample += frame->length;
      }
    
      /* Hardware monitoring clear input signal now */
      if (this->frame_info.monitor_mode == EMixMode.hardware) {
         CAudioBuffer(&ARRAY(&frame->audio).data()[i]).clear();
      }

      /* playback */
      CAudioBuffer(&ARRAY(&frame->audio).data()[i]).data_length_set(0);
      sample = 0;
      if (channel->play_ptr.pending_mode != EMESampleType.none) {
         if (channel->play_ptr.pending_sample >= frame->length) {
            channel->play_ptr.pending_sample -= frame->length;
         }
         else {
            if (channel->play_ptr.bank) {
               /* Write last contents of currently playing bank */
               CAudioBuffer(channel->play_ptr.bank).extract(&ARRAY(&frame->audio).data()[i],
                                                             channel->play_ptr.sample, 
                                                             channel->play_ptr.pending_sample);
//               CAudioBuffer(&ARRAY(&frame->audio).data()[i]).fade(0, SAMPLER_FADETIME, 0.0, 1.0, channel->play_ptr.sample);
//               CAudioBuffer(&ARRAY(&frame->audio).data()[i]).fade(
//               CAudioBuffer(channel->play_ptr.bank).length() - SAMPLER_FADETIME, 
//               CAudioBuffer(channel->play_ptr.bank).length(), 1.0, 0.0, channel->play_ptr.sample);
                                                             
            }
            
            channel->play_ptr.mode = channel->play_ptr.pending_mode;
            channel->play_ptr.bank = channel->play_ptr.pending_bank;
            channel->play_ptr.pending_mode = EMESampleType.none;
            channel->play_ptr.sample = 0;
            sample = channel->play_ptr.pending_sample;
         }  
      }
      if (channel->play_ptr.bank) {
         CAudioBuffer(channel->play_ptr.bank).extract(&this->mix_buffer,
                                                      channel->play_ptr.sample, 
                                                      frame->length - sample);
//         CAudioBuffer(&this->mix_buffer).fade(0, SAMPLER_FADETIME, 0.0, 1.0, channel->play_ptr.sample);
//         CAudioBuffer(&this->mix_buffer).fade(SAMPLER_FADETIME, SAMPLER_FADETIME * 2, 1.0, 0.0, channel->play_ptr.sample);
//         CAudioBuffer(&ARRAY(&frame->audio).data()[i]).fade(
//            CAudioBuffer(channel->play_ptr.bank).length() - SAMPLER_FADETIME, 
//            CAudioBuffer(channel->play_ptr.bank).length(), 1.0, 0.0, channel->play_ptr.sample);
         
         CAudioBuffer(&ARRAY(&frame->audio).data()[i]).write(&this->mix_buffer, 
                                                              frame->length - sample);
                                                              
         // playback MIDI    
         while (channel->play_ptr.midi_index < ARRAY(&channel->play_ptr.bank->midi).count()) {
            event = CMidiEvent(&ARRAY(&channel->play_ptr.bank->midi).data()[channel->play_ptr.midi_index]);
            if (event->time >= channel->play_ptr.sample + frame->length) {
               break;
            }

            new_event = *(TMidiEventContainer *)event;
//            CMidiEvent(&new_event)->time -= channel->play_ptr.sample;
            CMidiEvent(&new_event)->time = frame->length - 1;
            ARRAY(&frame->midi).item_add(new_event);
//printf("playback event time=%d, %d\n", event->time, CMidiEvent(&new_event)->time);
            
            channel->play_ptr.midi_index++;
         }

         channel->play_ptr.sample += frame->length;
      }
      CAudioBuffer(&ARRAY(&frame->audio).data()[i]).data_length_set(frame->length);
      channel = CSamplerChannel(CObject(this).child_next(CObject(channel)));
      i++;
   }
   return TRUE;
}/*CSampler::frame*/

bool CSampler::sample_filename_decode(const char *filename, int *channel, int *bank) {
    if (toupper(filename[0]) != 'C')
        return FALSE;

    if (!(isdigit(filename[1]) && isdigit(filename[2])))
        return FALSE;

    *channel = ((filename[1] - '0') * 10) + (filename[2] - '0');

    if (toupper(filename[3]) != 'B')
        return FALSE;

    if (!(isdigit(filename[4]) && isdigit(filename[5])))
        return FALSE;

    *bank = ((filename[4] - '0') * 10) + (filename[5] - '0');

    return TRUE;
}/*CSampler::sample_filename_decode*/

EDeviceError CSampler::clear(void) {
  CSamplerChannel *channel;
  CSamplerBank *bank;

  channel = CSamplerChannel(CObject(this).child_first());
  while (channel) {
     bank = CSamplerBank(CObject(channel).child_first());
     while (bank) {
        if (bank->active) {
           CObjPersistent(bank).attribute_update(ATTRIBUTE<CSamplerBank,active>);
           CObjPersistent(bank).attribute_set_int(ATTRIBUTE<CSamplerBank,active>, FALSE);
           CObjPersistent(bank).attribute_update_end();
        }

        bank = CSamplerBank(CObject(channel).child_next(CObject(bank)));
     }

     channel = CSamplerChannel(CObject(this).child_next(CObject(channel)));
  }

  return EDeviceError.noError;
}/*CSampler::clear*/

bool CSampler::sample_load(const char *dirname) {
  DIR *dp;
  struct dirent *ep;
  int channel_n, bank_n;
  CSamplerChannel *channel;
  CSamplerBank *bank;
  CAudioFileWave audio_file;
  char dirpath[80], fullname[80];

  CSampler(this).clear();

  sprintf(dirpath, "%s/", dirname);

  dp = opendir (dirpath);
  if (dp != NULL) {
      while ((ep = readdir(dp))) {
         if (strcmp(ep->d_name + strlen(ep->d_name) - 4, ".wav") == 0) {
            if (CSampler(this).sample_filename_decode(ep->d_name, &channel_n, &bank_n)) {
               channel = CSamplerChannel(CObject(this).child_n(channel_n));
               bank = CSamplerBank(CObject(channel).child_n(bank_n));
               new(&audio_file).CAudioFileWave(CAudioBuffer(bank));
//               CAudioBuffer(bank).data_allocate(30000000/*channel->index <= 9 ? 2000000 : (96000 * 60 * 6)*/);
//               CAudioBuffer(bank).data_length_set(0);
               CAudioBuffer(bank)->data_empty = FALSE;
               sprintf(fullname, "%s/%s", dirname, ep->d_name);
               CAudioFile(&audio_file).file_load(fullname);
               CObjPersistent(bank).attribute_update(ATTRIBUTE<CSamplerBank,active>);
               bank->active = TRUE;
               CObjPersistent(bank).attribute_update_end();
               delete(&audio_file);
            }
         }
      }
      (void) closedir (dp);
    }
  else
    puts ("Couldn't open the directory.");

  return TRUE;
}/*CSampler::sample_load*/

bool CSampler::sample_save(const char *dirname) {
  CSamplerChannel *channel;
  CSamplerBank *bank;
  CString filename;
  CAudioFileWave audio_file;
  int i;

  /*>>>create sample directory if it doesn't exist */

  new(&filename).CString(NULL);
  channel = CSamplerChannel(CObject(this).child_first());
  while (channel) {
     i = 0;
     bank = CSamplerBank(CObject(channel).child_first());
     while (bank) {
        if (bank->active) {
           CString(&filename).printf("%s/C%02dB%02d.wav", dirname, channel->index, i);

           new(&audio_file).CAudioFileWave(CAudioBuffer(bank));
           CAudioFile(&audio_file).file_save(CString(&filename).string());
           delete(&audio_file);
        }

        bank = CSamplerBank(CObject(channel).child_next(CObject(bank)));
        i++;
     }

     channel = CSamplerChannel(CObject(this).child_next(CObject(channel)));
  }
  delete(&filename);

  return TRUE;
}/*CSampler::sample_save*/

void CSamplerBank::new(void) {
   ARRAY(&this->midi).new();
}/*CSamplerBank::new*/

void CSamplerBank::CSamplerBank(void) {
   CAudioBuffer(this).CAudioBuffer(EAudioDataType.float, 0);
}/*CSamplerBank::CSamplerBank*/

void CSamplerBank::delete(void) {
  ARRAY(&this->midi).delete();
}/*CSamplerBank::delete*/

void CSamplerChannel::command(EMESampleType command, int bank_index, int sample) {
   CSamplerBank *bank = CSamplerBank(CObject(this).child_n(bank_index));
   CSampler *sampler = CSampler(CObject(this).parent());
sample = 0;
sampler->frame_info.latency_input = 0;
sampler->frame_info.latency_output = 0;

   CObjPersistent(bank).attribute_update(ATTRIBUTE<CSamplerBank,active>);
   CObjPersistent(bank).attribute_update(ATTRIBUTE<CSamplerBank,stop>);
   CObjPersistent(bank).attribute_update(ATTRIBUTE<CSamplerBank,play>);
   CObjPersistent(bank).attribute_update(ATTRIBUTE<CSamplerBank,record>);
   bank->stop = FALSE;
   switch (command) {
   case EMESampleType.stop:
      this->play_ptr.pending_mode = EMESampleType.stopPlay;
      this->play_ptr.pending_sample = sample;
      this->play_ptr.pending_bank = NULL;
      this->record_ptr.pending_mode = EMESampleType.stopRecord;
      this->record_ptr.pending_sample = sample;
      this->record_ptr.pending_bank = NULL;
      bank->play = FALSE;
      bank->record = FALSE;
      break;
   case EMESampleType.stopPlay:
      this->play_ptr.pending_mode = command;
      this->play_ptr.pending_sample = sample;
      this->play_ptr.pending_bank = NULL;
      bank->play = FALSE;
      break;
   case EMESampleType.stopRecord:
      sampler->mem_ptr += (this->record_ptr.sample + sampler->frame_info.latency_input + sampler->frame_info.latency_output) * sizeof(float);
      this->record_ptr.pending_mode = command;
      this->record_ptr.pending_sample = sample +
         (sampler->frame_info.latency_input + sampler->frame_info.latency_output);
      this->record_ptr.pending_bank = NULL;
      bank->record = FALSE;
      break;
   case EMESampleType.play:
   case EMESampleType.playHalf:
   case EMESampleType.playReverse:
      this->play_ptr.pending_mode = command;
      this->play_ptr.pending_bank = bank;
      this->play_ptr.pending_sample = sample;
      this->play_ptr.midi_index = 0;      
      bank->play = TRUE;
      break;
   case EMESampleType.record:
      ARRAY(&bank->midi).used_set(0);
      /*>>>better memory allocation */
//      CAudioBuffer(bank).data_allocate(3000000/*channel->index <= 9 ? 2000000 : (96000 * 60 * 6)*/);
//      CAudioBuffer(bank).data_allocate(0);
      CAudioBuffer(bank).data_allocate_buffer(sampler->mem_buf + sampler->mem_ptr, 30000000);
      CAudioBuffer(bank).data_length_set(0);
      bank->record = TRUE;
      bank->active = TRUE;
      this->record_ptr.pending_mode = command;
      this->record_ptr.pending_bank = bank;
      this->record_ptr.pending_sample = sample +
         (sampler->frame_info.latency_input + sampler->frame_info.latency_output);
      break;
   default:
      break;
   }

   CObjPersistent(bank).attribute_update_end();
}/*CSamplerChannel::command*/

void CSamplerChannel::CSamplerChannel(int index) {
   int i;
   CSamplerBank *bank;

   this->index = index;

   for (i = 0; i < SAMPLER_BANKS; i++) {
      bank = new.CSamplerBank();
      CObject(this).child_add(CObject(bank));
   }
}/*CSamplerChannel*/

//void CSamplerChannel::active_set(CSamplerBank *bank) {
//   if (this->bank_active && this->bank_active != bank) {
//      CSamplerBank(this->bank_active).command(EMESampleType.stop);
//   }
//   this->bank_active = bank;
//}/*CSamplerChannel::active_set*/

/*==========================================================================*/
MODULE::END/*===============================================================*/
/*==========================================================================*/
