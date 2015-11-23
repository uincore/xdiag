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

#include "stdafx.h"
#include "ChannelConfigurationDlg.h"
#include "FibexClass_extern.h"
#include "Utility\WaitIndicator.h"

struct LinProcolBaudRate
{
    std::string m_strProtocol;
    int m_nBaudRate;
};

static LinProcolBaudRate sg_LINPROTOCOL_BAUD[] =
{
    {"LIN 2.2", 19200},
    {"LIN 2.1", 19200},
    {"LIN 2.0", 19200},
    {"LIN 1.3", 9600},
    {"LIN 1.2", 9600},
    {"LIN 1.1", 9600},
};


IMPLEMENT_DYNAMIC(CChannelConfigurationDlg, CDialog)
CChannelConfigurationDlg::CChannelConfigurationDlg(CMsgSignal*& pMsgSignal ,CHANNEL_CONFIG ouFlexrayChannelConfig[], INT& nChannelConfigured, ETYPE_BUS eBusType, CWnd* pParent /*=nullptr*/)
    : CDialog(CChannelConfigurationDlg::IDD, pParent)
{
    m_eBusType = eBusType;
    m_nChannelConfigured = nChannelConfigured;
    m_pMsgSignal = pMsgSignal;
    for ( int i = 0 ; i < nChannelConfigured; i++ )
    {
        m_ouFlexrayChannelConfig[i] = ouFlexrayChannelConfig[i];
    }
    m_nCurrentChannel = -1;
}

CChannelConfigurationDlg::~CChannelConfigurationDlg()
{
}

void CChannelConfigurationDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_EDIT_FIBEX_PATH, m_strFibexFilePath);
    DDX_Control(pDX, IDC_EDIT_FIBEX_PATH, m_omFibexPath);
    DDX_Control(pDX, IDC_COMBO_CHANNEL, m_ComboChannelSelect);
    DDX_Control(pDX, IDC_LIST_ECU, m_omEcuList);
    DDX_Control(pDX, IDC_COMBO_CLUSTER, m_omComboCluster);
    DDX_Control(pDX, IDC_COMBO_LIN_PROTOCOL, m_omComboLinProtocol);
    DDX_Text(pDX, IDC_EDIT_LIN_BAUDRATE, m_nLinBaudRate);
    DDV_MinMaxInt(pDX, m_nLinBaudRate, 200, 30000);
}

BEGIN_MESSAGE_MAP(CChannelConfigurationDlg, CDialog)
    ON_WM_SYSCOMMAND()
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_BN_CLICKED(IDC_BUTTON_FIBEXPATH, OnBnClickedButtonFibexpath)
    ON_BN_CLICKED(IDC_BUTTON_LDF_EDIT, OnBnClickedButtonLDFEditor)
    ON_CBN_SELCHANGE(IDC_COMBO_CLUSTER, OnCbnSelchangeComboCluster)
    ON_CBN_SELCHANGE(IDC_COMBO_CHANNEL, OnCbnSelchangeComboChannel)
    ON_CBN_SELCHANGE(IDC_COMBO_LIN_PROTOCOL, OnCbnSelchangeComboProtocol)
    ON_BN_CLICKED(IDOK, onBtnOk)
    ON_BN_CLICKED(IDC_CHECK_OVERWRITE_SETTINGS, OnOverwriteCheckBoxClick)
END_MESSAGE_MAP()

void CChannelConfigurationDlg::OnOverwriteCheckBoxClick()
{
    CButton* pTempBtn = (CButton*)GetDlgItem(IDC_CHECK_OVERWRITE_SETTINGS);
    bool bCheck = false;
    if ( nullptr != pTempBtn )
    {
        bCheck = (pTempBtn->GetCheck() != 0);
    }

    CWnd* pTempChild = GetDlgItem(IDC_COMBO_LIN_PROTOCOL);
    if ( nullptr != pTempChild )
    {
        pTempChild->EnableWindow(bCheck);
    }

    pTempChild = GetDlgItem(IDC_EDIT_LIN_BAUDRATE);
    if ( nullptr != pTempChild )
    {
        pTempChild->EnableWindow(bCheck);
    }
}

