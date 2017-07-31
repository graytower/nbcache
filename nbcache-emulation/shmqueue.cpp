#include	"shmqueue.h"

QueueOper::QueueOper()
{
    m_tQueueKey = 0;
    m_pQueueCtl = NULL;
    m_pPkgRoot = NULL;
    m_iSemLockID = -1;
    bzero(m_sErrMsg, sizeof(m_sErrMsg));
}

QueueOper::~QueueOper()
{
    if (m_pQueueCtl)
    {
        shmdt(m_pQueueCtl);
        m_pQueueCtl = NULL;
        m_pPkgRoot = NULL;
    }
}

int QueueOper::CreateShm(key_t tShmKey, int iShmSize, int iFlag)
{
    int iShmId;
    
    if (iFlag == SHM_CREATE)
    {
        iShmId = shmget(tShmKey, iShmSize, IPC_CREAT | IPC_EXCL | 0666);
        return iShmId;
    }
    else
    {
        iShmId = shmget(tShmKey, 0, 0666);
        return iShmId;
    }
}

int QueueOper::CreateQueue(key_t tQueueKey, int iQueueSize)
{
    int iQueueShmId;
    char *pQueueShm = NULL;
    
    this->~QueueOper();
    
    if (iQueueSize <= 0)
        iQueueSize = DEFAULT_QUEUE_SIZE;
    
    m_tQueueKey = tQueueKey;
    if ((iQueueShmId = CreateShm(m_tQueueKey, iQueueSize + sizeof(QueueCtl),
            SHM_CREATE)) < 0)
    {
        snprintf(m_sErrMsg, sizeof(m_sErrMsg), "Create queue fail:%s",
                strerror(errno));
        return -1;
    }
    pQueueShm = (char *) shmat(iQueueShmId, NULL, 0);
    if (pQueueShm == NULL || (long) pQueueShm == -1
            || SemLockInit(m_tQueueKey, SHM_CREATE) < 0)
    {
        snprintf(m_sErrMsg, sizeof(m_sErrMsg), "Create queue fail:%s",
                strerror(errno));
        shmctl(iQueueShmId, IPC_RMID, (struct shmid_ds *) NULL);
        this->~QueueOper();
        return -1;
    }
    m_pQueueCtl = (QueueCtl *) pQueueShm;
    m_pPkgRoot = (char *) (pQueueShm + sizeof(QueueCtl));
    
    m_pQueueCtl->cValidFlag = 1;
    m_pQueueCtl->iPkgNum = 0;
    m_pQueueCtl->iSpaceSize = iQueueSize;
    m_pQueueCtl->iUsingSpaceSize = 0;
    m_pQueueCtl->iQueueHeadPos = 0;
    m_pQueueCtl->iQueueTailPos = 0;
    bzero(m_pPkgRoot, iQueueSize);
    
    return 0;
}

int QueueOper::AttachQueue(key_t tQueueKey)
{
    int iQueueShmId;
    char *pQueueShm = NULL;
    
    this->~QueueOper();
    
    m_tQueueKey = tQueueKey;
    if ((iQueueShmId = CreateShm(m_tQueueKey, 0, SHM_ATTACH)) < 0)
    {
        snprintf(m_sErrMsg, sizeof(m_sErrMsg), "Attach queue fail:%s",
                strerror(errno));
        return -1;
    }
    pQueueShm = (char *) shmat(iQueueShmId, NULL, 0);
    if (pQueueShm == NULL || (long) pQueueShm == -1
            || SemLockInit(m_tQueueKey, SHM_ATTACH) < 0)
    {
        snprintf(m_sErrMsg, sizeof(m_sErrMsg), "Attach queue fail:%s",
                strerror(errno));
        this->~QueueOper();
        return -1;
    }
    m_pQueueCtl = (QueueCtl *) pQueueShm;
    m_pPkgRoot = (char *) (pQueueShm + sizeof(QueueCtl));
    
    return 0;
}

//nick 这里没有实现像msgq当队列满时写入阻塞的功能
int QueueOper::PushDataIntoQueueBack(const char *sValue, int iValueLen)
{
    int iPkgLenWord;
    char sHeader[sizeof(int) + 1] =
        {
        0
        }; //nick 这个+1没有必要
    
    if (!m_pQueueCtl)
    {
        snprintf(m_sErrMsg, sizeof(m_sErrMsg),
                "Store data fail:Not init locally");
        return -3;
    }
    if (m_pQueueCtl->cValidFlag <= 0)
    {
        snprintf(m_sErrMsg, sizeof(m_sErrMsg), "Store data fail:Shm deleted");
        this->~QueueOper();
        return -4;
    }
    if (sValue == NULL || iValueLen <= 0 || iValueLen >= MAX_STOREBLK_LEN)
    {
        snprintf(m_sErrMsg, sizeof(m_sErrMsg),
                "Store data fail:Data null or too long");
        return -2;
    }
    iPkgLenWord = sizeof(int) + iValueLen;
    SemLock();
    if ((m_pQueueCtl->iUsingSpaceSize + iPkgLenWord) > m_pQueueCtl->iSpaceSize)
    {
        snprintf(m_sErrMsg, sizeof(m_sErrMsg), "Store data fail:Space full");
        SemUnLock();
        return -1;
    }
    memcpy(sHeader, &iPkgLenWord, sizeof(int));
    StoreData(sHeader, (int) sizeof(int));
    StoreData(sValue, iValueLen);
    m_pQueueCtl->iPkgNum++;
    m_pQueueCtl->iUsingSpaceSize += iPkgLenWord;
    SemUnLock();
    
    return 0;
}

