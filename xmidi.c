/*==========================================================================*/
MODULE::IMPORT/*============================================================*/
/*==========================================================================*/

#include "framework.c"

/*==========================================================================*/
MODULE::INTERFACE/*=========================================================*/
/*==========================================================================*/

#define IFACE_INPUT_INTERNAL       0
#define IFACE_INPUT_CONTROL        1
#define IFACE_INPUT_INSTRUMENT     2
#define IFACE_INPUT_MAX            2

#define IFACE_OUTPUT_INTERNAL      0
#define IFACE_OUTPUT_MAIN          1
#define IFACE_OUTPUT_ACTIVE        2
#define IFACE_OUTPUT_MAX           2

#define IFACE_OUTPUT_FILTER_ALL    0xFF

/* Based on MIDI Manufacturers Association MIDI Events DTD, see
   "http://www.midi.org/dtds/MIDIEvents10.dtd"
*/

class CXMidiHeader : CObjPersistent {
 private:
   void new(void);
   void delete(void);
 public:
   ALIAS<"header">;
   ATTRIBUTE<int type>;
   ATTRIBUTE<int tracks>;
   ATTRIBUTE<int division> {
      if (data) {
         this->division = *data;
      }
      else {
         this->division = 240;
      }
   };
   ATTRIBUTE<CString sampledir>;

   void CXMidiHeader(int type, int tracks, int division); 
};

class CXMidiPatterns : CObjPersistent {
 public:
   ALIAS<"patterns">;

   void CXMidiPatterns(void); 
};

ENUM:typedef<EMidiTrackMode> {
   {normal},
   {metro},
};

class CXMidiTrack : CObjPersistent {
 private:
   void new(void);
   void delete(void); 
 public:
   ALIAS<"track">;

   ELEMENT:OBJECT<CXMidiPatterns patterns, "patterns">; 
   ATTRIBUTE<CString trackName>;
   ATTRIBUTE<CString instrument>; 
   ATTRIBUTE<int channel>;
   ATTRIBUTE<bool record_enable, "recEnable">;
   ATTRIBUTE<EMidiTrackMode mode> {
      this->mode = data ? *data : EMidiTrackMode.normal;
   };
   void CXMidiTrack(void); 
};

class CXMidiMetaTrack : CXMidiTrack {
 public:
   ALIAS<"metatrack">;
   void CXMidiMetaTrack(void); 
};

class CXMidiSequence : CObjPersistent {
 public:
   ALIAS<"xmidi">;
   ELEMENT:OBJECT<CXMidiHeader header, "header">;
   ELEMENT:OBJECT<CXMidiMetaTrack metatrack, "metatrack">;
//   ELEMENT:OBJECT<CXMidiPatterns patterns, "patterns">;
 
   void CXMidiSequence(void);
};

class CMidiEvent : CObjPersistent {
 private:
   int output_filter;     
   int interface;
//   void new(void);
 public:
   void CMidiEvent(int channel, int time);
 
   ATTRIBUTE<int channel, "channel", PO_INHERIT>;
   ATTRIBUTE<int time, "time">;
};

class CMidiMetaEvent : CMidiEvent {
   void CMidiMetaEvent(int time);
};

class CMMETempo : CMidiMetaEvent {
 private:
   double tempo;
 public:
   void CMMETempo(int time, int beat_length);

   ALIAS<"tempo">;
   ATTRIBUTE<int beat_length, "beatLength">;
};

class CMMETime : CMidiMetaEvent {
 public:
   void CMMETime(int time, int numerator, int denominator);

   ALIAS<"timeSig">;
   ATTRIBUTE<int numerator>;
   ATTRIBUTE<int denominator>;
};

ARRAY:typedef<CMidiEvent>;
ARRAY:typedef<CMidiEvent *>;

/*>>>index numbers wrong! */

/* hard coded controller bindings for now */
#define MIDI_CONTROL_LEVEL        7
#define MIDI_CONTROL_PAN          10
#define MIDI_CONTROL_BASS         80
#define MIDI_CONTROL_TREBLE       81
#define MIDI_CONTROL_OUT_AUX1     82
#define MIDI_CONTROL_OUT_AUX2     83
#define MIDI_CONTROL_OUT_STEREO   84
#define MIDI_CONTROL_OUT_MONITOR  85
#define MIDI_CONTROL_OUT_PA1      86
#define MIDI_CONTROL_OUT_PA2      87
#define MIDI_CONTROL_ACTIVE_A     100
#define MIDI_CONTROL_ACTIVE_B     101
#define MIDI_CONTROL_REVERB       91
#define MIDI_CONTROL_CHORUS       93
#define MIDI_CONTROL_ALLNOTESOFF  123

