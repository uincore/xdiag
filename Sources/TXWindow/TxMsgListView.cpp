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
 * @brief Implementation file for CTxMsgListView class
 * @author Raja N
 * @copyright Copyright (c) 2011, Robert Bosch Engineering and Business Solutions. All rights reserved.
 *
 * Implementation file for CTxMsgListView class
 */

#include "TxWindow_stdafx.h"             // For standard include
#include "TxWindow_resource.h"
#include "Utility/SignalMatrix.h"       // For Signal Matrix Class Definition
#include "Utility/ComboItem.h"          // For Custom Combobox Implementation
#include "Utility/EditItem.h"           // For Custom Editbox Implementation
#include "Utility/RadixEdit.h"          // For the RAdix Edit control definition
#include "Utility/NumSpinCtrl.h"        // For the custom spin control
#include "Utility/NumEdit.h"            // For Custom Numeric Edit control Impl
#include "Utility/FFListctrl.h"         // For Flicker Free List class definition
#include "FlexListCtrl.h"       // For editable list control implementation
#include "TxMsgListView.h"      // For CTxMsgListView class definition
#include "TxMsgChildFrame.h"    // For Tx Child Window definition
#include "Utility/MultiLanguageSupport.h"

/** For global stop flag for Message Transmission */
extern BOOL g_bStopSelectedMsgTx;

IMPLEMENT_DYNCREATE(CTxMsgListView, CFormView)

CTxMsgListView::CTxMsgListView()
    : CFormView(CTxMsgListView::IDD)
{
    m_nSelectedMsgIndex = -1;
    m_bInitDlg = FALSE;
}

CTxMsgListView::~CTxMsgListView()
{
}

void CTxMsgListView::DoDataExchange(CDataExchange* pDX)
{
    CFormView::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LSTC_MSG_DETAILS, m_omLctrMsgList);
    DDX_Control(pDX, IDC_CBTN_DELETE_ALL_MSG, m_omButtonDeleteAllMsg);
    DDX_Control(pDX, IDC_CBTN_DELETE_SEL_MSG, m_omButtonDeleteSelMsg);
    DDX_Control(pDX, IDC_CBTN_SEND_MSG, m_omButtonSendMsg);
}

BEGIN_MESSAGE_MAP(CTxMsgListView, CFormView)
    ON_NOTIFY(LVN_COLUMNCLICK, IDC_LSTC_MSG_DETAILS, OnColumnclickLstcMsgDetails)
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_LSTC_MSG_DETAILS, OnItemchangedLstcMsgDetails)
    ON_BN_CLICKED(IDC_CBTN_SEND_MSG, OnSendSelectedMsg)
    ON_BN_CLICKED(IDC_CBTN_DELETE_SEL_MSG, OnDeleteSelectedMsg)
    ON_BN_CLICKED(IDC_CBTN_DELETE_ALL_MSG, OnDeleteAllMsg)
    ON_COMMAND(IDM_DELETE_ALL_MSG, OnDeleteAllMsg)
    ON_COMMAND(IDM_SEND_SEL_MSG, OnSendSelectedMsg)
    ON_COMMAND(IDM_DELETE_SEL_MSG, OnDeleteSelectedMsg)
    ON_NOTIFY(NM_RCLICK, IDC_LSTC_MSG_DETAILS, OnRightClickMsgDetails)
END_MESSAGE_MAP()

#ifdef _DEBUG
void CTxMsgListView::AssertValid() const
{
    CFormView::AssertValid();
}

void CTxMsgListView::Dump(CDumpContext& dc) const
{
    CFormView::Dump(dc);
}
#endif //_DEBUG


