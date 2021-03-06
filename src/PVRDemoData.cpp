/*
 *  Copyright (C) 2011-2020 Team Kodi (https://kodi.tv)
 *  Copyright (C) 2011 Pulse-Eight (http://www.pulse-eight.com/)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "util/XMLUtils.h"
#include "PVRDemoData.h"
#include "p8-platform/util/StringUtils.h"

using namespace std;
using namespace ADDON;

PVRDemoData::PVRDemoData(void)
{
  m_iEpgStart = -1;
  m_strDefaultIcon =  "http://www.royalty-free.tv/news/wp-content/uploads/2011/06/cc-logo1.jpg";
  m_strDefaultMovie = "";

  LoadDemoData();
}

PVRDemoData::~PVRDemoData(void)
{
  m_channels.clear();
  m_groups.clear();
}

std::string PVRDemoData::GetSettingsFile() const
{
  string settingFile = g_strClientPath;
  if (settingFile.at(settingFile.size() - 1) == '\\' ||
      settingFile.at(settingFile.size() - 1) == '/')
    settingFile.append("PVRDemoAddonSettings.xml");
  else
    settingFile.append("/PVRDemoAddonSettings.xml");
  return settingFile;
}

bool PVRDemoData::LoadDemoData(void)
{
  TiXmlDocument xmlDoc;
  string strSettingsFile = GetSettingsFile();

  if (!xmlDoc.LoadFile(strSettingsFile))
  {
    XBMC->Log(LOG_ERROR, "invalid demo data (no/invalid data file found at '%s')", strSettingsFile.c_str());
    return false;
  }

  TiXmlElement *pRootElement = xmlDoc.RootElement();
  if (strcmp(pRootElement->Value(), "demo") != 0)
  {
    XBMC->Log(LOG_ERROR, "invalid demo data (no <demo> tag found)");
    return false;
  }

  /* load channels */
  int iUniqueChannelId = 0;
  TiXmlElement *pElement = pRootElement->FirstChildElement("channels");
  if (pElement)
  {
    TiXmlNode *pChannelNode = NULL;
    while ((pChannelNode = pElement->IterateChildren(pChannelNode)) != NULL)
    {
      PVRDemoChannel channel;
      if (ScanXMLChannelData(pChannelNode, ++iUniqueChannelId, channel))
        m_channels.push_back(channel);
    }
  }

  /* load channel groups */
  int iUniqueGroupId = 0;
  pElement = pRootElement->FirstChildElement("channelgroups");
  if (pElement)
  {
    TiXmlNode *pGroupNode = NULL;
    while ((pGroupNode = pElement->IterateChildren(pGroupNode)) != NULL)
    {
      PVRDemoChannelGroup group;
      if (ScanXMLChannelGroupData(pGroupNode, ++iUniqueGroupId, group))
        m_groups.push_back(group);
    }
  }

  /* load EPG entries */
  pElement = pRootElement->FirstChildElement("epg");
  if (pElement)
  {
    TiXmlNode *pEpgNode = NULL;
    while ((pEpgNode = pElement->IterateChildren(pEpgNode)) != NULL)
    {
      ScanXMLEpgData(pEpgNode);
    }
  }

  /* load recordings */
  iUniqueGroupId = 0; // reset unique ids
  pElement = pRootElement->FirstChildElement("recordings");
  if (pElement)
  {
    TiXmlNode *pRecordingNode = NULL;
    while ((pRecordingNode = pElement->IterateChildren(pRecordingNode)) != NULL)
    {
      PVRDemoRecording recording;
      if (ScanXMLRecordingData(pRecordingNode, ++iUniqueGroupId, recording))
        m_recordings.push_back(recording);
    }
  }

  /* load deleted recordings */
  pElement = pRootElement->FirstChildElement("recordingsdeleted");
  if (pElement)
  {
    TiXmlNode *pRecordingNode = NULL;
    while ((pRecordingNode = pElement->IterateChildren(pRecordingNode)) != NULL)
    {
      PVRDemoRecording recording;
      if (ScanXMLRecordingData(pRecordingNode, ++iUniqueGroupId, recording))
        m_recordingsDeleted.push_back(recording);
    }
  }

  /* load timers */
  pElement = pRootElement->FirstChildElement("timers");
  if (pElement)
  {
    TiXmlNode *pTimerNode = NULL;
    while ((pTimerNode = pElement->IterateChildren(pTimerNode)) != NULL)
    {
      PVRDemoTimer timer;
      if (ScanXMLTimerData(pTimerNode, timer))
        m_timers.push_back(timer);
    }
  }

  return true;
}