ENUM:typedef<EMidiController> {
   {BankSelect},
   {ModulationWheel},
   {BreathControl},
   {FootController},
   {PortamentoTime},
   {DataEntry},
   {ChannelVolume},
   {Balance},
   {Pan},
   {ExpressionController},
   {EffectControl_1},
   {EffectControl_2},
   {GeneralPurpose_1},
   {GeneralPurpose_2},
   {GeneralPurpose_3},
   {GeneralPurpose_4},
   {SustainPedal},
   {Portamento},
   {Sustenuto},
   {SoftPedal},
   {LegatoFootswitch},
   {Hold_2},
   {SC_1_SoundVariation},
   {SC_2_Timbre},
   {SC_3_ReleaseTime},
   {SC_4_AttackTime},
   {SC_5_Brightness},
   {SC_6},
   {SC_7},
   {SC_8},
   {SC_9},
   {SC_10},
   {EffectsDepth},                /*reverb*/
   {EffectTremeloDepth},
   {EffectChorusDepth},
   {EffectCelesteDepth},          /*detune*/
   {EffectPhaserDepth},
   {AllSoundOff},
   {ResetAllControllers},
   {LocalControl},
   {AllNotesOff},
   {OmniModeOff},
   {OmniModeOn},
   {PolyModeOnOff},
   {PolyModeOn},
};

class CMENote : CMidiEvent {
 public:
   void CMENote(int channel, int time, int note, int velocity);
 
   ALIAS<"note">;
 
   ATTRIBUTE<int note, "note">;
   ATTRIBUTE<int velocity, "velocity">;
};

class CMENoteOff : CMidiEvent {
 public:
   void CMENoteOff(int channel, int time, int note);
 
   ALIAS<"noteOff">;
 
   ATTRIBUTE<int note, "note">;
};

class CMEPolyKeyPressure : CMidiEvent {
 public:
   void CMEPolyKeyPressure(void);
 
   ALIAS<"polyKeyPressure">;
 
   ATTRIBUTE<int note, "note">;
   ATTRIBUTE<int presssure, "pressure">;
};

class CMEControlChange : CMidiEvent {
 public:
   void CMEControlChange(int channel, int time, int control, int value);
 
   ALIAS<"controlChange">;
 
   ATTRIBUTE<int control, "control">;
   ATTRIBUTE<int value, "value">;
};

class CMEProgramChange : CMidiEvent {
 public:
   void CMEProgramChange(int channel, int time, int number);
 
   ALIAS<"programChange">;
 
   ATTRIBUTE<int number, "number">;
};

class CMEChannelKeyPressure : CMidiEvent {
 public:
   void CMEChannelKeyPressure(void);
 
   ALIAS<"channelKeyPressure">;
 
   ATTRIBUTE<int pressure, "pressure">;
};

class CMEPitchBendChange : CMidiEvent {
 public:
   void CMEPitchBendChange(int channel, int time, int value);
 
   ALIAS<"pitchBendChange">;
 
   ATTRIBUTE<int value, "value">;
};

/* Channel Mode Messages */

class CMEAllSoundOff : CMidiEvent {
 public:
   void CMEAllSoundOff(void);
 
   ALIAS<"allSoundOff">;
};

class CMEResetAllControllers : CMidiEvent {
 public:
   void CMEResetAllControllers(void);
 
   ALIAS<"resetAllControllers">;
};

class CMELocalControl : CMidiEvent {
 public:
   void CMELocalControl(void);
 
   ALIAS<"localControl">;
 
   ATTRIBUTE<bool value, "value">;
};

class CMEAllNotesOff : CMidiEvent {
 public:
   void CMEAllNotesOff(void);
 
   ALIAS<"allNotesOff">;
};

class CMEOmniOff : CMidiEvent {
 public:
   void CMEOmniOff(void);
 
   ALIAS<"omniOff">;
};

class CMEOmniOn : CMidiEvent {
 public:
   void CMEOmniOn(void);
 
   ALIAS<"omniOn">;
};

class CMEMonoMode : CMidiEvent {
 public:
   void CMEMonoMode(void);
 
   ALIAS<"monoMode">;
};

class CMEPolyMode : CMidiEvent {
 public:
   void CMEPolyMode(void);
 
   ALIAS<"polyMode">;
};