void CTxMsgListView::OnInitialUpdate()
{
    CFormView::OnInitialUpdate();
    // Initialise window pointer in the Tx child window
    CTxMsgChildFrame* pomChildFrame =
        (CTxMsgChildFrame* )pomGetParentWindow();
    // Update View Pointer
    if( pomChildFrame != nullptr )
    {
        pomChildFrame->vSetTxMsgViewPointers( eTxMsgMessageListView, this );
    }
    // set Init flag to TRUE
    m_bInitDlg = TRUE;

    // Init Message List Control
    CRect rListCtrlRect;
    CHAR caColumnName[defMESSAGE_FRAME_COLUMN][defSTRING_SIZE] =
    {
        defMESSAGE_ID,
        defSTR_CHANNEL_NAME,
        defMESSAGE_TYPE,
        defMESSSAGE_DLC,
        defMESSAGE_DATA_BYTES
    };
    //Calculate the total size of all column header
    m_omLctrMsgList.GetWindowRect( &rListCtrlRect);
    int nTotalColunmSize     = rListCtrlRect.right - rListCtrlRect.left;
    int nTotalStrLengthPixel = 0;

    int i;  //i declared outside the for loop
    for( i=0; i<defMESSAGE_FRAME_COLUMN; i++)
    {
        nTotalStrLengthPixel +=
            m_omLctrMsgList.GetStringWidth(caColumnName[i]) ;
    }
    //Insert each column name after calculating the size for the same.
    INT nFormat = 0;
    for(i=0; i<defMESSAGE_FRAME_COLUMN; i++)
    {
        int nColumnSize  = m_omLctrMsgList.GetStringWidth(_(caColumnName[i])) ;
        nColumnSize +=
            (nTotalColunmSize-nTotalStrLengthPixel)/defMESSAGE_FRAME_COLUMN;
        nFormat = LVCFMT_CENTER;
        // Switch Column Index
        switch( i )
        {
            case defMESSAGE_FRAME_COLUMN - 1 : // Data Bytes Column
                nColumnSize += static_cast <INT>(4.25*defDATA_BYTES_EXTRA);
                nFormat = LVCFMT_LEFT;
                break;
            case 0: // Message ID / Name Column. Don't alter this column
                break;
            case 1: // Channels Column
                nColumnSize -= static_cast <INT>(2.2*defDATA_BYTES_EXTRA);
                break;
            default: // Others
                nColumnSize -= static_cast <INT>(1.1*defDATA_BYTES_EXTRA );
        }
        // Insert the column in to the list
        m_omLctrMsgList.InsertColumn(i,_(caColumnName[i]),
                                     nFormat, nColumnSize);
    }
    // Set extended property
    // Enable Check box
    m_omLctrMsgList.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES );

    // Associate image list to the list item
    // Create only onece
    if( m_omImageList.m_hImageList == nullptr )
    {
        m_omImageList.Create( IDR_BMP_MSGSGDB,
                              defSIGNAL_ICON_SIZE,
                              1,
                              WHITE_COLOR );
    }
    // Set the Image List
    // Only if it is sucessfully created
    if( m_omImageList.m_hImageList != nullptr )
    {
        m_omLctrMsgList.SetImageList( &m_omImageList, LVSIL_SMALL);
    }
    // Associate Header control Image List
    if( m_omHeaderImageList.m_hImageList == nullptr )
    {
        m_omHeaderImageList.Create( IDR_BMP_CHECKBOX,
                                    defSIGNAL_ICON_SIZE,
                                    1,
                                    BLUE_COLOR );
    }
    // Set the Image List
    // Only if it is sucessfully created
    if( m_omHeaderImageList.m_hImageList != nullptr )
    {
        CHeaderCtrl* pHeader = m_omLctrMsgList.GetHeaderCtrl();
        if( pHeader != nullptr )
        {
            pHeader->SetImageList( &m_omHeaderImageList );
            HDITEM hditem;
            hditem.mask = HDI_IMAGE | HDI_FORMAT;
            if( pHeader->GetItem(0, &hditem ) == TRUE )
            {
                hditem.fmt |=  HDF_IMAGE;
                hditem.iImage = 0;
                pHeader->SetItem(0, &hditem );
            }
        }
    }

    m_omButtonSendMsg.EnableWindow(FALSE);
    m_omButtonDeleteSelMsg.EnableWindow(FALSE);

    // Set the selection to the first item
    m_omLctrMsgList.SetItemState( 0,
                                  LVIS_SELECTED | LVIS_FOCUSED,
                                  LVIS_SELECTED | LVIS_FOCUSED );
    m_bInitDlg = FALSE;
}

void CTxMsgListView::OnColumnclickLstcMsgDetails(NMHDR* pNMHDR, LRESULT* pResult)
{
    NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
    // If it is first column
    if( pNMListView->iSubItem == 0 )
    {
        // Get the status from Image Index
        BOOL bToBeChecked = FALSE;
        // Change header Control Image Index
        CHeaderCtrl* pHeader = m_omLctrMsgList.GetHeaderCtrl();
        if( pHeader != nullptr )
        {
            // Get Current Image Index
            HDITEM hditem;
            hditem.mask = HDI_IMAGE | HDI_FORMAT;
            if( pHeader->GetItem(0, &hditem ) == TRUE )
            {
                // Image Index To be Set
                int nNewImageIndex = 0;
                // Toggle Image Index
                if( hditem.iImage == 0 )
                {
                    bToBeChecked = TRUE;
                    nNewImageIndex = 1;
                }

                // Update Image Index
                hditem.fmt |=  HDF_IMAGE;
                hditem.iImage = nNewImageIndex;
                pHeader->SetItem(0, &hditem );
                // Update Message Check Value
                vSetMessageCheckValue( bToBeChecked );

                CTxFunctionsView* pView = (CTxFunctionsView*)
                                          pomGetFunctionsViewPointer();

                if( pView != nullptr )
                {
                    if(pView->m_CheckBoxAutoUpdate.GetCheck() == BST_CHECKED)
                    {
                        pView->vAccessButtonApply();
                        this->SetFocus();
                    }
                }
            }
        }
    }
    *pResult = 0;
}

void CTxMsgListView::vSetMessageCheckValue(BOOL bCheck)
{
    // Get Other View Pointers
    // Blocks View
    CTxMsgBlocksView* pomBlocksView = nullptr;
    pomBlocksView = ( CTxMsgBlocksView*)pomGetBlocksViewPointer();
    // Message Details View
    CTxMsgDetailsView* pomDetailsView = nullptr;
    pomDetailsView = (CTxMsgDetailsView*)pomGetDetailsViewPointer();
    // Functions view
    CTxFunctionsView* pomFunctionsView = nullptr;
    pomFunctionsView = (CTxFunctionsView*)pomGetFunctionsViewPointer();
    // If all pointers are valid
    if( pomBlocksView != nullptr && pomDetailsView != nullptr &&
            pomFunctionsView != nullptr )
    {
        // If selected message block index is valid
        if(pomBlocksView->m_nSelectedMsgBlockIndex != -1 )
        {
            PSMSGBLOCKLIST psCurrentMsgBlock = nullptr;
            // Get current block pointer
            psCurrentMsgBlock = pomBlocksView->psGetMsgBlockPointer(
                                    pomBlocksView->m_nSelectedMsgBlockIndex,
                                    pomBlocksView->m_psMsgBlockList );
            psCurrentMsgBlock->m_bModified = true;
            // Get the message list
            PSTXCANMSGLIST psMsgList = psCurrentMsgBlock->m_psTxCANMsgList;
            int nIndex = 0;
            // Set programmed UI update to TRUE
            BOOL bModified = FALSE;
            m_bInitDlg = TRUE;

            // Update check box status of all messages in this list
            while(psMsgList != nullptr )
            {
                if( psMsgList->m_sTxMsgDetails.m_bEnabled != bCheck )
                {
                    // Update message list
                    psMsgList->m_sTxMsgDetails.m_bEnabled = bCheck;
                    psMsgList->m_bModified = true;
                    // Update UI Control
                    m_omLctrMsgList.SetCheck( nIndex, bCheck );
                    // Update Modified flag
                    bModified = TRUE;
                }
                // Go to Next Node
                psMsgList = psMsgList->m_psNextMsgDetails;
                // Increment list item index count
                nIndex++;
            }
            // Set programmed UI update to FALSE
            m_bInitDlg = FALSE;
            // Enable Update button only if data got modified
            if( bModified == TRUE )
            {
                // If data is modified then update apply button
                if(pomFunctionsView->m_CheckBoxAutoUpdate.GetCheck() == BST_UNCHECKED)
                {
                    pomFunctionsView->m_omButtonApply.EnableWindow();
                }
            }
        }
    }
}