BOOL CChannelConfigurationDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    //Validate The Bus Type - FLEXRAY, LIN
    if ( m_eBusType != FLEXRAY && m_eBusType != LIN )
    {
        return S_FALSE;
    }

    //Controls Initialisation
    m_omEcuList.SetExtendedStyle(LVS_EX_CHECKBOXES | LVS_REPORT );
    m_omEcuList.InsertColumn(0, "Select ECU");
    // Set the width to occupy the whole list
    CRect omRect;
    m_omEcuList.GetWindowRect(&omRect);
    int nWidth = static_cast<int>(omRect.Width() * 0.95);

    // Set Col Width
    m_omEcuList.SetColumnWidth(0, nWidth);

    char chTemp[MAX_PATH];
    //INT nChannelCount = m_ouFlexrayChannelConfig.size();
    m_ComboChannelSelect.Clear();
    for ( int i = 0 ; i < m_nChannelConfigured ; i++ )
    {
        sprintf_s(chTemp, "Channel %d", i+1);
        m_ComboChannelSelect.AddString( chTemp);
    }

    //Update for 1st Channel
    m_nCurrentChannel = -1;
    m_ComboChannelSelect.SetCurSel(0);
    OnCbnSelchangeComboChannel();

    nUpdateLinSettings();

    nDisplayProtocolSettings(0);
    nEnableControls(m_eBusType);
    if(m_eBusType == LIN)
    {
        GetDlgItem(IDC_BUTTON_LDF_EDIT)->ShowWindow(TRUE);
        GetDlgItem(IDC_BUTTON_LDF_EDIT)->EnableWindow(TRUE);
    }
    return TRUE;
}

int CChannelConfigurationDlg::nUpdateLinSettings()
{
    CComboBox* pomCombo = (CComboBox*)GetDlgItem(IDC_COMBO_LIN_PROTOCOL);

    if ( pomCombo != nullptr )
    {
        for ( int i = 0 ; i < ( sizeof(sg_LINPROTOCOL_BAUD)/ sizeof(sg_LINPROTOCOL_BAUD[0])); i++ )
        {
            pomCombo->InsertString(i, sg_LINPROTOCOL_BAUD[i].m_strProtocol.c_str());
        }
        pomCombo->SetCurSel(0);
    }

    m_nLinBaudRate = 1900;
    UpdateData(FALSE);
    OnOverwriteCheckBoxClick();

    return 0;
}


int CChannelConfigurationDlg::nEnableControls( ETYPE_BUS eBusType)
{
    CWnd* pWnd = nullptr;
    if ( eBusType == FLEXRAY )
    {
        pWnd = GetDlgItem(IDC_STATIC_DATABASE_INFO);
        if ( pWnd != nullptr )
        {
            pWnd->SetWindowText("Import Flexray Database (FIBEX)");
        }

        pWnd = GetDlgItem(IDC_STATIC_CLUSTER_INFO);
        if ( pWnd != nullptr )
        {
            pWnd->SetWindowText("Select Flexray Cluster:");
        }

        pWnd = GetDlgItem(IDC_STATIC_EXTRA_CONFIG);
        if ( pWnd != nullptr )
        {
            pWnd->SetWindowText("Key Slot Configuration");
        }

        pWnd = GetDlgItem(IDC_STATIC_DBNAME);
        if ( pWnd != nullptr )
        {
            pWnd->SetWindowText("Fibex:");
        }

        pWnd = GetDlgItem(IDC_COMBO_LIN_PROTOCOL);
        if ( pWnd != nullptr )
        {
            pWnd->ShowWindow(FALSE);
        }

        pWnd = GetDlgItem(IDC_STATIC_LIN_PROTOCOL);
        if ( pWnd != nullptr )
        {
            pWnd->ShowWindow(FALSE);
        }

        pWnd = GetDlgItem(IDC_STATIC_LIN_BAUDRATE);
        if ( pWnd != nullptr )
        {
            pWnd->ShowWindow(FALSE);
        }

        pWnd = GetDlgItem(IDC_EDIT_LIN_BAUDRATE);
        if ( pWnd != nullptr )
        {
            pWnd->ShowWindow(FALSE);
        }

        pWnd = GetDlgItem(IDC_CHECK_OVERWRITE_SETTINGS);
        if ( pWnd != nullptr )
        {
            pWnd->ShowWindow(FALSE);
        }

        pWnd = GetDlgItem(IDC_ENABLE_MASTER_MODE);
        if ( pWnd != NULL )
        {
            pWnd->ShowWindow(FALSE);
        }

        pWnd = GetDlgItem(IDC_STATIC_LIN_OVERWRITE);
        if ( pWnd != NULL )
        {
            pWnd->ShowWindow(FALSE);
        }
    }
    else if ( eBusType == LIN )
    {
        pWnd = GetDlgItem(IDC_STATIC_DATABASE_INFO);
        if ( pWnd != nullptr )
        {
            pWnd->SetWindowText("Import LIN Database (LDF)");
        }

        pWnd = GetDlgItem(IDC_STATIC_CLUSTER_INFO);
        if ( pWnd != nullptr )
        {
            pWnd->SetWindowText("Select LIN Cluster:");
        }

        pWnd = GetDlgItem(IDC_STATIC_EXTRA_CONFIG);
        if ( pWnd != nullptr )
        {
            pWnd->SetWindowText("LIN Network Settings");
        }

        pWnd = GetDlgItem(IDC_STATIC_DBNAME);
        if ( pWnd != nullptr )
        {
            pWnd->SetWindowText("LDF:");
        }

        pWnd = GetDlgItem(IDC_COMBO_CLUSTER);
        if ( pWnd != nullptr )
        {
            pWnd->EnableWindow(FALSE);
        }
    }

    return S_OK;
}

