/*==========================================================================*/
MODULE::IMPORT/*============================================================*/
/*==========================================================================*/

#include "framework.c"
#include "..\devices.c"

/*==========================================================================*/
MODULE::INTERFACE/*=========================================================*/
/*==========================================================================*/

extern void ASIO_buffer_switch_notify(void);

/*==========================================================================*/
MODULE::IMPLEMENTATION/*====================================================*/
/*==========================================================================*/

#include <windows.h>

#include "asiosys.h"
#include "asio.h"

HMIDISTRM ASIO_midi_handle;
MMTIME ASIO_midi_time;

// some external references
//bool loadAsioDriver(char *name);
void removeCurrentAsioDriver(void);


enum {
	// number of input and outputs supported by the host application
	// you can change these to higher or lower values
	kMaxInputChannels = 32,
	kMaxOutputChannels = 32
};

// internal data storage
typedef struct {
	// ASIOInit()
	ASIODriverInfo driverInfo;

	// ASIOGetChannels()
	long           inputChannels;
	long           outputChannels;

	// ASIOGetBufferSize()
	long           minSize;
	long           maxSize;
	long           preferredSize;
	long           granularity;

	// ASIOGetSampleRate()
	ASIOSampleRate sampleRate;

	// ASIOOutputReady()
	bool           postOutput;

	// ASIOGetLatencies ()
	long           inputLatency;
	long           outputLatency;

	// ASIOCreateBuffers ()
	long inputBuffers;	// becomes number of actual created input buffers
	long outputBuffers;	// becomes number of actual created output buffers
	ASIOBufferInfo bufferInfos[kMaxInputChannels + kMaxOutputChannels]; // buffer info's

	// ASIOGetChannelInfo()
	ASIOChannelInfo channelInfos[kMaxInputChannels + kMaxOutputChannels]; // channel info's
	// The above two arrays share the same indexing, as the data in them are linked together

	// Information from ASIOGetSamplePosition()
	// data is converted to double floats for easier use, however 64 bit integer can be used, too
	double         nanoSeconds;
	double         samples;
	double         tcSamples;	// time code samples

	// bufferSwitchTimeInfo()
	ASIOTime       tInfo;			// time info state
	unsigned long  sysRefTime;      // system reference time, when bufferSwitch() was called

	// Signal the end of processing in this example
	bool           stopped;
} TDriverInfo;

class CDeviceAudioASIO : CDeviceAudio {
 private:
   HANDLE frameEvent;
   CFrame *frame;
   int index;
   bool ASIO_trigger; 

   ASIOCallbacks asioCallbacks;
   TDriverInfo driver_info;
   
   void settings_update(void);

   long init_asio_static_data(void);
   ASIOError create_asio_buffers(void);
 
 public:
   ALIAS<"deviceAudioASIO">;
   void CDeviceAudioASIO(void);
 
   ATTRIBUTE<bool panel> {
      ASIOControlPanel();
      CDevice(this).init(FALSE);
      CDevice(this).init(TRUE);      
      CDeviceAudioASIO(this).settings_update();
   };
    
   virtual EDeviceError init(bool init);
   virtual EDeviceError open(void);
   virtual EDeviceError close(void);
   virtual bool frame(CFrame *frame, EStreamMode mode);
};

#define TEST_RUN_TIME  20.0		// run for 20 seconds

// ASIO Callbacks

// conversion from 64 bit ASIOSample/ASIOTimeStamp to double float
//#if NATIVE_INT64
//	#define ASIO64toDouble(a)  (a)
//#else
	const double twoRaisedTo32 = 4294967296.;
	#define ASIO64toDouble(a)  ((a).lo + (a).hi * twoRaisedTo32)
//#endif

CDeviceAudioASIO *asiodev;