void CTxMsgListView::OnItemchangedLstcMsgDetails(NMHDR* pNMHDR,
        LRESULT* pResult)
{
    NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

    CTxMsgBlocksView* pomBlocksView = nullptr;
    pomBlocksView = ( CTxMsgBlocksView*)pomGetBlocksViewPointer();
    if( pomBlocksView != nullptr && m_bInitDlg == FALSE )
    {
        // Get new state of the list control items
        // and check for if it is selected and focused.
        UINT unCurrentState = pNMListView->uNewState & defMASK_LIST_CTRL;
        if(pNMListView->uNewState & (LVIS_FOCUSED|LVIS_SELECTED) )
        {
            if((TRUE == CTxMsgManager::s_TxFlags.nGetFlagStatus(TX_CONNECTED)) && (pNMListView->uNewState & LVIS_SELECTED))
                //&& (pNMListView->uNewState&LVIS_FOCUSED))
            {
                m_omButtonSendMsg.EnableWindow(TRUE);
            }
            if(FALSE == CTxMsgManager::s_TxFlags.nGetFlagStatus(TX_SENDMESG))
            {
                m_omButtonDeleteSelMsg.EnableWindow(!CTxMsgManager::s_TxFlags.nGetFlagStatus(TX_SENDMESG));
                m_omButtonDeleteAllMsg.EnableWindow(!CTxMsgManager::s_TxFlags.nGetFlagStatus(TX_SENDMESG));

            }
        }
        else if(pNMListView->uChanged == LVIF_STATE)
        {

            //Check for the count
            UINT unCount = m_omLctrMsgList.GetItemCount();
            if ( unCount <= 0)
            {
                m_omButtonDeleteAllMsg.EnableWindow(FALSE);
            }
            if(!(pNMListView->uNewState & defMASK_CHECK_UNCHECK))
            {
                m_omButtonSendMsg.EnableWindow(FALSE);
                m_omButtonDeleteSelMsg.EnableWindow(FALSE);
            }
        }
        if( pNMListView->uChanged  == LVIF_STATE )
        {
            PSTXCANMSGLIST   psTxMsgList = nullptr ;
            PSMSGBLOCKLIST    psMsgBlock = nullptr ;
            // check if the item is focused and selected.
            unCurrentState =
                pNMListView->uNewState &(LVIS_FOCUSED |LVIS_SELECTED);
            psMsgBlock = pomBlocksView->psGetMsgBlockPointer(
                             pomBlocksView->m_nSelectedMsgBlockIndex,
                             pomBlocksView->m_psMsgBlockList );
            // If yes, update message information for newly selected item.
            if(unCurrentState != FALSE)
            {
                psTxMsgList = psGetMsgDetailPointer( pNMListView->iItem,
                                                     psMsgBlock );
                m_nSelectedMsgIndex = pNMListView->iItem;
                if(psTxMsgList != nullptr )
                {
                    CTxMsgDetailsView* pView = (CTxMsgDetailsView*)
                                               pomGetDetailsViewPointer();
                    if( pView != nullptr )
                    {
                        pView->vSetValues(&(psTxMsgList->m_sTxMsgDetails));
                        // Insert Signal List Update Code Here
                        pView->bUpdateSignalList(
                            psTxMsgList->m_sTxMsgDetails.m_sTxMsg );
                        pView->vEnableAddButton( TRUE );
                        // Clear error message if any
                        pView->bSetStatusText("");
                        psMsgBlock->m_bModified = true;
                    }
                }
            }
            else
            {
                m_nSelectedMsgIndex = -1;
            }
        }
        if( pNMListView->uNewState & defMASK_CHECK_UNCHECK )
        {
            PSTXCANMSGLIST   psTxMsgList = nullptr ;
            PSMSGBLOCKLIST    psMsgBlock = nullptr ;

            psMsgBlock = pomBlocksView->psGetMsgBlockPointer(
                             pomBlocksView->m_nSelectedMsgBlockIndex,
                             pomBlocksView->m_psMsgBlockList );

            psTxMsgList = psGetMsgDetailPointer(pNMListView->iItem,psMsgBlock);
            m_nSelectedMsgIndex = pNMListView->iItem ;
            //pNMListView->uNewState =  (LVIS_FOCUSED|LVIS_SELECTED);
            //Ashwin changes for selecting the item during unchecking
            int     nChecked = 0, nUnCkecked = 0;
            for(int nCnt =0; nCnt< m_omLctrMsgList.GetItemCount(); nCnt++)
            {
                if(m_omLctrMsgList.GetCheck(nCnt) == TRUE)
                {
                    nChecked++;
                }
                else if(m_omLctrMsgList.GetCheck(nCnt) == FALSE)
                {
                    nUnCkecked++;
                }
                m_omLctrMsgList.SetItemState(nCnt, 0, LVIS_SELECTED);
            }


            if(nChecked == m_omLctrMsgList.GetItemCount())
            {
                vCheckHeaderCtrl(true);
            }
            else if(nUnCkecked == m_omLctrMsgList.GetItemCount())
            {
                vCheckHeaderCtrl(false);
            }
            m_omLctrMsgList.SetItemState(pNMListView->iItem, LVIS_SELECTED, LVIS_SELECTED);//LVIS_SELECTED

            if(psTxMsgList != nullptr )
            {
                int nCurrentState =
                    m_omLctrMsgList.GetCheck(pNMListView->iItem);
                CTxFunctionsView* pView = (CTxFunctionsView*)
                                          pomGetFunctionsViewPointer();
                if( pView != nullptr )
                {
                    if( nCurrentState !=
                            psTxMsgList->m_sTxMsgDetails.m_bEnabled )
                    {
                        psTxMsgList->m_sTxMsgDetails.m_bEnabled = nCurrentState;
                        psTxMsgList->m_bModified = true;
                        // Enable Update Button
                        if(pView->m_CheckBoxAutoUpdate.GetCheck() == BST_UNCHECKED)
                        {
                            pView->m_omButtonApply.EnableWindow();
                        }
                        psMsgBlock->m_bModified = true;
                    }
                }
                else
                {
                    m_omLctrMsgList.SetCheck( pNMListView->iItem,
                                              psTxMsgList->m_sTxMsgDetails.m_bEnabled );
                }

                if( pView != nullptr )
                {
                    if(pView->m_CheckBoxAutoUpdate.GetCheck() == BST_CHECKED)
                    {
                        pView->vAccessButtonApply();
                        this->SetFocus();
                    }
                }
            }

        }



    }
    *pResult = 0;
}

