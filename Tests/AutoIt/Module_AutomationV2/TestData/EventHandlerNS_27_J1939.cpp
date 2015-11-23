/* This file is generated by BUSMASTER */
/* VERSION [1.1] */
/* BUSMASTER VERSION [1.9.0] */
/* PROTOCOL [J1939] */

/* Start J1939 include header */
#include <Windows.h>
#include <J1939Includes.h>

/* End J1939 include header */


/* Start J1939 global variable */

/* End J1939 global variable */


/* Start J1939 Function Prototype  */
GCC_EXTERN void GCC_EXPORT OnTimer_1000_1000( );
GCC_EXTERN void GCC_EXPORT OnEvent_DataConf(UINT32 unPGN, BYTE bySrc, BYTE byDest, BOOL bSuccess);
/* End J1939 Function Prototype  */

/* Start J1939 generated function - OnTimer_1000_1000 */
void OnTimer_1000_1000( )
{
STJ1939_MSG msg;


msg.m_sMsgProperties.m_byChannel = 1;

msg.m_sMsgProperties.m_eType = MSG_TYPE_DATA;

msg.m_sMsgProperties.m_eDirection = DIR_TX;


msg.m_sMsgProperties.m_uExtendedID.m_s29BitId.m_bySrcAddress = 0x0;


msg.m_sMsgProperties.m_uExtendedID.m_unExtID =0xFF00;
//msg.m_sMsgProperties.m_uExtendedID.m_s29BitId.m_uPGN.m_sPGN.m_byPDU_Specific=0xFF;

//msg.m_sMsgProperties.m_uExtendedID.m_s29BitId.m_uPGN.m_sPGN.m_byPDU_Format =0x00;
msg.m_sMsgProperties.m_uExtendedID.m_s29BitId.m_uPGN.m_sPGN.m_byDataPage =0x0;

msg.m_sMsgProperties.m_uExtendedID.m_s29BitId.m_uPGN.m_sPGN.m_byReserved =0x0;
  msg.m_sMsgProperties.m_uExtendedID.m_s29BitId.m_uPGN.m_sPGN.m_byPriority =0x7;
msg.m_unDLC = 21;

msg.m_pbyData = new BYTE[15];
BYTE tmp[] = {12, 34, 45, 56, 78,10,11,14,15,16,17,18,19,20,21,22,23,24,25,26,27};

memcpy(msg.m_pbyData, tmp, 5);

Trace("J1939 Timer Handler");

SendMsg (&msg);
}/* End J1939 generated function - OnTimer_1000_1000 */
/* Start J1939 generated function - OnEvent_DataConf */
void OnEvent_DataConf(UINT32 unPGN, BYTE bySrc, BYTE byDest, BOOL bSuccess)
{
Trace("Message is transmitted from simulated node");
}/* End J1939 generated function - OnEvent_DataConf */