int QueueOper::ReadDataFromQueueHead(char *pToStoreSpace, int iSpaceLen)
{
    int iPkgLenWord;
    char sHeader[sizeof(int) + 1] =
        {
        0
        };
    
    if (!m_pQueueCtl)
    {
        snprintf(m_sErrMsg, sizeof(m_sErrMsg),
                "Read data fail:Not init locally");
        return -3;
    }
    if (m_pQueueCtl->cValidFlag <= 0)
    {
        snprintf(m_sErrMsg, sizeof(m_sErrMsg), "Read data fail:Shm deleted");
        this->~QueueOper();
        return -4;
    }
    if (pToStoreSpace == NULL || iSpaceLen <= 0)
    {
        snprintf(m_sErrMsg, sizeof(m_sErrMsg),
                "Read data fail:Input param not valid");
        return -2;
    }
    SemLock();
    if (m_pQueueCtl->iPkgNum == 0 || m_pQueueCtl->iUsingSpaceSize == 0) // 队列空直接返回
    {
        snprintf(m_sErrMsg, sizeof(m_sErrMsg), "Read data fail:Empty queue");
        SemUnLock();
        return -1;
    }
    ReadData(sHeader, sizeof(int));
    memcpy(&iPkgLenWord, sHeader, sizeof(int));
    ReadData(m_sBuffTmp, iPkgLenWord - (int) sizeof(int));
    m_pQueueCtl->iPkgNum--; // 包数量 -1 
    m_pQueueCtl->iUsingSpaceSize -= iPkgLenWord; // 队列有效字节数减去相应值
    SemUnLock();
    
    iPkgLenWord -= sizeof(int);
    m_sBuffTmp[iPkgLenWord] = 0;
    if (iPkgLenWord <= iSpaceLen)
        memcpy(pToStoreSpace, m_sBuffTmp, iPkgLenWord);
    else
        memcpy(pToStoreSpace, m_sBuffTmp, iSpaceLen); //nick 如果给的pToStoreSpace不够大，则数据后半截丢失
                
    return iPkgLenWord; //nick 通过返回值可以判断是否存在后半截丢失的情况
}

int QueueOper::StoreData(const char *sData, int iDataLen)
{
    int iCopyLen;
    
    if ((m_pQueueCtl->iQueueTailPos + iDataLen) > m_pQueueCtl->iSpaceSize)
        iCopyLen = m_pQueueCtl->iSpaceSize - m_pQueueCtl->iQueueTailPos;
    else
        iCopyLen = iDataLen;
    
    memcpy(m_pPkgRoot + m_pQueueCtl->iQueueTailPos, sData, iCopyLen);
    /**
     * 注意，这里把共享内容逻辑上头尾相接了起来
     */
    if (iCopyLen < iDataLen)
    {
        memcpy(m_pPkgRoot, sData + iCopyLen, iDataLen - iCopyLen);
    }
    m_pQueueCtl->iQueueTailPos += iDataLen;
    m_pQueueCtl->iQueueTailPos %= m_pQueueCtl->iSpaceSize;
    return 0;
}

int QueueOper::ReadData(char *sReadTo, int iDataLen)
{
    int iCopyLen;
    int iHeadPos;
    char *pPos = NULL;
    
    iHeadPos = m_pQueueCtl->iQueueHeadPos;
    
    if ((m_pQueueCtl->iSpaceSize - iHeadPos) < iDataLen)
        iCopyLen = m_pQueueCtl->iSpaceSize - iHeadPos; // 需要循环读的情况
    else
        iCopyLen = iDataLen;
    pPos = m_pPkgRoot + iHeadPos;
    memcpy(sReadTo, pPos, iCopyLen);
    if (iCopyLen < iDataLen)
        memcpy(sReadTo + iCopyLen, m_pPkgRoot, iDataLen - iCopyLen); // 循环读时,从头读剩余数据
    m_pQueueCtl->iQueueHeadPos += iDataLen; // 读指针移动
    m_pQueueCtl->iQueueHeadPos %= m_pQueueCtl->iSpaceSize;
    
    return 0;
}

