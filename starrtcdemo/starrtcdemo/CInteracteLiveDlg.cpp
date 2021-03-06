// CInteracteLiveDlg.cpp: 实现文件
//

#include "stdafx.h"
#include "starrtcdemo.h"
#include "CInteracteLiveDlg.h"
#include "afxdialogex.h"
#include "HttpClient.h"
#include "json.h"
#include "CropType.h"
#include "CInterfaceUrls.h"
#include "CCreateLiveDialog.h"

#include <opencv2/core/core.hpp>  

#include <opencv2/highgui/highgui.hpp>  

#include <opencv2/imgproc/imgproc.hpp>  
using namespace cv;
#define WM_RECV_LIVE_MSG WM_USER + 1



DWORD WINAPI SetShowPicThread(LPVOID p)
{
	CInteracteLiveDlg* pInteracteLiveDlg = (CInteracteLiveDlg*)p;

	while (WaitForSingleObject(pInteracteLiveDlg->m_hSetShowPicEvent, INFINITE) == WAIT_OBJECT_0)
	{
		if (pInteracteLiveDlg->m_bExit)
		{
			break;
		}
		if (pInteracteLiveDlg->m_pDataShowView != NULL)
		{
			pInteracteLiveDlg->m_pDataShowView->setShowPictures();
		}
	}
	return 0;
}

// CInteracteLiveDlg 对话框

IMPLEMENT_DYNAMIC(CInteracteLiveDlg, CDialogEx)

CInteracteLiveDlg::CInteracteLiveDlg(CUserManager* pUserManager, CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_LIVE, pParent)
{
	m_pUserManager = pUserManager;
	XHLiveManager::addChatroomGetListListener(this);
	m_pLiveManager = new XHLiveManager(this);
	m_pSoundManager = new CSoundManager(this);
	m_pDataShowView = NULL;

	m_hSetShowPicThread = NULL;
	m_hSetShowPicEvent = NULL;
	m_pCurrentLive = NULL;

	m_pConfig = NULL;

	m_nUpId = -1;
	m_bExit = false;
}

CInteracteLiveDlg::~CInteracteLiveDlg()
{
	m_bStop = true;
	m_bExit = true;
	m_pConfig = NULL;
	if (m_pSoundManager != NULL)
	{
		delete m_pSoundManager;
		m_pSoundManager = NULL;
	}
	
	if (m_pLiveManager != NULL)
	{
		delete m_pLiveManager;
		m_pLiveManager = NULL;
	}

	if (m_pDataShowView != NULL)
	{
		delete m_pDataShowView;
		m_pDataShowView = NULL;
	}

	if (m_hSetShowPicEvent != NULL)
	{
		SetEvent(m_hSetShowPicEvent);
		m_hSetShowPicEvent = NULL;
	}
	if (m_pCurrentLive != NULL)
	{
		delete m_pCurrentLive;
		m_pCurrentLive = NULL;
	}
	mVLivePrograms.clear();
}

void CInteracteLiveDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST1, m_liveList);
	DDX_Control(pDX, IDC_STATIC_LIVE_SHOW_AREA, m_ShowArea);
	DDX_Control(pDX, IDC_LIST2, m_liveListListControl);
	DDX_Control(pDX, IDC_BUTTON_LIVE_APPLY_UPLOAD, m_ApplyUploadButton);
	DDX_Control(pDX, IDC_BUTTON_LIVE_EXIT_UPLOAD, m_ExitUpload);
}


BEGIN_MESSAGE_MAP(CInteracteLiveDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON_LIVE_BRUSH_LIST, &CInteracteLiveDlg::OnBnClickedButtonLiveBrushList)
	ON_WM_PAINT()
	ON_MESSAGE(UPDATE_SHOW_PIC_MESSAGE, setShowPictures)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BUTTON_CREATE_NEW_LIVE, &CInteracteLiveDlg::OnBnClickedButtonCreateNewLive)
	ON_NOTIFY(NM_CLICK, IDC_LIST2, &CInteracteLiveDlg::OnNMClickList2)
	ON_BN_CLICKED(IDC_BUTTON_LIVE_APPLY_UPLOAD, &CInteracteLiveDlg::OnBnClickedButtonLiveApplyUpload)
	ON_MESSAGE(WM_RECV_LIVE_MSG, OnRecvLiveMsg)
	ON_BN_CLICKED(IDC_BUTTON_LIVE_EXIT_UPLOAD, &CInteracteLiveDlg::OnBnClickedButtonLiveExitUpload)