ASIOTime *ASIO_bufferSwitchTimeInfo(ASIOTime *timeInfo, long index, ASIOBool processNow)
{	// the actual processing callback.
	// Beware that this is normally in a seperate thread, hence be sure that you take care
	// about thread synchronization. This is omitted here for simplicity.
	static processedSamples = 0;
	int i, j;
   long buffSize;

   // store the timeInfo for later use
	asiodev->driver_info.tInfo = *timeInfo;

	// get the time stamp of the buffer, not necessary if no
	// synchronization to other media is required
	if (timeInfo->timeInfo.flags & kSystemTimeValid)
		asiodev->driver_info.nanoSeconds = ASIO64toDouble(timeInfo->timeInfo.systemTime);
	else
		asiodev->driver_info.nanoSeconds = 0;

	if (timeInfo->timeInfo.flags & kSamplePositionValid)
		asiodev->driver_info.samples = ASIO64toDouble(timeInfo->timeInfo.samplePosition);
	else
		asiodev->driver_info.samples = 0;

	if (timeInfo->timeCode.flags & kTcValid)
		asiodev->driver_info.tcSamples = ASIO64toDouble(timeInfo->timeCode.timeCodeSamples);
	else
		asiodev->driver_info.tcSamples = 0;

	// get the system reference time
//	asiodev->driver_info.sysRefTime = get_sys_reference_time();

#if WINDOWS && _DEBUG
	// a few debug messages for the Windows device driver developer
	// tells you the time when driver got its interrupt and the delay until the app receives
	// the event notification.
	static double last_samples = 0;
	char tmp[128];
	sprintf (tmp, "diff: %d / %d ms / %d ms / %d samples                 \n", asioDriverInfo.sysRefTime - (long)(asioDriverInfo.nanoSeconds / 1000000.0), asioDriverInfo.sysRefTime, (long)(asioDriverInfo.nanoSeconds / 1000000.0), (long)(asioDriverInfo.samples - last_samples));
	OutputDebugString (tmp);
	last_samples = asiodev->driver_info.samples;
#endif

	// buffer size in samples
	buffSize = asiodev->driver_info.preferredSize;

	// perform the processing
if (asiodev->frame) {
	for (i = asiodev->driver_info.inputBuffers + asiodev->driver_info.outputBuffers - 1; i >= 0; i--)
	{
		if (asiodev->driver_info.bufferInfos[i].isInput == TRUE) {
//         if (asiodev->driver_info.channelInfos[i].type != ASIOSTInt16LSB)
//            ASSERT("ASIO input - bad format");
//
//         memcpy(CAudioBuffer(&ARRAY(&asiodev->frame->audio).data()[i]).data_pointer_get(), 
//                asiodev->driver_info.bufferInfos[i].buffers[index],
//                buffSize * 2);
      }
		else {
			// OK do processing for the outputs only
           switch (asiodev->driver_info.channelInfos[i].type) {
           case ASIOSTInt16LSB:
               memcpy(asiodev->driver_info.bufferInfos[i].buffers[index],
                      CAudioBuffer(&ARRAY(&asiodev->frame->audio).data()[i - asiodev->driver_info.inputBuffers]).data_pointer_get(), 
                      buffSize * 2);
              break;
           case ASIOSTInt32LSB:
              for (j = 0; j < asiodev->driver_info.preferredSize; j++) {
                   ((long *)asiodev->driver_info.bufferInfos[i].buffers[index])[j] =
                   ((short *)CAudioBuffer(&ARRAY(&asiodev->frame->audio).data()[i - asiodev->driver_info.inputBuffers]).data_pointer_get())[j] << 16;
                 }                      
                 break;
           }
//         if (asiodev->driver_info.channelInfos[i].type != ASIOSTInt16LSB)
//            ASSERT("ASIO output - bad format");
         
#if 0
         /* impulse on buffer start */
       	for (j = 0; j < 100; j++) {
		      ((short *)asiodev->driver_info.bufferInfos[i].buffers[index])[j] = 16384;
 			}
//       	for (j = 0; j < buffSize; j++) {
//		      ((short *)asiodev->driver_info.bufferInfos[i].buffers[index])[j] = buffSize - j;
// 			}
#else
//			memset(asiodev->driver_info.bufferInfos[i].buffers[index], 0, buffSize * 2);

//          memcpy(asiodev->driver_info.bufferInfos[i].buffers[index],
//                CAudioBuffer(&ARRAY(&asiodev->frame->audio).data()[i - asiodev->driver_info.inputBuffers]).data_pointer_get(), 
//                buffSize * 2);
//          CAudioBuffer(&ARRAY(&asiodev->frame->audio).data()[i - asiodev->driver_info.inputBuffers])->data_empty = FALSE;
#endif
		}
	}
}
else {
   /*>>>dropped frame, warn*/
}

	// finally if the driver supports the ASIOOutputReady() optimization, do it here, all data are in place
	if (asiodev->driver_info.postOutput)
		ASIOOutputReady();

	if (processedSamples >= asiodev->driver_info.sampleRate * TEST_RUN_TIME)	// roughly measured
		asiodev->driver_info.stopped = TRUE;
	else
		processedSamples += buffSize;
   
   asiodev->index = index;
   asiodev->frame = NULL;
   
    SetEvent(asiodev->frameEvent);

    /* Nofitication trigger */ 
    ASIO_buffer_switch_notify();    

	return 0L;
}

