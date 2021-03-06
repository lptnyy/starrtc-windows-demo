#pragma once
#include "CUserManager.h"
#include "XHMeetingManager.h"
#include "CLiveProgram.h"
#include "IMeetingManagerListener.h"
#include "IChatroomGetListListener.h"
// CMultipleMeetingDialog 对话框
#include "CDataShowView.h"
#include "CDataControl.h"
#include "CSoundManager.h"
#include "CConfigManager.h"
class CMultipleMeetingDialog : public CDialogEx, public IMeetingManagerListener, public IChatroomGetListListener, public CLocalDataControl, public CPicControlCallback, public ISoundCallback
{
	DECLARE_DYNAMIC(CMultipleMeetingDialog)

public:
	CMultipleMeetingDialog(CUserManager* pUserManager, CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CMultipleMeetingDialog();

	// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_MULTIPLE_MEETING };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	/**
	 * 查询聊天室列表回调
	 */
	virtual int chatroomQueryAllListOK(list<ChatroomInfo>& listData);
	/**
	 * 有新用户加入会议
	 * @param meetingID 会议ID
	 * @param userID 新加入者ID
	 */
	virtual void onJoined(string meetID, string userID);

	/**
	 * 有人离开会议
	 * @param meetingID 会议ID
	 * @param userID 离开者ID
	 */
	virtual void onLeft(string meetingID, string userID);

	/**
	 * 一些异常情况引起的出错，请在收到该回调后主动断开会议
	 * @param meetingID 会议ID
	 * @param error 错误信息
	 */
	virtual void onMeetingError(string meetingID, string error);


	/**
	 * 聊天室成员数变化
	 * @param number 变化后的会议人数
	 */
	virtual void onMembersUpdated(int number);

	/**
	 * 自己被踢出聊天室
	 */
	virtual void onSelfKicked();

	/**
	 * 自己被踢出聊天室
	 */
	virtual void onSelfMuted(int seconds);

	/**
	 * 收到消息
	 * @param message
	 */
	virtual void onReceivedMessage(CIMMessage* pMessage);

	/**
	 * 收到私信消息
	 * @param message
	 */
	virtual void onReceivePrivateMessage(CIMMessage* pMessage);
	virtual int getRealtimeData(string strUserId, uint8_t* data, int len);
	virtual int getVideoRaw(string strUserId, int w, int h, uint8_t* videoData, int videoDataLen);
public:
	virtual void addUpId();
	virtual void insertVideoRaw(uint8_t* videoData, int dataLen, int isBig);
	virtual int cropVideoRawNV12(int w, int h, uint8_t* videoData, int dataLen, int yuvProcessPlan, int rotation, int needMirror, uint8_t* outVideoDataBig, uint8_t* outVideoDataSmall);
	virtual void drawPic(YUV_TYPE type, int w, int h, uint8_t* videoData, int videoDataLen);
public:
	virtual void changeShowStyle(string strUserId);
	virtual void closeCurrentLive();
public:
	virtual BOOL OnInitDialog();
	void getMeetingList();
	void setConfig(CConfigManager* pConfig);
	afx_msg void OnBnClickedButtonCreateMeeting();
	afx_msg void OnNMClickListcontrolMeetingList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedButtonMeetingListbrush();
public:
	virtual void getLocalSoundData(char* pData, int nLength);
	virtual void querySoundData(char** pData, int* nLength);
private:
	CUserManager* m_pUserManager;
	CConfigManager* m_pConfig;
	XHMeetingManager* m_pMeetingManager;
	CSoundManager* m_pSoundManager;
	CDataShowView* m_pDataShowView;
	CListCtrl m_MeetingList;
	CLiveProgram* m_pCurrentProgram;
	string m_CurrentMeetingId;
public:
	CStatic m_ShowArea;
	
};