END_MESSAGE_MAP()

LRESULT CInteracteLiveDlg::OnRecvLiveMsg(WPARAM wParam, LPARAM lParam)
{
	switch (wParam)
	{
	case 0:
		break;
	case 1:
	{
		if (IDYES == AfxMessageBox("是否上传数据", MB_YESNO))
		{
			m_ApplyUploadButton.EnableWindow(FALSE);
			m_ExitUpload.EnableWindow(FALSE);

			if (m_pDataShowView != NULL)
			{
				m_pDataShowView->removeAllUser();
				m_pDataShowView->setShowPictures();
			}
			stopGetData();
			if (m_pSoundManager != NULL)
			{
				m_pSoundManager->stopSoundData();
			}

			bool bRet = m_pLiveManager->startLive(m_pCurrentLive->m_strId.GetBuffer(0));
			if (bRet)
			{
				/*onActorJoined(m_pCurrentLive->m_strId.GetBuffer(0), m_pUserManager->m_ServiceParam.m_strUserId);
				startGetData((CROP_TYPE)m_pUserManager->m_ServiceParam.m_CropType, true);
				if (m_pSoundManager != NULL)
				{
					m_pSoundManager->startGetSoundData(true);
				}*/
				startGetData((CROP_TYPE)m_pUserManager->m_ServiceParam.m_CropType, true);
				if (m_pSoundManager != NULL)
				{
					m_pSoundManager->startSoundData(true);
				}
				m_ApplyUploadButton.EnableWindow(FALSE);
				m_ExitUpload.EnableWindow(TRUE);
			}
			else
			{
				AfxMessageBox("join live failed");
			}
		}
		else
		{
		}
	}
		break;
	case 2:
		break;
	}
	return 0;
}
/**
 * 查询聊天室列表回调
 */
int CInteracteLiveDlg::chatroomQueryAllListOK(list<ChatroomInfo>& listData)
{
	mVLivePrograms.clear();
	m_liveListListControl.DeleteAllItems();

	list<ChatroomInfo>::iterator iter = listData.begin();
	int i = 0;
	for (; iter != listData.end(); iter++)
	{
		CLiveProgram liveProgram;
		liveProgram.m_strName = (char*)(*iter).m_strName.c_str();
		liveProgram.m_strId = (char*)(*iter).m_strRoomId.c_str();
		liveProgram.m_strCreator = (char*)(*iter).m_strCreaterId.c_str();
		mVLivePrograms.push_back(liveProgram);
	}
	int nRowIndex = 0;
	CString strStatus = "";
	for (int i = 0; i < (int)mVLivePrograms.size(); i++)
	{
		m_liveListListControl.InsertItem(i, mVLivePrograms[i].m_strName);
		//m_liveList.AddString(mVLivePrograms[i].m_strName);
		m_liveListListControl.SetItemText(i, LIVE_VIDEO_ID, mVLivePrograms[i].m_strId);

		m_liveListListControl.SetItemText(i, LIVE_VIDEO_CREATER, mVLivePrograms[i].m_strCreator);
		if (mVLivePrograms[i].m_liveState)
		{
			strStatus = "正在直播";
		}
		else
		{
			strStatus = "直播未开始";
		}
		m_liveListListControl.SetItemText(i, LIVE_VIDEO_STATUS, strStatus);
	}
	return 0;
}
// CInteracteLiveDlg 消息处理程序
void CInteracteLiveDlg::getLiveList()
{
	char strListType[10] = { 0 };
	sprintf_s(strListType, "%d,%d", CHATROOM_LIST_TYPE_LIVE, CHATROOM_LIST_TYPE_LIVE_PUSH);

	if (m_pConfig != NULL && m_pConfig->m_bAEventCenterEnable)
	{
		list<ChatroomInfo> listData;
		CInterfaceUrls::demoQueryList(strListType, listData);
		chatroomQueryAllListOK(listData);
	}
	else
	{
		XHLiveManager::getLiveList("", strListType);
	}
}

void CInteracteLiveDlg::OnBnClickedButtonLiveBrushList()
{
	getLiveList();
}
LRESULT CInteracteLiveDlg::setShowPictures(WPARAM wParam, LPARAM lParam)
{
	if (m_pDataShowView != NULL)
	{
		m_pDataShowView->setShowPictures();
	}
	return 1L;
}

