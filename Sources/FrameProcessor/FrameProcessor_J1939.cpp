/**
  Project       :  Auto-SAT_Tools
  FileName      :  FrameProcessor_J1939.cpp
  Description   :  Source file for CFrameProcessor_J1939 class.
  Author(s)     :  Ratnadip Choudhury
  Date Created  :  1.12.2010
  Modified By   :
  Copyright (c) 2011, Robert Bosch Engineering and Business Solutions.  All rights reserved.
******************************************************************************/

#include "FrameProcessor_stdafx.h"
#include "Datatypes/MsgBufAll_DataTypes.h"
#include "DIL_Interface/DIL_Interface_extern.h"
#include "FrameProcessor_J1939.h"
#include "LogObjectJ1939.h"
#include "Utility\MultiLanguageSupport.h"
#include "Application/StdAfx.h"

USHORT CFrameProcessor_J1939::ushCalculateStrLen(bool bForHex, USHORT ushLength)
{
    USHORT Result = 0;

    if (bForHex)
    {
        Result = ushLength * 3 + 1;
    }
    else
    {
        Result = ushLength * 4 + 1;
    }

    return Result;
}

CFrameProcessor_J1939::CFrameProcessor_J1939():m_ouFormatMsgJ1939(m_ouRefTimer)
{
    // Get hold of J1939 DIL interface
    HRESULT Result = DIL_GetInterface(J1939, (void**) &m_pouDIL_J1939);
    ASSERT(S_OK == Result);
    ASSERT(nullptr != m_pouDIL_J1939);

    m_sJ1939ProcParams.m_pILog = nullptr;
    m_sJ1939ProcParams.dwClientID = 0x0;

    // Allocate necessary amount of memory.

    m_sJ1939Data.m_unDLC = MAX_DATA_LEN_J1939;
    m_sJ1939Data.m_pbyData = new BYTE[m_sJ1939Data.m_unDLC];// For basic data object
    ASSERT(nullptr != m_sJ1939Data.m_pbyData);

    // For raw data bytes. It should be equal to the size of m_sJ1939Data
    m_pbyJ1939Data = new BYTE[m_sJ1939Data.unGetSize()];
    ASSERT(nullptr != m_pbyJ1939Data);

    USHORT Length = ushCalculateStrLen(true, MAX_DATA_LEN_J1939);
    m_sCurrFormatDat.m_pcDataHex = new char[Length];
    ASSERT(nullptr != m_sCurrFormatDat.m_pcDataHex);
    memset(m_sCurrFormatDat.m_pcDataHex, '\0', Length * sizeof(char));
    Length = ushCalculateStrLen(false, MAX_DATA_LEN_J1939);
    m_sCurrFormatDat.m_pcDataDec = new char[Length];
    ASSERT(nullptr != m_sCurrFormatDat.m_pcDataDec);
    memset(m_sCurrFormatDat.m_pcDataDec, '\0', Length * sizeof(char));
    m_sDataCopyThread.m_hActionEvent = m_ouVSEBufJ1939.hGetNotifyingEvent();
    m_bIsJ1939DataLogged = FALSE;
}

CFrameProcessor_J1939::~CFrameProcessor_J1939()
{
    DELETE_ARRAY(m_pbyJ1939Data);
    DELETE_ARRAY(m_sJ1939Data.m_pbyData);

    vEmptyLogObjArray(m_omLogListTmp);
    vEmptyLogObjArray(m_omLogObjectArray);
}

BOOL CFrameProcessor_J1939::InitInstance(void)
{
    BOOL Result = this->CFrameProcessor_Common::InitInstance();

    return Result;
}

int CFrameProcessor_J1939::ExitInstance(void)
{
    int Result = this->CFrameProcessor_Common::ExitInstance();
    m_ouVSEBufJ1939.vClearMessageBuffer();

    return Result;
}

CBaseLogObject* CFrameProcessor_J1939::CreateNewLogObj(const CString& /* omStrVersion */)
{
    CLogObjectJ1939* pLogObj = nullptr;
    CString strVersion = CString(m_sJ1939ProcParams.m_acVersion);
    if (strVersion.IsEmpty())
    {
        strVersion.Format("%d.%d.%d", VERSION_MAJOR, VERSION_MINOR, VERSION_BUILD);
        //strVersion = omStrVersion;
    }
    pLogObj = new CLogObjectJ1939(strVersion);
    return (static_cast<CBaseLogObject*> (pLogObj));
}

