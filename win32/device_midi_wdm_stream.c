/*==========================================================================*/
MODULE::IMPORT/*============================================================*/
/*==========================================================================*/

#include "framework.c"
#include "..\devices.c"

/*==========================================================================*/
MODULE::INTERFACE/*=========================================================*/
/*==========================================================================*/

/*==========================================================================*/
MODULE::IMPLEMENTATION/*====================================================*/
/*==========================================================================*/

/* Notes:

*/

#include <windows.h>

extern HMIDISTRM ASIO_midi_handle;
extern MMTIME ASIO_midi_time;

typedef struct {
   MIDIHDR midiHdr;      
   unsigned long mevent[1000 * 3];
} TWDMMidiBuffer;

class CDeviceMIDIWDMStream : CDeviceMIDI {
 private:
   HMIDISTRM outHandle; 
 
   enum {st_idle, st_resync, st_run, st_calib, st_trim} state;
   TWDMMidiBuffer m_buf[8]; 
   int m_buf_n;
   long calib_event_time;
   long callback_time_last; 
   long time_last; 
   long frame_length;
   long sampling_rate;
   
   /* audio sync timing */
   long audio_frame_time;
   long audio_frame_calib_time;
   long audio_event_calib_time;
 
   int midi_device_resolve(EStreamMode mode, const char *device_name);
 public:
   ALIAS<"deviceMIDIWDMStream">;    
   void CDeviceMIDIWMD(void);    
    
   virtual EDeviceError open(TFrameInfo *info);
   virtual EDeviceError close(void);
   virtual bool frame(CFrame *frame, EStreamMode mode); 
};

/* ASIO sync, kludgy */
CDeviceMIDIWDMStream *audio_sync_midi_stream;
#if 0
void ASIO_buffer_switch_notify(void) {
   CDeviceMIDIWDMStream *this = audio_sync_midi_stream;
   static int last = 0;
   LARGE_INTEGER tick, freq;
    
   if (this) {
       QueryPerformanceCounter(&tick);
       QueryPerformanceFrequency(&freq);
       this->audio_frame_time = tick.u.LowPart / (freq.u.LowPart / this->sampling_rate);

//      ASIO_midi_time.wType = TIME_TICKS;       
//      midiStreamPosition(this->outHandle, &ASIO_midi_time, sizeof(ASIO_midi_time));       
//      printf("asio buffer switch (MIDI %d, %d, %d)\n", ASIO_midi_time.u.ticks, ASIO_midi_time.u.ticks - last, 
//             ASIO_midi_time.u.ticks % 2048);
//      last = ASIO_midi_time.u.ticks;
   }
}/*ASIO_buffer_switch_notify*/
#endif

void CALLBACK CDeviceMIDIWDMStream_Win32_midiCallback(HMIDIOUT handle, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2) {
   CDeviceMIDIWDMStream *this = CDeviceMIDIWDMStream((void *)dwInstance);
   LARGE_INTEGER tick, freq;              
    
   ASIO_midi_time.wType = TIME_TICKS;

   switch (uMsg) {
#if 0
   case MOM_OPEN:
      printf("callback - open\n");
      break;
   case MOM_CLOSE:
      printf("callback - close\n");
      break;
   case MOM_DONE:
      printf("callback - done\n");
      break;
#endif      
   /* Got some event with its MEVT_F_CALLBACK flag set */
   case MOM_POSITIONCB:
      if (this->state == st_calib) {
         QueryPerformanceCounter(&tick);
         QueryPerformanceFrequency(&freq);
//         printf("midi event (%d ms)\n", tick.u.LowPart / (freq.u.LowPart / 1000));
         this->audio_event_calib_time = tick.u.LowPart / (freq.u.LowPart / this->sampling_rate);          
         this->audio_event_calib_time -= 1024;
          
         midiStreamPosition(this->outHandle, &ASIO_midi_time, sizeof(ASIO_midi_time));
         this->callback_time_last = ASIO_midi_time.u.ticks;
//         printf("MIDI call %d %d\n", ASIO_midi_time.u.ticks, this->calib_event_time);

         this->state = st_trim;
      }
      break;
   }

//   TMidiEventContainer event;
    
//   switch (uMsg) {
//   case MIM_DATA:
//      CLEAR(&event);
//      TMidiEventContainer_new(&event, 
//                              (byte)(dwParam1 & 0xFF), (byte)((dwParam1 >> 8) & 0xFF), (byte)((dwParam1 >> 16) & 0xFF),
//                              dwParam2);
//      ARRAY(&this->input_event).item_add(event);
//      break;
//   }
}/*CDeviceMIDIWDMStream_Win32_midiCallback*/