void CChannelConfigurationDlg::OnBnClickedButtonFibexpath()
{
    CFileDialog* pomFibexDlg = nullptr;
    std::string strWaitText;
    if ( m_eBusType == FLEXRAY )
    {
        pomFibexDlg = new CFileDialog(TRUE, ".xml", 0, OFN_HIDEREADONLY | OFN_FILEMUSTEXIST, "FIBEX Files (*.xml)|*.xml||", this);
        pomFibexDlg->m_ofn.lpstrTitle = "Select A FIBEX File";
        strWaitText = "Parsing Fibex File. Please Wait...";
    }
    else if ( m_eBusType == LIN )
    {
        pomFibexDlg = new CFileDialog(TRUE, ".xml", 0, OFN_HIDEREADONLY | OFN_FILEMUSTEXIST, "LDF Files (*.ldf)|*.ldf||", this);
        pomFibexDlg->m_ofn.lpstrTitle = "Select A LDF File";
        strWaitText = "Parsing LDF File. Please Wait...";
    }

    if ( ( pomFibexDlg != nullptr ) && ( pomFibexDlg->DoModal() == IDOK ) )
    {
        CString strPath = pomFibexDlg->GetPathName();
        std::list<Cluster> ouClusterList;
        std::list<LinChannelParam> ouLinChannelParams;
        CWaitIndicator ouWaitIndicator;
        ouWaitIndicator.DisplayWindow(strWaitText.c_str(), this);

        int nResult = FCLASS_FAILURE;

        if ( m_eBusType == FLEXRAY )
        {
            nResult = m_pMsgSignal->hLoadFibexDBFile(strPath, ouClusterList);
        }
        else if ( m_eBusType == LIN )
        {
            nResult = m_pMsgSignal->hLoadLdfFile((LPCSTR)strPath, ouClusterList, ouLinChannelParams);

        }

        ouWaitIndicator.CloseWindow();
        m_nCurrentChannel = m_ComboChannelSelect.GetCurSel();

        if ((nResult == FCLASS_SUCCESS) && (!ouClusterList.empty()))
        {
            m_omComboCluster.ResetContent();
            //list<CHANNEL_CONFIG>::iterator itrChannelConfig = m_ouFlexrayChannelConfig.begin();
            //advance(itrChannelConfig, m_nCurrentChannel );
            //if ( itrChannelConfig != m_ouFlexrayChannelConfig.end() )
            {
                m_ouCurrentChannelCluster = ouClusterList;
                m_ouLinChannelParams = ouLinChannelParams;
                std::list<Cluster>::iterator itrCluster = ouClusterList.begin();
                //itrChannelConfig->m_ouClusterInfo = *itrCluster;

                for ( ; itrCluster != ouClusterList.end(); ++itrCluster)
                {
                    m_omComboCluster.AddString(itrCluster->m_strName.c_str());
                }
                m_omComboCluster.SetCurSel(0);

                if ( m_omComboCluster.GetCount() <=1 )
                {
                    m_omComboCluster.EnableWindow(FALSE);
                }


                nUpdateEcuList(0, 0);
                nUpdateLinParams(0,0);
                m_strFibexFilePath = strPath;
            }
        }
        else
        {
            if ( nResult ==  VERSION_NOT_SUPPORTED )
            {
                MessageBox("Unsupported FIBEX Version", "Error", MB_OK | MB_ICONERROR );
            }
            else
            {
                MessageBox("Invalid Input File", "Error", MB_OK | MB_ICONERROR );
            }
        }
        UpdateData(FALSE);
    }

}
void CChannelConfigurationDlg::OnBnClickedButtonLDFEditor()
{
    try
    {
        // Get the working directory
        char acPath[MAX_PATH] = "";
        GetModuleFileName( nullptr, acPath, MAX_PATH );
        PathRemoveFileSpec(acPath);
        CString strPath = acPath;
        strPath += "\\LDFEditor.exe";

        if(PathFileExists(strPath) == TRUE)
        {
            // Launch the converter utility
            PROCESS_INFORMATION sProcessInfo;
            STARTUPINFO sStartInfo;

            memset(&sProcessInfo, 0, sizeof(PROCESS_INFORMATION));
            memset(&sStartInfo, 0, sizeof(STARTUPINFO));
            std::string strCmdline = "\""+std::string(strPath)+"\""+" "+"\"" +std::string(m_strFibexFilePath)+"\"";
            // \" is added to allow spaces in the file path.
            int nSuccess = CreateProcess(strPath.GetBuffer(MAX_PATH),(LPSTR)strCmdline.c_str(),
                                         nullptr, nullptr, true, CREATE_NO_WINDOW, nullptr, nullptr,
                                         &sStartInfo, &sProcessInfo);
            if(!nSuccess)
            {
                AfxMessageBox("Unable to launch LDF Editor.",MB_ICONSTOP|MB_OK);
            }
        }
    }
    catch(...)
    {
    }
}
void CChannelConfigurationDlg::OnCbnSelchangeComboCluster()
{
}