int PVRDemoData::GetChannelsAmount(void)
{
  return m_channels.size();
}

PVR_ERROR PVRDemoData::GetChannels(ADDON_HANDLE handle, bool bRadio)
{
  for (const auto& channel : m_channels)
  {
    if (channel.bRadio == bRadio)
    {
      PVR_CHANNEL xbmcChannel = {};

      xbmcChannel.iUniqueId         = channel.iUniqueId;
      xbmcChannel.bIsRadio          = channel.bRadio;
      xbmcChannel.iChannelNumber    = channel.iChannelNumber;
      xbmcChannel.iSubChannelNumber = channel.iSubChannelNumber;
      strncpy(xbmcChannel.strChannelName, channel.strChannelName.c_str(), sizeof(xbmcChannel.strChannelName) - 1);
      xbmcChannel.iEncryptionSystem = channel.iEncryptionSystem;
      strncpy(xbmcChannel.strIconPath, channel.strIconPath.c_str(), sizeof(xbmcChannel.strIconPath) - 1);
      xbmcChannel.bIsHidden         = false;

      PVR->TransferChannelEntry(handle, &xbmcChannel);
    }
  }

  return PVR_ERROR_NO_ERROR;
}

bool PVRDemoData::GetChannel(const PVR_CHANNEL &channel, PVRDemoChannel &myChannel)
{
  for (const auto& thisChannel : m_channels)
  {
    if (thisChannel.iUniqueId == (int) channel.iUniqueId)
    {
      myChannel.iUniqueId         = thisChannel.iUniqueId;
      myChannel.bRadio            = thisChannel.bRadio;
      myChannel.iChannelNumber    = thisChannel.iChannelNumber;
      myChannel.iSubChannelNumber = thisChannel.iSubChannelNumber;
      myChannel.iEncryptionSystem = thisChannel.iEncryptionSystem;
      myChannel.strChannelName    = thisChannel.strChannelName;
      myChannel.strIconPath       = thisChannel.strIconPath;
      myChannel.strStreamURL      = thisChannel.strStreamURL;

      return true;
    }
  }

  return false;
}

int PVRDemoData::GetChannelGroupsAmount(void)
{
  return m_groups.size();
}