int CDeviceMIDIWDMStream::midi_device_resolve(EStreamMode mode, const char *device_name) {
   int numdevs, i;
   MIDIINCAPS mi_caps;
   MIDIOUTCAPS mo_caps;    

   switch (mode) {
   case EStreamMode.input:
      numdevs = midiInGetNumDevs();
      for (i = 0; i < numdevs; i++) {
         if (!midiInGetDevCaps(i, &mi_caps, sizeof(MIDIINCAPS))) {
            if (strcmp(mi_caps.szPname, device_name) == 0) {
               return i;
            }
         }
      }          
      break;
   case EStreamMode.output:
      numdevs = midiOutGetNumDevs();
      for (i = 0; i < numdevs; i++) {
         if (!midiOutGetDevCaps(i, &mo_caps, sizeof(MIDIOUTCAPS))) {
             if (strcmp(mo_caps.szPname, device_name) == 0) {
               return i;
            }
         }
      }          
      break;
   }
   
   return -1;
}/*CDeviceMIDIWDMStream::midi_device_resolve*/

EDeviceError CDeviceMIDIWDMStream::open(TFrameInfo *info) {
//static unsigned long initEvent[] = {240, 0, 0x007F3C90, /* A note-on */
//                           60, 0, 0x007F3F90}; /* A note-off. It's the last event in the array */   
static unsigned long initEvent[] = {0, 0, (MEVT_TEMPO << 24) | 100000}; /* Tempo Set */

   int result, device_id;
   MIDIPROPTIMEDIV timediv;
   static MIDIHDR midiHdr;   

   this->frame_length = info->frame_length;
   this->sampling_rate = info->sampling_rate;
printf("midi stream open %d, %d\n", info->sampling_rate, info->frame_length);
   initEvent[2] = (MEVT_TEMPO << 24) | 100000; 

   switch (CDeviceHardware(this)->mode) {
   case EStreamMode.output:
      /*look up device index*/
      device_id = CDeviceMIDIWDMStream(this).midi_device_resolve(CDeviceHardware(this)->mode, 
                                                                  CString(&CDeviceHardware(this)->device).string());
      if (device_id == -1) {
         return EDeviceError.notFound;
      }
      else {
         result = midiStreamOpen(&this->outHandle, &device_id, 1, (DWORD)CDeviceMIDIWDMStream_Win32_midiCallback, (DWORD)this, CALLBACK_FUNCTION);
//         result = midiStreamOpen(&this->outHandle, &device_id, 1, 0, 0, CALLBACK_NULL);
         timediv.cbStruct = sizeof(timediv);
         timediv.dwTimeDiv = info->sampling_rate / 10;
         midiStreamProperty(this->outHandle, (void *)&timediv, MIDIPROP_SET|MIDIPROP_TIMEDIV);
         
         midiHdr.lpData = (LPBYTE)&initEvent[0];
         /* Store its size in the MIDIHDR */
         midiHdr.dwBufferLength = midiHdr.dwBytesRecorded = sizeof(initEvent);
         /* Flags must be set to 0 */
         midiHdr.dwFlags = 0;
         /* Prepare the buffer and MIDIHDR */
         midiOutPrepareHeader((HMIDIOUT)this->outHandle,  &midiHdr, sizeof(MIDIHDR));
         midiStreamOut(this->outHandle, &midiHdr, sizeof(MIDIHDR));         
         midiStreamRestart(this->outHandle); 
         
         if (!result == 0) {
            return EDeviceError.deviceError;
         }
      }
      break;
   }

   audio_sync_midi_stream = this;
   return EDeviceError.noError;
}/*CDeviceMIDIWDMStream::open*/