void  CTxMsgListView::vCheckHeaderCtrl(bool bCheck)
{
    CHeaderCtrl* pHeader = m_omLctrMsgList.GetHeaderCtrl();
    if( pHeader != nullptr )
    {
        // Get Current Image Index
        HDITEM hditem;
        hditem.mask = HDI_IMAGE | HDI_FORMAT;
        if( pHeader->GetItem(0, &hditem ) == TRUE )
        {
            // Update Image Index
            hditem.fmt |=  HDF_IMAGE;
            hditem.iImage = bCheck;
            pHeader->SetItem(0, &hditem );
        }
    }
}

CWnd* CTxMsgListView::pomGetParentWindow() const
{
    CWnd* pWnd = nullptr;
    // Get Splitter window pointer
    pWnd = GetParent();
    // Get Tx Msg Child Window pointer from Splitter window pointer
    // At fourth Level
    // Splitter 3 -> Splitter 2 -> Splitter 1 -> Child Frame
    if( pWnd != nullptr )
    {
        pWnd = pWnd->GetParent();
    }
    if( pWnd != nullptr )
    {
        pWnd = pWnd->GetParent();
    }

    if( pWnd != nullptr )
    {
        pWnd = pWnd->GetParent();
    }

    // Return Tx Msg Child window pointer or nullptr incase of failure
    return pWnd;
}

CWnd* CTxMsgListView::pomGetBlocksViewPointer() const
{
    CWnd* pView = nullptr;
    // Get Child Frame Pointer
    CWnd* pWnd = nullptr;
    pWnd = pomGetParentWindow();
    // Get View Pointer
    if( pWnd != nullptr )
    {
        pView = ((CTxMsgChildFrame*)pWnd)->pomGetTxMsgViewPointers(
                    eTxMsgBlocksView );
    }
    // Return View pointer
    return pView;
}

CWnd* CTxMsgListView::pomGetDetailsViewPointer() const
{
    CWnd* pView = nullptr;
    // Get Child Frame Pointer
    CWnd* pWnd = nullptr;
    pWnd = pomGetParentWindow();
    // Get View Pointer
    if( pWnd != nullptr )
    {
        pView = ((CTxMsgChildFrame*)pWnd)->pomGetTxMsgViewPointers(
                    eTxMsgMessageDetailsView );
    }
    // Return View pointer
    return pView;
}

CWnd* CTxMsgListView::pomGetFunctionsViewPointer() const
{
    CWnd* pView = nullptr;
    // Get Child Frame Pointer
    CWnd* pWnd = nullptr;
    pWnd = pomGetParentWindow();
    // Get View Pointer
    if( pWnd != nullptr )
    {
        pView = ((CTxMsgChildFrame*)pWnd)->pomGetTxMsgViewPointers(
                    eTxMsgFunctionsView );
    }
    // Return View pointer
    return pView;
}

PSTXCANMSGLIST CTxMsgListView::psGetMsgDetailPointer(INT nIndex,
        SMSGBLOCKLIST* psCurrentMsgBlockList)
{
    PSTXCANMSGLIST psTxMsgList = nullptr;
    if(nIndex != -1 && psCurrentMsgBlockList != nullptr )
    {
        INT nCurrentIndex = 0;
        psTxMsgList = psCurrentMsgBlockList->m_psTxCANMsgList;
        while(nIndex != nCurrentIndex)
        {
            if(psTxMsgList != nullptr )
            {
                psTxMsgList = psTxMsgList->m_psNextMsgDetails;
            }
            nCurrentIndex++;
        }
    }
    return psTxMsgList;
}

