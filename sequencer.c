/*==========================================================================*/
MODULE::IMPORT/*============================================================*/
/*==========================================================================*/

#include "framework.c"
#include "xmidi.c"
#include "audiostream.c"
#include "devices.c"

/*==========================================================================*/
MODULE::INTERFACE/*=========================================================*/
/*==========================================================================*/

typedef struct {
    int bar;
    int beat;
    int tick;
} TMidiTime;

ATTRIBUTE:typedef<TMidiTime>;

typedef struct {
   int time_offset;
   CXMidiTrack *track;
   CObjPersistent *pattern;
   CMidiEvent *event;
} TSeqTrack;

ARRAY:typedef<TSeqTrack>;

class CSequencer : CDevice {
 private:
   ARRAY<TMidiEventContainer> output_event;

   /* State Machine*/
   CFsm fsm;
   STATE state_stopped(CEvent *event);
   STATE state_playing(CEvent *event);
      STATE state_recording(CEvent *event);

   STATE state_pending;

   /* Sequencer play state */
   TFrameInfo frame_info;

   int frame_clock_sent;
   int time_tempo_us;
   double time_ticks_per_frame;
   double time_ticks_per_beat;
   double time_tick;
   double time_beat_tick;

   int beats_bar;

   ARRAY<TSeqTrack> seq_track;
   ARRAY<TSeqTrack> pat_track;
   CXMidiTrack *metro_track;

   void midi_tempo_set(long midi_tempo);

   int midi_to_time(TMidiTime *midi_time);
   void time_to_midi(int tick, TMidiTime *midi_time);

   /*>>>for now, allocate tracks dynamically during playback*/
   void tracks_refresh(void);
   void tracks_release(void);
 private:
   CObjServer *server;
   void diag_view(void);
   CMidiEvent *event_find(CMidiEvent *event);

   void new(void);
   void delete(void);
   void frame_track(CFrame *frame, TSeqTrack *track);
 public:
   ALIAS<"sequencer">;

   ELEMENT:OBJECT<CXMidiSequence xmidi>;

   int loop_begin_tick;
   int loop_end_tick;
   ATTRIBUTE<TMidiTime loop_begin, "loopBegin"> {
       this->loop_begin = *data;
       this->loop_begin_tick = CSequencer(this).midi_to_time(&this->loop_begin);
   };
   ATTRIBUTE<TMidiTime loop_end, "loopEnd"> {
       this->loop_end = *data;
       this->loop_end_tick = CSequencer(this).midi_to_time(&this->loop_end);
   };
   ATTRIBUTE<TMidiTime position> {
      this->position = *data;
//      this->startTick = tick_current;
      this->time_tick = CSequencer(this).midi_to_time(&this->position);
      CSequencer(this).tracks_release();
      CSequencer(this).tracks_refresh();
   };
   ATTRIBUTE<int tickRaw> {
//      this->tickRaw = *data;
//      this->time_tick = this->tickRaw;
//      CSequencer(this).tracks_refresh();

       TMidiTime pos;
       CSequencer(this).time_to_midi(*data, &pos);
   };
   ATTRIBUTE<double tempo> {
      if (*data) {
         this->tempo = *data;
      }
      else {
         this->tempo = 120;
      }
   };

   ATTRIBUTE<int numerator>;
   ATTRIBUTE<int denominator>;

   ATTRIBUTE<bool stop> {
       this->state_pending = (STATE)&CSequencer::state_stopped;
   };
   ATTRIBUTE<bool play> {
       this->state_pending = (STATE)&CSequencer::state_playing;
   };
   ATTRIBUTE<bool record> {
       this->state_pending = (STATE)&CSequencer::state_recording;
   };
   ATTRIBUTE<bool rewind> {
   };
   /* Just for testing */
   ATTRIBUTE<int startTick> {
      this->startTick = *data;
   };

   void CSequencer(CObjServer *server);

   EDeviceError open(TFrameInfo *info);
   EDeviceError close();
   EDeviceError clear(void);

   void load(const char *file_name);
   void save(const char *file_name);

   virtual bool frame(CFrame *frame, EStreamMode mode);
   void snapshot_set(CFrame *frame);
};