void CFrameProcessor_J1939::DeleteLogObj(CBaseLogObject*& pouLogObj)
{
    CLogObjectJ1939* pLogObj = static_cast<CLogObjectJ1939*> (pouLogObj);
    if (nullptr != pLogObj)
    {
        delete pLogObj;
        pouLogObj = nullptr;
    }
    else
    {
        ASSERT(false);
    }
}

void CFrameProcessor_J1939::CreateTimeModeMapping(SYSTEMTIME& CurrSysTime,
        UINT64& unAbsTime)
{
    if (nullptr != m_pouDIL_J1939)
    {
        m_pouDIL_J1939->DILIJ_GetTimeModeMapping(CurrSysTime, unAbsTime);
    }
}

void CFrameProcessor_J1939::vRetrieveDataFromBuffer(void)
{
    static INT nType = 0;
    static HRESULT Result = 0;

    while (m_ouVSEBufJ1939.GetMsgCount() > 0)
    {
        INT nSize = MAX_MSG_LEN_J1939;
        // First read the J1939 message
        Result = m_ouVSEBufJ1939.ReadFromBuffer(nType, m_pbyJ1939Data, nSize);
        if (Result == ERR_READ_MEMORY_SHORT)
        {
            CString omBuf;
            omBuf.Format(_("ERR_READ_MEMORY_SHORT  nSize: %d"), nSize);
            //AfxMessageBox(omBuf);
        }
        else if (Result == EMPTY_APP_BUFFER)
        {
            //AfxMessageBox(_T("EMPTY_APP_BUFFER"));
        }

        if (m_bLogEnabled == TRUE)
        {
            //check for new logging session
            if(m_bResetAbsTime == TRUE)
            {
                //update msg reset flag
                m_ouFormatMsgJ1939.m_bResetMsgAbsTime = m_bResetAbsTime;
                m_ouFormatMsgJ1939.m_LogSysTime = m_LogSysTime;
                m_bResetAbsTime = FALSE;
            }
            // Save it into the J1939 message structure
            m_sJ1939Data.vSetDataStream(m_pbyJ1939Data);

            // Format current J1939 frame in the necessary settings
            m_ouFormatMsgJ1939.vFormatJ1939DataMsg(&m_sJ1939Data,
                                                   &m_sCurrFormatDat, m_bExprnFlag_Log);

            USHORT ushBlocks = (USHORT) (m_omLogObjectArray.GetSize());
            for (USHORT i = 0; i < ushBlocks; i++)
            {
                CBaseLogObject* pouLogObjBase = m_omLogObjectArray.GetAt(i);
                CLogObjectJ1939* pouLogObjCon = static_cast<CLogObjectJ1939*> (pouLogObjBase);
                BOOL bIsDataLogged = pouLogObjCon->bLogData(m_sCurrFormatDat);

                if(bIsDataLogged == TRUE)
                {
                    m_bIsJ1939DataLogged = TRUE;
                }
            }
        }
    }
}


/* STARTS FUNCTIONS WHOSE LOGICS ARE IMPLEMENTED IN THIS CLASS  */
// To initialise this module
HRESULT CFrameProcessor_J1939::FPJ1_DoInitialisation(CParamLoggerJ1939* psInitParams)
{
    HRESULT hResult = S_FALSE;

    if (psInitParams != nullptr)
    {
        m_sJ1939ProcParams = *psInitParams;
        ASSERT(nullptr != m_sJ1939ProcParams.m_pILog);

        m_ouVSEBufJ1939.vClearMessageBuffer();
        //m_sDataCopyThread.m_hActionEvent = m_ouVSEBufJ1939.hGetNotifyingEvent();

        if (this->CFrameProcessor_Common::DoInitialisation() == S_OK)
        {
            if (nullptr != m_pouDIL_J1939)
            {
                hResult = m_pouDIL_J1939->DILIJ_ManageMsgBuf(MSGBUF_ADD,
                          m_sJ1939ProcParams.dwClientID, &m_ouVSEBufJ1939);
                if (S_OK != hResult)
                {
                    ASSERT(false);
                }
            }
        }
        else
        {
            ASSERT(false);
        }
    }
    else
    {
        ASSERT(false);
    }

    return hResult;
}