void CTxMsgListView::OnSendSelectedMsg()
{
    PSTXSELMSGDATA psTxCanMsg = nullptr;
    UINT unTotalSelection = m_omLctrMsgList.GetSelectedCount();

    CTxMsgBlocksView* pomBlocksView = nullptr;
    pomBlocksView = ( CTxMsgBlocksView*)pomGetBlocksViewPointer();
    if( pomBlocksView != nullptr )
    {
        // Get the total selected count.
        if(unTotalSelection > 0 )
        {
            // create the pointer and check if it has created successfully.
            psTxCanMsg  = new STXSELMSGDATA;
            if(psTxCanMsg != nullptr )
            {
                // Fill the structure and create memory for array of message
                // frame to be transmitted.
                psTxCanMsg->m_unCount = unTotalSelection;
                psTxCanMsg->m_psTxMsg = new STCAN_MSG[unTotalSelection];
                if(psTxCanMsg->m_psTxMsg != nullptr )
                {
                    // get the current message block
                    PSMSGBLOCKLIST psMsgBlock = nullptr;
                    psMsgBlock = pomBlocksView->psGetMsgBlockPointer(
                                     pomBlocksView->m_nSelectedMsgBlockIndex,
                                     pomBlocksView->m_psMsgBlockList );
                    if(psMsgBlock != nullptr )
                    {
                        PSTXCANMSGLIST  psTxMsgList = nullptr;
                        INT nCurrentIndex           = -1;
                        // Get the message which is selected for transmission
                        // Since selection may be random so get the selection
                        // index one by one and get the pointer for that message
                        // frame . After that copy it to the structure.
                        for(UINT i =0 ; i<unTotalSelection; i++)
                        {
                            nCurrentIndex = m_omLctrMsgList.GetNextItem(
                                                nCurrentIndex,
                                                LVNI_SELECTED );
                            // If failure to get the index then assign Invalid
                            // message ID to avoid any problem in transmission.
                            if(nCurrentIndex != -1 )
                            {
                                psTxMsgList = psGetMsgDetailPointer (
                                                  nCurrentIndex,
                                                  psMsgBlock );
                                if(psTxMsgList != nullptr )
                                {
                                    memcpy( &(psTxCanMsg->m_psTxMsg[i]),
                                            &(psTxMsgList->m_sTxMsgDetails.m_sTxMsg),
                                            sizeof(STCAN_MSG) );
                                }
                                else
                                {
                                    psTxCanMsg->m_psTxMsg[i].m_unMsgID =
                                        static_cast <UINT> (-1);
                                }
                            }
                            else
                            {
                                psTxCanMsg->m_psTxMsg[i].m_unMsgID =
                                    static_cast <UINT> (-1 );
                            }
                        }
                        g_bStopSelectedMsgTx = FALSE;
                        // Get handle of thread and assign it to pulic data
                        // member in app class. This will be used to terminate
                        // the thread.
                        CWinThread* pomThread = nullptr ;
                        pomThread =
                            AfxBeginThread( CTxMsgManager::s_unSendSelectedMsg,
                                            psTxCanMsg );
                        if(pomThread != nullptr  &&
                                CTxMsgManager::s_sUtilThread.m_hThread
                                == nullptr )
                        {
                            CTxMsgManager::s_sUtilThread.m_hThread
                                = pomThread->m_hThread;
                        }
                    }
                }
            }
        }
    }
}

void CTxMsgListView::OnDeleteSelectedMsg()
{
    CTxMsgBlocksView* pomBlocksView = nullptr;
    pomBlocksView = ( CTxMsgBlocksView*)pomGetBlocksViewPointer();

    CTxMsgDetailsView* pomDetailsView = nullptr;
    pomDetailsView = (CTxMsgDetailsView*)pomGetDetailsViewPointer();

    CTxFunctionsView* pomFunctionsView = nullptr;
    pomFunctionsView = (CTxFunctionsView*)pomGetFunctionsViewPointer();

    if( pomBlocksView != nullptr && pomDetailsView != nullptr &&
            pomFunctionsView != nullptr )
    {
        if ( AfxMessageBox( _(defDEL_SEL_MSG_FRAME),
                            MB_YESNO|MB_ICONQUESTION) == IDYES)
        {
            if(m_nSelectedMsgIndex != -1)
            {
                if(pomBlocksView->m_nSelectedMsgBlockIndex != -1 )
                {
                    PSMSGBLOCKLIST psCurrentMsgBlock = nullptr;
                    psCurrentMsgBlock = pomBlocksView->psGetMsgBlockPointer(
                                            pomBlocksView->m_nSelectedMsgBlockIndex,
                                            pomBlocksView->m_psMsgBlockList);

                    if(psCurrentMsgBlock != nullptr )
                    {
                        BOOL bReturn = FALSE;
                        bReturn = bDeleteMsgFromBlock(psCurrentMsgBlock);
                        if(bReturn == TRUE )
                        {
                            if(pomFunctionsView->m_CheckBoxAutoUpdate.GetCheck() == BST_UNCHECKED)
                                pomFunctionsView->m_omButtonApply.
                                EnableWindow(TRUE);

                            pomDetailsView->vEnableAddButton( TRUE );
                            // Update Modified Flag
                            pomBlocksView->m_bModified = TRUE;
                        }
                        // Set the focus to list control back if it is not empty
                        // Shift the selection to item under fucus as selection
                        // deos not move up if an selected item is deleted.
                        m_nSelectedMsgIndex =
                            m_omLctrMsgList.GetNextItem(-1,LVIS_FOCUSED);
                        if(m_nSelectedMsgIndex != -1 )
                        {
                            m_omLctrMsgList.SetItemState(
                                m_nSelectedMsgIndex,
                                LVIS_SELECTED,
                                LVIS_SELECTED);
                        }
                        //changes added to update the Global in case of autoupdate
                        CTxFunctionsView* pView =
                            ( CTxFunctionsView* )pomGetFunctionsViewPointer();
                        if( pView != nullptr )
                        {
                            if(pView->m_CheckBoxAutoUpdate.GetCheck() == BST_CHECKED)
                            {
                                pView->vAccessButtonApply();
                                /*this->SetFocus();*/
                                m_omLctrMsgList.SetFocus();
                            }
                        }

                    }
                }
            }
        }
    }
}