PVR_ERROR PVRDemoData::GetChannelGroups(ADDON_HANDLE handle, bool bRadio)
{
  for (unsigned int iGroupPtr = 0; iGroupPtr < m_groups.size(); iGroupPtr++)
  {
    PVRDemoChannelGroup &group = m_groups.at(iGroupPtr);
    if (group.bRadio == bRadio)
    {
      PVR_CHANNEL_GROUP xbmcGroup = {};

      xbmcGroup.bIsRadio = bRadio;
      xbmcGroup.iPosition = group.iPosition;
      strncpy(xbmcGroup.strGroupName, group.strGroupName.c_str(), sizeof(xbmcGroup.strGroupName) - 1);

      PVR->TransferChannelGroup(handle, &xbmcGroup);
    }
  }

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR PVRDemoData::GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP &group)
{
  for (unsigned int iGroupPtr = 0; iGroupPtr < m_groups.size(); iGroupPtr++)
  {
    PVRDemoChannelGroup &myGroup = m_groups.at(iGroupPtr);
    if (!strcmp(myGroup.strGroupName.c_str(),group.strGroupName))
    {
      for (unsigned int iChannelPtr = 0; iChannelPtr < myGroup.members.size(); iChannelPtr++)
      {
        int iId = myGroup.members.at(iChannelPtr) - 1;
        if (iId < 0 || iId > (int)m_channels.size() - 1)
          continue;
        PVRDemoChannel &channel = m_channels.at(iId);
        PVR_CHANNEL_GROUP_MEMBER xbmcGroupMember = {};

        strncpy(xbmcGroupMember.strGroupName, group.strGroupName, sizeof(xbmcGroupMember.strGroupName) - 1);
        xbmcGroupMember.iChannelUniqueId  = channel.iUniqueId;
        xbmcGroupMember.iChannelNumber    = channel.iChannelNumber;
        xbmcGroupMember.iSubChannelNumber = channel.iSubChannelNumber;

        PVR->TransferChannelGroupMember(handle, &xbmcGroupMember);
      }
    }
  }

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR PVRDemoData::GetEPGForChannel(ADDON_HANDLE handle, int iChannelUid, time_t iStart, time_t iEnd)
{
  if (m_iEpgStart == -1)
    m_iEpgStart = iStart;

  time_t iLastEndTime = m_iEpgStart + 1;
  int iAddBroadcastId = 0;

  for (unsigned int iChannelPtr = 0; iChannelPtr < m_channels.size(); iChannelPtr++)
  {
    PVRDemoChannel &myChannel = m_channels.at(iChannelPtr);
    if (myChannel.iUniqueId != iChannelUid)
      continue;

    while (iLastEndTime < iEnd && myChannel.epg.size() > 0)
    {
      time_t iLastEndTimeTmp = 0;
      for (unsigned int iEntryPtr = 0; iEntryPtr < myChannel.epg.size(); iEntryPtr++)
      {
        PVRDemoEpgEntry &myTag = myChannel.epg.at(iEntryPtr);

        EPG_TAG tag = {};

        tag.iUniqueBroadcastId = myTag.iBroadcastId + iAddBroadcastId;
        tag.iUniqueChannelId   = iChannelUid;
        tag.strTitle           = myTag.strTitle.c_str();
        tag.startTime          = myTag.startTime + iLastEndTime;
        tag.endTime            = myTag.endTime + iLastEndTime;
        tag.strPlotOutline     = myTag.strPlotOutline.c_str();
        tag.strPlot            = myTag.strPlot.c_str();
        tag.strIconPath        = myTag.strIconPath.c_str();
        tag.iGenreType         = myTag.iGenreType;
        tag.iGenreSubType      = myTag.iGenreSubType;
        tag.iFlags             = EPG_TAG_FLAG_UNDEFINED;
        tag.iSeriesNumber      = myTag.iSeriesNumber;
        tag.iEpisodeNumber     = myTag.iEpisodeNumber;;
        tag.iEpisodePartNumber = EPG_TAG_INVALID_SERIES_EPISODE;
        tag.strEpisodeName     = myTag.strEpisodeName.c_str();
        tag.strFirstAired = "";

        iLastEndTimeTmp = tag.endTime;

        PVR->TransferEpgEntry(handle, &tag);
      }

      iLastEndTime = iLastEndTimeTmp;
      iAddBroadcastId += myChannel.epg.size();
    }
  }

  return PVR_ERROR_NO_ERROR;
}

int PVRDemoData::GetRecordingsAmount(bool bDeleted)
{
  return bDeleted ? m_recordingsDeleted.size() : m_recordings.size();
}

PVR_ERROR PVRDemoData::GetRecordings(ADDON_HANDLE handle, bool bDeleted)
{
  std::vector<PVRDemoRecording> *recordings = bDeleted ? &m_recordingsDeleted : &m_recordings;

  for (std::vector<PVRDemoRecording>::iterator it = recordings->begin() ; it != recordings->end() ; it++)
  {
    PVRDemoRecording &recording = *it;

    PVR_RECORDING xbmcRecording = {};
    xbmcRecording.iSeriesNumber = PVR_RECORDING_INVALID_SERIES_EPISODE;
    xbmcRecording.iEpisodeNumber = PVR_RECORDING_INVALID_SERIES_EPISODE;

    xbmcRecording.iDuration     = recording.iDuration;
    xbmcRecording.iGenreType    = recording.iGenreType;
    xbmcRecording.iGenreSubType = recording.iGenreSubType;
    xbmcRecording.recordingTime = recording.recordingTime;
    xbmcRecording.iEpisodeNumber = recording.iEpisodeNumber;
    xbmcRecording.iSeriesNumber = recording.iSeriesNumber;
    xbmcRecording.bIsDeleted    = bDeleted;
    xbmcRecording.channelType   = recording.bRadio ? PVR_RECORDING_CHANNEL_TYPE_RADIO : PVR_RECORDING_CHANNEL_TYPE_TV;

    strncpy(xbmcRecording.strChannelName, recording.strChannelName.c_str(), sizeof(xbmcRecording.strChannelName) - 1);
    strncpy(xbmcRecording.strPlotOutline, recording.strPlotOutline.c_str(), sizeof(xbmcRecording.strPlotOutline) - 1);
    strncpy(xbmcRecording.strPlot,        recording.strPlot.c_str(),        sizeof(xbmcRecording.strPlot) - 1);
    strncpy(xbmcRecording.strRecordingId, recording.strRecordingId.c_str(), sizeof(xbmcRecording.strRecordingId) - 1);
    strncpy(xbmcRecording.strTitle,       recording.strTitle.c_str(),       sizeof(xbmcRecording.strTitle) - 1);
    strncpy(xbmcRecording.strEpisodeName, recording.strEpisodeName.c_str(), sizeof(xbmcRecording.strEpisodeName) - 1);
    strncpy(xbmcRecording.strDirectory,   recording.strDirectory.c_str(),   sizeof(xbmcRecording.strDirectory) - 1);

    /* TODO: PVR API 5.0.0: Implement this */
    xbmcRecording.iChannelUid = PVR_CHANNEL_INVALID_UID;

    PVR->TransferRecordingEntry(handle, &xbmcRecording);
  }

  return PVR_ERROR_NO_ERROR;
}

std::string PVRDemoData::GetRecordingURL(const PVR_RECORDING &recording)
{
  for (const auto& thisRecording : m_recordings)
  {
    if (thisRecording.strRecordingId == recording.strRecordingId)
    {
      return thisRecording.strStreamURL;
    }
  }

  return "";
}

int PVRDemoData::GetTimersAmount(void)
{
  return m_timers.size();
}

PVR_ERROR PVRDemoData::GetTimers(ADDON_HANDLE handle)
{
  unsigned int i = PVR_TIMER_NO_CLIENT_INDEX + 1;
  for (std::vector<PVRDemoTimer>::iterator it = m_timers.begin() ; it != m_timers.end() ; it++)
  {
    PVRDemoTimer &timer = *it;

    PVR_TIMER xbmcTimer = {};

    /* TODO: Implement own timer types to get support for the timer features introduced with PVR API 1.9.7 */
    xbmcTimer.iTimerType = PVR_TIMER_TYPE_NONE;

    xbmcTimer.iClientIndex      = i++;
    xbmcTimer.iClientChannelUid = timer.iChannelId;
    xbmcTimer.startTime         = timer.startTime;
    xbmcTimer.endTime           = timer.endTime;
    xbmcTimer.state             = timer.state;

    strncpy(xbmcTimer.strTitle, timer.strTitle.c_str(), sizeof(timer.strTitle) - 1);
    strncpy(xbmcTimer.strSummary, timer.strSummary.c_str(), sizeof(timer.strSummary) - 1);

    PVR->TransferTimerEntry(handle, &xbmcTimer);
  }

  return PVR_ERROR_NO_ERROR;
}

bool PVRDemoData::ScanXMLChannelData(const TiXmlNode* pChannelNode, int iUniqueChannelId, PVRDemoChannel& channel)
{
  std::string strTmp;
  channel.iUniqueId = iUniqueChannelId;

  /* channel name */
  if (!XMLUtils::GetString(pChannelNode, "name", strTmp))
    return false;
  channel.strChannelName = strTmp;

  /* radio/TV */
  XMLUtils::GetBoolean(pChannelNode, "radio", channel.bRadio);

  /* channel number */
  if (!XMLUtils::GetInt(pChannelNode, "number", channel.iChannelNumber))
    channel.iChannelNumber = iUniqueChannelId;

  /* sub channel number */
  if (!XMLUtils::GetInt(pChannelNode, "subnumber", channel.iSubChannelNumber))
    channel.iSubChannelNumber = 0;

  /* CAID */
  if (!XMLUtils::GetInt(pChannelNode, "encryption", channel.iEncryptionSystem))
    channel.iEncryptionSystem = 0;

  /* icon path */
  if (!XMLUtils::GetString(pChannelNode, "icon", strTmp))
    channel.strIconPath = m_strDefaultIcon;
  else
    channel.strIconPath = g_strClientPath + strTmp;

  /* stream url */
  if (!XMLUtils::GetString(pChannelNode, "stream", strTmp))
    channel.strStreamURL = m_strDefaultMovie;
  else
    channel.strStreamURL = strTmp;

  return true;
}

bool PVRDemoData::ScanXMLChannelGroupData(const TiXmlNode* pGroupNode, int iUniqueGroupId, PVRDemoChannelGroup& group)
{
  std::string strTmp;
  group.iGroupId = iUniqueGroupId;

  /* group name */
  if (!XMLUtils::GetString(pGroupNode, "name", strTmp))
    return false;
  group.strGroupName = strTmp;

  /* radio/TV */
  XMLUtils::GetBoolean(pGroupNode, "radio", group.bRadio);

  /* sort position */
  XMLUtils::GetInt(pGroupNode, "position", group.iPosition);

  /* members */
  const TiXmlNode* pMembers = pGroupNode->FirstChild("members");
  const TiXmlNode* pMemberNode = nullptr;
  while (pMembers != nullptr && (pMemberNode = pMembers->IterateChildren(pMemberNode)) != nullptr)
  {
    int iChannelId = atoi(pMemberNode->FirstChild()->Value());
    if (iChannelId > -1)
      group.members.push_back(iChannelId);
  }

  return true;
}

bool PVRDemoData::ScanXMLEpgData(const TiXmlNode* pEpgNode)
{
  std::string strTmp;
  int iTmp;
  PVRDemoEpgEntry entry;

  /* broadcast id */
  if (!XMLUtils::GetInt(pEpgNode, "broadcastid", entry.iBroadcastId))
    return false;

  /* channel id */
  if (!XMLUtils::GetInt(pEpgNode, "channelid", iTmp))
    return false;
  PVRDemoChannel& channel = m_channels.at(iTmp - 1);
  entry.iChannelId = channel.iUniqueId;

  /* title */
  if (!XMLUtils::GetString(pEpgNode, "title", strTmp))
    return false;
  entry.strTitle = strTmp;

  /* start */
  if (!XMLUtils::GetInt(pEpgNode, "start", iTmp))
    return false;
  entry.startTime = iTmp;

  /* end */
  if (!XMLUtils::GetInt(pEpgNode, "end", iTmp))
    return false;
  entry.endTime = iTmp;

  /* plot */
  if (XMLUtils::GetString(pEpgNode, "plot", strTmp))
    entry.strPlot = strTmp;

  /* plot outline */
  if (XMLUtils::GetString(pEpgNode, "plotoutline", strTmp))
    entry.strPlotOutline = strTmp;

  if (!XMLUtils::GetInt(pEpgNode, "series", entry.iSeriesNumber))
    entry.iSeriesNumber = EPG_TAG_INVALID_SERIES_EPISODE;

  if (!XMLUtils::GetInt(pEpgNode, "episode", entry.iEpisodeNumber))
    entry.iEpisodeNumber = EPG_TAG_INVALID_SERIES_EPISODE;

  if (XMLUtils::GetString(pEpgNode, "episodetitle", strTmp))
    entry.strEpisodeName = strTmp;

  /* icon path */
  if (XMLUtils::GetString(pEpgNode, "icon", strTmp))
    entry.strIconPath = strTmp;

  /* genre type */
  XMLUtils::GetInt(pEpgNode, "genretype", entry.iGenreType);

  /* genre subtype */
  XMLUtils::GetInt(pEpgNode, "genresubtype", entry.iGenreSubType);

  XBMC->Log(LOG_DEBUG, "loaded EPG entry '%s' channel '%d' start '%d' end '%d'", entry.strTitle.c_str(), entry.iChannelId, entry.startTime, entry.endTime);

  channel.epg.push_back(entry);

  return true;
}

bool PVRDemoData::ScanXMLRecordingData(const TiXmlNode* pRecordingNode, int iUniqueGroupId, PVRDemoRecording& recording)
{
  std::string strTmp;

  /* radio/TV */
  XMLUtils::GetBoolean(pRecordingNode, "radio", recording.bRadio);

  /* recording title */
  if (!XMLUtils::GetString(pRecordingNode, "title", strTmp))
    return false;
  recording.strTitle = strTmp;

  /* recording url */
  if (!XMLUtils::GetString(pRecordingNode, "url", strTmp))
    recording.strStreamURL = m_strDefaultMovie;
  else
    recording.strStreamURL = strTmp;

  /* recording path */
  if (XMLUtils::GetString(pRecordingNode, "directory", strTmp))
    recording.strDirectory = strTmp;

  strTmp = StringUtils::Format("%d", iUniqueGroupId);
  recording.strRecordingId = strTmp;

  /* channel name */
  if (XMLUtils::GetString(pRecordingNode, "channelname", strTmp))
    recording.strChannelName = strTmp;

  /* plot */
  if (XMLUtils::GetString(pRecordingNode, "plot", strTmp))
    recording.strPlot = strTmp;

  /* plot outline */
  if (XMLUtils::GetString(pRecordingNode, "plotoutline", strTmp))
    recording.strPlotOutline = strTmp;

  /* Episode Name */
  if (XMLUtils::GetString(pRecordingNode, "episodetitle", strTmp))
    recording.strEpisodeName = strTmp;

  /* Series Number */
  if (!XMLUtils::GetInt(pRecordingNode, "series", recording.iSeriesNumber))
    recording.iSeriesNumber = 0;

  /* Episode Number */
  if (!XMLUtils::GetInt(pRecordingNode, "episode", recording.iEpisodeNumber))
    recording.iEpisodeNumber = 0;

  /* genre type */
  XMLUtils::GetInt(pRecordingNode, "genretype", recording.iGenreType);

  /* genre subtype */
  XMLUtils::GetInt(pRecordingNode, "genresubtype", recording.iGenreSubType);

  /* duration */
  XMLUtils::GetInt(pRecordingNode, "duration", recording.iDuration);

  /* recording time */
  if (XMLUtils::GetString(pRecordingNode, "time", strTmp))
  {
    time_t timeNow = time(nullptr);
    struct tm* now = localtime(&timeNow);

    auto delim = strTmp.find(':');
    if (delim != std::string::npos)
    {
      now->tm_hour = std::stoi(StringUtils::Left(strTmp, delim));
      now->tm_min  = std::stoi(StringUtils::Mid(strTmp, (delim + 1)));
      now->tm_mday--; // yesterday

      recording.recordingTime = mktime(now);
    }
  }

  return true;
}

bool PVRDemoData::ScanXMLTimerData(const TiXmlNode* pTimerNode, PVRDemoTimer& timer)
{
  std::string strTmp;
  int iTmp;

  time_t timeNow = time(nullptr);
  struct tm* now = localtime(&timeNow);

  /* channel id */
  if (!XMLUtils::GetInt(pTimerNode, "channelid", iTmp))
    return false;
  PVRDemoChannel &channel = m_channels.at(iTmp - 1);
  timer.iChannelId = channel.iUniqueId;

  /* state */
  if (XMLUtils::GetInt(pTimerNode, "state", iTmp))
    timer.state = (PVR_TIMER_STATE) iTmp;

  /* title */
  if (!XMLUtils::GetString(pTimerNode, "title", strTmp))
    return false;
  timer.strTitle = strTmp;

  /* summary */
  if (!XMLUtils::GetString(pTimerNode, "summary", strTmp))
    return false;
  timer.strSummary = strTmp;

  /* start time */
  if (XMLUtils::GetString(pTimerNode, "starttime", strTmp))
  {
    auto delim = strTmp.find(':');
    if (delim != std::string::npos)
    {
      now->tm_hour = std::stoi(StringUtils::Left(strTmp, delim));
      now->tm_min  = std::stoi(StringUtils::Mid(strTmp, delim + 1));

      timer.startTime = mktime(now);
    }
  }

  /* end time */
  if (XMLUtils::GetString(pTimerNode, "endtime", strTmp))
  {
    auto delim = strTmp.find(':');
    if (delim != std::string::npos)
    {
      now->tm_hour = std::stoi(StringUtils::Left(strTmp, delim));
      now->tm_min  = std::stoi(StringUtils::Mid(strTmp, delim + 1));

      timer.endTime = mktime(now);
    }
  }

  XBMC->Log(LOG_DEBUG, "loaded timer '%s' channel '%d' start '%d' end '%d'", timer.strTitle.c_str(), timer.iChannelId, timer.startTime, timer.endTime);
  return true;
}