int QueueOper::ClearQueue()
{
    if (!m_pQueueCtl)
    {
        snprintf(m_sErrMsg, sizeof(m_sErrMsg),
                "Clear queue fail:Not init locally");
        return -1;
    }
    SemLock();
    m_pQueueCtl->iPkgNum = 0;
    m_pQueueCtl->iUsingSpaceSize = 0;
    m_pQueueCtl->iQueueHeadPos = 0;
    m_pQueueCtl->iQueueTailPos = 0;
    bzero(m_pPkgRoot, m_pQueueCtl->iSpaceSize);
    SemUnLock();
    return 0;
}

int QueueOper::DestroyQueue()
{
    int iQueueShmId;
    
    if (!m_pQueueCtl)
    {
        snprintf(m_sErrMsg, sizeof(m_sErrMsg),
                "Destory queue fail:Not init locally");
        return -1;
    }
    m_pQueueCtl->cValidFlag = 0;
    this->~QueueOper();
    iQueueShmId = CreateShm(m_tQueueKey, 0, SHM_ATTACH);
    shmctl(iQueueShmId, IPC_RMID, (struct shmid_ds *) NULL);
    SemLockRm();
    
    return 0;
}

int QueueOper::GetInQueuePkgNum()
{
    if (!m_pQueueCtl)
    {
        snprintf(m_sErrMsg, sizeof(m_sErrMsg),
                "Get queue pkg num fail:Not init locally");
        return -1;
    }
    return m_pQueueCtl->iPkgNum;
}

int QueueOper::SemLockInit(key_t tSemKey, int iFlag)
{
    if (iFlag == SHM_CREATE)
    {
        m_iSemLockID = semget(tSemKey, 1, IPC_CREAT | 0666);
        if (m_iSemLockID < 0)
        {
            snprintf(m_sErrMsg, sizeof(m_sErrMsg), "Sem init fail:%s",
                    strerror(errno));
            return -1;
        }
        if (semctl(m_iSemLockID, 0, SETVAL, (int) 1) < 0)
        {
            snprintf(m_sErrMsg, sizeof(m_sErrMsg), "Sem init fail:%s",
                    strerror(errno));
            SemLockRm();
            return -2;
        }
        return m_iSemLockID;
    }
    m_iSemLockID = semget(tSemKey, 1, 0666);
    if (m_iSemLockID < 0)
    {
        snprintf(m_sErrMsg, sizeof(m_sErrMsg), "Sem init fail:%s",
                strerror(errno));
        return -1;
    }
    return m_iSemLockID;
}

void QueueOper::SemLockRm()
{
    if (m_iSemLockID < 0)
        return;

    semctl(m_iSemLockID, IPC_RMID, 0);
    m_iSemLockID = -1;
}

int QueueOper::SemLock()
{
    sembuf stSemBuf;
    
    if (m_iSemLockID < 0)
    {
        snprintf(m_sErrMsg, sizeof(m_sErrMsg), "Sem lock fail:Not init");
        return -1;
    }
    stSemBuf.sem_num = 0;
    stSemBuf.sem_op = -1;
    stSemBuf.sem_flg = SEM_UNDO;
    
    //nick 阻塞
    if (semop(m_iSemLockID, &stSemBuf, 1) < 0)
    {
        snprintf(m_sErrMsg, sizeof(m_sErrMsg), "Sem lock fail:%s",
                strerror(errno));
        return -1;
    }
    return 0;
}

int QueueOper::SemUnLock()
{
    sembuf stSemBuf;
    
    if (m_iSemLockID < 0)
    {
        snprintf(m_sErrMsg, sizeof(m_sErrMsg), "Sem unlock fail:Not init");
        return -1;
    }
    stSemBuf.sem_num = 0;
    stSemBuf.sem_op = 1;
    stSemBuf.sem_flg = SEM_UNDO;
    
    if (semop(m_iSemLockID, &stSemBuf, 1) < 0)
    {
        snprintf(m_sErrMsg, sizeof(m_sErrMsg), "Sem unlock fail:%s",
                strerror(errno));
        return -1;
    }
    return 0;
}

char *QueueOper::GetErrMsg()
{
    return m_sErrMsg;
}

//added by ryansu 2011-12-12

//返回共享内存总空间大小
int QueueOper::GetTotalSize()
{
    return m_pQueueCtl->iSpaceSize;
}

//返回已使用空间
int QueueOper::GetUsingSize()
{
    return m_pQueueCtl->iUsingSpaceSize;
}

//返回已使用率
float QueueOper::GetUsingRate()
{
    return ((float) m_pQueueCtl->iUsingSpaceSize)
            / ((float) m_pQueueCtl->iSpaceSize);
}

//end of add

