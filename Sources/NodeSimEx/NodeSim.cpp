/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * \file      NodeSim.cpp
 * \author    Ratnadip Choudhury
 * \copyright Copyright (c) 2011, Robert Bosch Engineering and Business Solutions. All rights reserved.
 */
// NodeSimEx.cpp : Defines the initialization routines for the DLL.
//
#include "NodeSimEx_stdafx.h"
#include "Utility/XMLUtils.h"
#include "include/BaseDefs.h"
#include "GlobalObj.h"
#include "ExecuteManager.h"
#include "EditFrameWnd.h"
#include "SimSysManager.h"
#include "Utility/Utility.h"

#include "DIL_Interface/DIL_Interface_extern.h"
#include "NodeSim.h"

#define defSTR_SIMSYS_WINDOW_TITLE              "Configure Simulated Systems"

ETYPE_BUS CEditFrameWnd::sm_eBus = CAN;
ETYPE_BUS CFileView::sm_eBus = CAN;

CNodeSim::CNodeSim(ETYPE_BUS eBus)
{
    m_eBus = eBus;
}
CNodeSim::~CNodeSim()
{

}
int CNodeSim::ExitInstance(void)
{
    //All the memory release should happen here
    if ( CGlobalObj::ouGetObj(m_eBus).m_pEditorDocTemplate != nullptr)
    {
        delete CGlobalObj::ouGetObj(m_eBus).m_pEditorDocTemplate;
        CGlobalObj::ouGetObj(m_eBus).m_pEditorDocTemplate = nullptr;
    }
    CGlobalObj::ouGetObj(m_eBus).m_ArrAPIsList.RemoveAll();
    CGlobalObj::sm_hWndMDIParentFrame = nullptr;
    CGlobalObj::ouGetObj(m_eBus).m_omStrSourceFilePathName.Empty();

    if ( CGlobalObj::ouGetObj(m_eBus).m_pSimSysDataPtr != nullptr)
    {
        delete[] CGlobalObj::ouGetObj(m_eBus).m_pSimSysDataPtr;
        CGlobalObj::ouGetObj(m_eBus).m_pSimSysDataPtr = nullptr;
    }
    //remove the singleton class objects
    CSimSysManager::vClearObj(m_eBus);
    CExecuteManager::vClearObj(m_eBus);
    return 0;
}

