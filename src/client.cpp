/*
 *  Copyright (C) 2011-2020 Team Kodi (https://kodi.tv)
 *  Copyright (C) 2011 Pulse-Eight (http://www.pulse-eight.com/)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "client.h"
#include "kodi/xbmc_pvr_dll.h"
#include "PVRDemoData.h"
#include <p8-platform/util/util.h>

using namespace std;
using namespace ADDON;

#ifdef TARGET_WINDOWS
#define snprintf _snprintf
#endif

bool           m_bCreated       = false;
ADDON_STATUS   m_CurStatus      = ADDON_STATUS_UNKNOWN;
PVRDemoData   *m_data           = NULL;
PVRDemoChannel m_currentChannel;

/* User adjustable settings are saved here.
 * Default values are defined inside client.h
 * and exported to the other source files.
 */
std::string g_strUserPath             = "";
std::string g_strClientPath           = "";

CHelper_libXBMC_addon *XBMC           = NULL;
CHelper_libXBMC_pvr   *PVR            = NULL;

extern "C" {

void ADDON_ReadSettings(void)
{
  //STUB
}

ADDON_STATUS ADDON_Create(void* hdl, void* props)
{
  if (!hdl || !props)
    return ADDON_STATUS_UNKNOWN;

  PVR_PROPERTIES* pvrprops = (PVR_PROPERTIES*)props;

  XBMC = new CHelper_libXBMC_addon;
  if (!XBMC->RegisterMe(hdl))
  {
    SAFE_DELETE(XBMC);
    return ADDON_STATUS_PERMANENT_FAILURE;
  }

  PVR = new CHelper_libXBMC_pvr;
  if (!PVR->RegisterMe(hdl))
  {
    SAFE_DELETE(PVR);
    SAFE_DELETE(XBMC);
    return ADDON_STATUS_PERMANENT_FAILURE;
  }

  XBMC->Log(LOG_DEBUG, "%s - Creating the PVR demo add-on", __FUNCTION__);

  m_CurStatus     = ADDON_STATUS_UNKNOWN;
  g_strUserPath   = pvrprops->strUserPath;
  g_strClientPath = pvrprops->strClientPath;

  ADDON_ReadSettings();

  m_data = new PVRDemoData;

  PVR_MENUHOOK hook;
  hook.iHookId = 1;
  hook.category = PVR_MENUHOOK_SETTING;
  hook.iLocalizedStringId = 30000;
  PVR->AddMenuHook(&hook);

  hook.iHookId = 2;
  hook.category = PVR_MENUHOOK_ALL;
  hook.iLocalizedStringId = 30001;
  PVR->AddMenuHook(&hook);

  hook.iHookId = 3;
  hook.category = PVR_MENUHOOK_CHANNEL;
  hook.iLocalizedStringId = 30002;
  PVR->AddMenuHook(&hook);

  m_CurStatus = ADDON_STATUS_OK;
  m_bCreated = true;
  return m_CurStatus;
}

ADDON_STATUS ADDON_GetStatus()
{
  return m_CurStatus;
}

void ADDON_Destroy()
{
  delete m_data;
  m_bCreated = false;
  m_CurStatus = ADDON_STATUS_UNKNOWN;
}

ADDON_STATUS ADDON_SetSetting(const char *settingName, const void *settingValue)
{
  return ADDON_STATUS_OK;
}

/***********************************************************
 * PVR Client AddOn specific public library functions
 ***********************************************************/

void OnSystemSleep()
{
}

void OnSystemWake()
{
}

void OnPowerSavingActivated()
{
}

void OnPowerSavingDeactivated()
{
}

PVR_ERROR GetAddonCapabilities(PVR_ADDON_CAPABILITIES* pCapabilities)
{
  pCapabilities->bSupportsEPG             = true;
  pCapabilities->bSupportsTV              = true;
  pCapabilities->bSupportsRadio           = true;
  pCapabilities->bSupportsChannelGroups   = true;
  pCapabilities->bSupportsRecordings      = true;
  pCapabilities->bSupportsRecordingsUndelete = true;
  pCapabilities->bSupportsTimers          = true;
  pCapabilities->bSupportsRecordingsRename = false;
  pCapabilities->bSupportsRecordingsLifetimeChange = false;
  pCapabilities->bSupportsDescrambleInfo = false;

  return PVR_ERROR_NO_ERROR;
}

const char *GetBackendName(void)
{
  static const char *strBackendName = "pulse-eight demo pvr add-on";
  return strBackendName;
}

const char *GetBackendVersion(void)
{
  static string strBackendVersion = "0.1";
  return strBackendVersion.c_str();
}

const char *GetConnectionString(void)
{
  static string strConnectionString = "connected";
  return strConnectionString.c_str();
}

const char *GetBackendHostname(void)
{
  return "";
}

PVR_ERROR GetDriveSpace(long long *iTotal, long long *iUsed)
{
  *iTotal = 1024 * 1024 * 1024;
  *iUsed  = 0;
  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR GetEPGForChannel(ADDON_HANDLE handle, int iChannelUid, time_t iStart, time_t iEnd)
{
  if (m_data)
    return m_data->GetEPGForChannel(handle, iChannelUid, iStart, iEnd);

  return PVR_ERROR_SERVER_ERROR;
}

PVR_ERROR IsEPGTagPlayable(const EPG_TAG*, bool* bIsPlayable)
{
  *bIsPlayable = true;
  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR GetEPGTagStreamProperties(const EPG_TAG* tag, PVR_NAMED_VALUE* properties, unsigned int* iPropertiesCount)
{
  if (!tag || !properties || !iPropertiesCount)
    return PVR_ERROR_SERVER_ERROR;

  if (*iPropertiesCount < 1)
    return PVR_ERROR_INVALID_PARAMETERS;

  strncpy(properties[0].strName, PVR_STREAM_PROPERTY_STREAMURL, sizeof(properties[0].strName) - 1);
  strncpy(properties[0].strValue, "http://distribution.bbb3d.renderfarming.net/video/mp4/bbb_sunflower_1080p_30fps_normal.mp4", sizeof(properties[0].strValue) - 1);
  *iPropertiesCount = 1;
  return PVR_ERROR_NO_ERROR;
}

int GetChannelsAmount(void)
{
  if (m_data)
    return m_data->GetChannelsAmount();

  return -1;
}

PVR_ERROR GetChannels(ADDON_HANDLE handle, bool bRadio)
{
  if (m_data)
    return m_data->GetChannels(handle, bRadio);

  return PVR_ERROR_SERVER_ERROR;
}

PVR_ERROR GetChannelStreamProperties(const PVR_CHANNEL* channel, PVR_NAMED_VALUE* properties, unsigned int* iPropertiesCount)
{
  if (!channel || !properties || !iPropertiesCount)
    return PVR_ERROR_SERVER_ERROR;

  if (*iPropertiesCount < 2)
    return PVR_ERROR_INVALID_PARAMETERS;

  if (m_data)
  {
    PVRDemoChannel addonChannel;
    m_data->GetChannel(*channel, addonChannel);

    strncpy(properties[0].strName, PVR_STREAM_PROPERTY_STREAMURL, sizeof(properties[0].strName) - 1);
    strncpy(properties[0].strValue, addonChannel.strStreamURL.c_str(), sizeof(properties[0].strValue) - 1);
    strncpy(properties[1].strName, PVR_STREAM_PROPERTY_ISREALTIMESTREAM, sizeof(properties[1].strName) - 1);
    strncpy(properties[1].strValue, "true", sizeof(properties[1].strValue) - 1);
    *iPropertiesCount = 2;
    return PVR_ERROR_NO_ERROR;
  }

  return PVR_ERROR_SERVER_ERROR;
}

int GetChannelGroupsAmount(void)
{
  if (m_data)
    return m_data->GetChannelGroupsAmount();

  return -1;
}

PVR_ERROR GetChannelGroups(ADDON_HANDLE handle, bool bRadio)
{
  if (m_data)
    return m_data->GetChannelGroups(handle, bRadio);

  return PVR_ERROR_SERVER_ERROR;
}

PVR_ERROR GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP &group)
{
  if (m_data)
    return m_data->GetChannelGroupMembers(handle, group);

  return PVR_ERROR_SERVER_ERROR;
}

PVR_ERROR SignalStatus(PVR_SIGNAL_STATUS &signalStatus)
{
  snprintf(signalStatus.strAdapterName, sizeof(signalStatus.strAdapterName), "pvr demo adapter 1");
  snprintf(signalStatus.strAdapterStatus, sizeof(signalStatus.strAdapterStatus), "OK");

  return PVR_ERROR_NO_ERROR;
}

int GetRecordingsAmount(bool deleted)
{
  if (m_data)
    return m_data->GetRecordingsAmount(deleted);

  return -1;
}

PVR_ERROR GetRecordings(ADDON_HANDLE handle, bool deleted)
{
  if (m_data)
    return m_data->GetRecordings(handle, deleted);

  return PVR_ERROR_SERVER_ERROR;
}

PVR_ERROR GetRecordingStreamProperties(const PVR_RECORDING* recording, PVR_NAMED_VALUE* properties, unsigned int* iPropertiesCount)
{
  if (!recording || !properties || !iPropertiesCount)
    return PVR_ERROR_SERVER_ERROR;

  if (*iPropertiesCount < 1)
    return PVR_ERROR_INVALID_PARAMETERS;

  if (m_data)
  {
    PVRDemoRecording addonRecording;
    std::string streamURL = m_data->GetRecordingURL(*recording);

    strncpy(properties[0].strName, PVR_STREAM_PROPERTY_STREAMURL, sizeof(properties[0].strName) - 1);
    strncpy(properties[0].strValue, streamURL.c_str(), sizeof(properties[0].strValue) - 1);
    *iPropertiesCount = 1;
    return PVR_ERROR_NO_ERROR;
  }

  return PVR_ERROR_SERVER_ERROR;
}

PVR_ERROR GetTimerTypes(PVR_TIMER_TYPE types[], int *size)
{
  /* TODO: Implement this to get support for the timer features introduced with PVR API 1.9.7 */
  return PVR_ERROR_NOT_IMPLEMENTED;
}

int GetTimersAmount(void)
{
  if (m_data)
    return m_data->GetTimersAmount();

  return -1;
}

PVR_ERROR GetTimers(ADDON_HANDLE handle)
{
  if (m_data)
    return m_data->GetTimers(handle);

  /* TODO: Change implementation to get support for the timer features introduced with PVR API 1.9.7 */
  return PVR_ERROR_NOT_IMPLEMENTED;
}

PVR_ERROR CallMenuHook(const PVR_MENUHOOK& menuhook, const PVR_MENUHOOK_DATA&)
{
  int iMsg;
  switch (menuhook.iHookId)
  {
    case 1:
      iMsg = 30010;
      break;
    case 2:
      iMsg = 30011;
      break;
    case 3:
      iMsg = 30012;
      break;
    default:
      return PVR_ERROR_INVALID_PARAMETERS;
  }
  char* msg = XBMC->GetLocalizedString(iMsg);
  XBMC->QueueNotification(QUEUE_INFO, msg);
  XBMC->FreeString(msg);

  return PVR_ERROR_NO_ERROR;
}

/** UNUSED API FUNCTIONS */
PVR_ERROR OpenDialogChannelScan(void) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR DeleteChannel(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR RenameChannel(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR OpenDialogChannelSettings(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR OpenDialogChannelAdd(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
bool OpenRecordedStream(const PVR_RECORDING &recording) { return false; }
void CloseRecordedStream(void) {}
int ReadRecordedStream(unsigned char *pBuffer, unsigned int iBufferSize) { return 0; }
long long SeekRecordedStream(long long iPosition, int iWhence /* = SEEK_SET */) { return 0; }
long long LengthRecordedStream(void) { return 0; }
void DemuxReset(void) {}
void DemuxFlush(void) {}
bool OpenLiveStream(const PVR_CHANNEL&) { return false; }
void CloseLiveStream(void) {}
int ReadLiveStream(unsigned char *pBuffer, unsigned int iBufferSize) { return 0; }
long long SeekLiveStream(long long iPosition, int iWhence /* = SEEK_SET */) { return -1; }
long long LengthLiveStream(void) { return -1; }
PVR_ERROR DeleteRecording(const PVR_RECORDING &recording) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR RenameRecording(const PVR_RECORDING &recording) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR SetRecordingPlayCount(const PVR_RECORDING &recording, int count) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR SetRecordingLastPlayedPosition(const PVR_RECORDING &recording, int lastplayedposition) { return PVR_ERROR_NOT_IMPLEMENTED; }
int GetRecordingLastPlayedPosition(const PVR_RECORDING &recording) { return -1; }
PVR_ERROR GetRecordingEdl(const PVR_RECORDING&, PVR_EDL_ENTRY[], int*) { return PVR_ERROR_NOT_IMPLEMENTED; };
PVR_ERROR GetRecordingSize(const PVR_RECORDING* recording, int64_t* sizeInBytes) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR AddTimer(const PVR_TIMER &timer) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR DeleteTimer(const PVR_TIMER &timer, bool bForceDelete) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR UpdateTimer(const PVR_TIMER &timer) { return PVR_ERROR_NOT_IMPLEMENTED; }
void DemuxAbort(void) {}
DemuxPacket* DemuxRead(void) { return NULL; }
void FillBuffer(bool mode) {}
void PauseStream(bool bPaused) {}
bool CanPauseStream(void) { return false; }
bool CanSeekStream(void) { return false; }
bool SeekTime(double,bool,double*) { return false; }
void SetSpeed(int) {};
bool IsRealTimeStream(void) { return true; }
PVR_ERROR UndeleteRecording(const PVR_RECORDING& recording) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR DeleteAllRecordingsFromTrash() { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR SetEPGTimeFrame(int) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR GetDescrambleInfo(PVR_DESCRAMBLE_INFO*) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR SetRecordingLifetime(const PVR_RECORDING*) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR GetStreamProperties(PVR_STREAM_PROPERTIES*) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR GetStreamTimes(PVR_STREAM_TIMES*) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR IsEPGTagRecordable(const EPG_TAG*, bool*) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR GetEPGTagEdl(const EPG_TAG* epgTag, PVR_EDL_ENTRY edl[], int *size) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR GetStreamReadChunkSize(int* chunksize) { return PVR_ERROR_NOT_IMPLEMENTED; }
  
} // extern "C"