CLiveProgram* CInteracteLiveDlg::getLiveProgram(CString strId, CString strName, CString strCreator)
{
	CLiveProgram* pRet = NULL;
	for (int i = 0; i < (int)mVLivePrograms.size(); i++)
	{
		if (//mVLivePrograms[i].m_strId == strId &&
			mVLivePrograms[i].m_strName == strName)// &&
			//mVLivePrograms[i].m_strCreator == strCreator)
		{
			pRet = new CLiveProgram();
			pRet->m_strId = mVLivePrograms[i].m_strId;
			pRet->m_strName = mVLivePrograms[i].m_strName;
			pRet->m_strCreator = mVLivePrograms[i].m_strCreator;
			pRet->m_liveState = mVLivePrograms[i].m_liveState;
			break;
		}
	}
	return pRet;
}

void CInteracteLiveDlg::setConfig(CConfigManager* pConfig)
{
	m_pConfig = pConfig;
}

void CInteracteLiveDlg::changeShowStyle(string strUserId)
{
	string changeUserId = "";
	if (m_pDataShowView != NULL)
	{
		changeUserId = m_pDataShowView->changeShowStyle(strUserId, true);
		m_pDataShowView->setShowPictures();
	}
	if (m_pLiveManager != NULL)
	{
		m_pLiveManager->changeToBig(strUserId);
		if (changeUserId != "")
		{
			m_pLiveManager->changeToSmall(changeUserId);
		}
	}
}
void CInteracteLiveDlg::closeCurrentLive()
{
	m_ApplyUploadButton.EnableWindow(FALSE);
	m_ExitUpload.EnableWindow(FALSE);
	if (m_pDataShowView != NULL)
	{
		if (m_pLiveManager != NULL)
		{
			m_pLiveManager->leaveLive();
		}

		stopGetData();
		if (m_pSoundManager != NULL)
		{
			m_pSoundManager->stopSoundData();
		}

		m_pDataShowView->removeAllUser();
		SetEvent(m_hSetShowPicEvent);
	}
	if (m_pCurrentLive != NULL)
	{
		delete m_pCurrentLive;
		m_pCurrentLive = NULL;
	}
}

BOOL CInteracteLiveDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	LONG lStyle;
	lStyle = GetWindowLong(m_liveListListControl.m_hWnd, GWL_STYLE);
	lStyle &= ~LVS_TYPEMASK;
	lStyle |= LVS_REPORT;
	SetWindowLong(m_liveListListControl.m_hWnd, GWL_STYLE, lStyle);

	DWORD dwStyleLiveList = m_liveListListControl.GetExtendedStyle();
	dwStyleLiveList |= LVS_EX_FULLROWSELECT;                                        //选中某行使整行高亮(LVS_REPORT)
	dwStyleLiveList |= LVS_EX_GRIDLINES;                                            //网格线(LVS_REPORT)
	//dwStyle |= LVS_EX_CHECKBOXES;                                            //CheckBox
	m_liveListListControl.SetExtendedStyle(dwStyleLiveList);

	m_liveListListControl.InsertColumn(LIVE_VIDEO_ID, _T("ID"), LVCFMT_LEFT, 110);
	m_liveListListControl.InsertColumn(LIVE_VIDEO_NAME, _T("Name"), LVCFMT_LEFT, 120);
	m_liveListListControl.InsertColumn(LIVE_VIDEO_CREATER, _T("Creator"), LVCFMT_LEFT, 80);
	m_liveListListControl.InsertColumn(LIVE_VIDEO_STATUS, _T("liveState"), LVCFMT_LEFT, 100);

	
	getLiveList();

	CRect rect;
	::GetWindowRect(m_ShowArea, rect);
	CRect dlgRect;
	::GetWindowRect(this->m_hWnd, dlgRect);
	int left = rect.left - dlgRect.left - 7;
	int top = rect.top - dlgRect.top - 25;

	CRect showRect(left, top, left + rect.Width() - 5, top + rect.Height() - 15);
	m_hSetShowPicEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_pDataShowView = new CDataShowView();
	m_pDataShowView->setDrawRect(showRect);

	CPicControl *pPicControl = new CPicControl();
	pPicControl->Create(_T(""), WS_CHILD | WS_VISIBLE | SS_BITMAP, showRect, this, WM_USER + 100);
	//mShowPicControlVector[i] = pPicControl;
	pPicControl->ShowWindow(SW_SHOW);
	DWORD dwStyle = ::GetWindowLong(pPicControl->GetSafeHwnd(), GWL_STYLE);
	::SetWindowLong(pPicControl->GetSafeHwnd(), GWL_STYLE, dwStyle | SS_NOTIFY);

	m_pDataShowView->m_pPictureControlArr.push_back(pPicControl);
	pPicControl->setInfo(this);

	CRect rectClient = showRect;
	CRect rectChild(rectClient.right - (int)(rectClient.Width()*0.25), rectClient.top, rectClient.right, rectClient.bottom);

	for (int n = 0; n < 6; n++)
	{
		CPicControl *pPictureControl = new CPicControl();
		pPictureControl->setInfo(this);
		rectChild.top = rectClient.top + (long)(n * rectClient.Height()*0.25);
		rectChild.bottom = rectClient.top + (long)((n + 1) * rectClient.Height()*0.25);
		pPictureControl->Create(_T(""), WS_CHILD | WS_VISIBLE | SS_BITMAP, rectChild, this, WM_USER + 100 + n + 1);
		m_pDataShowView->m_pPictureControlArr.push_back(pPictureControl);
		DWORD dwStyle1 = ::GetWindowLong(pPictureControl->GetSafeHwnd(), GWL_STYLE);
		::SetWindowLong(pPictureControl->GetSafeHwnd(), GWL_STYLE, dwStyle1 | SS_NOTIFY);
	}

	m_hSetShowPicThread = CreateThread(NULL, 0, SetShowPicThread, (void*)this, 0, 0); // 创建线程
	m_ApplyUploadButton.EnableWindow(FALSE);
	m_ExitUpload.EnableWindow(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control				  // 异常: OCX 属性页应返回 FALSE
}