void CChannelConfigurationDlg::OnCbnSelchangeComboProtocol()
{
    int nSel = m_omComboLinProtocol.GetCurSel();

    if ( nSel >= 0 )
    {
        m_nLinBaudRate = sg_LINPROTOCOL_BAUD[nSel].m_nBaudRate;
        UpdateData(FALSE);
    }
}

void CChannelConfigurationDlg::OnCbnSelchangeComboChannel()
{
    //Save Current Values to the old Channel
    if ( -1 != m_nCurrentChannel )
    {

        std::list<Cluster>::iterator itFlexConfig = m_ouCurrentChannelCluster.begin();

        advance(itFlexConfig, m_omComboCluster.GetCurSel());

        if ( itFlexConfig != m_ouCurrentChannelCluster.end() )
        {
            if (  m_nCurrentChannel < m_nChannelConfigured )
            {
                //1. Save Cluster Info
                m_ouFlexrayChannelConfig[m_nCurrentChannel].m_ouClusterInfo = *itFlexConfig;

                //2. Save ECU Info
                m_ouFlexrayChannelConfig[m_nCurrentChannel].m_strSlectedEculist.clear();


                std::list<ECU_Struct> ouEcuList;
                m_ouFlexrayChannelConfig[m_nCurrentChannel].m_ouClusterInfo.GetECUList(ouEcuList);
                std::list<ECU_Struct>::iterator itrEcu = ouEcuList.begin();


                int nEcuCount = m_omEcuList.GetItemCount();
                for ( int i = 0; i < nEcuCount; i++ )
                {
                    itrEcu = ouEcuList.begin();
                    if ( m_omEcuList.GetCheck(i) == TRUE )
                    {
                        advance(itrEcu, i);
                        if ( itrEcu != ouEcuList.end())
                        {
                            m_ouFlexrayChannelConfig[m_nCurrentChannel].m_strSlectedEculist.push_back( itrEcu->m_strEcuId);
                        }
                    }
                }

                //3. Fibex Path
                m_ouFlexrayChannelConfig[m_nCurrentChannel].m_strDataBasePath = m_strFibexFilePath;

            }
        }
    }

    //Update the selected channel Information

    INT nSelcetedIndex = m_ComboChannelSelect.GetCurSel();
    m_nCurrentChannel = nSelcetedIndex;
    m_ouCurrentChannelCluster.clear();
    m_omEcuList.DeleteAllItems();
    if ( nSelcetedIndex < m_nChannelConfigured )// m_ouFlexrayChannelConfig.size())
    {
        m_strFibexFilePath = m_ouFlexrayChannelConfig[nSelcetedIndex].m_strDataBasePath.c_str();

        m_ouCurrentChannelCluster.push_back(m_ouFlexrayChannelConfig[nSelcetedIndex].m_ouClusterInfo);

        m_omComboCluster.Clear();
        m_omComboCluster.AddString( m_ouFlexrayChannelConfig[nSelcetedIndex].m_ouClusterInfo.m_strName.c_str());
        m_omComboCluster.SetCurSel(0);
        std::list<ECU_Struct> ouEcuList;
        m_ouFlexrayChannelConfig[nSelcetedIndex].m_ouClusterInfo.GetECUList(ouEcuList);

        INT nIndex = 0;
        for (std::list<ECU_Struct>::iterator itrEcu = ouEcuList.begin(); itrEcu!= ouEcuList.end(); ++itrEcu)
        {
            m_omEcuList.InsertItem(nIndex, itrEcu->m_strECUName.c_str() );

            if ( bIsEcuSlected(m_ouFlexrayChannelConfig[nSelcetedIndex].m_strSlectedEculist,  itrEcu->m_strEcuId) )
            {
                m_omEcuList.SetCheck(nIndex);
            }
            nIndex++;
        }
        nDisplayProtocolSettings(nSelcetedIndex);
    }
    UpdateData(FALSE);
}