HRESULT CNodeSim::FE_CreateFuncEditorTemplate(HWND handle, S_EXFUNC_PTR& sExInitStruct)
{
    //AFX_MANAGE_STATE(AfxGetStaticModuleState());
    if (CGlobalObj::ouGetObj(m_eBus).m_pEditorDocTemplate == nullptr)
    {
        CEditFrameWnd::sm_eBus = m_eBus;
        CFileView::sm_eBus = m_eBus;
        CGlobalObj::ouGetObj(m_eBus).m_pEditorDocTemplate = new CMultiDocTemplate(IDI_ICON_FN_EDITOR,

                RUNTIME_CLASS(CFunctionEditorDoc),
                RUNTIME_CLASS(CEditFrameWnd), // custom MDI child frame
                RUNTIME_CLASS(CFileView));
        //AddDocTemplate(CGlobalObj::m_pEditorDocTemplate);
    }
    CGlobalObj::sm_hWndMDIParentFrame = handle;
    NS_UpdateFuncStructsNodeSimEx((PVOID)&sExInitStruct, UPDATE_ALL);

    CFrameWnd* pParent = (CFrameWnd*)CWnd::FromHandle(CGlobalObj::sm_hWndMDIParentFrame);
    // Get Window rectangle from configuration module
    if ((CGlobalObj::ouGetObj(m_eBus).m_wWindowPlacement.length == 0) ||
            (CGlobalObj::ouGetObj(m_eBus).m_wWindowPlacement.rcNormalPosition.top == -1))
    {
        CGlobalObj::ouGetObj(m_eBus).bGetDefaultValue(SIMSYS_WND_PLACEMENT, CGlobalObj::ouGetObj(m_eBus).m_wWindowPlacement);
    }
    // Check for window pointer
    if( CGlobalObj::ouGetObj(m_eBus).m_pomSimSysWnd == nullptr )
    {
        // Create New Instance
        CGlobalObj::ouGetObj(m_eBus).m_pomSimSysWnd = new CSimSysWnd(m_eBus);
        if( CGlobalObj::ouGetObj(m_eBus).m_pomSimSysWnd != nullptr )
        {
            // Register Window Class
            LPCTSTR strMDIClass = AfxRegisterWndClass(
                                      CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS,
                                      LoadCursor(nullptr, IDC_CROSS), 0,
                                      AfxGetApp()->LoadIcon(IDI_ICON_FN_EDITOR) );
            // Set the size got from configuration module
            CRect omRect(CGlobalObj::ouGetObj(m_eBus).m_wWindowPlacement.rcNormalPosition);
            // Create Simulated system Configuration window
            CString omTitle;
            omTitle.Format("%s - %s", defSTR_SIMSYS_WINDOW_TITLE, CGlobalObj::omGetBusName(m_eBus));
            if( CGlobalObj::ouGetObj(m_eBus).m_pomSimSysWnd->Create( strMDIClass,
                    omTitle.GetBuffer(MAX_PATH),
                    WS_CHILD | WS_OVERLAPPEDWINDOW,
                    omRect, (CMDIFrameWnd*)pParent ) == TRUE )
            {

                // Show window and set focus
                CGlobalObj::ouGetObj(m_eBus).m_pomSimSysWnd->ShowWindow( SW_SHOW );
                CGlobalObj::ouGetObj(m_eBus).m_pomSimSysWnd->SetFocus();
                if ((CGlobalObj::ouGetObj(m_eBus).m_wWindowPlacement.rcNormalPosition.bottom == 0) &&
                        (CGlobalObj::ouGetObj(m_eBus).m_wWindowPlacement.rcNormalPosition.left   == 0) &&
                        (CGlobalObj::ouGetObj(m_eBus).m_wWindowPlacement.rcNormalPosition.right  == 0) &&
                        (CGlobalObj::ouGetObj(m_eBus).m_wWindowPlacement.rcNormalPosition.top    == 0))
                {
                    //Propably new configuration
                    CGlobalObj::ouGetObj(m_eBus).bGetDefaultValue(SIMSYS_WND_PLACEMENT, CGlobalObj::ouGetObj(m_eBus).m_wWindowPlacement);
                }
                CGlobalObj::ouGetObj(m_eBus).m_pomSimSysWnd->m_wWndPlacement = CGlobalObj::ouGetObj(m_eBus).m_wWindowPlacement;
                CGlobalObj::ouGetObj(m_eBus).m_pomSimSysWnd->SetWindowPlacement
                (&(CGlobalObj::ouGetObj(m_eBus).m_pomSimSysWnd->m_wWndPlacement));
                CGlobalObj::ouGetObj(m_eBus).m_pomSimSysWnd->ShowWindow( SW_RESTORE );

            }
        }
    }
    // If already exist then activate and set the focus
    else
    {
        CGlobalObj::ouGetObj(m_eBus).m_pomSimSysWnd->ShowWindow( SW_RESTORE );
        CGlobalObj::ouGetObj(m_eBus).m_pomSimSysWnd->MDIActivate();
        CGlobalObj::ouGetObj(m_eBus).m_pomSimSysWnd->SetActiveWindow();
    }
    return S_OK;
}



void CNodeSim::NS_ManageOnKeyHandler(UCHAR ucKey)
{
    CExecuteManager::ouGetExecuteManager(m_eBus).vManageOnKeyHandler(ucKey);
}

void CNodeSim::NS_ManageBusEventHandler(eBUSEVEHANDLER eBusEvent)
{
    CExecuteManager::ouGetExecuteManager(m_eBus).vManageBusEventHandler(eBusEvent);
}

void CNodeSim::NS_ManageOnMessageHandler(void* psRxMsgInfo)
{
    STCAN_TIME_MSG* psRxMsg = (STCAN_TIME_MSG*)psRxMsgInfo;
    DWORD ix = CAN_MONITOR_CLIENT_ID;
    CExecuteManager::ouGetExecuteManager(m_eBus).vManageOnMessageHandlerCAN_(psRxMsg, ix);
}