/**
 * 有主播加入
 * @param liveID 直播ID
 * @param actorID 新加入的主播ID
 */
void CInteracteLiveDlg::onActorJoined(string liveID, string actorID)
{
	if (m_pDataShowView != NULL)
	{
		int nCount = m_pDataShowView->getUserCount();
		bool bBig = false;
		if (nCount == 0)
		{
			bBig = true;
		}
		m_pDataShowView->addUser(actorID, bBig);
		SetEvent(m_hSetShowPicEvent);
	}
}
/**
 * 有主播离开
 * @param liveID 直播ID
 * @param actorID 离开的主播ID
 */
void CInteracteLiveDlg::onActorLeft(string liveID, string actorID)
{
	if (m_pDataShowView != NULL)
	{
		m_pDataShowView->removeUser(actorID);
		m_pLiveManager->changeToSmall(actorID);
		string strUserId = "";
		bool bLeft = m_pDataShowView->isLeftOneUser(strUserId);
		if (bLeft && strUserId != "")
		{
			m_pLiveManager->changeToBig(strUserId);
			m_pDataShowView->changeShowStyle(strUserId, true);
		}
		SetEvent(m_hSetShowPicEvent);
	}
}

/**
 * 房主收到其他用户的连麦申请
 * @param liveID
 * @param applyUserID
 */
void CInteracteLiveDlg::onApplyBroadcast(string liveID, string applyUserID)
{
	CString strMsg;
	strMsg.Format("是否同意用户:%s申请连麦", applyUserID.c_str());
	if (m_pLiveManager != NULL)
	{
		if (IDYES == AfxMessageBox(strMsg, MB_YESNO))
		{		
			m_pLiveManager->agreeApplyToBroadcaster(applyUserID);
			onActorJoined(liveID, applyUserID);
		}
		else
		{
			m_pLiveManager->refuseApplyToBroadcaster(applyUserID);
		}
	}
}

/**
 * 申请连麦用户收到的回复
 * @param liveID
 * @param result
 */
void CInteracteLiveDlg::onApplyResponsed(string liveID, bool bAgree)
{
	if (bAgree)
	{
		PostMessage(WM_RECV_LIVE_MSG, 1, 0);
	}
	else
	{
		AfxMessageBox("对方拒绝连麦请求");
	}
}

/**
* 普通用户收到连麦邀请
* @param liveID 直播ID
* @param applyUserID 发出邀请的人的ID（主播ID）
*/
void CInteracteLiveDlg::onInviteBroadcast(string liveID, string applyUserID)
{
}