BOOL CTxMsgListView::bDeleteMsgFromBlock(SMSGBLOCKLIST* psMsgCurrentBlock)
{
    BOOL bReturn = FALSE;
    // Check for current message block pointer to be valid.
    if(psMsgCurrentBlock != nullptr )
    {
        PSTXCANMSGLIST psDelTxCANMsgList = nullptr;
        PSTXCANMSGLIST psTxCANMsgList = nullptr;
        // Get the total selected items in message details list control.
        UINT  unSelectedCount = m_omLctrMsgList.GetSelectedCount();
        // If there is selection then delete it one by one.
        if (unSelectedCount > 0)
        {
            for (UINT i=0; i < unSelectedCount; i++)
            {
                // Get the first selection from the begining. Search for
                // selection is always from start to ensure that selected
                // index is matching with the nodes in the list.
                int nItem = m_omLctrMsgList.GetNextItem(-1, LVNI_SELECTED);
                if(nItem != -1)
                {
                    // Get the current pointer
                    psDelTxCANMsgList =
                        psGetMsgDetailPointer(nItem,psMsgCurrentBlock);
                    // Get the previous pointer.
                    psTxCANMsgList    =
                        psGetMsgDetailPointer(nItem-1,psMsgCurrentBlock);
                    // If it is valid the next element in both node is updated
                    // so that the current selected node is removed from list.
                    // If both the pointers are not null i.e. the node is
                    // in between the list.
                    //  If psTxCANMsgList is null then the node is at the start
                    // and if the both are null it is invalid pointers.
                    if(psDelTxCANMsgList != nullptr && psTxCANMsgList != nullptr )
                    {
                        psTxCANMsgList->m_psNextMsgDetails =
                            psDelTxCANMsgList->m_psNextMsgDetails;
                        bReturn = TRUE;
                    }
                    else if(psDelTxCANMsgList != nullptr &&
                            psDelTxCANMsgList ==
                            psMsgCurrentBlock->m_psTxCANMsgList )
                    {
                        psMsgCurrentBlock->m_psTxCANMsgList =
                            psDelTxCANMsgList->m_psNextMsgDetails;
                        bReturn = TRUE;
                    }
                    // if node  it is successfully removed then delete the
                    // memory for that node.
                    if(bReturn == TRUE)
                    {
                        delete psDelTxCANMsgList;
                        psDelTxCANMsgList = nullptr;
                        m_omLctrMsgList.DeleteItem(nItem);
                        //Decrement Message Count
                        psMsgCurrentBlock->m_unMsgCount--;
                        //Disable Delete All if the list is empty
                        UINT unCount =  m_omLctrMsgList.GetItemCount();
                        if( unCount <= 0 )
                        {
                            m_omButtonDeleteAllMsg.EnableWindow(FALSE);
                        }
                    }
                }
            }// for
        }
    }
    if(bReturn == TRUE )
    {
        m_nSelectedMsgIndex = -1;
    }
    return bReturn;
}

void CTxMsgListView::OnDeleteAllMsg()
{
    CTxMsgBlocksView* pomBlocksView = nullptr;
    pomBlocksView = ( CTxMsgBlocksView*)pomGetBlocksViewPointer();

    CTxMsgDetailsView* pomDetailsView = nullptr;
    pomDetailsView = (CTxMsgDetailsView*)pomGetDetailsViewPointer();

    CTxFunctionsView* pomFunctionsView = nullptr;
    pomFunctionsView = (CTxFunctionsView*)pomGetFunctionsViewPointer();

    if( pomBlocksView != nullptr && pomDetailsView != nullptr &&
            pomFunctionsView != nullptr )
    {
        // give a warning message before deleting it.
        if ( AfxMessageBox( _(defDEL_ALL_MSG_FRAME),
                            MB_YESNO|MB_ICONQUESTION) == IDYES)
        {
            // check for valid message block selection index.
            if( pomBlocksView->m_nSelectedMsgBlockIndex != -1)
            {
                BOOL bDeleted = FALSE;
                PSMSGBLOCKLIST psMsgCurrentBlock = nullptr;
                // Get the current message block pointer.
                psMsgCurrentBlock = pomBlocksView->psGetMsgBlockPointer(
                                        pomBlocksView->m_nSelectedMsgBlockIndex,
                                        pomBlocksView->m_psMsgBlockList );
                if(psMsgCurrentBlock != nullptr )
                {
                    // Call function to delete all message and clear the list
                    // control if the delete is successfull.
                    bDeleted = bDeleteAllMsgFromBlock(psMsgCurrentBlock);
                    if(bDeleted == TRUE )
                    {
                        m_omLctrMsgList.DeleteAllItems();
                        //Disable Delete All button
                        m_omButtonDeleteAllMsg.EnableWindow(FALSE);
                        m_omButtonDeleteSelMsg.EnableWindow(FALSE);

                        if(pomFunctionsView->m_CheckBoxAutoUpdate.GetCheck() == BST_UNCHECKED)
                        {
                            pomFunctionsView->m_omButtonApply.EnableWindow(TRUE);
                        }
                        // Update Add button status
                        pomDetailsView->vEnableAddButton( TRUE );
                        // Update Modified Flag
                        pomBlocksView->m_bModified = TRUE;
                    }
                    m_omButtonSendMsg.EnableWindow(FALSE);

                    //changes added to update the Global in case of autoupdate
                    CTxFunctionsView* pView =
                        ( CTxFunctionsView* )pomGetFunctionsViewPointer();
                    if( pView != nullptr )
                    {
                        if(pView->m_CheckBoxAutoUpdate.GetCheck() == BST_CHECKED)
                        {
                            pView->vAccessButtonApply();
                            this->SetFocus();
                        }
                    }
                }
            }
        }
    }
}