/*==========================================================================*/
MODULE::IMPLEMENTATION/*====================================================*/
/*==========================================================================*/

bool ATTRIBUTE:convert<TMidiTime>(struct tag_CObjPersistent *object,
                                   const TAttributeType *dest_type, const TAttributeType *src_type,
                                   int dest_index, int src_index,
                                   void *dest, const void *src) {
   TMidiTime *midi_time;
   int *value;
   CString *string;

   if (dest_type == &ATTRIBUTE:type<TMidiTime> && src_type == &ATTRIBUTE:type<int>
       && dest_index != -1) {
      midi_time  = (TMidiTime *)dest;
      value = (int *)src;
      switch (dest_index) {
      case 0:
         midi_time->bar = *value;
         break;
      case 1:
         midi_time->beat = *value;
         break;
      case 2:
         midi_time->tick = *value;
         break;
      default:
         return FALSE;
      }
      return TRUE;
   }

   if (dest_type == &ATTRIBUTE:type<int> && src_type == &ATTRIBUTE:type<TMidiTime>
       && src_index != -1) {
      midi_time  = (TMidiTime *)src;
      value = (int *)dest;
      switch (src_index) {
      case 0:
         *value = midi_time->bar;
         break;
      case 1:
         *value = midi_time->beat;
         break;
      case 2:
         *value = midi_time->tick;
         break;
      default:
         return FALSE;
      }
      return TRUE;
   }

   if (dest_type == &ATTRIBUTE:type<CString> && src_type == &ATTRIBUTE:type<TMidiTime>) {
      midi_time  = (TMidiTime *)src;
      string = (CString *)dest;
      if (src_index == -1) {
          CString(string).printf("%d:%d:%d", midi_time->bar, midi_time->beat, midi_time->tick);
      }
      else {
         switch (src_index) {
         case 0:
            CString(string).printf("%d", midi_time->bar);
            break;
         case 1:
            CString(string).printf("%d", midi_time->beat);
            break;
         case 2:
            CString(string).printf("%d", midi_time->tick);
            break;
         default:
            return FALSE;
         }
      }
      return TRUE;
   }

   if (dest_type == &ATTRIBUTE:type<TMidiTime> && src_type == &ATTRIBUTE:type<CString>) {
      midi_time  = (TMidiTime *)dest;
      string = (CString *)src;
      if (dest_index == -1) {
          return FALSE;
      }
      else {
         switch (dest_index) {
         case 0:
            midi_time->bar = atoi(CString(string).string());
            break;
         case 1:
            midi_time->beat = atoi(CString(string).string());
            break;
         case 2:
            midi_time->tick = atoi(CString(string).string());
            break;
         default:
            return FALSE;
         }
      }
      return TRUE;
   }

   return FALSE;
}

#include "midifile.c"

void CSequencer::new(void) {
   new(&this->xmidi).CXMidiSequence();
   CObject(&this->xmidi).parent_set(CObject(this));
   ARRAY(&this->seq_track).new();
   ARRAY(&this->seq_track).used_set(64);
   ARRAY(&this->seq_track).used_set(0);
   ARRAY(&this->pat_track).new();
   ARRAY(&this->pat_track).used_set(64);
   ARRAY(&this->pat_track).used_set(0);

   ARRAY(&this->output_event).new();
   ARRAY(&this->output_event).used_set(128);
   ARRAY(&this->output_event).used_set(0);
}/*CSequencer::new*/