void CNodeSim::NS_ManageOnErrorHandler(eERROR_STATE eErrorCode,void* pvErrorVal)
{
    SCAN_ERR* psCanErr = (SCAN_ERR*)pvErrorVal;
    CExecuteManager::ouGetExecuteManager(m_eBus).vManageOnErrorHandlerCAN(eErrorCode, *psCanErr, 0);
}
HRESULT CNodeSim::NS_DLLBuildAll()
{
    return CExecuteManager::ouGetExecuteManager(m_eBus).
           bDLLBuildAll();
}
HRESULT CNodeSim::NS_DllUnloadAll(CStringArray* pomStrErrorFiles)
{
    return CExecuteManager::ouGetExecuteManager(m_eBus).
           bDllUnloadAll(pomStrErrorFiles);
}

HRESULT CNodeSim::NS_DLLBuildLoadAllEnabled()
{
    return CExecuteManager::ouGetExecuteManager(m_eBus).
           bDLLBuildLoadAllEnabled();
}
HRESULT CNodeSim::NS_DLLUnloadAllEnabled()
{
    return CExecuteManager::ouGetExecuteManager(m_eBus).
           bDLLUnloadAllEnabled();
}
void CNodeSim::NS_SetHandlersHelpText(CStringArray* pomTextArray)
{
    CGlobalObj::ouGetObj(m_eBus).m_omFnEditHelpText.RemoveAll();
    CGlobalObj::ouGetObj(m_eBus).m_omFnEditHelpText.Append(*pomTextArray);
}

void CNodeSim::NS_UpdateFuncStructsNodeSimEx(PVOID pvFuncStructs, E_UPDATE_TYPE eType)
{
    if (pvFuncStructs != nullptr)
    {
        switch (eType)
        {
            case UPDATE_ALL:
            {
                S_EXFUNC_PTR* psExInitStruct = (S_EXFUNC_PTR*)pvFuncStructs;
                CGlobalObj::sm_hWndMDIParentFrame = psExInitStruct->m_hWmdMDIParentFrame;
                //CGlobalObj::g_pouTraceWnd = psExInitStruct->m_pouTraceWnd;
                //CGlobalObj::g_pNetwork_McNet = psExInitStruct->m_pNetWorkMcNet;
                CGlobalObj::ouGetObj(m_eBus).m_ArrAPIsList.RemoveAll();
                CGlobalObj::ouGetObj(m_eBus).m_ArrAPIsList.Append(psExInitStruct->m_omAPIList);
                CGlobalObj::ouGetObj(m_eBus).m_ArrAPIFuncNames.RemoveAll();
                CGlobalObj::ouGetObj(m_eBus).m_ArrAPIFuncNames.Append(psExInitStruct->m_omAPINames);
                CGlobalObj::ouGetObj(m_eBus).m_omObjWrapperName = psExInitStruct->m_omObjWrapperName;
                CGlobalObj::ouGetObj(m_eBus).m_omMsgStructName = psExInitStruct->m_omStructName;
                CGlobalObj::ouGetObj(m_eBus).m_omMsgStructFile = psExInitStruct->m_omStructFile;
                CGlobalObj::ouGetObj(m_eBus).m_omDefinedMsgHeaders.RemoveAll();
                CGlobalObj::ouGetObj(m_eBus).m_omDefinedMsgHeaders.Copy(psExInitStruct->m_omDefinedMsgHeaders);
                CGlobalObj::ouGetObj(m_eBus).m_odMsgNameMsgCodeListDb.RemoveAll();      //CAPL_DB_NAME_CHANGE
                CGlobalObj::ouGetObj(m_eBus).m_odMsgNameMsgCodeListDb.AddTail(&(psExInitStruct->m_odMsgNameMsgCodeListDB));
                CGlobalObj::ouGetObj(m_eBus).m_omErrorHandlerList.RemoveAll();
                CGlobalObj::ouGetObj(m_eBus).m_omErrorHandlerList.Copy(psExInitStruct->m_omErrorHandlerList);
                //CGlobalObj::g_podNodeToDllMap = psExInitStruct->m_podNodeToDllMap;
                //CGlobalObj::g_podKeyPanelEntryList = psExInitStruct->m_podKeyPanelEntryList;
                CGlobalObj::ouGetObj(m_eBus).m_omAppDirectory = psExInitStruct->m_omAppDirectory;
                //CGlobalObj::TransmitMsg        = psExInitStruct->Send_Msg;
                CGlobalObj::ouGetObj(m_eBus).m_pfEnableDisableLog = psExInitStruct->EnDisableLog;
                CGlobalObj::ouGetObj(m_eBus).m_pfDisconnectTool   = psExInitStruct->DisConnectTool;
                CGlobalObj::ouGetObj(m_eBus).m_pfWriteToLog       = psExInitStruct->WriteToLog;
                CGlobalObj::sm_pouITraceWndPtr  = psExInitStruct->m_pouITraceWndPtr;
            }
            break;
            case UPDATE_DATABASE_MSGS:
            {
                //CAPL_DB_NAME_CHANGE
                CMsgNameMsgCodeListDataBase* podMsgNameCodeListDb = (CMsgNameMsgCodeListDataBase*)pvFuncStructs;
                CGlobalObj::ouGetObj(m_eBus).m_odMsgNameMsgCodeListDb.RemoveAll();
                CGlobalObj::ouGetObj(m_eBus).m_odMsgNameMsgCodeListDb.AddTail(podMsgNameCodeListDb);
            }
            break;
            case UPDATE_UNIONS_HEADER_FILES:
            {
                CStringArray* omDefMsgHeadrs = (CStringArray*)pvFuncStructs;
                CGlobalObj::ouGetObj(m_eBus).m_omDefinedMsgHeaders.RemoveAll();
                CGlobalObj::ouGetObj(m_eBus).m_omDefinedMsgHeaders.Copy(*omDefMsgHeadrs);
            }
            break;
        }
    }
}