//----------------------------------------------------------------------------------
void ASIO_bufferSwitch(long index, ASIOBool processNow)
{	// the actual processing callback.
	// Beware that this is normally in a seperate thread, hence be sure that you take care
	// about thread synchronization. This is omitted here for simplicity.

	// as this is a "back door" into the bufferSwitchTimeInfo a timeInfo needs to be created
	// though it will only set the timeInfo.samplePosition and timeInfo.systemTime fields and the according flags
	ASIOTime  timeInfo;
	memset (&timeInfo, 0, sizeof (timeInfo));

	// get the time stamp of the buffer, not necessary if no
	// synchronization to other media is required
	if(ASIOGetSamplePosition(&timeInfo.timeInfo.samplePosition, &timeInfo.timeInfo.systemTime) == ASE_OK) {
		timeInfo.timeInfo.flags = kSystemTimeValid | kSamplePositionValid;
    }

	ASIO_bufferSwitchTimeInfo (&timeInfo, index, processNow);
}


//----------------------------------------------------------------------------------
void ASIO_sampleRateChanged(ASIOSampleRate sRate)
{
	// do whatever you need to do if the sample rate changed
	// usually this only happens during external sync.
	// Audio processing is not stopped by the driver, actual sample rate
	// might not have even changed, maybe only the sample rate status of an
	// AES/EBU or S/PDIF digital input at the audio device.
	// You might have to update time/sample related conversion routines, etc.
}

//----------------------------------------------------------------------------------
long ASIO_asioMessages(long selector, long value, void* message, double* opt)
{
	// currently the parameters "value", "message" and "opt" are not used.
	long ret = 0;
	switch(selector)
	{
		case kAsioSelectorSupported:
			if(value == kAsioResetRequest
			|| value == kAsioEngineVersion
			|| value == kAsioResyncRequest
			|| value == kAsioLatenciesChanged
			// the following three were added for ASIO 2.0, you don't necessarily have to support them
			|| value == kAsioSupportsTimeInfo
			|| value == kAsioSupportsTimeCode
			|| value == kAsioSupportsInputMonitor)
				ret = 1L;
			break;
		case kAsioResetRequest:
			// defer the task and perform the reset of the driver during the next "safe" situation
			// You cannot reset the driver right now, as this code is called from the driver.
			// Reset the driver is done by completely destruct is. I.e. ASIOStop(), ASIODisposeBuffers(), Destruction
			// Afterwards you initialize the driver again.
			asiodev->driver_info.stopped;  // In this sample the processing will just stop
			ret = 1L;
			break;
		case kAsioResyncRequest:
			// This informs the application, that the driver encountered some non fatal data loss.
			// It is used for synchronization purposes of different media.
			// Added mainly to work around the Win16Mutex problems in Windows 95/98 with the
			// Windows Multimedia system, which could loose data because the Mutex was hold too long
			// by another thread.
			// However a driver can issue it in other situations, too.
			ret = 1L;
			break;
		case kAsioLatenciesChanged:
			// This will inform the host application that the drivers were latencies changed.
			// Beware, it this does not mean that the buffer sizes have changed!
			// You might need to update internal delay data.
			ret = 1L;
			break;
		case kAsioEngineVersion:
			// return the supported ASIO version of the host application
			// If a host applications does not implement this selector, ASIO 1.0 is assumed
			// by the driver
			ret = 2L;
			break;
		case kAsioSupportsTimeInfo:
			// informs the driver wether the asioCallbacks.bufferSwitchTimeInfo() callback
			// is supported.
			// For compatibility with ASIO 1.0 drivers the host application should always support
			// the "old" bufferSwitch method, too.
			ret = 1;
			break;
		case kAsioSupportsTimeCode:
			// informs the driver wether application is interested in time code info.
			// If an application does not need to know about time code, the driver has less work
			// to do.
			ret = 0;
			break;
	}
	return ret;
}