int CChannelConfigurationDlg::nDisplayProtocolSettings(int nChannelIndex)
{
    if ( m_eBusType == LIN )
    {
        BOOL bOverrite = m_ouFlexrayChannelConfig[nChannelIndex].m_ouLinParams.m_bOverWriteSettings;

        ((CButton*)GetDlgItem(IDC_CHECK_OVERWRITE_SETTINGS))->SetCheck(bOverrite);

        CComboBox* pomCombo = (CComboBox*)GetDlgItem(IDC_COMBO_LIN_PROTOCOL);
        pomCombo->EnableWindow(bOverrite);

        GetDlgItem(IDC_EDIT_LIN_BAUDRATE)->EnableWindow(bOverrite);

        m_nLinBaudRate = 19200;
        pomCombo->SetCurSel(0);
        for ( int i = 0 ; i < ( sizeof(sg_LINPROTOCOL_BAUD)/ sizeof(sg_LINPROTOCOL_BAUD[0])); i++ )
        {
            if ( m_ouFlexrayChannelConfig[nChannelIndex].m_ouLinParams.m_strProtocolVersion == sg_LINPROTOCOL_BAUD[i].m_strProtocol )
            {
                pomCombo->SetCurSel(i);
                if ( bOverrite == false )
                {
                    m_nLinBaudRate = sg_LINPROTOCOL_BAUD[i].m_nBaudRate;
                }
                else
                {
                    m_nLinBaudRate =  m_ouFlexrayChannelConfig[nChannelIndex].m_ouLinParams.m_nBaudRate;
                }

                break;
            }
        }
        ((CButton*)GetDlgItem(IDC_ENABLE_MASTER_MODE))->SetCheck(m_ouFlexrayChannelConfig[nChannelIndex].m_ouLinParams.m_bIsMasterMode);
        UpdateData(FALSE);
    }
    return 0;
}


bool CChannelConfigurationDlg::bIsEcuSlected(std::list<std::string>& ouEcuList, std::string strEcuName)
{
    bool bFound = false;
    for (std::list<std::string>::iterator itrEcu = ouEcuList.begin() ; itrEcu!= ouEcuList.end(); ++itrEcu)
    {
        if ( strEcuName == *itrEcu )
        {
            bFound = true;
        }
    }
    return bFound;
}

INT CChannelConfigurationDlg::nUpdateLinParams(INT /* nChannelIndex */, INT nClusterIndex)
{
    std::list<LinChannelParam>::iterator itrCluster =  m_ouLinChannelParams.begin();
    advance(itrCluster, nClusterIndex);
    if ( itrCluster != m_ouLinChannelParams.end() )
    {
        char chText[MAX_PATH];
        sprintf(chText, "%d", itrCluster->m_nBaudRate);
        GetDlgItem(IDC_EDIT_LIN_BAUDRATE)->SetWindowText(chText);

        int nIndex = m_omComboLinProtocol.FindString(0, itrCluster->m_strProtocolVersion.c_str());
        if ( nIndex != 0 )
        {
            m_omComboLinProtocol.SetCurSel(nIndex);
        }

        ((CButton*)GetDlgItem(IDC_ENABLE_MASTER_MODE))->SetCheck(itrCluster->m_bIsMasterMode);
    }
    return 0;
}

