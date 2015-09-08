/*
	Copyright (c) 2013-2014 EasyDarwin.ORG.  All rights reserved.
	Github: https://github.com/EasyDarwin
	WEChat: EasyDarwin
	Website: http://www.EasyDarwin.org
*/
#define _CRTDBG_MAP_ALLOC
#include <stdio.h>
#include "EasyRTMPAPI.h"
#include "EasyRTSPClientAPI.h"
#include "windows.h"

#define RTSPURL "rtsp://admin:admin@192.168.1.108/cam/realmonitor?channel=1&subtype=1"

#define SRTMP "rtmp://www.easydss.com/oflaDemo/rtsp"

Easy_RTMP_Handle rtmpHandle = 0;
Easy_RTSP_Handle fRTSPHandle = 0;

/* RTSPClient获取数据后回调给上层 */
int Easy_APICALL __RTSPSourceCallBack( int _chid, int *_chPtr, int _frameType, char *pbuf, RTSP_FRAME_INFO *frameinfo)
{
	bool bRet = false;

	//目前只处理视频
	if (_frameType == EASY_SDK_VIDEO_FRAME_FLAG)
	{
		if(frameinfo && frameinfo->length)
		{
			if ( frameinfo->type == EASY_SDK_VIDEO_FRAME_I)
			{
				/* 关键帧是SPS、PPS、IDR(均包含00 00 00 01)的组合,reserved1是sps结尾的偏移,reserved2是pps结尾的偏移 */
				if(rtmpHandle == 0)
				{
					rtmpHandle = EasyRTMP_Session_Create();

					bRet = EasyRTMP_Connect(rtmpHandle, SRTMP);
					if (!bRet)
					{
						printf("Fail to EasyRTMP_Connect ...\n");
					}

					bRet = EasyRTMP_InitMetadata(rtmpHandle, (const char*)pbuf+4,
						frameinfo->reserved1-4, (const char*)pbuf+frameinfo->reserved1+4 ,
						frameinfo->reserved2 - frameinfo->reserved1 -4, 25, 8000);
					if (bRet)
					{
						printf("Fail to InitMetadata ...\n");
					}
				}
				unsigned int timestamp = frameinfo->timestamp_sec*1000 + frameinfo->timestamp_usec/1000;
				bRet = EasyRTMP_SendH264Packet(rtmpHandle, (unsigned char*)pbuf+frameinfo->reserved2+4, frameinfo->length-frameinfo->reserved2-4, true, timestamp);
				if (!bRet)
				{
					printf("Fail to EasyRTMP_SendH264Packet(I-frame) ...\n");
				}
				else
				{
					printf("I");
				}
			}
			else if( frameinfo->type == EASY_SDK_VIDEO_FRAME_P)
			{
				if(rtmpHandle)
				{
					unsigned int timestamp = frameinfo->timestamp_sec*1000 + frameinfo->timestamp_usec/1000;
					bRet = EasyRTMP_SendH264Packet(rtmpHandle, (unsigned char*)pbuf+4, frameinfo->length-4, false, timestamp);
					if (!bRet)
					{
						printf("Fail to EasyRTMP_SendH264Packet(P-frame) ...\n");
					}
					else
					{
						printf("P");
					}
				}
			}				
		}	
	}
	return 0;
}

int main()
{
	//创建EasyRTSPClient
	EasyRTSP_Init(&fRTSPHandle);
	if (NULL == fRTSPHandle) return 0;

	unsigned int mediaType = EASY_SDK_VIDEO_FRAME_FLAG | EASY_SDK_AUDIO_FRAME_FLAG;

	//设置数据回调
	EasyRTSP_SetCallback(fRTSPHandle, __RTSPSourceCallBack);
	//打开RTSP网络串流
	EasyRTSP_OpenStream(fRTSPHandle, 0, RTSPURL, RTP_OVER_TCP, mediaType, 0, 0, NULL, 1000, 0);

	while(1)
	{
		Sleep(10);	
	};

	//是否RTMP推送
    EasyRTMP_Session_Release(rtmpHandle);
    rtmpHandle = 0;
   
	//关闭EasyRTSPClient拉取
	EasyRTSP_CloseStream(fRTSPHandle);
	//释放EasyRTSPClient
	EasyRTSP_Deinit(&fRTSPHandle);
	fRTSPHandle = NULL;

    return 0;
}