long CDeviceAudioASIO::init_asio_static_data(void) {	
    TDriverInfo *asioDriverInfo = &this->driver_info;
        // collect the informational data of the driver
	// get the number of available channels
	if(ASIOGetChannels(&asioDriverInfo->inputChannels, &asioDriverInfo->outputChannels) == ASE_OK)
	{
//		printf ("ASIOGetChannels (inputs: %d, outputs: %d);\n", asioDriverInfo->inputChannels, asioDriverInfo->outputChannels);

		// get the usable buffer sizes
		if(ASIOGetBufferSize(&asioDriverInfo->minSize, &asioDriverInfo->maxSize, &asioDriverInfo->preferredSize, &asioDriverInfo->granularity) == ASE_OK)
		{
//			printf ("ASIOGetBufferSize (min: %d, max: %d, preferred: %d, granularity: %d);\n",
//					 asioDriverInfo->minSize, asioDriverInfo->maxSize,
//					 asioDriverInfo->preferredSize, asioDriverInfo->granularity);

			// get the currently selected sample rate
			if(ASIOGetSampleRate(&asioDriverInfo->sampleRate) == ASE_OK)
			{
//				printf ("ASIOGetSampleRate (sampleRate: %f);\n", asioDriverInfo->sampleRate);
				if (asioDriverInfo->sampleRate <= 0.0 || asioDriverInfo->sampleRate > 96000.0)
				{
					// Driver does not store it's internal sample rate, so set it to a know one.
					// Usually you should check beforehand, that the selected sample rate is valid
					// with ASIOCanSampleRate().
					if(ASIOSetSampleRate(44100.0) == ASE_OK)
					{
						if(ASIOGetSampleRate(&asioDriverInfo->sampleRate) == ASE_OK) {
//							printf ("ASIOGetSampleRate (sampleRate: %f);\n", asioDriverInfo->sampleRate);
                                                }
						else
							return -6;
					}
					else
						return -5;
				}

				// check wether the driver requires the ASIOOutputReady() optimization
				// (can be used by the driver to reduce output latency by one block)
				if(ASIOOutputReady() == ASE_OK)
					asioDriverInfo->postOutput = TRUE;
				else
					asioDriverInfo->postOutput = FALSE;
//				printf ("ASIOOutputReady(); - %s\n", asioDriverInfo->postOutput ? "Supported" : "Not supported");

				return 0;
			}
			return -3;
		}
		return -2;
	}
	return -1;
}