void CSequencer::CSequencer(CObjServer *server) {
   CMidiFile midifile;
//   CFile file;

   this->server = server;

   new(&this->fsm).CFsm(CObject(this), (STATE)&CSequencer::state_stopped);
   CFsm(&this->fsm).init();

   CSequencer(this).clear();

   new(&midifile).CMidiFile();
//   CMidiFile(&midifile).file_import("thetrees.mid", &this->xmidi);
//   CMidiFile(&midifile).file_import("bark.mid", &this->xmidi);
//   CMidiFile(&midifile).file_import("takethetime.mid", &this->xmidi);
//   CMidiFile(&midifile).file_import("subdivisions.mid", &this->xmidi);
//   CMidiFile(&midifile).file_import("eyes.mid", &this->xmidi);
//   CMidiFile(&midifile).file_import("wfs2.mid", &this->xmidi);
//   CMidiFile(&midifile).file_import("testpat.mid", &this->xmidi);
//   CMidiFile(&midifile).file_import("killer.mid", &this->xmidi);
   delete(&midifile);

//   CSequencer(this).diag_view();
}/*CSequecner::CSequencer*/
void CSequencer::delete(void) {
   ARRAY(&this->output_event).delete();

   ARRAY(&this->pat_track).delete();
   ARRAY(&this->seq_track).delete();
   delete(&this->xmidi);
}/*CSequencer::delete*/

EDeviceError CSequencer::clear(void) {
    CObject *object, *next;

    object = CObject(&this->xmidi).child_first();
    while (object) {
        next = CObject(&this->xmidi).child_next(object);
        delete(object);
        object = next;
    }

    object = CObject(&this->xmidi.metatrack).child_first();
    while (object) {
        next = CObject(&this->xmidi.metatrack).child_next(object);
        delete(object);
        object = next;
    }

//    object = CObject(&this->xmidi.patterns).child_first();
//    while (object) {
//        next = CObject(&this->xmidi.patterns).child_next(object);
//        delete(object);
//        object = next;
//    }

    /* Default tempo */
    this->xmidi.header.division = 120;
    CSequencer(this).midi_tempo_set(500000);
    this->numerator = 4;
    this->denominator = 4;

    return EDeviceError.noError;
}/*CSequencer::clear*/

void CSequencer::load(const char *file_name) {
   CFile file;

   CSequencer(this).clear();

   new(&file).CFile();
   CIOObject(&file).open(file_name, O_RDONLY);
   CObjPersistent(&this->xmidi).state_xml_load(CIOObject(&file), NULL, TRUE);
   CIOObject(&file).close();
   delete(&file);
}/*CSequencer::load*/

void CSequencer::save(const char *file_name) {
   CFile file;

   new(&file).CFile();
   CIOObject(&file).open(file_name, O_WRONLY | O_CREAT | O_TRUNC);
   CObjPersistent(&this->xmidi).state_xml_store(CIOObject(&file), NULL);
   delete(&file);
}/*CSequencer::save*/

EDeviceError CSequencer::open(TFrameInfo *info) {
   this->frame_info = *info;

   return EDeviceError.noError;
}/*CSequencer::open*/

EDeviceError CSequencer::close(void) {
   return EDeviceError.noError;
}/*CSequencer::close*/

void CSequencer::midi_tempo_set(long midi_tempo) {
   double time_tick = midi_tempo / this->xmidi.header.division;
   double time_frame;

   CObjPersistent(this).attribute_update(ATTRIBUTE<CSequencer,tempo>);
   this->tempo = (double)60000000 / (double)midi_tempo;
   CObjPersistent(this).attribute_update_end();

   if (this->frame_info.sampling_rate) {
       time_frame = (1000000 / (double)this->frame_info.sampling_rate) * (double)this->frame_info.frame_length;
       this->time_ticks_per_frame = ( time_frame / time_tick );
   }

    //>>>testing hack
#if 0
    this->time_ticks_per_frame = ceil(this->time_ticks_per_frame);
#endif
};

