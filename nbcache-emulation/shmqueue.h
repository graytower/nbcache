#ifndef		_SHM_QUEUE_
#define		_SHM_QUEUE_

#include	<stdio.h>
#include	<unistd.h>
#include	<errno.h>
#include	<string.h>
#include	<sys/types.h>
#include	<sys/ipc.h>
#include	<sys/shm.h>
#include	<sys/sem.h>

#define		SHM_CREATE		1
#define		SHM_ATTACH		2

#define		DEFAULT_QUEUE_SIZE		4200 * 20001
//#define		DEFAULT_QUEUE_SIZE		75 * 20001
#define		MAX_STOREBLK_LEN		4200
//#define		DEFAULT_QUEUE_SIZE		20 * 1024 * 1024
//#define		MAX_STOREBLK_LEN		1024 * 1024

/*
#pragma	pack(1)
typedef	struct
{
	int		iPkgTotalLen;
}PkgHead;
#pragma	pack()
*/

#pragma	pack(1)
typedef	struct
{
	char	cValidFlag;
	int	iPkgNum;	// 当前Queue中Pkg个数

	int	iSpaceSize;	// 空间大小
	int	iUsingSpaceSize;

	int	iQueueHeadPos;	// Queue头位置，根据读写动态变化
	int	iQueueTailPos;	// Queue尾位置，根据读写动态变化
}QueueCtl;
#pragma	pack()


class	QueueOper
{
private:
	key_t		m_tQueueKey;
	QueueCtl	*m_pQueueCtl; //nick 指向共享内存的开始，存放QueueCtl数据
	char		*m_pPkgRoot; //nick 指向共享内存开始的QueueCtl数据的后一字节
	int		m_iSemLockID; // nick 
	char		m_sErrMsg[256];
	char		m_sBuffTmp[MAX_STOREBLK_LEN + 1]; // 临时存放读取出的数据块

private:
	int		SemLockInit(key_t,int ); //nick 注意这里的信号量和共享内存key一致，这样才能同步多进程，信号量为1
	void		SemLockRm(); // 删除信号量
	int		SemLock();
	int		SemUnLock();
	int		CreateShm(key_t,int,int);
	/**
	 * nick 将数据【直接】复制存放到共享队列中
	 */
	int		StoreData(const char *sData,int iDataLen);
	int		ReadData(char *,int);

public:
	QueueOper();
	~QueueOper();
	/*
		CreateQueue(key_t tQueueKey,int iQueueSize);
		创建tQueueKey指定的Shm-Queue,iQueueSize表示准备创建的队列数据空间大小
		若tQueueKey对应队列已存在，创建会失败
		返回值:
		-1	：	失败,可参考errno寻找具体原因
		0	：	成功。	
	*/
	int		CreateQueue(key_t tQueueKey,int iQueueSize = DEFAULT_QUEUE_SIZE);
	/*
		AttachQueue(key_t tQueueKey);
		关联当前进程到tQueueKey指定的Shm-Queue.
		返回值：
		-1	：	失败,可参考errno寻找具体原因
		0	：	成功。
	*/
	int		AttachQueue(key_t tQueueKey);
	/*
		PushDataIntoQueueBack(char *sValue,int iValueLen);
		将sValue保存在当前关联的Shm-Queue尾
		返回值:
		-4	:	Shm-Queue已删除
		-3	:	未关联Queue
		-2	:	期望存入的数据空或过大,超过MAX_STOREBLK_LEN
		-1	:	Shm-Queue空闲空间不足
		0	:	成功	
	*/
	int		PushDataIntoQueueBack(const char *,int );
	/*
		ReadDataFromQueueHead(char *pToStoreSpace,int iSpaceLen)
		从当前关联的Shm-Queue的头读取一个Pkg存放在pToStoreSpace指向、长为
		iSpaceLen的空间中,*建议此空间尽量大,如等于MAX_STOREBLK_LEN,避免包
		的后半截丢失。

		返回值：
		-4	：	Shm-Queue已删除
		-3	：	未关联Queue
		-2	：	入口参数错误
		-1	：	队列当前空,无数据
		>0	:	读取到的Pkg大小(bytes)	
	*/
	int		ReadDataFromQueueHead(char *,int);
	/*
		ClearQueue();
		清空当前Queue。
		返回值：
		-1	：	失败,未指定对应Queue
		0	：	成功
	*/
	int		ClearQueue();
	/*
		GetQueueSize()
		获取当前Queue中包个数。
		返回值：
		-1	：	失败,未指定对应Queue
		>=0	:	queue中包个数	
	*/
	int		GetInQueuePkgNum();
	/*
		DestroyQueue()
		销毁当前关联的Shm-Queue
		返回值:
		-1	:	失败,未指定对应Queue
		0	:	成功
	*/
	int		DestroyQueue();
	char	*GetErrMsg();

	//added by ryansu 2011-12-12
	
	//返回共享内存总空间大小
	int GetTotalSize();

	//返回已使用空间
	int GetUsingSize();

	//返回已使用率
	float GetUsingRate();

	//end of add
};


#endif