ASIOError CDeviceAudioASIO::create_asio_buffers(void) {
        // create buffers for all inputs and outputs of the card with the 
	// preferredSize from ASIOGetBufferSize() as buffer size
	long i;
	ASIOError result;
       	TDriverInfo *asioDriverInfo = &this->driver_info;

	// fill the bufferInfos from the start without a gap
	ASIOBufferInfo *info = asioDriverInfo->bufferInfos;

	// prepare inputs (Though this is not necessaily required, no opened inputs will work, too
	if (asioDriverInfo->inputChannels > kMaxInputChannels)
		asioDriverInfo->inputBuffers = kMaxInputChannels;
	else
		asioDriverInfo->inputBuffers = asioDriverInfo->inputChannels;
	for(i = 0; i < asioDriverInfo->inputBuffers; i++, info++)
	{
		info->isInput = ASIOTrue;
		info->channelNum = i;
		info->buffers[0] = info->buffers[1] = 0;
	}

	// prepare outputs
	if (asioDriverInfo->outputChannels > kMaxOutputChannels)
		asioDriverInfo->outputBuffers = kMaxOutputChannels;
	else
		asioDriverInfo->outputBuffers = asioDriverInfo->outputChannels;
	for(i = 0; i < asioDriverInfo->outputBuffers; i++, info++)
	{
		info->isInput = ASIOFalse;
		info->channelNum = i;
		info->buffers[0] = info->buffers[1] = 0;
	}

	result = ASIOCreateBuffers(asioDriverInfo->bufferInfos,
		asioDriverInfo->inputBuffers + asioDriverInfo->outputBuffers,
		asioDriverInfo->preferredSize, &this->asioCallbacks);
	if (result == ASE_OK)
	{
		// now get all the buffer details, sample word length, name, word clock group and activation
		for (i = 0; i < asioDriverInfo->inputBuffers + asioDriverInfo->outputBuffers; i++)
		{
			asioDriverInfo->channelInfos[i].channel = asioDriverInfo->bufferInfos[i].channelNum;
			asioDriverInfo->channelInfos[i].isInput = asioDriverInfo->bufferInfos[i].isInput;
			result = ASIOGetChannelInfo(&asioDriverInfo->channelInfos[i]);
			if (result != ASE_OK)
				break;
		}

		if (result == ASE_OK)
		{
			// get the input and output latencies
			// Latencies often are only valid after ASIOCreateBuffers()
			// (input latency is the age of the first sample in the currently returned audio block)
			// (output latency is the time the first sample in the currently returned audio block requires to get to the output)
			result = ASIOGetLatencies(&asioDriverInfo->inputLatency, &asioDriverInfo->outputLatency);
			if (result == ASE_OK) {
//				printf ("ASIOGetLatencies (input: %d, output: %d);\n", asioDriverInfo->inputLatency, asioDriverInfo->outputLatency);
                        }
		}
	}
	return result;
}

void CDeviceAudioASIO::settings_update(void) {
   ASIOSampleRate sampleRate;
   long inputChannels, outputChannels;
   long minSize, maxSize, preferredSize, granularity;
    
   if (ASIOGetSampleRate(&sampleRate) == ASE_OK) {
      CObjPersistent(this).attribute_update(ATTRIBUTE<CDeviceAudio,sampling_rate>);
      CObjPersistent(this).attribute_default_set(ATTRIBUTE<CDeviceAudio,sampling_rate>, TRUE);
      CDeviceAudio(this)->sampling_rate = sampleRate;
   }       
   if (ASIOGetChannels(&inputChannels, &outputChannels) == ASE_OK) {
      CObjPersistent(this).attribute_update(ATTRIBUTE<CDeviceAudio,channels_in>);
      CObjPersistent(this).attribute_default_set(ATTRIBUTE<CDeviceAudio,channels_in>, TRUE);
      CDeviceAudio(this)->channels_in = inputChannels;
      CObjPersistent(this).attribute_update(ATTRIBUTE<CDeviceAudio,channels_out>);
      CObjPersistent(this).attribute_default_set(ATTRIBUTE<CDeviceAudio,channels_out>, TRUE);
      CDeviceAudio(this)->channels_out = outputChannels;
   }
   
   if (ASIOGetBufferSize(&minSize, &maxSize, &preferredSize, &granularity) == ASE_OK) {
      CObjPersistent(this).attribute_update(ATTRIBUTE<CDeviceAudio,frame_length>);
      CObjPersistent(this).attribute_default_set(ATTRIBUTE<CDeviceAudio,frame_length>, TRUE);
      CDeviceAudio(this)->frame_length = preferredSize;
   }
       
   CObjPersistent(this).attribute_update_end();
}/*CDeviceAudioASIO::settings_update*/

static bool exception_set = FALSE;

int ASIO_unhandled_exception(void) {
WARN("ASIO kill begin");
    ASIOStop();    
    ASIODisposeBuffers();
    ASIOExit();
    removeCurrentAsioDriver();
WARN("ASIO kill end");    

    /*>>>for now, need a clean exit for respawn */
//    exit(0);
    return EXCEPTION_CONTINUE_SEARCH;
}