EDeviceError CDeviceMIDIWDMStream::close(void) {
   switch (CDeviceHardware(this)->mode) {
   case EStreamMode.output:
      midiStreamClose(this->outHandle);
      break;
   }
    
   return EDeviceError.noError;    
}/*CDeviceMIDIWDMStream::close*/

bool CDeviceMIDIWDMStream::frame(CFrame *frame, EStreamMode mode) {
   int i, j, evt;
   ARRAY<byte> outdata;
   ulong longoutdata;   
   int result;
   long tempo;
   static int count = 0;
   int trim_time = 0, trim_space;
   static int avg_index = 0, avg_count = 0;
    
   if (mode == EStreamMode.output && CDeviceHardware(this)->mode == EStreamMode.output) {
      ARRAY(&outdata).new();
      CLEAR(&this->m_buf[this->m_buf_n].midiHdr);
      evt = 0;
#if 1
      switch (this->state) {
      case st_idle:
//          /* for testing, delay inital playback */
//printf("initial frame\n");             
//         this->m_buf[this->m_buf_n].mevent[0 + (evt * 3)] = (this->frame_length  * 2) + 1024;
//         this->m_buf[this->m_buf_n].mevent[1 + (evt * 3)] = 0;
//         this->m_buf[this->m_buf_n].mevent[2 + (evt * 3)] = MEVT_NOP;//0x007F4290;
//         this->m_buf[this->m_buf_n].midiHdr.dwBufferLength  += (3 * sizeof(long));
//         this->m_buf[this->m_buf_n].midiHdr.dwBytesRecorded += (3 * sizeof(long));
//         evt++;
      case st_resync:
      case st_run:
         ASIO_midi_time.wType = TIME_TICKS;      
         midiStreamPosition(this->outHandle, &ASIO_midi_time, sizeof(ASIO_midi_time));
         this->calib_event_time = ASIO_midi_time.u.ticks;

         /* time NOP event calibration */
         this->m_buf[this->m_buf_n].mevent[0 + (evt * 3)] = 0;
         this->m_buf[this->m_buf_n].mevent[1 + (evt * 3)] = 0;
         this->m_buf[this->m_buf_n].mevent[2 + (evt * 3)] = (MEVT_NOP << 24) | MEVT_F_CALLBACK;
         this->m_buf[this->m_buf_n].midiHdr.dwBufferLength  += (3 * sizeof(long));
         this->m_buf[this->m_buf_n].midiHdr.dwBytesRecorded += (3 * sizeof(long));
         evt++;
         this->audio_frame_calib_time = this->audio_frame_time;
         this->state = st_calib;
         break;
      case st_calib:
         break;
      case st_trim:
//         if (this->callback_time_last - this->calib_event_time == 0) {
//printf("trim frame\n");             
//            this->m_buf[this->m_buf_n].mevent[0 + (evt * 3)] = this->frame_length * 4;
//            this->m_buf[this->m_buf_n].mevent[1 + (evt * 3)] = 0;
//            this->m_buf[this->m_buf_n].mevent[2 + (evt * 3)] = MEVT_NOP;//0x007F4290;
//            this->m_buf[this->m_buf_n].midiHdr.dwBufferLength  += (3 * sizeof(long));
//            this->m_buf[this->m_buf_n].midiHdr.dwBytesRecorded += (3 * sizeof(long));
//            evt++;
//         }
      
         trim_time = this->frame_length - (this->audio_event_calib_time - this->audio_frame_calib_time);
//printf("trim time %d\n", trim_time);
         this->state = st_run;
         break;
      }
#endif
       
#if 0
      tempo = 500000;
      /* Set tempo for next frame based on current drift error */
      this->m_buf[this->m_buf_n].mevent[(evt * 3) + 0] = 0;
      this->m_buf[this->m_buf_n].mevent[(evt * 3) + 1] = 0;
      this->m_buf[this->m_buf_n].mevent[(evt * 3) + 2] = (MEVT_TEMPO << 24) | tempo;
      this->m_buf[this->m_buf_n].midiHdr.dwBufferLength  += (3 * sizeof(long));
      this->m_buf[this->m_buf_n].midiHdr.dwBytesRecorded += (3 * sizeof(long));
      evt++;      
#endif

#if 1
      this->time_last = 0;
      for (i = 0; i < ARRAY(&frame->midi).count(); i++) {
         if (TMidiEventContainer_encode(&ARRAY(&frame->midi).data()[i], &outdata, NULL)) {
             longoutdata = 0;
             for (j = 0; j < ARRAY(&outdata).count(); j++) {
                longoutdata |= ARRAY(&outdata).data()[j] << (j * 8); 
             }

            if (CMidiEvent(&ARRAY(&frame->midi).data()[i])->time == -1) {
                /* untimed event, output immediately */
                midiOutShortMsg((HMIDIOUT)this->outHandle, longoutdata);
            }
            else {
                this->m_buf[this->m_buf_n].mevent[(evt * 3)] = (CMidiEvent(&ARRAY(&frame->midi).data()[i])->time - this->time_last);
                this->m_buf[this->m_buf_n].mevent[(evt * 3) + 1] = 0;
                this->m_buf[this->m_buf_n].mevent[(evt * 3) + 2] = longoutdata;
                this->time_last = CMidiEvent(&ARRAY(&frame->midi).data()[i])->time;
                this->m_buf[this->m_buf_n].midiHdr.dwBufferLength  += (3 * sizeof(long));
                this->m_buf[this->m_buf_n].midiHdr.dwBytesRecorded += (3 * sizeof(long));
                evt++;
            }
         }
      }
#endif

#if 1
//    trim_space = this->frame_length - this->time_last;
      trim_space = this->frame_length - this->time_last + trim_time;
//printf("trim time %d\n", trim_space);      
      if (trim_space < 0)
          trim_space = 0;
      
      /* NOP event fills blank space at end of buffer */
      this->m_buf[this->m_buf_n].mevent[(evt * 3) + 0] = trim_space;
      this->m_buf[this->m_buf_n].mevent[(evt * 3) + 1] = 0;
      this->m_buf[this->m_buf_n].mevent[(evt * 3) + 2] = (MEVT_NOP << 24);
      this->m_buf[this->m_buf_n].midiHdr.dwBufferLength  += (3 * sizeof(long));
      this->m_buf[this->m_buf_n].midiHdr.dwBytesRecorded += (3 * sizeof(long));
      evt++;
#endif
      if (this->m_buf[this->m_buf_n].midiHdr.dwBufferLength != 0) {
         count++;
         /* Prepare the buffer and MIDIHDR */
         this->m_buf[this->m_buf_n].midiHdr.lpData = (LPBYTE)this->m_buf[this->m_buf_n].mevent;
         this->m_buf[this->m_buf_n].midiHdr.dwFlags = 0;      
         result = midiOutPrepareHeader((HMIDIOUT)this->outHandle,  &this->m_buf[this->m_buf_n].midiHdr, sizeof(MIDIHDR));
         result = midiStreamOut(this->outHandle, &this->m_buf[this->m_buf_n].midiHdr, sizeof(MIDIHDR));
         this->m_buf_n = ((this->m_buf_n + 1) % 8);
     }

      
      ARRAY(&outdata).delete();      
   }
   
   return TRUE;
}/*CDeviceMIDIWDMStream::frame*/

/*==========================================================================*/
MODULE::END/*===============================================================*/
/*==========================================================================*/