void CSequencer::frame_track(CFrame *frame, TSeqTrack *track) {
   CMidiEvent *check_event, *event, *pending_event;
   double time;
   double tick_end, tick_min, tick_max;
   TSeqTrack *new_track;

    tick_end = this->time_tick + this->time_ticks_per_frame;
//    if (this->loop_end_tick && tick_end >= this->loop_end_tick) {
//       return;
//    }

    check_event = track->event;
    pending_event = NULL;
    while (check_event && (check_event->time + track->time_offset) < tick_end) {
         tick_min = this->time_tick;
         tick_max = tick_min + this->time_ticks_per_frame;

         if ((check_event->time + track->time_offset) >= tick_min &&
             (check_event->time + track->time_offset) < tick_max) {
            if (CObject(check_event).obj_class() == &class(CMEUsePattern)) {
               ARRAY(&this->pat_track).item_add_empty();
               new_track = ARRAY(&this->pat_track).item_last();
               new_track->track = track->track;
               new_track->time_offset = CMidiEvent(check_event)->time;
               new_track->pattern = CObjPersistent(check_event);
               new_track->event   = CMidiEvent(CObject(CMEUsePattern(check_event)->link.object).child_first());
            }
            if (CObjClass_is_derived(&class(CMidiMetaEvent), CObject(check_event).obj_class())) {
               /* MIDI Meta event, process now */
               if (CObject(check_event).obj_class() == &class(CMMETempo)) {
                  CSequencer(this).midi_tempo_set(CMMETempo(check_event)->beat_length);
                  CMMETempo(check_event)->tempo = this->tempo;
               }
               if (CObject(check_event).obj_class() == &class(CMMETime)) {
//                  printf("time set %d:%d:%d - %d (%f) (%f)\n",  this->bar, this->beat, this->tick,
//                         CMMETime(check_event)->numerator, this->time_beat_tick, this->time_tick);
                  this->numerator = CMMETime(check_event)->numerator;
                  this->denominator = CMMETime(check_event)->denominator;
                  this->time_ticks_per_beat = 4 * this->xmidi.header.division / CMMETime(check_event)->denominator;
               }
            }

            /* MIDI event, pass through to output*/
            ARRAY(&frame->midi).item_add(*(TMidiEventContainer *)check_event);
            event = CMidiEvent(ARRAY(&frame->midi).item_last());

            time = (check_event->time + track->time_offset) - this->time_tick;
            event->time = (time * this->frame_info.frame_length / this->time_ticks_per_frame);
         }
         else {
             if (!pending_event) {
                 pending_event = check_event;
             }
         }

      check_event = (CMidiEvent *)CObject(track->pattern).child_next(CObject(check_event));
      while (check_event && !CObjClass_is_derived(&class(CMidiEvent), CObject(check_event).obj_class())) {
         check_event = (CMidiEvent *)CObject(track->pattern).child_next(CObject(check_event));
      }
   }

   if (pending_event) {
      track->event = pending_event;
   }
   else {
      track->event = check_event;
   }
}/*CSequencer::frame_track*/

/*>>>hack!, perhaps scan for song change message in carbon.c frame processing */
extern void carbon_quickload(int index);