INT CChannelConfigurationDlg::nUpdateEcuList(INT /* nChannelIndex */, INT nClusterIndex)
{
    std::list<Cluster>::iterator itrCluster =  m_ouCurrentChannelCluster.begin();
    advance(itrCluster, nClusterIndex);
    if ( itrCluster != m_ouCurrentChannelCluster.end() )
    {
        m_omEcuList.DeleteAllItems();
        int i = 0;
        for (std::map<ECU_ID, ECU_Struct>::iterator itrECU = itrCluster->m_ouEcuList.begin(); itrECU != itrCluster->m_ouEcuList.end(); ++itrECU)
        {
            m_omEcuList.InsertItem(i, itrECU->second.m_strECUName.c_str());
            ++i;
        }
    }

    return 0;
}

INT CChannelConfigurationDlg::nUpdateEcuList( Cluster& ouCluster )
{
    std::list<ECU_Struct> ouEcuList;
    ouCluster.GetECUList(ouEcuList);
    INT nIndex = 0;
    for (std::list<ECU_Struct>::iterator itrEcu; itrEcu!= ouEcuList.end(); ++itrEcu)
    {
        m_omEcuList.InsertItem(nIndex, itrEcu->m_strECUName.c_str() );
        m_omEcuList.SetCheck(nIndex);
        nIndex++;
    }

    return 0;
}

int CChannelConfigurationDlg::nSaveProtocolSettings(int /* nIndex */)
{
    UpdateData(TRUE);
    if ( m_eBusType == LIN )
    {
        int nSel = ((CComboBox*)GetDlgItem(IDC_COMBO_LIN_PROTOCOL))->GetCurSel();
        if ( nSel >= 0)
        {
            m_ouFlexrayChannelConfig[m_nCurrentChannel].m_ouLinParams.m_strProtocolVersion = sg_LINPROTOCOL_BAUD[nSel].m_strProtocol;
            m_ouFlexrayChannelConfig[m_nCurrentChannel].m_ouLinParams.m_nBaudRate = m_nLinBaudRate;
            m_ouFlexrayChannelConfig[m_nCurrentChannel].m_ouLinParams.m_bOverWriteSettings = (((CButton*)GetDlgItem(IDC_CHECK_OVERWRITE_SETTINGS))->GetCheck() != 0);
            m_ouFlexrayChannelConfig[m_nCurrentChannel].m_ouLinParams.m_bIsMasterMode = ((CButton*)GetDlgItem(IDC_ENABLE_MASTER_MODE))->GetCheck();
            if( m_ouLinChannelParams.size() > 0 )
            {
                m_ouFlexrayChannelConfig[m_nCurrentChannel].m_ouLinParams.ouLinParams = m_ouLinChannelParams.begin()->ouLinParams;
            }
        }


        m_ouFlexrayChannelConfig[m_nCurrentChannel].m_strDataBasePath = m_strFibexFilePath;
    }

    return 0;
}

void CChannelConfigurationDlg::onBtnOk()
{
    //Save Current Values to the old Channel
    if ( -1 != m_nCurrentChannel )
    {

        std::list<Cluster>::iterator itFlexConfig = m_ouCurrentChannelCluster.begin();

        advance(itFlexConfig, m_omComboCluster.GetCurSel());

        if ( itFlexConfig != m_ouCurrentChannelCluster.end() )
        {
            if (  m_nCurrentChannel < m_nChannelConfigured )
            {
                //1. Save Cluster Info
                m_ouFlexrayChannelConfig[m_nCurrentChannel].m_ouClusterInfo = *itFlexConfig;

                //2. Save ECU Info
                m_ouFlexrayChannelConfig[m_nCurrentChannel].m_strSlectedEculist.clear();


                std::list<ECU_Struct> ouEcuList;
                m_ouFlexrayChannelConfig[m_nCurrentChannel].m_ouClusterInfo.GetECUList(ouEcuList);
                std::list<ECU_Struct>::iterator itrEcu = ouEcuList.begin();


                int nEcuCount = m_omEcuList.GetItemCount();
                for ( int i = 0; i < nEcuCount; i++ )
                {
                    itrEcu = ouEcuList.begin();
                    if ( m_omEcuList.GetCheck(i) == TRUE )
                    {
                        advance(itrEcu, i);
                        if ( itrEcu != ouEcuList.end())
                        {
                            m_ouFlexrayChannelConfig[m_nCurrentChannel].m_strSlectedEculist.push_back( itrEcu->m_strEcuId);
                        }
                    }
                }

                //3. Fibex Path
                m_ouFlexrayChannelConfig[m_nCurrentChannel].m_strDataBasePath = m_strFibexFilePath;

                //Protocol;
                nSaveProtocolSettings(m_nCurrentChannel);
            }
        }
    }
    OnOK();
}