void CNodeSim::NS_SetJ1939ActivationStatus(bool bActivated)
{
    CGlobalObj::ouGetObj(m_eBus).bJ1939Activated  = bActivated;
}

void CNodeSim::NS_GetSimSysConfigData(BYTE*& pDesBuffer, int& nBuffSize)
{
    //First update pSimSysDataPtr
    CSimSysManager::ouGetSimSysManager(m_eBus).vSaveSimSysWndConfig();
    if (nBuffSize < CGlobalObj::ouGetObj(m_eBus).m_nSimSysDataSize)
    {
        if (pDesBuffer != nullptr)
        {
            delete[] pDesBuffer;
        }
        nBuffSize = CGlobalObj::ouGetObj(m_eBus).m_nSimSysDataSize;
        pDesBuffer = new BYTE[nBuffSize];
    }
    memcpy (pDesBuffer, CGlobalObj::ouGetObj(m_eBus).m_pSimSysDataPtr, nBuffSize);
}

bool CNodeSim::NS_GetSimSysConfigData(xmlNodePtr& pNodePtr)
{
    if( m_eBus == J1939 )
    {
        pNodePtr = xmlNewNode(nullptr, BAD_CAST DEF_J1939_SIM_SYS);
    }
    else if( m_eBus == LIN )
    {
        pNodePtr = xmlNewNode(nullptr, BAD_CAST DEF_LIN_SIM_SYS);
    }
    else
    {
        pNodePtr = xmlNewNode(nullptr, BAD_CAST DEF_CAN_SIM_SYS);
    }

    CSimSysManager::ouGetSimSysManager(m_eBus).bGetConfigData(pNodePtr);
    return true;
}
void CNodeSim::NS_SetSimSysConfigData(BYTE* pSrcBuffer, int nBuffSize)
{
    if ( CGlobalObj::ouGetObj(m_eBus).m_pSimSysDataPtr != nullptr)
    {
        delete[] CGlobalObj::ouGetObj(m_eBus).m_pSimSysDataPtr;
        CGlobalObj::ouGetObj(m_eBus).m_pSimSysDataPtr = nullptr;
    }
    if (nBuffSize > 0)
    {
        CGlobalObj::ouGetObj(m_eBus).m_pSimSysDataPtr = new BYTE[nBuffSize];
        memcpy(CGlobalObj::ouGetObj(m_eBus).m_pSimSysDataPtr, pSrcBuffer, nBuffSize);
        CGlobalObj::ouGetObj(m_eBus).m_nSimSysDataSize = nBuffSize;
    }

    //Update Internal Data
    CSimSysManager::ouGetSimSysManager(m_eBus).vLoadSimSysWndConfig();
    if(pSrcBuffer == nullptr)
    {
        if(CGlobalObj::ouGetObj(m_eBus).m_pomSimSysWnd != nullptr)
        {
            CGlobalObj::ouGetObj(m_eBus).m_pomSimSysWnd->ShowWindow( SW_HIDE );
            CGlobalObj::ouGetObj(m_eBus).bCloseFunctionEditorFrame();
        }
    }
}