/* 14 bit Control Changes */

class CMEControlChange14 : CMidiEvent {
 public:
   void CMEControlChange14(void);
 
   ALIAS<"polyMode">;
 
   ATTRIBUTE<int control, "control">;
   ATTRIBUTE<int value, "value">;
};

class CMERPNChange : CMidiEvent {
 public:
   void CMERPNChange(void);
 
   ALIAS<"RPNChange">;
 
   ATTRIBUTE<int rpn, "RPN">;
   ATTRIBUTE<int value, "value">;
};

class CMENRPNChange : CMidiEvent {
 public:
   void CMENRPNChange(void);
 
   ALIAS<"NRPNChange">;
 
   ATTRIBUTE<int rpn, "NRPN">;
   ATTRIBUTE<int value, "value">;
};

/* System Messenges  */

class CMESysEx : CMidiEvent {
 public:
   void CMESysEx(int time, int count, byte *data);
 
   ALIAS<"sysEx">;
   int data_count;
   byte data[16];
};

class CMESysExDeviceID: CMidiEvent {
 public:
   void CMESysExDeviceID(void);
 
   ALIAS<"sysExDeviceID">;
 
   ATTRIBUTE<int multiplier, "multiplier">;
   ATTRIBUTE<int offset, "offset">;
};

class CMESysExChannel : CMidiEvent {
 public:
   void CMESysExChannel(void);
 
   ALIAS<"sysExChannel">;
 
   ATTRIBUTE<int multiplier, "multiplier">;
   ATTRIBUTE<int offset, "offset">;
};

ENUM:typedef<EMEMTCQuarterFrame> {
   {FrameLSNibble},
   {FrameMSNibble},
   {SecsLSNibble},
   {SecsMSNibble},
   {MinsLSNibble},
   {MinsMSNibble},
   {HrsLSNibble},
   {HrsMSNibbleSMPTEType},
};

class CMEMTCQuarterFrame : CMidiEvent {
 public:
   void CMEMTCQuarterFrame(void);
 
   ALIAS<"MTCQuarterFrame">;

   ATTRIBUTE<EMEMTCQuarterFrame message_type, "messageType">;
   ATTRIBUTE<int data_nibble, "dataNibble">;
};

class CMESongPositionPointer : CMidiEvent {
 public:
   void CMESongPositionPointer(void);
 
   ALIAS<"songPositionPointer">;
 
   ATTRIBUTE<int position, "position">;
};

class CMESongSelect : CMidiEvent {
 public:
   void CMESongSelect(int number, int time);
 
   ALIAS<"songSelect">;
 
   ATTRIBUTE<int number, "number">;
};

class CMETuneRequest : CMidiEvent {
 public:
   void CMETuneRequest(void);
 
   ALIAS<"tuneRequest">;
};

class CMETimingClock : CMidiEvent {
 public:
   void CMETimingClock(void);
 
   ALIAS<"timingClock">;
};

class CMEInit : CMidiEvent {
 public:
   void CMEInit(void);
 
   ALIAS<"init">;
};

class CMEStart : CMidiEvent {
 private:
   bool recording;
 public:
   void CMEStart(int time, bool recording);
 
   ALIAS<"start">;
};

class CMEContinue : CMidiEvent {
 public:
   void CMEContinue(void);
 
   ALIAS<"continue">;
};

class CMEStop : CMidiEvent {
 public:
   void CMEStop(int time);
 
   ALIAS<"stop">;
};

class CMEActiveSensing : CMidiEvent {
 public:
   void CMEActiveSensing(void);
 
   ALIAS<"activeSensing">;
};

class CMESystemRequest : CMidiEvent {
 public:
   void CMESystemRequest(void);
 
   ALIAS<"systemRequest">;
};

typedef struct {
   byte data[sizeof(CMESysEx)];
} TMidiEventContainer;

void TMidiEventContainer_new(TMidiEventContainer *this, int timestamp, int count, byte *data);
bool TMidiEventContainer_encode_append(TMidiEventContainer *this, ARRAY<byte> *data, int *timestamp);

ARRAY:typedef<TMidiEventContainer>;

/* >>> Enum::typedef Entities for these*/

typedef enum {
   ME_NoteOff    = 8,
   ME_Note       = 9,
   ME_AfterTouch = 10,
   ME_Control    = 11,
   ME_Program    = 12,
   ME_Presure    = 13,
   ME_PitchWheel = 14,
   ME_System     = 15,
} TMidiEventType;