bool CSequencer::frame(CFrame *frame, EStreamMode mode) {
   int i;
   TSeqTrack *track;
   CMidiEvent *event;
   CMEControlChange cc_event;
   CMETimingClock tc_event;
   CMEStart seq_event;
   CMENote note_event;
   CObjPersistent *mtrack;
   double tick_end;

   /* handle input events >>>sequencer controls */
   for (i = 0; i < ARRAY(&frame->midi).count(); i++) {
      event = CMidiEvent(&ARRAY(&frame->midi).data()[i]);
      if (CObject(event).obj_class() == &class(CMESongSelect)) {
          carbon_quickload(CMESongSelect(event)->number);
      }
      if (CObject(event).obj_class() == &class(CMEStop)) {
         this->state_pending = (STATE)&CSequencer::state_stopped;
      }
   }

   if (this->state_pending) {
      if (this->state_pending == (STATE)&CSequencer::state_stopped) {
         /* Send all notes off on all channels */
         mtrack = CObjPersistent(CObject(&this->xmidi).child_first());
         while (mtrack) {
            if (CObject(mtrack).obj_class() == &class(CXMidiTrack)) {
               new(&cc_event).CMEControlChange(CXMidiTrack(mtrack)->channel, 0, 123, 0);
               ARRAY(&frame->midi).item_add(*(TMidiEventContainer *)&cc_event);
               delete(&cc_event);
            }
            mtrack = CObjPersistent(CObject(&this->xmidi).child_next(CObject(mtrack)));
         }
         new(&seq_event).CMEStop(0);
         ARRAY(&frame->midi).item_add(*(TMidiEventContainer *)&seq_event);
      }
      CFsm(&this->fsm).transition(this->state_pending);
      
      if (this->state_pending == (STATE)&CSequencer::state_playing ||
          this->state_pending == (STATE)&CSequencer::state_recording) {
          new(&seq_event).CMEStart(0, this->state_pending == (STATE)&CSequencer::state_recording);
          ARRAY(&frame->midi).item_add(*(TMidiEventContainer *)&seq_event);
          new(&seq_event).CMMETempo(0, 0);
          CMMETempo(&seq_event)->tempo = this->tempo;
          ARRAY(&frame->midi).item_add(*(TMidiEventContainer *)&seq_event);
      }
      
      this->state_pending = NULL;
   }

   if (CFsm(&this->fsm).in_state((STATE)&CSequencer::state_playing)) {
//      CObjPersistent(this).attribute_update(ATTRIBUTE<CSequencer,position>);
      this->time_beat_tick += this->time_ticks_per_frame;

      /* Latency compensate clocks messages */
      tick_end = this->time_beat_tick + (this->time_ticks_per_frame * 10);
      if (tick_end >= this->time_ticks_per_beat &&
          this->position.beat + 1 > this->numerator && !this->frame_clock_sent) {
         new(&tc_event).CMETimingClock();
         ARRAY(&frame->midi).item_add(*(TMidiEventContainer *)&tc_event);
         this->frame_clock_sent = 1;
      }

      if (this->time_beat_tick >= this->time_ticks_per_beat) {
         /* Generate metronome */
         if (this->metro_track) {
            if (this->position.beat == this->numerator) {
			   new(&note_event).CMENote(this->metro_track->channel, 0, 72, this->position.beat == this->numerator ? 127 : 32);
               ARRAY(&frame->midi).item_add(*(TMidiEventContainer *)&note_event);
			}
         }
         CObjPersistent(this).attribute_update(ATTRIBUTE<CSequencer,position>);
         this->time_beat_tick -= this->time_ticks_per_beat;
         this->position.beat++;
         if (this->position.beat > this->numerator) {
             this->frame_clock_sent = 0;
             this->position.beat = 1;
             this->position.bar++;
         }

#if 0
         if (this->position.bar == this->loop_end.bar) {
            /* should adjust 'time_tick' for tick/frame offset */
            this->time_tick = CSequencer(this).midi_to_time(&this->loop_begin);
            CSequencer(this).tracks_release();
            CSequencer(this).tracks_refresh();
         }
#endif

         CObjPersistent(this).attribute_update_end(void);
      }

      this->position.tick = this->time_beat_tick;
      this->tickRaw = this->time_tick;
//      CObjPersistent(this).attribute_update_end(void);

      for (i = 0; i < ARRAY(&this->seq_track).count(); i++) {
         track = &ARRAY(&this->seq_track).data()[i];
         CSequencer(this).frame_track(frame, track);
      }

      /*>>>release marked unused tracks*/
      for (i = 0; i < ARRAY(&this->pat_track).count(); i++) {
         track = &ARRAY(&this->pat_track).data()[i];
         CSequencer(this).frame_track(frame, track);

         if (!track->event) {
            /*>>>remove not working properly*/
//            ARRAY(&this->pat_track).item_remove(track);
            memcpy(&ARRAY(&this->pat_track).data()[i], &ARRAY(&this->pat_track).data()[i + 1],
                   (ARRAY(&this->pat_track).count() - i) * sizeof(TSeqTrack));
            ARRAY(&this->pat_track).used_set(ARRAY(&this->pat_track).count() - 1);
         }
      }

      this->time_tick += this->time_ticks_per_frame;

      if (this->loop_end_tick > 0 && this->time_tick >= this->loop_end_tick) {
         /* should adjust 'time_tick' for tick/frame offset */
         this->time_tick = this->loop_begin_tick;
         CSequencer(this).tracks_release();
         CSequencer(this).tracks_refresh();
      }
   }

   /* output side, send pending events */
   for (i = 0; i < ARRAY(&this->output_event).count(); i++) {
       ARRAY(&frame->midi).item_add(ARRAY(&this->output_event).data()[i]);
   }
   ARRAY(&this->output_event).used_set(0);

   return TRUE;
}/*CSequencer::frame*/