/**
* 主播收到的邀请连麦结果
* @param liveID 直播ID
* @param result 邀请结果
*/
void CInteracteLiveDlg::onInviteResponsed(string liveID)
{
}

/**
 * 一些异常情况引起的出错，请在收到该回调后主动断开直播
 * @param liveID 直播ID
 * @param error 错误信息
 */
void CInteracteLiveDlg::onLiveError(string liveID, string error)
{
	m_ApplyUploadButton.EnableWindow(FALSE);
	m_ExitUpload.EnableWindow(FALSE);
	if (m_pDataShowView != NULL)
	{
		m_pDataShowView->removeAllUser();
		m_pDataShowView->setShowPictures();
	}
	stopGetData();
	if (m_pSoundManager != NULL)
	{
		m_pSoundManager->stopSoundData();
	}
	if (m_pCurrentLive != NULL)
	{
		delete m_pCurrentLive;
		m_pCurrentLive = NULL;
	}
}

/**
 * 聊天室成员数变化
 * @param number
 */
void CInteracteLiveDlg::onMembersUpdated(int number)
{
}

/**
 * 自己被踢出聊天室
 */
void CInteracteLiveDlg::onSelfKicked()
{
}

/**
 * 自己被踢出聊天室
 */
void CInteracteLiveDlg::onSelfMuted(int seconds)
{
}

/**
* 连麦者的连麦被强制停止
* @param liveID 直播ID
*/
void CInteracteLiveDlg::onCommandToStopPlay(string  liveID)
{
	m_ApplyUploadButton.EnableWindow(FALSE);
	m_ExitUpload.EnableWindow(FALSE);

	if (m_pDataShowView != NULL)
	{
		m_pDataShowView->removeAllUser();
		m_pDataShowView->setShowPictures();
	}
	stopGetData();
	if (m_pSoundManager != NULL)
	{
		m_pSoundManager->stopSoundData();
	}
	if (m_pLiveManager != NULL)
	{
		m_pLiveManager->leaveLive();
	}
	if (m_pCurrentLive != NULL)
	{
		delete m_pCurrentLive;
		m_pCurrentLive = NULL;
	}
}
/**
 * 收到消息
 * @param message
 */
void CInteracteLiveDlg::onReceivedMessage(CIMMessage* pMessage)
{
}

/**
 * 收到私信消息
 * @param message
 */
void CInteracteLiveDlg::onReceivePrivateMessage(CIMMessage* pMessage)
{
}


int CInteracteLiveDlg::getRealtimeData(string strUserId, uint8_t* data, int len)
{
	return 0;
}
int CInteracteLiveDlg::getVideoRaw(string strUserId, int w, int h, uint8_t* videoData, int videoDataLen)
{
	if (m_pDataShowView != NULL)
	{
		m_pDataShowView->drawPic(FMT_YUV420P, strUserId, w, h, videoData, videoDataLen);
	}
	return 0;
}

void CInteracteLiveDlg::OnPaint()
{
	CPaintDC dc(this); // device context for painting
					   // TODO: 在此处添加消息处理程序代码
					   // 不为绘图消息调用 __super::OnPaint()
	if (m_pDataShowView != NULL)
	{
		if (m_pDataShowView != NULL)
		{
			CDC* pDC = m_pDataShowView->m_pPictureControlArr[0]->GetWindowDC();    //获得显示控件的DC
			pDC->SetStretchBltMode(COLORONCOLOR);
			FillRect(dc.m_hDC, &m_pDataShowView->m_DrawRect, CBrush(RGB(0, 0, 0)));
			m_pDataShowView->m_pPictureControlArr[0]->ReleaseDC(pDC);
		}
	}
}

void CInteracteLiveDlg::OnDestroy()
{
	if (m_pSoundManager != NULL)
	{
		m_pSoundManager->stopSoundData();
		Sleep(100);
		delete m_pSoundManager;
		m_pSoundManager = NULL;
	}
	closeCurrentLive();
	__super::OnDestroy();
}