typedef enum {
   MM_SeqNo   = 0x00,
   MM_Text    = 0x01,
   MM_Copyright = 0x02,
   MM_TrackName = 0x03,
   MM_Instrument = 0x04,
   MM_Lyric = 0x05,
   MM_Marker = 0x06,
   MM_CuePoint = 0x07,
   MM_ProgramName = 0x08,
   MM_PortName = 0x09,
   MM_EndOfTrack = 0x2F,
   MM_Tempo = 0x51,
   MM_SMPTEOffset = 0x54,
   MM_TimeSignature = 0x58,
   MM_KeySignature = 0x59,
   MM_ProprietaryEvent = 0x7F,
} TMidiMetaType;

#define MIDI_CHANNEL(b)   (b & 0x0F)
#define MIDI_TYPE(b)      (b >> 4)
#define MIDI_STATUS(t, c) (((t) << 4) | (c))

/* Extra */
ENUM:typedef<EMESampleType> {
   {none},
   {record},
   {play},
   {playHalf},
   {playReverse},
   {stop},   
   {stopPlay},
   {stopRecord},
   {selectA},
   {selectB},   
};

class CMESample : CMidiEvent {
 private:
   void new(void);
 public:
   ALIAS<"sample">;    
   ATTRIBUTE<int bank>;
   ATTRIBUTE<EMESampleType type>;
 
   void CMESample(int channel, int time, EMESampleType type); 
};

class CXMidiPattern : CObjPersistent {
 private:
   void new(void);
   void delete(void);
 public:
   ALIAS<"pattern">;
   ATTRIBUTE<int channel, "channel", PO_INHERIT>; 
   ATTRIBUTE<CString id>;
   ATTRIBUTE<int length>;

   void CXMidiPattern(void); 
};

class CMEUsePattern : CMidiEvent {
 private:
   void new(void);
   void delete(void);
 
   void notify_attribute_update(ARRAY<const TAttribute *> *attribute, bool changing);
   void resolve(void);
 public:
   void CMEUsePattern(int channel, int time, CXMidiPattern *usepattern);
 
   ALIAS<"usepattern">;
   OPTION<nochild>; 
 
   ATTRIBUTE<CXLink link, "xlink:href">;
};

/*==========================================================================*/
MODULE::IMPLEMENTATION/*====================================================*/
/*==========================================================================*/

void CXMidiHeader::new(void) {
    new(&this->sampledir).CString(NULL);
}/*CXMidiHeader::new*/

void CXMidiHeader::CXMidiHeader(int type, int tracks, int division) {
   this->type = type;
   this->tracks = tracks;
   this->division = division;
}/*CXMidiHeader::CXMidiHeader*/

void CXMidiHeader::delete(void) {
    delete(&this->sampledir);
}/*CXMidiHeader::delete*/

void CXMidiSequence::CXMidiSequence(void) {
   new(&this->header).CXMidiHeader(0, 0, 0);
   CObject(&this->header).parent_set(CObject(this));
   new(&this->metatrack).CXMidiMetaTrack();
   CObject(&this->metatrack).parent_set(CObject(this));    
//   new(&this->patterns).CXMidiPatterns();
//   CObject(&this->patterns).parent_set(CObject(this));    
}/*CXMidiSequence::CXMidiSequence*/

void CXMidiTrack::new(void) {
   new(&this->trackName).CString(NULL);
   new(&this->instrument).CString(NULL);    
   new(&this->patterns).CXMidiPatterns();
   CObject(&this->patterns).parent_set(CObject(this));
   CObjPersistent(this).attribute_default_set(ATTRIBUTE<CXMidiTrack,patterns>, TRUE);    
    
   CObjPersistent(this).attribute_default_set(ATTRIBUTE<CXMidiTrack,record_enable>, TRUE);
   this->record_enable = TRUE;    
}/*CXMidiTrack::new*/

void CXMidiTrack::CXMidiTrack(void) {
}/*CXMidiTrack::CXMidiTrack*/

void CXMidiTrack::delete(void) {
   delete(&this->patterns);
   delete(&this->instrument);
   delete(&this->trackName);
   }/*CXMidiTrack::delete*/
    
void CXMidiMetaTrack::CXMidiMetaTrack(void) {
}/*CXMidiMetaTrack::CXMidiMetaTrack*/

void CXMidiPatterns::CXMidiPatterns(void) {
}/*CXMidiPatterns::CXMidiPatterns*/