CMidiEvent *CSequencer::event_find(CMidiEvent *event) {
   CXMidiTrack *track;
   CMidiEvent *track_event;

   track = (CXMidiTrack *)CObject(&this->xmidi).child_first();
   while (track && !CObjClass_is_derived(&class(CXMidiTrack), CObject(track).obj_class())) {
      track = (CXMidiTrack *)CObject(&this->xmidi).child_next(CObject(track));
   }
   while (track) {
       if (track->channel == event->channel) {
           track_event = (CMidiEvent *)CObject(track).child_first();
           while (track_event) {
              if (CObjClass_is_derived(&class(CMEControlChange), CObject(track_event).obj_class())) {
                 if (CMEControlChange(track_event)->control == CMEControlChange(event)->control && event->time == 0) {
                     return track_event;
                 }
              }
              track_event = (CMidiEvent *)CObject(track).child_next(CObject(track_event));
           }
       }

       track = (CXMidiTrack *)CObject(&this->xmidi).child_next(CObject(track));
       while (track && !CObjClass_is_derived(&class(CXMidiTrack), CObject(track).obj_class())) {
          track = (CXMidiTrack *)CObject(&this->xmidi).child_next(CObject(track));
       }
   }

   return NULL;
}/*CSequencer::event_find*/

void CSequencer::snapshot_set(CFrame *frame) {
   /* Set all control changes at time 0 in the current sequence to current mixer values */
   CMidiEvent *event, *track_event;
   int i;

   for (i = 0; i < ARRAY(&frame->midi).count(); i++) {
      event = CMidiEvent(&ARRAY(&frame->midi).data()[i]);
      track_event = CSequencer(this).event_find(event);
      if (track_event) {
          CMEControlChange(track_event)->value = CMEControlChange(event)->value;
      }
   }
}/*CSequencer::snapshot_set*/

int CSequencer::midi_to_time(TMidiTime *midi_time) {
   CObjPersistent *track;
   CMidiEvent *event;
   int bar = 1, beat = 1;
   int tick_current = 0, ticks_per_beat = this->xmidi.header.division;
   int numerator = 4;

   track = CObjPersistent(CXMidiTrack(&this->xmidi.metatrack));
   event = (CMidiEvent *)CObject(track).child_first();
   while (event && !CObjClass_is_derived(&class(CMidiEvent), CObject(event).obj_class())) {
      event = (CMidiEvent *)CObject(track).child_next(CObject(event));
   }

   while (bar < midi_time->bar) {
      while (event && tick_current >= event->time) {
         if (CObject(event).obj_class() == &class(CMMETime)) {
            numerator = CMMETime(event)->numerator;
            ticks_per_beat = 4 * this->xmidi.header.division / CMMETime(event)->denominator;
         }
         event = (CMidiEvent *)CObject(track).child_next(CObject(event));
         while (event && !CObjClass_is_derived(&class(CMidiEvent), CObject(event).obj_class())) {
            event = (CMidiEvent *)CObject(track).child_next(CObject(event));
         }
      }
      beat++;
      if (beat > numerator) {
         beat = 1;
         bar++;
      }

      tick_current += ticks_per_beat;
   }

   return tick_current;
}/*CSequencer::midi_to_time*/

void CSequencer::time_to_midi(int tick, TMidiTime *midi_time) {
   CObjPersistent *track;
   CMidiEvent *event;
   int bar = 1, beat = 1;
   int tick_current = 0, ticks_per_beat = this->xmidi.header.division;
   int numerator = 4;

   track = CObjPersistent(CXMidiTrack(&this->xmidi.metatrack));
   event = (CMidiEvent *)CObject(track).child_first();
   while (event && !CObjClass_is_derived(&class(CMidiEvent), CObject(event).obj_class())) {
      event = (CMidiEvent *)CObject(track).child_next(CObject(event));
   }

   while (tick_current <= tick - ticks_per_beat) {
      while (event && tick_current >= event->time) {
         if (CObject(event).obj_class() == &class(CMMETime)) {
            numerator = CMMETime(event)->numerator;
            ticks_per_beat = 4 * this->xmidi.header.division / CMMETime(event)->denominator;
         }
         event = (CMidiEvent *)CObject(track).child_next(CObject(event));
         while (event && !CObjClass_is_derived(&class(CMidiEvent), CObject(event).obj_class())) {
            event = (CMidiEvent *)CObject(track).child_next(CObject(event));
         }
      }
      beat++;
      if (beat > numerator) {
         beat = 1;
         bar++;
      }
      tick_current += ticks_per_beat;
   }

   midi_time->tick = 0;
   midi_time->beat = beat;
   midi_time->bar = bar;
}/*CSequencer::time_to_midi*/