void CInteracteLiveDlg::OnBnClickedButtonCreateNewLive()
{
	m_ApplyUploadButton.EnableWindow(FALSE);
	m_ExitUpload.EnableWindow(FALSE);

	if (m_pDataShowView != NULL)
	{
		m_pDataShowView->removeAllUser();
		m_pDataShowView->setShowPictures();
	}
	stopGetData();
	if (m_pSoundManager != NULL)
	{
		m_pSoundManager->stopSoundData();
	}
	if (m_pCurrentLive != NULL)
	{
		delete m_pCurrentLive;
		m_pCurrentLive = NULL;
	}

	CString strName = "";
	bool bPublic = false;
	XH_CHATROOM_TYPE chatRoomType = XH_CHATROOM_TYPE::XH_CHATROOM_TYPE_GLOBAL_PUBLIC;
	XH_LIVE_TYPE channelType = XH_LIVE_TYPE::XH_LIVE_TYPE_GLOBAL_PUBLIC;
	CCreateLiveDialog dlg(m_pUserManager);
	if (dlg.DoModal() == IDOK)
	{
		strName = dlg.m_strLiveName;
		bPublic = dlg.m_bPublic;
	}
	else
	{
		return;
	}
	if (m_pLiveManager != NULL)
	{
		string strLiveId = m_pLiveManager->createLive(strName.GetBuffer(0), chatRoomType, channelType);
		if (strLiveId != "")
		{
			string strInfo = "{\"id\":\"";
			strInfo += strLiveId;
			strInfo += "\",\"creator\":\"";
			strInfo += m_pUserManager->m_ServiceParam.m_strUserId;
			strInfo += "\",\"name\":\"";
			strInfo += strName.GetBuffer(0);
			strInfo += "\"}";
			if (m_pConfig != NULL && m_pConfig->m_bAEventCenterEnable)
			{
				CInterfaceUrls::demoSaveToList(m_pUserManager->m_ServiceParam.m_strUserId, CHATROOM_LIST_TYPE_LIVE, strLiveId, strInfo);
			}
			else
			{	
				m_pLiveManager->saveToList(m_pUserManager->m_ServiceParam.m_strUserId, CHATROOM_LIST_TYPE_LIVE, strLiveId, strInfo);
			}
			getLiveList();
			bool bRet = m_pLiveManager->startLive(strLiveId);
			if (bRet)
			{
				if (m_pCurrentLive == NULL)
				{
					m_pCurrentLive = new CLiveProgram();
				}
				m_pCurrentLive->m_strId = strLiveId.c_str();
				m_pCurrentLive->m_strName = strName;
				m_pCurrentLive->m_strCreator = m_pUserManager->m_ServiceParam.m_strUserId.c_str();
				startGetData((CROP_TYPE)m_pUserManager->m_ServiceParam.m_CropType, true);
				if (m_pSoundManager != NULL)
				{
					m_pSoundManager->startSoundData(true);
				}
			}
		}
		else
		{
			AfxMessageBox("创建直播失败!");
		}
	}
}

void CInteracteLiveDlg::OnNMClickList2(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
	m_ApplyUploadButton.EnableWindow(FALSE);
	m_ExitUpload.EnableWindow(FALSE);
	if (m_pDataShowView != NULL)
	{
		m_pDataShowView->removeAllUser();
		m_pDataShowView->setShowPictures();
	}
	stopGetData();
	if (m_pSoundManager != NULL)
	{
		m_pSoundManager->stopSoundData();
	}
	if (m_pCurrentLive != NULL)
	{
		delete m_pCurrentLive;
		m_pCurrentLive = NULL;
	}

	int nItem = -1;
	if (pNMItemActivate != NULL)
	{
		nItem = pNMItemActivate->iItem;
		if (nItem >= 0)
		{
			CString strId = m_liveListListControl.GetItemText(nItem, LIVE_VIDEO_ID);
			CString strName = m_liveListListControl.GetItemText(nItem, LIVE_VIDEO_NAME);
			CString strCreater = m_liveListListControl.GetItemText(nItem, LIVE_VIDEO_CREATER);

			CLiveProgram* pLiveProgram = getLiveProgram(strId, strName, strCreater);
			CString strUserId = m_pUserManager->m_ServiceParam.m_strUserId.c_str();
			if (pLiveProgram != NULL)
			{
				//if (/*pLiveProgram->m_liveState == false && */pLiveProgram->m_strCreator != strUserId)
				//{
				//	AfxMessageBox("直播尚未开始");
				//	return;
				//}
				string strId = pLiveProgram->m_strId.GetBuffer(0);
				if (strId.length() == 32)
				{
					if (pLiveProgram->m_strCreator == strUserId)
					{
						bool bRet = m_pLiveManager->startLive(strId);
						if (bRet)
						{
							if (m_pCurrentLive == NULL)
							{
								m_pCurrentLive = new CLiveProgram();
							}
							m_pCurrentLive->m_strId = pLiveProgram->m_strId;
							m_pCurrentLive->m_strName = pLiveProgram->m_strName;
							m_pCurrentLive->m_strCreator = pLiveProgram->m_strCreator;

							startGetData((CROP_TYPE)m_pUserManager->m_ServiceParam.m_CropType, true);
							if (m_pSoundManager != NULL)
							{
								m_pSoundManager->startSoundData(true);
							}
						}
						else
						{
							AfxMessageBox("开启直播失败");
						}
					}
					else
					{
						bool bRet = m_pLiveManager->watchLive(strId);
						if (bRet)
						{
							m_ApplyUploadButton.EnableWindow(TRUE);
							if (m_pCurrentLive == NULL)
							{
								m_pCurrentLive = new CLiveProgram();
							}
							m_pCurrentLive->m_strId = pLiveProgram->m_strId;
							m_pCurrentLive->m_strName = pLiveProgram->m_strName;
							m_pCurrentLive->m_strCreator = pLiveProgram->m_strCreator;
							
							if (m_pSoundManager != NULL)
							{
								m_pSoundManager->startSoundData(false);
							}
						}
						else
						{
							AfxMessageBox("观看直播失败");
						}
					}
				}
				else
				{
					CString strMessage = "";
					strMessage.Format("err id %s", strId.c_str());
					AfxMessageBox(strMessage);
				}
				delete pLiveProgram;
				pLiveProgram = NULL;
			}

		}
	}
}