//void CMidiEvent::new(void) {
//    /*>>>for now, just here to avoid call CObjPersistent::new(), which does dynamic memory allocation  */
//}/*CMidiEvent::new*/

void CMidiEvent::CMidiEvent(int channel, int time) {
   this->channel = channel;
   this->time = time;    
}/*CMidiEvent::CMidiEvent*/

void CMidiMetaEvent::CMidiMetaEvent(int time) {
   CMidiEvent(this).CMidiEvent(0, time);
}/*CMidiMetaEvent::CMidiMetaEvent*/

void CMMETempo::CMMETempo(int time, int beat_length) {
   this->beat_length = beat_length;

   CMidiMetaEvent(this).CMidiMetaEvent(time);
}/*CMMETempo::CMMETempo*/

void CMMETime::CMMETime(int time, int numerator, int denominator) {
   this->numerator = numerator;
   this->denominator = denominator;

   CMidiMetaEvent(this).CMidiMetaEvent(time);
}/*CMMETime::CMMETime*/

void CMENote::CMENote(int channel, int time, int note, int velocity) {
   this->note = note;
   this->velocity = velocity;
   CMidiEvent(this).CMidiEvent(channel, time);
}/*CMENote::CMENote*/

void CMENoteOff::CMENoteOff(int channel, int time, int note) {
   this->note = note;
   CMidiEvent(this).CMidiEvent(channel, time);
}/*CMENoteOff::CMENoteOff*/

void CMEControlChange::CMEControlChange(int channel, int time, int control, int value) {
   this->control = control;
   this->value = value;
   CMidiEvent(this).CMidiEvent(channel, time);
}/*CMEControlChange::CMEControlChange*/

void CMEProgramChange::CMEProgramChange(int channel, int time, int number) {
   this->number = number;
   CMidiEvent(this).CMidiEvent(channel, time);
}/*CMEProgramChange::CMEProgramChange*/

void CMEPitchBendChange::CMEPitchBendChange(int channel, int time, int value) {
   this->value = value;
   CMidiEvent(this).CMidiEvent(channel, time);
}/*CMEPitchBendChange::CMEPitchBendChange*/

void CMESongSelect::CMESongSelect(int number, int time) {
    this->number = number;
    CMidiEvent(this).CMidiEvent(0, time);
}/*CMESongSelect::CMESongSelect*/

void CMESysEx::CMESysEx(int time, int count, byte *data) {
    if (count > sizeof(this->data)) {
       count = sizeof(this->data);
    }
    this->data_count = count;
    memcpy(this->data, data, count);
    CMidiEvent(this).CMidiEvent(0, time);
    CMidiEvent(this)->channel = 16;
}/*CMESysEx::CMESysEx*/

void CMETimingClock::CMETimingClock(void) {
   CMidiEvent(this)->channel = 16;
}/*CMETimingClock::CMETimingClock*/

void CMESample::new(void) {
   CObjPersistent(this).attribute_default_set(ATTRIBUTE<CMidiEvent,channel>, TRUE); 
}/*CMESample::new*/

void CMESample::CMESample(int channel, int time, EMESampleType type) {
   this->type = type;
   CMidiEvent(this).CMidiEvent(channel, time);
}/*CMESample::CMESample*/    

void CMEInit::CMEInit(void) {
   CMidiEvent(this).CMidiEvent(0, 0);
}

void CMEStart::CMEStart(int time, bool recording) {
   CMidiEvent(this).CMidiEvent(0, time);
   this->recording = recording;
}

void CMEStop::CMEStop(int time) {
   CMidiEvent(this).CMidiEvent(0, time);
}

void TMidiEventContainer_new(TMidiEventContainer *this, int timestamp, int count, byte *data) {
   byte channel = (data[0] & 0x0F) + 1;
   byte type    = data[0] >> 4;

   switch (type) {
   case ME_Note:
      if (data[2] == 0) {
         new(this).CMENoteOff(channel, timestamp, data[1]);          
      }
      else {
         new(this).CMENote(channel, timestamp, data[1], data[2]);
      }
      break;
   case ME_NoteOff:
       new(this).CMENoteOff(channel, timestamp, data[1]);                 
       break;
   case ME_Control:
      new(this).CMEControlChange(channel, timestamp, data[1], data[2]);
      break;
   case ME_Program:
      new(this).CMEProgramChange(channel, timestamp, data[1] + 1);
      break;
   case ME_PitchWheel:
      new(this).CMEPitchBendChange(channel, timestamp, (data[2] << 7 | data[1]) - 8192);       
      break;
   case ME_System:
      new(this).CMESysEx(timestamp, count, data);
      break;
   default:
      /* remove this once all event types are supported */    
      new(this).CMidiEvent(channel, timestamp);
      break;
   }
}/*TMidiEventContainer_new*/