// To modify the filtering scheme of a logging block
HRESULT CFrameProcessor_J1939::FPJ1_ApplyFilteringScheme(USHORT ushLogBlkID,
        const SFILTERAPPLIED_J1939& sFilterObj)
{
    HRESULT hResult = S_FALSE;

    if (bIsEditingON())
    {
        if (m_omLogListTmp.GetSize() > ushLogBlkID)
        {
            CBaseLogObject* pouBaseLogObj = m_omLogListTmp.GetAt(ushLogBlkID);
            CLogObjectJ1939* pLogObj = static_cast<CLogObjectJ1939*> (pouBaseLogObj);

            if (nullptr != pLogObj)
            {
                pLogObj->SetFilterInfo(sFilterObj);
                hResult = S_OK;
            }
            else
            {
                ASSERT(false);
            }
        }
    }

    return hResult;
}

// Getter for the filtering scheme of a logging block
HRESULT CFrameProcessor_J1939::FPJ1_GetFilteringScheme(USHORT ushLogBlk,
        SFILTERAPPLIED_J1939& sFilterObj)
{
    HRESULT hResult = S_FALSE;

    CBaseLogObject* pouBaseLogObj = FindLoggingBlock(ushLogBlk);
    CLogObjectJ1939* pouLogObj = static_cast<CLogObjectJ1939*> (pouBaseLogObj);

    if (nullptr != pouLogObj)
    {
        pouLogObj->GetFilterInfo(sFilterObj);
        hResult = S_OK;
    }
    else
    {
        ASSERT(false);
    }

    return hResult;
}

/* ENDS FUNCTIONS WHOSE LOGICS ARE IMPLEMENTED IN THIS CLASS  */


// USE COMMON BASE CLASS ALIAS FUNCTIONS: START

HRESULT CFrameProcessor_J1939::FPJ1_EnableLoggingBlock(USHORT ushBlk, bool bEnable)
{
    return EnableLoggingBlock(ushBlk, bEnable);
}

HRESULT CFrameProcessor_J1939::FPJ1_EnableLogging(bool bEnable)
{
    return EnableLogging(bEnable, J1939);
}

void CFrameProcessor_J1939::FPJ1_vCloseLogFile(void)
{
    USHORT ushBlocks = (USHORT) (m_omLogObjectArray.GetSize());

    CBaseLogObject* pouCurrLogObj = nullptr;
    for (USHORT i = 0; i < ushBlocks; i++)
    {
        pouCurrLogObj = m_omLogObjectArray.GetAt(i);

        if (pouCurrLogObj != nullptr)
        {
            pouCurrLogObj->bStopOnlyLogging();
        }
    }
}

HRESULT CFrameProcessor_J1939::FPJ1_EnableFilter(USHORT ushBlk, bool bEnable)
{
    return EnableFilter(ushBlk, bEnable);
}

bool CFrameProcessor_J1939::FPJ1_IsLoggingON(void)
{
    return IsLoggingON();
}

bool CFrameProcessor_J1939::FPJ1_IsJ1939DataLogged(void)
{
    return IsJ1939DataLogged();
}

bool CFrameProcessor_J1939::FPJ1_IsJ1939ThreadBlocked(void)
{
    return IsThreadBlocked();
}

void CFrameProcessor_J1939::FPJ1_DisableJ1939DataLogFlag(void)
{
    DisableJ1939DataLogFlag();
}

// Query function - current filtering status
bool CFrameProcessor_J1939::FPJ1_IsFilterON(void)
{
    return IsFilterON();
}

// To log a string
HRESULT CFrameProcessor_J1939::FPJ1_LogString(CString& omStr)
{
    return LogString(omStr);
}

// To add a logging block; must be in editing mode
HRESULT CFrameProcessor_J1939::FPJ1_AddLoggingBlock(const SLOGINFO& sLogObject)
{
    return AddLoggingBlock(sLogObject);
}

// To remove a logging block by its index in the list; editing mode prerequisite
HRESULT CFrameProcessor_J1939::FPJ1_RemoveLoggingBlock(USHORT ushBlk)
{
    return RemoveLoggingBlock(ushBlk);
}