void CSequencer::tracks_refresh(void) {
   int i;
   CObjPersistent *track;
   CMidiEvent *event;
   int tempo = 500000;  /*>>>shouldn't need this default */
   int tick_current = 0;

//   CSequencer(this).tracks_release();

   this->numerator = 4;
   this->time_beat_tick = 0;
   this->position.bar = 1;
   this->position.beat = 1;
   this->position.tick = 0;
   this->time_ticks_per_beat = this->xmidi.header.division;

   i = 0;
   /* meta track */
   track = CObjPersistent(CXMidiTrack(&this->xmidi.metatrack));
   ARRAY(&this->seq_track).used_set(0);
   ARRAY(&this->seq_track).item_add_empty();
   memset(&ARRAY(&this->seq_track).data()[0], 0, sizeof(TSeqTrack));
   ARRAY(&this->seq_track).data()[0].pattern = track;
   event = (CMidiEvent *)CObject(track).child_first();
   while (event && !CObjClass_is_derived(&class(CMidiEvent), CObject(event).obj_class())) {
      event = (CMidiEvent *)CObject(track).child_next(CObject(event));
   }

   while (tick_current <= this->time_tick) {
      while (event && tick_current >= event->time) {
         if (CObject(event).obj_class() == &class(CMMETempo)) {
            tempo = CMMETempo(event)->beat_length;
         }
         if (CObject(event).obj_class() == &class(CMMETime)) {
            this->numerator = CMMETime(event)->numerator;
            this->denominator = CMMETime(event)->denominator;
            this->time_ticks_per_beat = 4 * this->xmidi.header.division / CMMETime(event)->denominator;
         }
         event = (CMidiEvent *)CObject(track).child_next(CObject(event));
         while (event && !CObjClass_is_derived(&class(CMidiEvent), CObject(event).obj_class())) {
            event = (CMidiEvent *)CObject(track).child_next(CObject(event));
         }
      }

      if (tick_current < this->time_tick) {
         this->position.beat++;
         if (this->position.beat > this->numerator) {
            this->position.beat = 1;
            this->position.bar++;
         }
      }
      tick_current += this->time_ticks_per_beat;
   }
   tick_current = this->time_tick;

   this->position.tick = tick_current - this->time_tick;
   this->tickRaw = this->time_tick;


   CSequencer(this).midi_tempo_set(tempo);
   ARRAY(&this->seq_track).data()[0].event = event;

   i++;
   track = CObjPersistent(CObject(&this->xmidi).child_first());

   this->metro_track = NULL;
   while (track) {
      if (CObject(track).obj_class() == &class(CXMidiTrack)) {
         if (CXMidiTrack(track)->mode == EMidiTrackMode.metro) {
            this->metro_track = CXMidiTrack(track);
         }
         ARRAY(&this->seq_track).item_add_empty();
         memset(&ARRAY(&this->seq_track).data()[i], 0, sizeof(TSeqTrack));
         ARRAY(&this->seq_track).data()[i].track = CXMidiTrack(track);
         ARRAY(&this->seq_track).data()[i].pattern = CObjPersistent(CXMidiTrack(track));

         event = (CMidiEvent *)(CObject(track).child_first());
         while (event && !CObjClass_is_derived(&class(CMidiEvent), CObject(event).obj_class())) {
            event = (CMidiEvent *)CObject(track).child_next(CObject(event));
         }

         while (event && event->time < this->time_tick) {
            event = (CMidiEvent *)CObject(track).child_next(CObject(event));
            while (event && !CObjClass_is_derived(&class(CMidiEvent), CObject(event).obj_class())) {
               event = (CMidiEvent *)CObject(track).child_next(CObject(event));
            }
         }
         ARRAY(&this->seq_track).data()[i].event = event;
         i++;
      }
      track = CObjPersistent(CObject(&this->xmidi).child_next(CObject(track)));
   }
}/*CSequencer::tracks_refresh*/