void CNodeSim::NS_SetSimSysConfigData(xmlDocPtr pXmlDoc)
{
    if ( CGlobalObj::ouGetObj(m_eBus).m_pSimSysDataPtr != nullptr)
    {
        delete[] CGlobalObj::ouGetObj(m_eBus).m_pSimSysDataPtr;
        CGlobalObj::ouGetObj(m_eBus).m_pSimSysDataPtr = nullptr;
    }
    if (pXmlDoc != nullptr)
    {
        /*CGlobalObj::ouGetObj(m_eBus).m_pSimSysDataPtr = new BYTE[nBuffSize];
        memcpy(CGlobalObj::ouGetObj(m_eBus).m_pSimSysDataPtr, pSrcBuffer, nBuffSize);
        CGlobalObj::ouGetObj(m_eBus).m_nSimSysDataSize = nBuffSize;*/
        CSimSysManager::ouGetSimSysManager(m_eBus).vLoadSimSysWndConfig(pXmlDoc, m_eBus);

        /* Close the previosly opened function editor view */
        if(CGlobalObj::ouGetObj(m_eBus).m_pomSimSysWnd != nullptr)
        {
            CGlobalObj::ouGetObj(m_eBus).m_pomSimSysWnd->ShowWindow( SW_HIDE );
            CGlobalObj::ouGetObj(m_eBus).bCloseFunctionEditorFrame();
        }
    }
    //Update Internal Data

}

void CNodeSim::NS_SetSimSysConfigData(xmlNodePtr pXmlNode)
{
    if ( CGlobalObj::ouGetObj(m_eBus).m_pSimSysDataPtr != nullptr)
    {
        delete[] CGlobalObj::ouGetObj(m_eBus).m_pSimSysDataPtr;
        CGlobalObj::ouGetObj(m_eBus).m_pSimSysDataPtr = nullptr;
    }
    if (pXmlNode != nullptr)
    {
        /*CGlobalObj::ouGetObj(m_eBus).m_pSimSysDataPtr = new BYTE[nBuffSize];
        memcpy(CGlobalObj::ouGetObj(m_eBus).m_pSimSysDataPtr, pSrcBuffer, nBuffSize);
        CGlobalObj::ouGetObj(m_eBus).m_nSimSysDataSize = nBuffSize;*/
        CSimSysManager::ouGetSimSysManager(m_eBus).vSetConfigData(pXmlNode);
    }
    //Update Internal Data


}
int  CNodeSim::NS_nOnBusConnected(bool bConnected)
{
    if(bConnected)
    {
        m_n64TimeElapsedSinceConnection = gnGetCpuClocks();
    }
    else
    {
        m_n64TimeElapsedSinceConnection = 0;
    }
    return TRUE;
}
BOOL CNodeSim::NS_IsSimSysConfigChanged()
{
    return CSimSysManager::ouGetSimSysManager(m_eBus).bIsConfigChanged();
}

HRESULT CNodeSim::FE_OpenFunctioneditorFile(CString omStrNewCFileName, HWND hMainFrame, S_EXFUNC_PTR& sExInitStruct)
{
    if (CGlobalObj::ouGetObj(m_eBus).m_pEditorDocTemplate == nullptr)
    {
        CGlobalObj::ouGetObj(m_eBus).m_pEditorDocTemplate = new CMultiDocTemplate(IDI_ICON_FN_EDITOR,

                RUNTIME_CLASS(CFunctionEditorDoc),
                RUNTIME_CLASS(CEditFrameWnd), // custom MDI child frame
                RUNTIME_CLASS(CFileView));
        //AddDocTemplate(CGlobalObj::m_pEditorDocTemplate);
    }
    CGlobalObj::sm_hWndMDIParentFrame = hMainFrame;
    NS_UpdateFuncStructsNodeSimEx((PVOID)&sExInitStruct, UPDATE_ALL);

    return CGlobalObj::ouGetObj(m_eBus).bOpenFunctioneditorfile(omStrNewCFileName) ? S_OK : S_FALSE;
}

BOOL CNodeSim::InitInstance(void)
{
    return TRUE;
}

void CNodeSim::NS_SetLINConfig(ClusterConfig& ouLINConfig)
{
    CSimSysManager::ouGetSimSysManager(m_eBus).vSetDatabaseConfiguration(&ouLINConfig);
}