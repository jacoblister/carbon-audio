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

ENUM:typedef<EMidiFileChunk> {
   {header},
   {track},
};

struct tag_CXMidiSequence;

class CMidiFile : CObject {
 private:
   CFile file;
   struct tag_CXMidiSequence *sequence;

   bool read_chunk(EMidiFileChunk *type, ARRAY<byte> *data);
   bool process_chunk(EMidiFileChunk type, ARRAY<byte> *data);
 public:
   void CMidiFile(void);

   bool file_import(const char *filename, struct tag_CXMidiSequence *sequence);
//   bool file_export(const char *filename, CXMidiSequence *sequence);
};

/*==========================================================================*/
MODULE::IMPLEMENTATION/*====================================================*/
/*==========================================================================*/

/*>>>for htonl, should be something else we can include for this*/
#if OS_Win32
#include <winsock2.h>
#else
extern unsigned long int ntohl(unsigned long int netlong);
extern unsigned short int ntohs(unsigned short int netshort);
#endif

void CMidiFile::CMidiFile(void) {
}/*CMidiFile::CMidiFile*/

bool CMidiFile::read_chunk(EMidiFileChunk *type, ARRAY<byte> *data) {
   char chunk_type[4];
   long chunk_length = 0;
   int bytes_read = 0, offset = 0;

   CIOObject(&this->file).read(chunk_type, 4);
   if (strncmp(chunk_type, "MThd", 4) == 0) {
      *type = EMidiFileChunk.header;
   }
   else if (strncmp(chunk_type, "MTrk", 4) == 0) {
      *type = EMidiFileChunk.track;
   }
   else
      return FALSE;

   CIOObject(&this->file).read(&chunk_length, 4);
   chunk_length = ntohl(chunk_length);

   ARRAY(data).used_set(chunk_length);
   while (chunk_length > 0) {
      bytes_read = CIOObject(&this->file).read(ARRAY(data).data() + offset, chunk_length);
      offset       += bytes_read;
      chunk_length -= bytes_read;
   }

   return TRUE;
}/*CMidiFile::read_chunk*/

bool CMidiFile::process_chunk(EMidiFileChunk type, ARRAY<byte> *data) {
   CXMidiHeader *header;
   CXMidiTrack *track;
   short hr_type, hr_tracks, hr_division;
   byte *pointer;
   long absolute_time, read_delta_time;
   byte read_time;
   long tempo;
   byte midi_event_data[3];

   byte status;
   byte expected;
   TMidiEventContainer *event;

//   printf("process chunk size - %d\n", ARRAY(data).count());

   pointer = ARRAY(data).data();
   switch (type) {
   case EMidiFileChunk.header:
      hr_type = *(short *)pointer;
      hr_type = ntohs(hr_type);
      hr_tracks = *(short *)&pointer[2];
      hr_tracks = ntohs(hr_tracks);
      hr_division = *(short *)&pointer[4];
      hr_division = ntohs(hr_division);

      header = &this->sequence->header;
      header->type = hr_type;
      header->tracks = hr_tracks;
      header->division = hr_division;
//      header = new.CXMidiHeader(hr_type, hr_tracks, hr_division);
//      CObject(this->sequence).child_add(CObject(header));
      break;
   case EMidiFileChunk.track:
      absolute_time = 0;
      track = new.CXMidiTrack();
      CObject(this->sequence).child_add(CObject(track));

      while (pointer < ARRAY(data).data() + ARRAY(data).count()) {
         /* read delta time */
         read_delta_time = *pointer;
         pointer++;
         if (read_delta_time & 0x80) {
            read_delta_time &= 0x7F;
            do {
               read_delta_time = (read_delta_time << 7) + ((read_time = *pointer) & 0x7F);
               pointer++;
            } while (read_time & 0x80);
         }
//printf("read delta time %d\n", read_delta_time);
         absolute_time += read_delta_time;

         /*>>>duplicated code for MIDI stream decoder, put somewhere better */
         if (*pointer & 0x80) {                   /* if status byte */
	         status = *pointer;
             /* New event, get expected data length */
            switch (MIDI_TYPE(*pointer)) {
            case ME_System:
               if (*pointer == 0xFF) {
                  if (pointer[2] & 0x80) {
                     ASSERT("CMidiFile::process_chunk - large meta event");
                  }

                  switch (pointer[1]) {
                  case MM_TrackName:
                     CString(&track->trackName).setn((char *)&pointer[3], pointer[2]);
                     break;
                  case MM_Instrument:
                     CString(&track->instrument).setn((char *)&pointer[3], pointer[2]);
                     break;
                  case MM_Marker:
                     printf("marker tick %ld\n", absolute_time);
                     break;
                  case MM_Tempo:
                     tempo = (pointer[3] << 16) | (pointer[4] << 8) | pointer[5];
//                     printf("tempo (%ld) tick %ld\n", tempo, absolute_time);
                     event = (TMidiEventContainer *)new.CMMETempo(absolute_time, tempo);
                     CObjPersistent(event).attribute_default_set(ATTRIBUTE<CMidiEvent,channel>, TRUE);
                     CObject(&this->sequence->metatrack).child_add(CObject(event));
                     break;
                  case MM_TimeSignature:
                     event = (TMidiEventContainer *)new.CMMETime(absolute_time, pointer[3], pow(2,pointer[4]));
                     CObjPersistent(event).attribute_default_set(ATTRIBUTE<CMidiEvent,channel>, TRUE);
                     CObject(&this->sequence->metatrack).child_add(CObject(event));
                     break;
                  case MM_KeySignature:
                     printf("key signature tick %ld\n", absolute_time);
                     break;
                  }
                  pointer += pointer[2] + 3;
               }
               else {
                  /* system exclusive, skip data */
//                  printf("sysex\n");
                  while (*pointer != 0xF7) {
                     /*>>>watch we don't go off the end off array*/
                     pointer++;
                  }
                  pointer++;
               }
               continue;
            case ME_Note:
            case ME_NoteOff:
            case ME_Control:
            case ME_PitchWheel:
               expected = 2;
               break;
            case ME_Program:
            case ME_Presure:
               expected = 1;
               break;
            default:
               expected = 0;
            }
            pointer++;
         }

         /*>>>should use a better allocation scheme for events */
         event = malloc(sizeof(TMidiEventContainer));
         CLEAR(event);
		 midi_event_data[0] = status;
		 midi_event_data[1] = pointer[0];
		 midi_event_data[2] = pointer[1];
         TMidiEventContainer_new(event, absolute_time, 3, midi_event_data);		 
         pointer += expected;

         track->channel = CMidiEvent(event)->channel;
         CObjPersistent(event).attribute_default_set(ATTRIBUTE<CMidiEvent,channel>, TRUE);
         CObject(track).child_add(CObject(event));
      }

      break;
   }
   return TRUE;
}/*CMidiFile::process_chunk*/

bool CMidiFile::file_import(const char *filename, CXMidiSequence *sequence) {
   ARRAY<byte> chunk;
   EMidiFileChunk type;

   this->sequence = sequence;

   ARRAY(&chunk).new();

   new(&this->file).CFile();
   CIOObject(&this->file).open(filename, O_RDONLY | O_BINARY);
   while (CMidiFile(this).read_chunk(&type, &chunk)) {
      CMidiFile(this).process_chunk(type, &chunk);
   }
   CIOObject(&this->file).close();
   delete(&this->file);

   ARRAY(&chunk).delete();

   return FALSE;
}/*CMidiFile::import*/

/*==========================================================================*/
MODULE::END/*===============================================================*/
/*==========================================================================*/