void CInteracteLiveDlg::addUpId()
{
	onActorJoined(m_pCurrentLive->m_strId.GetBuffer(0), m_pUserManager->m_ServiceParam.m_strUserId);
}
void CInteracteLiveDlg::insertVideoRaw(uint8_t* videoData, int dataLen, int isBig)
{
	if (m_pLiveManager != NULL)
	{
		m_pLiveManager->insertVideoRaw(videoData, dataLen, isBig);
	}	
}

int CInteracteLiveDlg::cropVideoRawNV12(int w, int h, uint8_t* videoData, int dataLen, int yuvProcessPlan, int rotation, int needMirror, uint8_t* outVideoDataBig, uint8_t* outVideoDataSmall)
{
	int ret = 0;
	if (m_pLiveManager != NULL)
	{
		ret = m_pLiveManager->cropVideoRawNV12(w, h, videoData, dataLen, yuvProcessPlan, 0, 0, outVideoDataBig, outVideoDataSmall);
	}
	return ret;
}
void CInteracteLiveDlg::drawPic(YUV_TYPE type, int w, int h, uint8_t* videoData, int videoDataLen)
{
	if (m_pDataShowView != NULL)
	{
		m_pDataShowView->drawPic(type, m_pUserManager->m_ServiceParam.m_strUserId, w, h, videoData, videoDataLen);
	}
}
void CInteracteLiveDlg::getLocalSoundData(char* pData, int nLength)
{
	if (m_pLiveManager != NULL)
	{
		m_pLiveManager->insertAudioRaw((uint8_t*)pData, nLength);
	}
}

void CInteracteLiveDlg::querySoundData(char** pData, int* nLength)
{
	if (m_pLiveManager != NULL)
	{
		m_pLiveManager->querySoundData((uint8_t**)pData, nLength);
	}
}


void CInteracteLiveDlg::OnBnClickedButtonLiveApplyUpload()
{
	m_pLiveManager->applyToBroadcaster(m_pCurrentLive->m_strCreator.GetBuffer(0));
}


void CInteracteLiveDlg::OnBnClickedButtonLiveExitUpload()
{
	m_ApplyUploadButton.EnableWindow(FALSE);
	m_ExitUpload.EnableWindow(FALSE);
	if (m_pDataShowView != NULL)
	{
		m_pDataShowView->removeAllUser();
		m_pDataShowView->setShowPictures();
	}
	stopGetData();
	if (m_pSoundManager != NULL)
	{
		m_pSoundManager->stopSoundData();
	}

	bool bRet = m_pLiveManager->watchLive(m_pCurrentLive->m_strId.GetBuffer(0));
	if (bRet)
	{
		m_ApplyUploadButton.EnableWindow(TRUE);
		if (m_pSoundManager != NULL)
		{
			m_pSoundManager->startSoundData(false);
		}
	}
	else
	{
		AfxMessageBox("join live failed");
	}
}