void CSequencer::tracks_release(void) {
   ARRAY(&this->seq_track).used_set(0);
   ARRAY(&this->pat_track).used_set(0);
}/*CSequencer::tracks_release*/

STATE CSequencer::state_stopped(CEvent *event) {
   if (CObject(event).obj_class() == &class(CEventState)) {
      switch (CEventState(event)->type) {
      case EEventState.enter:
         CObjPersistent(this).attribute_update(ATTRIBUTE<CSequencer,stop>);
         CObjPersistent(this).attribute_update(ATTRIBUTE<CSequencer,play>);
         CObjPersistent(this).attribute_update(ATTRIBUTE<CSequencer,record>);
         this->stop = FALSE;
         this->play = FALSE;
         this->record = FALSE;
         CObjPersistent(this).attribute_update_end();
         return NULL;
      case EEventState.leave:
         break;
      default:
         break;
      }
   }
   return (STATE)&CFsm::top;
}/*CSequencer::state_stopped*/

STATE CSequencer::state_playing(CEvent *event) {
   if (CObject(event).obj_class() == &class(CEventState)) {
      switch (CEventState(event)->type) {
      case EEventState.enter:
         /*>>>kludge*/
         CObjPersistent(this).attribute_update(ATTRIBUTE<CSequencer,stop>);
         CObjPersistent(this).attribute_update(ATTRIBUTE<CSequencer,play>);
         CObjPersistent(this).attribute_update(ATTRIBUTE<CSequencer,record>);
         this->stop = FALSE;
         this->play = TRUE;
         this->record = FALSE;
         CObjPersistent(this).attribute_update_end();

         /* kludge, for now */
//         this->time_tick = this->startTick;
         CSequencer(this).tracks_release();
         CSequencer(this).tracks_refresh();
         return NULL;
      case EEventState.leave:
         CSequencer(this).tracks_release();
	 break;
      default:
         break;
      }
   }
   return (STATE)&CFsm::top;
}/*CSequencer::state_playing*/

STATE CSequencer::state_recording(CEvent *event) {
   if (CObject(event).obj_class() == &class(CEventState)) {
      switch (CEventState(event)->type) {
      case EEventState.enter:
         CObjPersistent(this).attribute_update(ATTRIBUTE<CSequencer,stop>);
         CObjPersistent(this).attribute_update(ATTRIBUTE<CSequencer,play>);
         CObjPersistent(this).attribute_update(ATTRIBUTE<CSequencer,record>);
         this->stop = FALSE;
         this->play = FALSE;
         this->record = TRUE;
         this->time_tick = 0;
         CObjPersistent(this).attribute_update_end();

         /* kludge, for now */
         this->time_tick = this->startTick;

         CSequencer(this).tracks_release();
         CSequencer(this).tracks_refresh();
         return NULL;
      case EEventState.leave:
	 break;
      default:
         break;
      }
   }

   return (STATE)&CSequencer::state_playing;
}/*CSequencer::state_recording*/

void CSequencer::diag_view(void) {
   CGLayout *layout;
//   CObjPersistent *device_tree;
   CGTree *tree;
   CGWindow *window;

   layout = new.CGLayout(0, 0, this->server, CObjPersistent(this));
   CGCanvas(layout).colour_background_set(GCOL_NONE);
   CGLayout(layout).render_set(EGLayoutRender.none);

   tree = new.CGTree(this->server, CObjPersistent(this), 0, 0, 0, 0);
   CObject(layout).child_add_front(CObject(tree));

   window = new.CGWindow("Sequencer Tree", CGCanvas(layout), NULL);
   CGWindow(window).show(TRUE);
}/*CSequencer::diag_view*/