bool TMidiEventContainer_encode_append(TMidiEventContainer *this, ARRAY<byte> *data, int *timestamp) {
   int i = ARRAY(data).count();
   if (CObject(this).obj_class() == &class(CMEControlChange)) {
      ARRAY(data).used_set(i + 3);
      ARRAY(data).data()[i + 0] = MIDI_STATUS(ME_Control, CMidiEvent(this)->channel - 1);
      ARRAY(data).data()[i + 1] = CMEControlChange(this)->control;
      ARRAY(data).data()[i + 2] = CMEControlChange(this)->value;
   }
   else if (CObject(this).obj_class() == &class(CMEProgramChange)) {
      ARRAY(data).used_set(i + 2);
      ARRAY(data).data()[i + 0] = MIDI_STATUS(ME_Program, CMidiEvent(this)->channel - 1);
      ARRAY(data).data()[i + 1] = CMEProgramChange(this)->number - 1;
   }
   else if (CObject(this).obj_class() == &class(CMENote)) {
      ARRAY(data).used_set(i + 3);
      ARRAY(data).data()[i + 0] = MIDI_STATUS(ME_Note, CMidiEvent(this)->channel - 1);
      ARRAY(data).data()[i + 1] = CMENote(this)->note;
      ARRAY(data).data()[i + 2] = CMENote(this)->velocity;
   }
   else if (CObject(this).obj_class() == &class(CMENoteOff)) {
      ARRAY(data).used_set(i + 3);
      ARRAY(data).data()[i + 0] = MIDI_STATUS(ME_Note, CMidiEvent(this)->channel - 1);
      ARRAY(data).data()[i + 1] = CMENoteOff(this)->note;
      ARRAY(data).data()[i + 2] = 0;
   }
   else if (CObject(this).obj_class() == &class(CMEPitchBendChange)) {
      ARRAY(data).used_set(i + 3);
      ARRAY(data).data()[i + 0] = MIDI_STATUS(ME_PitchWheel, CMidiEvent(this)->channel - 1);
      ARRAY(data).data()[i + 2] = (CMEPitchBendChange(this)->value + 8192) >> 7;
      ARRAY(data).data()[i + 1] = (CMEPitchBendChange(this)->value + 8192) & 127;
   }
   if (CObject(this).obj_class() == &class(CMESysEx)) {
      ARRAY(data).used_set(i + CMESysEx(this)->data_count);
      memcpy(ARRAY(data).data(), CMESysEx(this)->data, CMESysEx(this)->data_count);
//      ARRAY(data).data()[i + 0] = MIDI_STATUS(ME_Control, CMidiEvent(this)->channel - 1);
//      ARRAY(data).data()[i + 1] = CMEControlChange(this)->control;
//      ARRAY(data).data()[i + 2] = CMEControlChange(this)->value;
   }
   else {
      return FALSE;
   }

   return TRUE;
}/*TMidiEventContainer_encode*/

/* Extra */

void CXMidiPattern::new(void) {
   new(&this->id).CString(NULL);
}/*CXMidiPattern::new*/

void CXMidiPattern::delete(void) {
   delete(&this->id);
}/*CXMidiPattern::delete*/

void CMEUsePattern::new(void) {
   CObjPersistent(this).attribute_default_set(ATTRIBUTE<CMidiEvent,channel>, TRUE); 
   new(&this->link).CXLink(NULL, NULL);
}/*CMEUsePattern::new*/

void CMEUsePattern::CMEUsePattern(int channel, int time, CXMidiPattern *usepattern) {
   CMidiEvent(this).CMidiEvent(channel, time);
}/*CMEUsePattern::CMEUsePattern*/

void CMEUsePattern::delete(void) {
   delete(&this->link);
}/*CMEUsePattern::delete*/

void CMEUsePattern::resolve(void) {
   CXMidiPattern(CXLink(&this->link).object_resolve());
}/*CMEUsePattern::resolve*/

void CMEUsePattern::notify_attribute_update(ARRAY<const TAttribute *> *attribute, bool changing) {
   if (!changing) {
      CMEUsePattern(this).resolve();
   }
}/*CMEUsePattern::notify_attribute_update*/

/*==========================================================================*/
MODULE::END/*===============================================================*/
/*==========================================================================*/