EDeviceError CDeviceAudioASIO::init(bool init) {
   if (!exception_set) {
       SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)ASIO_unhandled_exception);
       exception_set = TRUE;
   }
    
   if (init) {
      if (!loadAsioDriver(CString(&CDeviceHardware(this)->device).string()))
         ASSERT("Load ASIO driver failed\n");
      if (ASIOInit(&this->driver_info.driverInfo) != ASE_OK)
         ASSERT("Init ASIO driver failed\n");      

      if (CDeviceAudioASIO(this).init_asio_static_data() != 0)
         ASSERT("Init statis data ASIO driver failed\n");

      asiodev = this;
      this->asioCallbacks.bufferSwitch = &ASIO_bufferSwitch;
      this->asioCallbacks.sampleRateDidChange = &ASIO_sampleRateChanged;
      this->asioCallbacks.asioMessage = &ASIO_asioMessages;
      this->asioCallbacks.bufferSwitchTimeInfo = &ASIO_bufferSwitchTimeInfo;
      CDeviceAudioASIO(this).settings_update();      
   }
   else {
      ASIODisposeBuffers();
      ASIOExit();
      removeCurrentAsioDriver();
   }
   return EDeviceError.noError;   
}/*CDeviceAudioASIO::init*/

EDeviceError CDeviceAudioASIO::open(TFrameInfo *info) {
   class:base.open(info);   
   
   this->ASIO_trigger = info->sync_trigger == ESyncTrigger.ASIO;

   timeBeginPeriod(1);

   switch (CDeviceHardware(this)->mode) {
   case EStreamMode.duplex:
      this->frameEvent = CreateEvent(NULL, FALSE, FALSE, NULL);    
   
      if (CDeviceAudioASIO(this).create_asio_buffers() != ASE_OK)
         return EDeviceError.deviceError;
      if (ASIOStart() != ASE_OK)
         return EDeviceError.deviceError;          
      break;
   default:
       return EDeviceError.badParameters;
   }
   
   CDeviceAudioASIO(this).settings_update();

   return EDeviceError.noError;   
}/*CDeviceAudioASIO::open*/

EDeviceError CDeviceAudioASIO::close(void) {
   ASIOStop();
#if 0
   ASIODisposeBuffers();
   ASIOExit();

   removeCurrentAsioDriver();
#endif   
    
   timeEndPeriod(1);    
    
   CloseHandle(this->frameEvent);
   
   class:base.close();

   return EDeviceError.noError;    
}/*CDeviceAudioASIO::close*/

bool CDeviceAudioASIO::frame(CFrame *frame, EStreamMode mode) { 
   int i, j;
   
   if (mode == EStreamMode.input) {
	   for (i = this->driver_info.inputBuffers + this->driver_info.outputBuffers - 1; i >= 0; i--) {
		   if (this->driver_info.bufferInfos[i].isInput == TRUE) {
               switch (this->driver_info.channelInfos[i].type) {
               case ASIOSTInt16LSB:
                  memcpy(CAudioBuffer(&ARRAY(&frame->audio).data()[i]).data_pointer_get(), 
                         asiodev->driver_info.bufferInfos[i].buffers[this->index],
                         asiodev->driver_info.preferredSize * 2);
                  break;
               case ASIOSTInt32LSB:
                  for (j = 0; j < asiodev->driver_info.preferredSize; j++) {
                     ((short *)CAudioBuffer(&ARRAY(&frame->audio).data()[i]).data_pointer_get())[j] =
                     ((long *)asiodev->driver_info.bufferInfos[i].buffers[this->index])[j] >> 16;
                  }                      
                  break;
               }
            CAudioBuffer(&ARRAY(&frame->audio).data()[i])->data_empty = FALSE; 
         }
      }
      class:base.frame(frame, mode);         
   }
   if (mode == EStreamMode.output) {
      class:base.frame(frame, mode);               
      this->frame = frame;            

      if (!this->ASIO_trigger) {
         WaitForSingleObject(this->frameEvent, INFINITE);
      }
   }
    
   return TRUE;   
}/*CDeviceAudioASIO::frame*/

/*==========================================================================*/
MODULE::END/*===============================================================*/
/*==========================================================================*/