BOOL CTxMsgListView::bDeleteAllMsgFromBlock(SMSGBLOCKLIST* psMsgCurrentBlock)
{
    BOOL bReturn = TRUE;
    if(psMsgCurrentBlock != nullptr )
    {
        PSTXCANMSGLIST psTxCANMsgList = nullptr;
        PSTXCANMSGLIST psTxCANMsgListDel = nullptr;
        psTxCANMsgList = psMsgCurrentBlock->m_psTxCANMsgList;
        while(psTxCANMsgList != nullptr )
        {
            psTxCANMsgListDel = psTxCANMsgList;
            psTxCANMsgList    = psTxCANMsgList->m_psNextMsgDetails;
            delete psTxCANMsgListDel;
            psTxCANMsgListDel = nullptr;
        }
        psMsgCurrentBlock->m_psTxCANMsgList = nullptr;
        psMsgCurrentBlock->m_unMsgCount = 0;
        // Set index to Invalid
        m_nSelectedMsgIndex = -1;
    }
    else
    {
        bReturn = FALSE;
    }
    return bReturn;
}

void CTxMsgListView::vUpdateMsgListDisplay(sTXCANMSGDETAILS sMsgDetail,
        INT nCurrentIndex)
{
    CString omStrMsgID( "" );
    CString omStrMsgData( "" );
    CString omStrMsgType( "" );
    CString omStrDLC( "" );
    CString omStrChannel( "" );
    INT nIndex           = -1;
    UINT unImageID       = 0;
    CString omStrFormat( "" );
    CString omStrMsgName( "" );
    // Set Edit flag to TRUE
    m_bInitDlg = TRUE;

    // format the message data length
    // Get the base and accordingly change the format for Format function of
    // CString class.
    if( TRUE == CTxMsgManager::s_TxFlags.nGetFlagStatus(TX_HEX) )
    {
        omStrMsgID.Format(defFORMAT_MSGID_HEX,sMsgDetail.m_sTxMsg.m_unMsgID);
        omStrFormat = defFORMAT_DATA_HEX;
    }
    else
    {
        omStrMsgID.Format( defFORMAT_MSGID_DECIMAL,
                           sMsgDetail.m_sTxMsg.m_unMsgID );
        omStrFormat = defFORMAT_DATA_DECIMAL;
    }    // Message Details View
    CTxMsgDetailsView* pomDetailsView = nullptr;
    CMsgSignal* pDBptr = nullptr;
    pomDetailsView = (CTxMsgDetailsView*)pomGetDetailsViewPointer();
    if (nullptr != pomDetailsView)
    {
        pDBptr =  pomDetailsView->m_pouDBPtr;
    }
    CString omStrUpdatedMsgLength = "";
    // See if the msg ID is in the database
    // If yes load appropriate image
    if (nullptr != pDBptr)
    {
        omStrMsgName =
            pDBptr->omStrGetMessageNameFromMsgCode(sMsgDetail.m_sTxMsg.m_unMsgID);
        //omStrUpdatedMsgLength = pDBptr->omStrGetMessageLengthFromMsgCode(sMsgDetail.m_sTxMsg.m_unMsgID);
    }
    if ( omStrMsgName.IsEmpty() == FALSE )
    {
        unImageID = 1;// Database Image
        // Add message Name with the entry
        omStrMsgID += " [" +
                      pDBptr->omStrGetMessageNameFromMsgCode(sMsgDetail.m_sTxMsg.m_unMsgID )
                      + "]";
    }
    else
    {
        unImageID = 2;// Non-Database image
    }

    // Format channel ID
    omStrChannel.Format("%d", sMsgDetail.m_sTxMsg.m_ucChannel );

    // Format Message Type
    omStrFormat.Insert(omStrFormat.GetLength(),defEMPTY_CHAR);
    CString omStrTemp = "";

    for(INT i=0; i<sMsgDetail.m_sTxMsg.m_ucDataLen; i++)
    {
        omStrTemp.Format(omStrFormat,sMsgDetail.m_sTxMsg.m_ucData[i]);
        omStrMsgData +=  omStrTemp;
    }
    // Format the Message type
    if(sMsgDetail.m_sTxMsg.m_ucEXTENDED == TRUE)
    {
        omStrMsgType = defMSGID_EXTENDED;
    }
    else
    {
        omStrMsgType = defMSGID_STD;
    }
    if(sMsgDetail.m_sTxMsg.m_ucRTR == TRUE)
    {
        omStrMsgType += defMSGID_RTR;
    }
    // Format the DLC
    omStrDLC.Format("%d",sMsgDetail.m_sTxMsg.m_ucDataLen);
    // Get the current count if this is new items and insert it
    // as new item. Otherwise set the current item text.
    if(nCurrentIndex == -1)
    {
        INT nCount = m_omLctrMsgList.GetItemCount();
        if(nCount != -1 )
        {
            nIndex = m_omLctrMsgList.
                     InsertItem(nCount,omStrMsgID,unImageID);
        }
    }
    else
    {
        LVITEM sItem;
        sItem.mask      = LVIF_IMAGE;
        sItem.iItem     = nCurrentIndex;
        sItem.iSubItem  = defMAIN_ITEM;
        sItem.iImage    = unImageID;
        m_omLctrMsgList.
        SetItemText(nCurrentIndex,defMAIN_ITEM,omStrMsgID);
        m_omLctrMsgList.SetItem(&sItem);
        nIndex = nCurrentIndex;
    }
    // Set the list control item with string formatted for each column.
    if(nIndex != -1 )
    {
        // Update Channel ID
        m_omLctrMsgList.SetItemText( nIndex,
                                     defSUBITEM_MSGDETAILS_CHANNEL_ID,
                                     omStrChannel );
        // Update Message Type
        m_omLctrMsgList.SetItemText( nIndex,
                                     defSUBITEM_MSGDETAILS_TYPE,
                                     omStrMsgType );
        // Update Message Length
        m_omLctrMsgList.SetItemText( nIndex,
                                     defSUBITEM_MSGDETAILS_DLC,
                                     omStrDLC );
        // Update Data Bytes with Dirty Flag
        if( sMsgDetail.m_bIsMsgDirty == TRUE &&
                unImageID == 1 &&
                sMsgDetail.m_sTxMsg.m_ucDataLen > 0 )
        {
            omStrMsgData +=defASSETRIC;
        }
        m_omLctrMsgList.SetItemText( nIndex,
                                     defSUBITEM_MSGDETAILS_DATA,
                                     omStrMsgData);
        // Update Check box status
        m_omLctrMsgList.SetCheck( nIndex, sMsgDetail.m_bEnabled );
    }

    // Set Edit flag to FALSE
    m_bInitDlg = FALSE;
}

