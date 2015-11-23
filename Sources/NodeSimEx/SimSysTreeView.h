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
 * \file      SimSysTreeView.h
 * \brief     This header file contains the defintion of class
 * \author    Ratnadip Choudhury
 * \copyright Copyright (c) 2011, Robert Bosch Engineering and Business Solutions. All rights reserved.
 *
 * This header file contains the defintion of class
 */

#pragma once

#include "Include/Basedefs.h"
#include "SimSysNodeInfo.h"

class CSimSysTreeView : public CTreeView
{
public:
    CSimSysTreeView();           // protected constructor used by dynamic creation
    DECLARE_DYNCREATE(CSimSysTreeView)

    // Attributes
public:

    // Operations
public:


    void vSetAllDllloaded();
    void vSetDllLoaded();

    // to populate the tree view.
    BOOL bPopulateTree();

    // Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CSimSysTreeView)
public:
    virtual void OnInitialUpdate();
protected:
    virtual void OnDraw(CDC* pDC);      // overridden to draw this view
    virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

    //}}AFX_VIRTUAL

    // Implementation
protected:
    virtual ~CSimSysTreeView();
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif

    // Generated message map functions
protected:
    //{{AFX_MSG(CSimSysTreeView)
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnTreeViewRightclick(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnSelchanged(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnTreeViewDblclick(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnRemoveNode();
    afx_msg void OnConfigNode();
    afx_msg void OnAddNode();
    afx_msg void OnEditNode();
    afx_msg void OnEnableDisableNode();
    afx_msg void OnBuildNode();
    afx_msg void OnTreeGetInfoTip(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnUpdateAddNode(CCmdUI* pCmdUI);
    afx_msg void OnUpdateConfigNode(CCmdUI* pCmdUI);
    afx_msg void OnUpdateEditNode(CCmdUI* pCmdUI);
    afx_msg void OnUpdateBuildNode(CCmdUI* pCmdUI);

    afx_msg void OnUpdateRemoveNode(CCmdUI* pCmdUI);
    afx_msg void OnPaint();

    afx_msg void OnKeyDown(UINT ch,UINT Count,UINT Flags);

    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
private:
    ETYPE_BUS m_eBus;
    CMenu* m_pomSubMenu;

    CMenu* m_pomContextMenu;
    CImageList m_omImageList;

    CString omStrSimsysToBeDeleted;

    HMODULE m_hModAdvancedUILib;

    CPoint m_omRightClickPoint;


    HTREEITEM m_hSelectedTreeItem;
    BOOL bCreateImageList();

    void vSaveSimsSysIfModified(CString omSimSysName) ;
    void vModifyToolbarIcon(BYTE bytItemIndex, bool bItemON, UINT nTBIDON, UINT nTBIDOFF);
    PSNODEINFO GetNodeInfo(HTREEITEM hTreeItem);
    void vDisplayRootMenu();
    void vDisplayNodeMenu();

public:
    static ETYPE_BUS CSimSysTreeView::sm_eBus;
};