// Getter for total number of logging blocks
USHORT CFrameProcessor_J1939::FPJ1_GetLoggingBlockCount(void)
{
    return GetLoggingBlockCount();
}

// To clear the logging block list
HRESULT CFrameProcessor_J1939::FPJ1_ClearLoggingBlockList(void)
{
    return ClearLoggingBlockList();
}

// Getter for a logging block by specifying its index in the list
HRESULT CFrameProcessor_J1939::FPJ1_GetLoggingBlock(USHORT ushBlk, SLOGINFO& sLogObject)
{
    return GetLoggingBlock(ushBlk, sLogObject);
}

// Setter for a logging block by specifying its index in the list
HRESULT CFrameProcessor_J1939::FPJ1_SetLoggingBlock(USHORT ushBlk,
        const SLOGINFO& sLogObject)
{
    return SetLoggingBlock(ushBlk, sLogObject);
}

// To reset or revoke the modifications made
HRESULT CFrameProcessor_J1939::FPJ1_Reset(void)
{
    return Reset();
}

// To confirm the modifications made
HRESULT CFrameProcessor_J1939::FPJ1_Confirm(void)
{
    return Confirm();
}

// To start logging block editing session
HRESULT CFrameProcessor_J1939::FPJ1_StartEditingSession(void)
{
    return StartEditingSession();
}

// To stop logging block editing session
HRESULT CFrameProcessor_J1939::FPJ1_StopEditingSession(bool bConfirm)
{
    return StopEditingSession(bConfirm);
}

// Getter for the logging configuration data
HRESULT CFrameProcessor_J1939::FPJ1_GetConfigData(BYTE** ppvConfigData, UINT& unLength)
{
    return GetConfigData(ppvConfigData, unLength);
}

// Getter for the logging configuration data
HRESULT CFrameProcessor_J1939::FPJ1_GetConfigData(xmlNodePtr pNodePtr)
{
    return GetConfigData(pNodePtr);
}

// Setter for the logging configuration data
HRESULT CFrameProcessor_J1939::FPJ1_SetConfigData(BYTE* pvDataStream, const CString& omStrVersion)
{
    return SetConfigData(pvDataStream, omStrVersion);
}

HRESULT CFrameProcessor_J1939::FPJ1_SetConfigData(xmlDocPtr pDoc)
{
    return SetConfigData(pDoc, J1939);
}

// Empty log object
void CFrameProcessor_J1939::vEmptyLogObjArray(CLogObjArray& omLogObjArray)
{
    USHORT ushBlocks = (USHORT) (omLogObjArray.GetSize());

    if (ushBlocks > 0)
    {
        for (USHORT i = 0; i < ushBlocks; i++)
        {
            CBaseLogObject* pouCurrLogObj = omLogObjArray.GetAt(i);
            DeleteLogObj(pouCurrLogObj);
        }
    }
    omLogObjArray.RemoveAll();
}

//Setter for database files associated
HRESULT CFrameProcessor_J1939::FPJ1_SetDatabaseFiles(const CStringArray& omList)
{
    return SetDatabaseFiles(omList);
}

// To update the channel baud rate info to logger
HRESULT CFrameProcessor_J1939::FPJ1_SetChannelBaudRateDetails
(SCONTROLLER_DETAILS* controllerDetails,
 int nNumChannels,ETYPE_BUS eBus)
{
    HRESULT hResult = S_OK;
    SetChannelBaudRateDetails(controllerDetails, nNumChannels,eBus);
    return hResult;
}

void CFrameProcessor_J1939::vSetMeasurementFileName()
{
    USHORT ushBlocks = (USHORT) (m_omLogObjectArray.GetSize());
    for (USHORT i = 0; i < ushBlocks; i++)
    {
        CBaseLogObject* pouLogObjBase = m_omLogObjectArray.GetAt(i);
        CLogObjectJ1939* pouLogObjCon = static_cast<CLogObjectJ1939*> (pouLogObjBase);
        pouLogObjCon->m_sLogInfo.m_sLogAdvStngs.m_nConnectionCount++;
        pouLogObjCon->vSetMeasurementFileName();
    }
}