void CTxMsgListView::OnRightClickMsgDetails(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
    bDisplayPopMenu(m_omLctrMsgList,IDM_SEND_OPRNS);
    *pResult = 0;
}

BOOL CTxMsgListView::bDisplayPopMenu(CListCtrl& omList,UINT nIDResource )
{
    BOOL bReturn = FALSE;
    // Get selected item's index
    INT nIndex =
        omList.GetNextItem( -1, LVNI_SELECTED );
    // create menu
    CMenu* pMainMenu = new CMenu;
    // Assert
    if ( pMainMenu != nullptr )
    {
        // Load menu
        if ( pMainMenu->LoadMenu( nIDResource ))
        {
            // Get submenu
            CMenu* pSubMenu = pMainMenu->GetSubMenu( 0 );
            // Assert
            if ( pSubMenu != nullptr )
            {
                POINT point;
                // Get cursor position wrt screen co-ord
                GetCursorPos(&point);
                // If it is from Message List
                // Disable Delete All menu items if the the item count is 0
                if( nIDResource == IDM_SEND_OPRNS )
                {
                    UINT unCount = m_omLctrMsgList.GetItemCount();
                    if ( unCount <= 0)
                    {
                        pSubMenu->EnableMenuItem( IDM_DELETE_ALL_MSG,
                                                  MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
                    }
                }

                if(nIndex == -1 )
                {
                    // if selection is not valid, disble the menu.
                    pSubMenu->EnableMenuItem(IDM_SEND_SEL_MSG,
                                             MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
                    pSubMenu->EnableMenuItem(IDM_DELETE_SEL_MSG,
                                             MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);

                    if(TRUE == CTxMsgManager::s_TxFlags.nGetFlagStatus(TX_SENDMESG))
                    {
                        pSubMenu->EnableMenuItem( IDM_DELETE_ALL_MSG,
                                                  MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
                    }
                }
                else
                {
                    // If selection is valid and transmission is on. disable
                    // Delete and delete all menu.
                    if(TRUE == CTxMsgManager::s_TxFlags.nGetFlagStatus(TX_SENDMESG))
                    {
                        pSubMenu->EnableMenuItem(IDM_DELETE_SEL_MSG,
                                                 MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
                        pSubMenu->EnableMenuItem(IDM_DELETE_ALL_MSG,
                                                 MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
                    }
                    // If tool is not connected disable Send menu.
                    if(FALSE == CTxMsgManager::s_TxFlags.nGetFlagStatus(TX_CONNECTED))
                    {
                        pSubMenu->EnableMenuItem(IDM_SEND_SEL_MSG,
                                                 MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
                    }
                }
                // Display menu
                pSubMenu->TrackPopupMenu(
                    TPM_LEFTALIGN |TPM_RIGHTBUTTON,
                    point.x,
                    point.y,
                    this,
                    nullptr);
                // Clean up
                pMainMenu->Detach();
                pMainMenu->DestroyMenu();
                delete pMainMenu;
                pMainMenu = nullptr;
                bReturn = TRUE;
            }
        }
    }
    return bReturn;
}
