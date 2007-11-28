/*------------------------------------------------------------*/
/* ACULAB plc                                                 */
/*------------------------------------------------------------*/
/*                                                            */
/*                                                            */
/* Program File Name : pipe_interface.h                       */
/*                                                            */
/*           Purpose : Common data structures used by host    */
/*                     and service for pipe communication     */
/*                     under Windows NT/2000.                 */
/*                                                            */
/*           Create Date : 17th April 2002                    */
/*                                                            */
/*                                                            */
/*------------------------------------------------------------*/
/*                                                            */
/*                                                            */
/* Change History                                             */
/*                                                            */
/* rev: v5.10.0     07/03/2003 for v5.10.0 Release            */
/*                                                            */
/*                                                            */
/*------------------------------------------------------------*/

#ifndef PIPE_INTERFACE_H
#define PIPE_INTERFACE_H

typedef struct acu_service_msg {
  ACU_UINT           abi;
#ifdef _WIN32
  HANDLE             pendingMsgEvent;
  HANDLE             readMsgEvent;
  DWORD              srcProcessId;
#else
  ACU_ULONG          pendingMsgEvent;
  ACU_ULONG          readMsgEvent;
  ACU_ULONG          srcProcessId;
#endif
  ACU_INT            voip_card;
  ACU_INT            message_type;
  ACU_INT            function;
  ACU_INT            valid;
  ACU_INT            command_error;
}ACU_SERVICE_MSG;


#endif
