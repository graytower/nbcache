#include <iostream>
#include <sys/time.h>
#include <string>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <pthread.h>
#include <sstream>
#include <vector>
#include <map>
#include "shmqueue.h"

using namespace std;

#define URLSIZE 45       // size of url
#define QUEUESIZE 20000  // max length of inside_que
#define FILE_NUM 500     // total number of files
#define SEG_SIZE 4096    // data segment size(4096Byte)
#define REQ_NUM 20001    // total number of requests

map<string, string> url_file;  // url-file hash table in content store
char pbuf[REQ_NUM][SEG_SIZE];
map<string, int> fib;          // fib table

struct pkt_head{          // interest packet head
    char url_name[URLSIZE];
    int segID;
    unsigned long pkt_start;
    int pid;
    unsigned long r_start;
    int flag;
};
struct inside_que{     // inside_que in one router for multi-threaded communication
    struct pkt_head pkt_arr[QUEUESIZE];
    int head, tail;
    int full, empty;
    pthread_mutex_t *mut;
    pthread_cond_t *notFull, *notEmpty;
};
struct para_intrstin{    // parameter for cs thread
    QueueOper *shm1;
    key_t queKey1;
    struct inside_que *iq;
    QueueOper *shm2;
    key_t queKey2;
    pthread_mutex_t *quelock;
    pthread_mutex_t *quemeasure_lock;
};
struct para_intrstout{    // parameter for pit/fib thread
    QueueOper *shm1;
    key_t queKey1;      // port 1
    QueueOper *shm2;
    key_t queKey2;      // port 2
    struct inside_que *iq;
};
struct para_data{         // parameter for data processing thread
	QueueOper *shm1;
	key_t queKey1;
	QueueOper *shm2;
	key_t queKey2;
    pthread_mutex_t *quelock;
};
struct para_measure{      // parameter for measuring router's neighbor_que load
    QueueOper *shm;
    key_t queKey;
    pthread_mutex_t *quemeasure_lock;
};

void *cs_action(void *args);     // cs thread function
void *pit_fib(void *args);       // pit/fib thread function
void *data_pro(void *args);      // data processing thread function
void *load_measure(void *args);  // router's neighbor_que load measuring function

// some functions for inside_que
struct inside_que *in_queInit(void);
void in_queDelete(struct inside_que *q);
void in_queAdd(struct inside_que *q, char url_in[], int ID_in, unsigned long pkt_start, int pid, unsigned long r_start, int flag);
void in_queDel(struct inside_que *q, struct pkt_head *out);

void split(const std::string &s, char delim, std::vector<std::string> &elems);

int main(){
    // create some neighbor_ques for multi-process communication
	int res = 1;
    QueueOper que = QueueOper();    
    key_t tQueueKey = 21;
    res = que.CreateQueue(tQueueKey);
    printf("CreateQueue %d\n", res);
    QueueOper que1 = QueueOper();    
    key_t tQueueKey1 = 1;
    res = que1.CreateQueue(tQueueKey1);
    printf("CreateQueue1 %d\n", res);
    QueueOper que2 = QueueOper();
    key_t tQueueKey2 = 2;
    res = que2.CreateQueue(tQueueKey2);
    printf("CreateQueue2 %d\n", res);
    QueueOper que3 = QueueOper();
    key_t tQueueKey3 = 3;
    res = que3.CreateQueue(tQueueKey3);
    printf("CreateQueue3 %d\n", res);
    QueueOper que4 = QueueOper();
    key_t tQueueKey4 = 4;
    res = que4.CreateQueue(tQueueKey4);
    printf("CreateQueue4 %d\n", res);
    QueueOper que5 = QueueOper();
    key_t tQueueKey5 = 5;
    res = que5.CreateQueue(tQueueKey5);
    printf("CreateQueue5 %d\n", res);
    QueueOper que6 = QueueOper();
    key_t tQueueKey6 = 6;
    res = que6.CreateQueue(tQueueKey6);
    printf("CreateQueue6 %d\n", res);
    QueueOper que7 = QueueOper();
    key_t tQueueKey7 = 7;
    res = que7.CreateQueue(tQueueKey7);
    printf("CreateQueue7 %d\n", res);
    QueueOper que8 = QueueOper();
    key_t tQueueKey8 = 8;
    res = que8.CreateQueue(tQueueKey8);
    printf("CreateQueue8 %d\n", res);
    QueueOper que9 = QueueOper();
    key_t tQueueKey9 = 9;
    res = que9.CreateQueue(tQueueKey9);
    printf("CreateQueue9 %d\n", res);
    QueueOper que10 = QueueOper();
    key_t tQueueKey10 = 10;
    res = que10.CreateQueue(tQueueKey10);
    printf("CreateQueue10 %d\n", res);
    QueueOper que12 = QueueOper();
    key_t tQueueKey12 = 12;
    res = que12.CreateQueue(tQueueKey12);
    printf("CreateQueue12 %d\n", res);
    QueueOper que13 = QueueOper();
    key_t tQueueKey13 = 13;
    res = que13.CreateQueue(tQueueKey13);
    printf("CreateQueue13 %d\n", res);
    QueueOper que14 = QueueOper();
    key_t tQueueKey14 = 14;
    res = que14.CreateQueue(tQueueKey14);
    printf("CreateQueue14 %d\n", res);
    QueueOper que15 = QueueOper();
    key_t tQueueKey15 = 15;
    res = que15.CreateQueue(tQueueKey15);
    printf("CreateQueue15 %d\n", res);
    QueueOper que16 = QueueOper();
    key_t tQueueKey16 = 16;
    res = que16.CreateQueue(tQueueKey16);
    printf("CreateQueue16 %d\n", res);
    QueueOper que17 = QueueOper();
    key_t tQueueKey17 = 17;
    res = que17.CreateQueue(tQueueKey17);
    printf("CreateQueue17 %d\n", res);
    QueueOper que19 = QueueOper();
    key_t tQueueKey19 = 19;
    res = que19.CreateQueue(tQueueKey19);
    printf("CreateQueue19 %d\n", res);
    QueueOper que20 = QueueOper();
    key_t tQueueKey20 = 20;
    res = que20.CreateQueue(tQueueKey20);
    printf("CreateQueue20 %d\n", res);

    // initialize all urls
    string url[FILE_NUM] = {
        "google.comgoogle.comgoogle.comgoogle.comgoogl",
        "facebook.comfacebook.comfacebook.comfacebook.",
        "youtube.comyoutube.comyoutube.comyoutube.comy",
        "baidu.combaidu.combaidu.combaidu.combaidu.com",
        "yahoo.comyahoo.comyahoo.comyahoo.comyahoo.com",
        "amazon.comamazon.comamazon.comamazon.comamazo",
        "wikipedia.orgwikipedia.orgwikipedia.orgwikipe",
        "qq.comqq.comqq.comqq.comqq.comqq.comqq.comqq.",
        "twitter.comtwitter.comtwitter.comtwitter.comt",
        "google.co.ingoogle.co.ingoogle.co.ingoogle.co",
        "taobao.comtaobao.comtaobao.comtaobao.comtaoba",
        "live.comlive.comlive.comlive.comlive.comlive.",
        "sina.com.cnsina.com.cnsina.com.cnsina.com.cns",
        "linkedin.comlinkedin.comlinkedin.comlinkedin.",
        "yahoo.co.jpyahoo.co.jpyahoo.co.jpyahoo.co.jpy",
        "weibo.comweibo.comweibo.comweibo.comweibo.com",
        "ebay.comebay.comebay.comebay.comebay.comebay.",
        "google.co.jpgoogle.co.jpgoogle.co.jpgoogle.co",
        "yandex.ruyandex.ruyandex.ruyandex.ruyandex.ru",
        "hao123.comhao123.comhao123.comhao123.comhao12",
        "vk.comvk.comvk.comvk.comvk.comvk.comvk.comvk.",
        "bing.combing.combing.combing.combing.combing.",
        "google.degoogle.degoogle.degoogle.degoogle.de",
        "t.cot.cot.cot.cot.cot.cot.cot.cot.cot.cot.cot",
        "msn.commsn.commsn.commsn.commsn.commsn.commsn",
        "instagram.cominstagram.cominstagram.cominstag",
        "amazon.co.jpamazon.co.jpamazon.co.jpamazon.co",
        "google.co.ukgoogle.co.ukgoogle.co.ukgoogle.co",
        "aliexpress.comaliexpress.comaliexpress.comali",
        "apple.comapple.comapple.comapple.comapple.com",
        "pinterest.compinterest.compinterest.compinter",
        "blogspot.comblogspot.comblogspot.comblogspot.",
        "ask.comask.comask.comask.comask.comask.comask",
        "wordpress.comwordpress.comwordpress.comwordpr",
        "tmall.comtmall.comtmall.comtmall.comtmall.com",
        "google.frgoogle.frgoogle.frgoogle.frgoogle.fr",
        "reddit.comreddit.comreddit.comreddit.comreddi",
        "google.com.brgoogle.com.brgoogle.com.brgoogle",
        "paypal.compaypal.compaypal.compaypal.compaypa",
        "onclickads.netonclickads.netonclickads.netonc",
        "mail.rumail.rumail.rumail.rumail.rumail.rumai",
        "tumblr.comtumblr.comtumblr.comtumblr.comtumbl",
        "sohu.comsohu.comsohu.comsohu.comsohu.comsohu.",
        "microsoft.commicrosoft.commicrosoft.commicros",
        "imgur.comimgur.comimgur.comimgur.comimgur.com",
        "google.rugoogle.rugoogle.rugoogle.rugoogle.ru",
        "xvideos.comxvideos.comxvideos.comxvideos.comx",
        "google.itgoogle.itgoogle.itgoogle.itgoogle.it",
        "imdb.comimdb.comimdb.comimdb.comimdb.comimdb.",
        "google.esgoogle.esgoogle.esgoogle.esgoogle.es",
        "gmw.cngmw.cngmw.cngmw.cngmw.cngmw.cngmw.cngmw",
        "netflix.comnetflix.comnetflix.comnetflix.comn",
        "fc2.comfc2.comfc2.comfc2.comfc2.comfc2.comfc2",
        "360.cn360.cn360.cn360.cn360.cn360.cn360.cn360",
        "amazon.deamazon.deamazon.deamazon.deamazon.de",
        "googleadservices.comgoogleadservices.comgoogl",
        "stackoverflow.comstackoverflow.comstackoverfl",
        "alibaba.comalibaba.comalibaba.comalibaba.coma",
        "go.comgo.comgo.comgo.comgo.comgo.comgo.comgo.",
        "google.com.mxgoogle.com.mxgoogle.com.mxgoogle",
        "google.cagoogle.cagoogle.cagoogle.cagoogle.ca",
        "google.com.hkgoogle.com.hkgoogle.com.hkgoogle",
        "ok.ruok.ruok.ruok.ruok.ruok.ruok.ruok.ruok.ru",
        "craigslist.orgcraigslist.orgcraigslist.orgcra",
        "tianya.cntianya.cntianya.cntianya.cntianya.cn",
        "amazon.co.ukamazon.co.ukamazon.co.ukamazon.co",
        "pornhub.compornhub.compornhub.compornhub.comp",
        "rakuten.co.jprakuten.co.jprakuten.co.jprakute",
        "blogger.comblogger.comblogger.comblogger.comb",
        "naver.comnaver.comnaver.comnaver.comnaver.com",
        "espn.go.comespn.go.comespn.go.comespn.go.come",
        "google.com.trgoogle.com.trgoogle.com.trgoogle",
        "xhamster.comxhamster.comxhamster.comxhamster.",
        "soso.comsoso.comsoso.comsoso.comsoso.comsoso.",
        "cnn.comcnn.comcnn.comcnn.comcnn.comcnn.comcnn",
        "outbrain.comoutbrain.comoutbrain.comoutbrain.",
        "nicovideo.jpnicovideo.jpnicovideo.jpnicovideo",
        "kat.crkat.crkat.crkat.crkat.crkat.crkat.crkat",
        "google.co.idgoogle.co.idgoogle.co.idgoogle.co",
        "dropbox.comdropbox.comdropbox.comdropbox.comd",
        "bbc.co.ukbbc.co.ukbbc.co.ukbbc.co.ukbbc.co.uk",
        "diply.comdiply.comdiply.comdiply.comdiply.com",
        "github.comgithub.comgithub.comgithub.comgithu",
        "flipkart.comflipkart.comflipkart.comflipkart.",
        "googleusercontent.comgoogleusercontent.comgoo",
        "amazon.inamazon.inamazon.inamazon.inamazon.in",
        "adcash.comadcash.comadcash.comadcash.comadcas",
        "xinhuanet.comxinhuanet.comxinhuanet.comxinhua",
        "google.com.augoogle.com.augoogle.com.augoogle",
        "ebay.deebay.deebay.deebay.deebay.deebay.deeba",
        "google.plgoogle.plgoogle.plgoogle.plgoogle.pl",
        "google.co.krgoogle.co.krgoogle.co.krgoogle.co",
        "popads.netpopads.netpopads.netpopads.netpopad",
        "dailymotion.comdailymotion.comdailymotion.com",
        "pixnet.netpixnet.netpixnet.netpixnet.netpixne",
        "nytimes.comnytimes.comnytimes.comnytimes.comn",
        "ebay.co.ukebay.co.ukebay.co.ukebay.co.ukebay.",
        "sogou.comsogou.comsogou.comsogou.comsogou.com",
        "bbc.combbc.combbc.combbc.combbc.combbc.combbc",
        "booking.combooking.combooking.combooking.comb",
        "jd.comjd.comjd.comjd.comjd.comjd.comjd.comjd.",
        "dailymail.co.ukdailymail.co.ukdailymail.co.uk",
        "adobe.comadobe.comadobe.comadobe.comadobe.com",
        "163.com163.com163.com163.com163.com163.com163",
        "livedoor.jplivedoor.jplivedoor.jplivedoor.jpl",
        "adnetworkperformance.comadnetworkperformance.",
        "bongacams.combongacams.combongacams.combongac",
        "wikia.comwikia.comwikia.comwikia.comwikia.com",
        "chase.comchase.comchase.comchase.comchase.com",
        "indiatimes.comindiatimes.comindiatimes.comind",
        "china.comchina.comchina.comchina.comchina.com",
        "coccoc.comcoccoc.comcoccoc.comcoccoc.comcocco",
        "china.com.cnchina.com.cnchina.com.cnchina.com",
        "huffingtonpost.comhuffingtonpost.comhuffingto",
        "uol.com.bruol.com.bruol.com.bruol.com.bruol.c",
        "directrev.comdirectrev.comdirectrev.comdirect",
        "buzzfeed.combuzzfeed.combuzzfeed.combuzzfeed.",
        "xnxx.comxnxx.comxnxx.comxnxx.comxnxx.comxnxx.",
        "dmm.co.jpdmm.co.jpdmm.co.jpdmm.co.jpdmm.co.jp",
        "google.com.twgoogle.com.twgoogle.com.twgoogle",
        "alipay.comalipay.comalipay.comalipay.comalipa",
        "youku.comyouku.comyouku.comyouku.comyouku.com",
        "google.com.sagoogle.com.sagoogle.com.sagoogle",
        "amazon.cnamazon.cnamazon.cnamazon.cnamazon.cn",
        "google.com.eggoogle.com.eggoogle.com.eggoogle",
        "blogspot.inblogspot.inblogspot.inblogspot.inb",
        "google.com.argoogle.com.argoogle.com.argoogle",
        "ameblo.jpameblo.jpameblo.jpameblo.jpameblo.jp",
        "google.co.thgoogle.co.thgoogle.co.thgoogle.co",
        "microsoftonline.commicrosoftonline.commicroso",
        "theguardian.comtheguardian.comtheguardian.com",
        "amazon.framazon.framazon.framazon.framazon.fr",
        "amazonaws.comamazonaws.comamazonaws.comamazon",
        "chinadaily.com.cnchinadaily.com.cnchinadaily.",
        "bankofamerica.combankofamerica.combankofameri",
        "ettoday.netettoday.netettoday.netettoday.nete",
        "google.com.pkgoogle.com.pkgoogle.com.pkgoogle",
        "slideshare.netslideshare.netslideshare.netsli",
        "etsy.cometsy.cometsy.cometsy.cometsy.cometsy.",
        "yelp.comyelp.comyelp.comyelp.comyelp.comyelp.",
        "walmart.comwalmart.comwalmart.comwalmart.comw",
        "cnet.comcnet.comcnet.comcnet.comcnet.comcnet.",
        "daum.netdaum.netdaum.netdaum.netdaum.netdaum.",
        "globo.comglobo.comglobo.comglobo.comglobo.com",
        "cntv.cncntv.cncntv.cncntv.cncntv.cncntv.cncnt",
        "twitch.tvtwitch.tvtwitch.tvtwitch.tvtwitch.tv",
        "tudou.comtudou.comtudou.comtudou.comtudou.com",
        "tradeadexchange.comtradeadexchange.comtradead",
        "aol.comaol.comaol.comaol.comaol.comaol.comaol",
        "whatsapp.comwhatsapp.comwhatsapp.comwhatsapp.",
        "stackexchange.comstackexchange.comstackexchan",
        "quora.comquora.comquora.comquora.comquora.com",
        "flickr.comflickr.comflickr.comflickr.comflick",
        "indeed.comindeed.comindeed.comindeed.comindee",
        "google.nlgoogle.nlgoogle.nlgoogle.nlgoogle.nl",
        "office.comoffice.comoffice.comoffice.comoffic",
        "weather.comweather.comweather.comweather.comw",
        "amazon.itamazon.itamazon.itamazon.itamazon.it",
        "redtube.comredtube.comredtube.comredtube.comr",
        "naver.jpnaver.jpnaver.jpnaver.jpnaver.jpnaver",
        "soundcloud.comsoundcloud.comsoundcloud.comsou",
        "snapdeal.comsnapdeal.comsnapdeal.comsnapdeal.",
        "reimageplus.comreimageplus.comreimageplus.com",
        "adf.lyadf.lyadf.lyadf.lyadf.lyadf.lyadf.lyadf",
        "ilividnewtab.comilividnewtab.comilividnewtab.",
        "bp.blogspot.combp.blogspot.combp.blogspot.com",
        "douban.comdouban.comdouban.comdouban.comdouba",
        "wellsfargo.comwellsfargo.comwellsfargo.comwel",
        "forbes.comforbes.comforbes.comforbes.comforbe",
        "vice.comvice.comvice.comvice.comvice.comvice.",
        "zillow.comzillow.comzillow.comzillow.comzillo",
        "youporn.comyouporn.comyouporn.comyouporn.comy",
        "tubecup.comtubecup.comtubecup.comtubecup.comt",
        "office365.comoffice365.comoffice365.comoffice",
        "google.co.zagoogle.co.zagoogle.co.zagoogle.co",
        "gmail.comgmail.comgmail.comgmail.comgmail.com",
        "google.co.vegoogle.co.vegoogle.co.vegoogle.co",
        "leboncoin.frleboncoin.frleboncoin.frleboncoin",
        "salesforce.comsalesforce.comsalesforce.comsal",
        "godaddy.comgodaddy.comgodaddy.comgodaddy.comg",
        "vimeo.comvimeo.comvimeo.comvimeo.comvimeo.com",
        "google.grgoogle.grgoogle.grgoogle.grgoogle.gr",
        "detail.tmall.comdetail.tmall.comdetail.tmall.",
        "ikea.comikea.comikea.comikea.comikea.comikea.",
        "kakaku.comkakaku.comkakaku.comkakaku.comkakak",
        "target.comtarget.comtarget.comtarget.comtarge",
        "goo.ne.jpgoo.ne.jpgoo.ne.jpgoo.ne.jpgoo.ne.jp",
        "about.comabout.comabout.comabout.comabout.com",
        "foxnews.comfoxnews.comfoxnews.comfoxnews.comf",
        "tripadvisor.comtripadvisor.comtripadvisor.com",
        "livejournal.comlivejournal.comlivejournal.com",
        "bestbuy.combestbuy.combestbuy.combestbuy.comb",
        "allegro.plallegro.plallegro.plallegro.plalleg",
        "avito.ruavito.ruavito.ruavito.ruavito.ruavito",
        "adplxmd.comadplxmd.comadplxmd.comadplxmd.coma",
        "wordpress.orgwordpress.orgwordpress.orgwordpr",
        "themeforest.netthemeforest.netthemeforest.net",
        "theladbible.comtheladbible.comtheladbible.com",
        "9gag.com9gag.com9gag.com9gag.com9gag.com9gag.",
        "feedly.comfeedly.comfeedly.comfeedly.comfeedl",
        "w3schools.comw3schools.comw3schools.comw3scho",
        "deviantart.comdeviantart.comdeviantart.comdev",
        "nih.govnih.govnih.govnih.govnih.govnih.govnih",
        "washingtonpost.comwashingtonpost.comwashingto",
        "nfl.comnfl.comnfl.comnfl.comnfl.comnfl.comnfl",
        "wikihow.comwikihow.comwikihow.comwikihow.comw",
        "doublepimp.comdoublepimp.comdoublepimp.comdou",
        "google.com.uagoogle.com.uagoogle.com.uagoogle",
        "skype.comskype.comskype.comskype.comskype.com",
        "files.wordpress.comfiles.wordpress.comfiles.w",
        "businessinsider.combusinessinsider.combusines",
        "gfycat.comgfycat.comgfycat.comgfycat.comgfyca",
        "taboola.comtaboola.comtaboola.comtaboola.comt",
        "mozilla.orgmozilla.orgmozilla.orgmozilla.orgm",
        "softonic.comsoftonic.comsoftonic.comsoftonic.",
        "loading-delivery2.comloading-delivery2.comloa",
        "telegraph.co.uktelegraph.co.uktelegraph.co.uk",
        "americanexpress.comamericanexpress.comamerica",
        "mediafire.commediafire.commediafire.commediaf",
        "google.cngoogle.cngoogle.cngoogle.cngoogle.cn",
        "zol.com.cnzol.com.cnzol.com.cnzol.com.cnzol.c",
        "onet.plonet.plonet.plonet.plonet.plonet.plone",
        "avg.comavg.comavg.comavg.comavg.comavg.comavg",
        "pixiv.netpixiv.netpixiv.netpixiv.netpixiv.net",
        "mystart.commystart.commystart.commystart.comm",
        "nametests.comnametests.comnametests.comnamete",
        "ups.comups.comups.comups.comups.comups.comups",
        "google.com.cogoogle.com.cogoogle.com.cogoogle",
        "akamaihd.netakamaihd.netakamaihd.netakamaihd.",
        "trackingclick.nettrackingclick.nettrackingcli",
        "amazon.esamazon.esamazon.esamazon.esamazon.es",
        "wix.comwix.comwix.comwix.comwix.comwix.comwix",
        "hclips.comhclips.comhclips.comhclips.comhclip",
        "blogspot.com.brblogspot.com.brblogspot.com.br",
        "doorblog.jpdoorblog.jpdoorblog.jpdoorblog.jpd",
        "google.com.sggoogle.com.sggoogle.com.sggoogle",
        "archive.orgarchive.orgarchive.orgarchive.orga",
        "huanqiu.comhuanqiu.comhuanqiu.comhuanqiu.comh",
        "weebly.comweebly.comweebly.comweebly.comweebl",
        "usps.comusps.comusps.comusps.comusps.comusps.",
        "secureserver.netsecureserver.netsecureserver.",
        "comcast.netcomcast.netcomcast.netcomcast.netc",
        "force.comforce.comforce.comforce.comforce.com",
        "homedepot.comhomedepot.comhomedepot.comhomede",
        "google.begoogle.begoogle.begoogle.begoogle.be",
        "wikimedia.orgwikimedia.orgwikimedia.orgwikime",
        "bitauto.combitauto.combitauto.combitauto.comb",
        "steamcommunity.comsteamcommunity.comsteamcomm",
        "addthis.comaddthis.comaddthis.comaddthis.coma",
        "google.rogoogle.rogoogle.rogoogle.rogoogle.ro",
        "ndtv.comndtv.comndtv.comndtv.comndtv.comndtv.",
        "zhihu.comzhihu.comzhihu.comzhihu.comzhihu.com",
        "google.com.nggoogle.com.nggoogle.com.nggoogle",
        "shutterstock.comshutterstock.comshutterstock.",
        "ebay-kleinanzeigen.deebay-kleinanzeigen.deeba",
        "xywy.comxywy.comxywy.comxywy.comxywy.comxywy.",
        "gamer.com.twgamer.com.twgamer.com.twgamer.com",
        "mercadolivre.com.brmercadolivre.com.brmercado",
        "tlbb8.comtlbb8.comtlbb8.comtlbb8.comtlbb8.com",
        "bilibili.combilibili.combilibili.combilibili.",
        "detik.comdetik.comdetik.comdetik.comdetik.com",
        "terraclicks.comterraclicks.comterraclicks.com",
        "github.iogithub.iogithub.iogithub.iogithub.io",
        "terrapops.comterrapops.comterrapops.comterrap",
        "google.com.phgoogle.com.phgoogle.com.phgoogle",
        "ifeng.comifeng.comifeng.comifeng.comifeng.com",
        "web.deweb.deweb.deweb.deweb.deweb.deweb.deweb",
        "answers.comanswers.comanswers.comanswers.coma",
        "popcash.netpopcash.netpopcash.netpopcash.netp",
        "mailchimp.commailchimp.commailchimp.commailch",
        "bild.debild.debild.debild.debild.debild.debil",
        "sourceforge.netsourceforge.netsourceforge.net",
        "steampowered.comsteampowered.comsteampowered.",
        "people.com.cnpeople.com.cnpeople.com.cnpeople",
        "orange.frorange.frorange.frorange.frorange.fr",
        "hdfcbank.comhdfcbank.comhdfcbank.comhdfcbank.",
        "hp.comhp.comhp.comhp.comhp.comhp.comhp.comhp.",
        "uptodown.comuptodown.comuptodown.comuptodown.",
        "gmx.netgmx.netgmx.netgmx.netgmx.netgmx.netgmx",
        "wordreference.comwordreference.comwordreferen",
        "google.segoogle.segoogle.segoogle.segoogle.se",
        "speedtest.netspeedtest.netspeedtest.netspeedt",
        "usatoday.comusatoday.comusatoday.comusatoday.",
        "xfinity.comxfinity.comxfinity.comxfinity.comx",
        "fbcdn.netfbcdn.netfbcdn.netfbcdn.netfbcdn.net",
        "rambler.rurambler.rurambler.rurambler.rurambl",
        "varzesh3.comvarzesh3.comvarzesh3.comvarzesh3.",
        "dmm.comdmm.comdmm.comdmm.comdmm.comdmm.comdmm",
        "att.comatt.comatt.comatt.comatt.comatt.comatt",
        "webmd.comwebmd.comwebmd.comwebmd.comwebmd.com",
        "google.atgoogle.atgoogle.atgoogle.atgoogle.at",
        "hootsuite.comhootsuite.comhootsuite.comhootsu",
        "stumbleupon.comstumbleupon.comstumbleupon.com",
        "goodreads.comgoodreads.comgoodreads.comgoodre",
        "groupon.comgroupon.comgroupon.comgroupon.comg",
        "bloomberg.combloomberg.combloomberg.combloomb",
        "livejasmin.comlivejasmin.comlivejasmin.comliv",
        "icicibank.comicicibank.comicicibank.comicicib",
        "wp.plwp.plwp.plwp.plwp.plwp.plwp.plwp.plwp.pl",
        "youm7.comyoum7.comyoum7.comyoum7.comyoum7.com",
        "spiegel.despiegel.despiegel.despiegel.despieg",
        "capitalone.comcapitalone.comcapitalone.comcap",
        "fedex.comfedex.comfedex.comfedex.comfedex.com",
        "spaceshipads.comspaceshipads.comspaceshipads.",
        "blog.jpblog.jpblog.jpblog.jpblog.jpblog.jpblo",
        "caijing.com.cncaijing.com.cncaijing.com.cncai",
        "google.com.pegoogle.com.pegoogle.com.pegoogle",
        "t-online.det-online.det-online.det-online.det",
        "thesaurus.comthesaurus.comthesaurus.comthesau",
        "adidas.tmall.comadidas.tmall.comadidas.tmall.",
        "mashable.commashable.commashable.commashable.",
        "google.ptgoogle.ptgoogle.ptgoogle.ptgoogle.pt",
        "haiwainet.cnhaiwainet.cnhaiwainet.cnhaiwainet",
        "blogfa.comblogfa.comblogfa.comblogfa.comblogf",
        "spotify.comspotify.comspotify.comspotify.coms",
        "engadget.comengadget.comengadget.comengadget.",
        "wsj.comwsj.comwsj.comwsj.comwsj.comwsj.comwsj",
        "2ch.net2ch.net2ch.net2ch.net2ch.net2ch.net2ch",
        "amazon.caamazon.caamazon.caamazon.caamazon.ca",
        "watsons.tmall.comwatsons.tmall.comwatsons.tma",
        "xuite.netxuite.netxuite.netxuite.netxuite.net",
        "pandora.compandora.compandora.compandora.comp",
        "life.com.twlife.com.twlife.com.twlife.com.twl",
        "mama.cnmama.cnmama.cnmama.cnmama.cnmama.cnmam",
        "samsung.comsamsung.comsamsung.comsamsung.coms",
        "accuweather.comaccuweather.comaccuweather.com",
        "bet365.combet365.combet365.combet365.combet36",
        "ign.comign.comign.comign.comign.comign.comign",
        "hulu.comhulu.comhulu.comhulu.comhulu.comhulu.",
        "udn.comudn.comudn.comudn.comudn.comudn.comudn",
        "pconline.com.cnpconline.com.cnpconline.com.cn",
        "seznam.czseznam.czseznam.czseznam.czseznam.cz",
        "media.tumblr.commedia.tumblr.commedia.tumblr.",
        "mlb.commlb.commlb.commlb.commlb.commlb.commlb",
        "nownews.comnownews.comnownews.comnownews.comn",
        "chaoshi.tmall.comchaoshi.tmall.comchaoshi.tma",
        "google.aegoogle.aegoogle.aegoogle.aegoogle.ae",
        "paytm.compaytm.compaytm.compaytm.compaytm.com",
        "styletv.com.cnstyletv.com.cnstyletv.com.cnsty",
        "gsmarena.comgsmarena.comgsmarena.comgsmarena.",
        "extratorrent.ccextratorrent.ccextratorrent.cc",
        "verizonwireless.comverizonwireless.comverizon",
        "badoo.combadoo.combadoo.combadoo.combadoo.com",
        "ebay.inebay.inebay.inebay.inebay.inebay.ineba",
        "1905.com1905.com1905.com1905.com1905.com1905.",
        "youth.cnyouth.cnyouth.cnyouth.cnyouth.cnyouth",
        "google.chgoogle.chgoogle.chgoogle.chgoogle.ch",
        "dell.comdell.comdell.comdell.comdell.comdell.",
        "kaskus.co.idkaskus.co.idkaskus.co.idkaskus.co",
        "reuters.comreuters.comreuters.comreuters.comr",
        "chaturbate.comchaturbate.comchaturbate.comcha",
        "abs-cbnnews.comabs-cbnnews.comabs-cbnnews.com",
        "livedoor.bizlivedoor.bizlivedoor.bizlivedoor.",
        "zendesk.comzendesk.comzendesk.comzendesk.comz",
        "39.net39.net39.net39.net39.net39.net39.net39.",
        "1688.com1688.com1688.com1688.com1688.com1688.",
        "rediff.comrediff.comrediff.comrediff.comredif",
        "putlocker.isputlocker.isputlocker.isputlocker",
        "bleacherreport.combleacherreport.combleacherr",
        "trello.comtrello.comtrello.comtrello.comtrell",
        "twimg.comtwimg.comtwimg.comtwimg.comtwimg.com",
        "ijreview.comijreview.comijreview.comijreview.",
        "onlinesbi.comonlinesbi.comonlinesbi.comonline",
        "google.clgoogle.clgoogle.clgoogle.clgoogle.cl",
        "reference.comreference.comreference.comrefere",
        "likes.comlikes.comlikes.comlikes.comlikes.com",
        "warmportrait.comwarmportrait.comwarmportrait.",
        "rt.comrt.comrt.comrt.comrt.comrt.comrt.comrt.",
        "jabong.comjabong.comjabong.comjabong.comjabon",
        "sahibinden.comsahibinden.comsahibinden.comsah",
        "google.czgoogle.czgoogle.czgoogle.czgoogle.cz",
        "google.com.bdgoogle.com.bdgoogle.com.bdgoogle",
        "tistory.comtistory.comtistory.comtistory.comt",
        "icloud.comicloud.comicloud.comicloud.comiclou",
        "mydomainadvisor.commydomainadvisor.commydomai",
        "enet.com.cnenet.com.cnenet.com.cnenet.com.cne",
        "mega.nzmega.nzmega.nzmega.nzmega.nzmega.nzmeg",
        "iqiyi.comiqiyi.comiqiyi.comiqiyi.comiqiyi.com",
        "impress.co.jpimpress.co.jpimpress.co.jpimpres",
        "yaolan.comyaolan.comyaolan.comyaolan.comyaola",
        "milliyet.com.trmilliyet.com.trmilliyet.com.tr",
        "smzdm.comsmzdm.comsmzdm.comsmzdm.comsmzdm.com",
        "quikr.comquikr.comquikr.comquikr.comquikr.com",
        "google.azgoogle.azgoogle.azgoogle.azgoogle.az",
        "google.iegoogle.iegoogle.iegoogle.iegoogle.ie",
        "kickstarter.comkickstarter.comkickstarter.com",
        "rutracker.orgrutracker.orgrutracker.orgrutrac",
        "macys.commacys.commacys.commacys.commacys.com",
        "infusionsoft.cominfusionsoft.cominfusionsoft.",
        "baike.combaike.combaike.combaike.combaike.com",
        "ask.fmask.fmask.fmask.fmask.fmask.fmask.fmask",
        "evernote.comevernote.comevernote.comevernote.",
        "cnzz.comcnzz.comcnzz.comcnzz.comcnzz.comcnzz.",
        "haosou.comhaosou.comhaosou.comhaosou.comhaoso",
        "lady8844.comlady8844.comlady8844.comlady8844.",
        "kouclo.comkouclo.comkouclo.comkouclo.comkoucl",
        "slickdeals.netslickdeals.netslickdeals.netsli",
        "google.dzgoogle.dzgoogle.dzgoogle.dzgoogle.dz",
        "oeeee.comoeeee.comoeeee.comoeeee.comoeeee.com",
        "liveinternet.ruliveinternet.ruliveinternet.ru",
        "cbssports.comcbssports.comcbssports.comcbsspo",
        "kohls.comkohls.comkohls.comkohls.comkohls.com",
        "thefreedictionary.comthefreedictionary.comthe",
        "theverge.comtheverge.comtheverge.comtheverge.",
        "gameforge.comgameforge.comgameforge.comgamefo",
        "oracle.comoracle.comoracle.comoracle.comoracl",
        "ebay.itebay.itebay.itebay.itebay.itebay.iteba",
        "ce.cnce.cnce.cnce.cnce.cnce.cnce.cnce.cnce.cn",
        "google.hugoogle.hugoogle.hugoogle.hugoogle.hu",
        "eksisozluk.comeksisozluk.comeksisozluk.comeks",
        "tube8.comtube8.comtube8.comtube8.comtube8.com",
        "babytree.combabytree.combabytree.combabytree.",
        "chinatimes.comchinatimes.comchinatimes.comchi",
        "ebay.com.auebay.com.auebay.com.auebay.com.aue",
        "google.nogoogle.nogoogle.nogoogle.nogoogle.no",
        "4shared.com4shared.com4shared.com4shared.com4",
        "teepr.comteepr.comteepr.comteepr.comteepr.com",
        "taleo.nettaleo.nettaleo.nettaleo.nettaleo.net",
        "elpais.comelpais.comelpais.comelpais.comelpai",
        "repubblica.itrepubblica.itrepubblica.itrepubb",
        "hurriyet.com.trhurriyet.com.trhurriyet.com.tr",
        "blogimg.jpblogimg.jpblogimg.jpblogimg.jpblogi",
        "meetup.commeetup.commeetup.commeetup.commeetu",
        "goal.comgoal.comgoal.comgoal.comgoal.comgoal.",
        "scribd.comscribd.comscribd.comscribd.comscrib",
        "eastday.comeastday.comeastday.comeastday.come",
        "ppomppu.co.krppomppu.co.krppomppu.co.krppompp",
        "newegg.comnewegg.comnewegg.comnewegg.comneweg",
        "libero.itlibero.itlibero.itlibero.itlibero.it",
        "sberbank.rusberbank.rusberbank.rusberbank.rus",
        "photobucket.comphotobucket.comphotobucket.com",
        "list-manage.comlist-manage.comlist-manage.com",
        "yandex.uayandex.uayandex.uayandex.uayandex.ua",
        "ewt.ccewt.ccewt.ccewt.ccewt.ccewt.ccewt.ccewt",
        "slack.comslack.comslack.comslack.comslack.com",
        "ltn.com.twltn.com.twltn.com.twltn.com.twltn.c",
        "lifehacker.comlifehacker.comlifehacker.comlif",
        "upornia.comupornia.comupornia.comupornia.comu",
        "gizmodo.comgizmodo.comgizmodo.comgizmodo.comg",
        "onedio.comonedio.comonedio.comonedio.comonedi",
        "olx.inolx.inolx.inolx.inolx.inolx.inolx.inolx",
        "neobux.comneobux.comneobux.comneobux.comneobu",
        "buzzfil.netbuzzfil.netbuzzfil.netbuzzfil.netb",
        "livedoor.comlivedoor.comlivedoor.comlivedoor.",
        "google.co.ilgoogle.co.ilgoogle.co.ilgoogle.co",
        "citi.comciti.comciti.comciti.comciti.comciti.",
        "marca.commarca.commarca.commarca.commarca.com",
        "uploaded.netuploaded.netuploaded.netuploaded.",
        "vid.mevid.mevid.mevid.mevid.mevid.mevid.mevid",
        "ameba.jpameba.jpameba.jpameba.jpameba.jpameba",
        "qunar.comqunar.comqunar.comqunar.comqunar.com",
        "torrentz.eutorrentz.eutorrentz.eutorrentz.eut",
        "aparat.comaparat.comaparat.comaparat.comapara",
        "espncricinfo.comespncricinfo.comespncricinfo.",
        "cloudfront.netcloudfront.netcloudfront.netclo",
        "stockstar.comstockstar.comstockstar.comstocks",
        "gap.comgap.comgap.comgap.comgap.comgap.comgap",
        "time.comtime.comtime.comtime.comtime.comtime.",
        "fiverr.comfiverr.comfiverr.comfiverr.comfiver",
        "kinopoisk.rukinopoisk.rukinopoisk.rukinopoisk",
        "naukri.comnaukri.comnaukri.comnaukri.comnaukr",
        "xe.comxe.comxe.comxe.comxe.comxe.comxe.comxe.",
        "xda-developers.comxda-developers.comxda-devel",
        "disqus.comdisqus.comdisqus.comdisqus.comdisqu",
        "kompas.comkompas.comkompas.comkompas.comkompa",
        "free.frfree.frfree.frfree.frfree.frfree.frfre",
        "pinimg.compinimg.compinimg.compinimg.compinim",
        "lowes.comlowes.comlowes.comlowes.comlowes.com",
        "liputan6.comliputan6.comliputan6.comliputan6.",
        "retailmenot.comretailmenot.comretailmenot.com",
        "savefrom.netsavefrom.netsavefrom.netsavefrom.",
        "4dsply.com4dsply.com4dsply.com4dsply.com4dspl",
        "shopclues.comshopclues.comshopclues.comshopcl",
        "hm.comhm.comhm.comhm.comhm.comhm.comhm.comhm.",
        "techcrunch.comtechcrunch.comtechcrunch.comtec",
        "independent.co.ukindependent.co.ukindependent",
        "youboy.comyouboy.comyouboy.comyouboy.comyoubo",
        "justdial.comjustdial.comjustdial.comjustdial.",
        "battle.netbattle.netbattle.netbattle.netbattl",
        "nbcnews.comnbcnews.comnbcnews.comnbcnews.comn",
        "surveymonkey.comsurveymonkey.comsurveymonkey.",
        "hotels.comhotels.comhotels.comhotels.comhotel",
        "epweike.comepweike.comepweike.comepweike.come",
        "nyaa.senyaa.senyaa.senyaa.senyaa.senyaa.senya",
        "mobile.demobile.demobile.demobile.demobile.de",
        "admtpmp127.comadmtpmp127.comadmtpmp127.comadm",
        "timeanddate.comtimeanddate.comtimeanddate.com",
        "nordstrom.comnordstrom.comnordstrom.comnordst",
        "liveadexchanger.comliveadexchanger.comliveade",
        "lenovo.comlenovo.comlenovo.comlenovo.comlenov",
        "autohome.com.cnautohome.com.cnautohome.com.cn",
        "zippyshare.comzippyshare.comzippyshare.comzip",
        "rbc.rurbc.rurbc.rurbc.rurbc.rurbc.rurbc.rurbc",
        "freegameszonetab.comfreegameszonetab.comfreeg",
        "gamefaqs.comgamefaqs.comgamefaqs.comgamefaqs.",
        "bhaskar.combhaskar.combhaskar.combhaskar.comb",
        "tabelog.comtabelog.comtabelog.comtabelog.comt",
        "corriere.itcorriere.itcorriere.itcorriere.itc",
        "google.figoogle.figoogle.figoogle.figoogle.fi",
        "expedia.comexpedia.comexpedia.comexpedia.come"};
    // initialize all file names
    string f[FILE_NUM] = {
        "f0.txt",
        "f1.txt",
        "f2.txt",
        "f3.txt",
        "f4.txt",
        "f5.txt",
        "f6.txt",
        "f7.txt",
        "f8.txt",
        "f9.txt",
        "f10.txt",
        "f11.txt",
        "f12.txt",
        "f13.txt",
        "f14.txt",
        "f15.txt",
        "f16.txt",
        "f17.txt",
        "f18.txt",
        "f19.txt",
        "f20.txt",
        "f21.txt",
        "f22.txt",
        "f23.txt",
        "f24.txt",
        "f25.txt",
        "f26.txt",
        "f27.txt",
        "f28.txt",
        "f29.txt",
        "f30.txt",
        "f31.txt",
        "f32.txt",
        "f33.txt",
        "f34.txt",
        "f35.txt",
        "f36.txt",
        "f37.txt",
        "f38.txt",
        "f39.txt",
        "f40.txt",
        "f41.txt",
        "f42.txt",
        "f43.txt",
        "f44.txt",
        "f45.txt",
        "f46.txt",
        "f47.txt",
        "f48.txt",
        "f49.txt",
        "f50.txt",
        "f51.txt",
        "f52.txt",
        "f53.txt",
        "f54.txt",
        "f55.txt",
        "f56.txt",
        "f57.txt",
        "f58.txt",
        "f59.txt",
        "f60.txt",
        "f61.txt",
        "f62.txt",
        "f63.txt",
        "f64.txt",
        "f65.txt",
        "f66.txt",
        "f67.txt",
        "f68.txt",
        "f69.txt",
        "f70.txt",
        "f71.txt",
        "f72.txt",
        "f73.txt",
        "f74.txt",
        "f75.txt",
        "f76.txt",
        "f77.txt",
        "f78.txt",
        "f79.txt",
        "f80.txt",
        "f81.txt",
        "f82.txt",
        "f83.txt",
        "f84.txt",
        "f85.txt",
        "f86.txt",
        "f87.txt",
        "f88.txt",
        "f89.txt",
        "f90.txt",
        "f91.txt",
        "f92.txt",
        "f93.txt",
        "f94.txt",
        "f95.txt",
        "f96.txt",
        "f97.txt",
        "f98.txt",
        "f99.txt",
        "f100.txt",
        "f101.txt",
        "f102.txt",
        "f103.txt",
        "f104.txt",
        "f105.txt",
        "f106.txt",
        "f107.txt",
        "f108.txt",
        "f109.txt",
        "f110.txt",
        "f111.txt",
        "f112.txt",
        "f113.txt",
        "f114.txt",
        "f115.txt",
        "f116.txt",
        "f117.txt",
        "f118.txt",
        "f119.txt",
        "f120.txt",
        "f121.txt",
        "f122.txt",
        "f123.txt",
        "f124.txt",
        "f125.txt",
        "f126.txt",
        "f127.txt",
        "f128.txt",
        "f129.txt",
        "f130.txt",
        "f131.txt",
        "f132.txt",
        "f133.txt",
        "f134.txt",
        "f135.txt",
        "f136.txt",
        "f137.txt",
        "f138.txt",
        "f139.txt",
        "f140.txt",
        "f141.txt",
        "f142.txt",
        "f143.txt",
        "f144.txt",
        "f145.txt",
        "f146.txt",
        "f147.txt",
        "f148.txt",
        "f149.txt",
        "f150.txt",
        "f151.txt",
        "f152.txt",
        "f153.txt",
        "f154.txt",
        "f155.txt",
        "f156.txt",
        "f157.txt",
        "f158.txt",
        "f159.txt",
        "f160.txt",
        "f161.txt",
        "f162.txt",
        "f163.txt",
        "f164.txt",
        "f165.txt",
        "f166.txt",
        "f167.txt",
        "f168.txt",
        "f169.txt",
        "f170.txt",
        "f171.txt",
        "f172.txt",
        "f173.txt",
        "f174.txt",
        "f175.txt",
        "f176.txt",
        "f177.txt",
        "f178.txt",
        "f179.txt",
        "f180.txt",
        "f181.txt",
        "f182.txt",
        "f183.txt",
        "f184.txt",
        "f185.txt",
        "f186.txt",
        "f187.txt",
        "f188.txt",
        "f189.txt",
        "f190.txt",
        "f191.txt",
        "f192.txt",
        "f193.txt",
        "f194.txt",
        "f195.txt",
        "f196.txt",
        "f197.txt",
        "f198.txt",
        "f199.txt",
        "f200.txt",
        "f201.txt",
        "f202.txt",
        "f203.txt",
        "f204.txt",
        "f205.txt",
        "f206.txt",
        "f207.txt",
        "f208.txt",
        "f209.txt",
        "f210.txt",
        "f211.txt",
        "f212.txt",
        "f213.txt",
        "f214.txt",
        "f215.txt",
        "f216.txt",
        "f217.txt",
        "f218.txt",
        "f219.txt",
        "f220.txt",
        "f221.txt",
        "f222.txt",
        "f223.txt",
        "f224.txt",
        "f225.txt",
        "f226.txt",
        "f227.txt",
        "f228.txt",
        "f229.txt",
        "f230.txt",
        "f231.txt",
        "f232.txt",
        "f233.txt",
        "f234.txt",
        "f235.txt",
        "f236.txt",
        "f237.txt",
        "f238.txt",
        "f239.txt",
        "f240.txt",
        "f241.txt",
        "f242.txt",
        "f243.txt",
        "f244.txt",
        "f245.txt",
        "f246.txt",
        "f247.txt",
        "f248.txt",
        "f249.txt",
        "f250.txt",
        "f251.txt",
        "f252.txt",
        "f253.txt",
        "f254.txt",
        "f255.txt",
        "f256.txt",
        "f257.txt",
        "f258.txt",
        "f259.txt",
        "f260.txt",
        "f261.txt",
        "f262.txt",
        "f263.txt",
        "f264.txt",
        "f265.txt",
        "f266.txt",
        "f267.txt",
        "f268.txt",
        "f269.txt",
        "f270.txt",
        "f271.txt",
        "f272.txt",
        "f273.txt",
        "f274.txt",
        "f275.txt",
        "f276.txt",
        "f277.txt",
        "f278.txt",
        "f279.txt",
        "f280.txt",
        "f281.txt",
        "f282.txt",
        "f283.txt",
        "f284.txt",
        "f285.txt",
        "f286.txt",
        "f287.txt",
        "f288.txt",
        "f289.txt",
        "f290.txt",
        "f291.txt",
        "f292.txt",
        "f293.txt",
        "f294.txt",
        "f295.txt",
        "f296.txt",
        "f297.txt",
        "f298.txt",
        "f299.txt",
        "f300.txt",
        "f301.txt",
        "f302.txt",
        "f303.txt",
        "f304.txt",
        "f305.txt",
        "f306.txt",
        "f307.txt",
        "f308.txt",
        "f309.txt",
        "f310.txt",
        "f311.txt",
        "f312.txt",
        "f313.txt",
        "f314.txt",
        "f315.txt",
        "f316.txt",
        "f317.txt",
        "f318.txt",
        "f319.txt",
        "f320.txt",
        "f321.txt",
        "f322.txt",
        "f323.txt",
        "f324.txt",
        "f325.txt",
        "f326.txt",
        "f327.txt",
        "f328.txt",
        "f329.txt",
        "f330.txt",
        "f331.txt",
        "f332.txt",
        "f333.txt",
        "f334.txt",
        "f335.txt",
        "f336.txt",
        "f337.txt",
        "f338.txt",
        "f339.txt",
        "f340.txt",
        "f341.txt",
        "f342.txt",
        "f343.txt",
        "f344.txt",
        "f345.txt",
        "f346.txt",
        "f347.txt",
        "f348.txt",
        "f349.txt",
        "f350.txt",
        "f351.txt",
        "f352.txt",
        "f353.txt",
        "f354.txt",
        "f355.txt",
        "f356.txt",
        "f357.txt",
        "f358.txt",
        "f359.txt",
        "f360.txt",
        "f361.txt",
        "f362.txt",
        "f363.txt",
        "f364.txt",
        "f365.txt",
        "f366.txt",
        "f367.txt",
        "f368.txt",
        "f369.txt",
        "f370.txt",
        "f371.txt",
        "f372.txt",
        "f373.txt",
        "f374.txt",
        "f375.txt",
        "f376.txt",
        "f377.txt",
        "f378.txt",
        "f379.txt",
        "f380.txt",
        "f381.txt",
        "f382.txt",
        "f383.txt",
        "f384.txt",
        "f385.txt",
        "f386.txt",
        "f387.txt",
        "f388.txt",
        "f389.txt",
        "f390.txt",
        "f391.txt",
        "f392.txt",
        "f393.txt",
        "f394.txt",
        "f395.txt",
        "f396.txt",
        "f397.txt",
        "f398.txt",
        "f399.txt",
        "f400.txt",
        "f401.txt",
        "f402.txt",
        "f403.txt",
        "f404.txt",
        "f405.txt",
        "f406.txt",
        "f407.txt",
        "f408.txt",
        "f409.txt",
        "f410.txt",
        "f411.txt",
        "f412.txt",
        "f413.txt",
        "f414.txt",
        "f415.txt",
        "f416.txt",
        "f417.txt",
        "f418.txt",
        "f419.txt",
        "f420.txt",
        "f421.txt",
        "f422.txt",
        "f423.txt",
        "f424.txt",
        "f425.txt",
        "f426.txt",
        "f427.txt",
        "f428.txt",
        "f429.txt",
        "f430.txt",
        "f431.txt",
        "f432.txt",
        "f433.txt",
        "f434.txt",
        "f435.txt",
        "f436.txt",
        "f437.txt",
        "f438.txt",
        "f439.txt",
        "f440.txt",
        "f441.txt",
        "f442.txt",
        "f443.txt",
        "f444.txt",
        "f445.txt",
        "f446.txt",
        "f447.txt",
        "f448.txt",
        "f449.txt",
        "f450.txt",
        "f451.txt",
        "f452.txt",
        "f453.txt",
        "f454.txt",
        "f455.txt",
        "f456.txt",
        "f457.txt",
        "f458.txt",
        "f459.txt",
        "f460.txt",
        "f461.txt",
        "f462.txt",
        "f463.txt",
        "f464.txt",
        "f465.txt",
        "f466.txt",
        "f467.txt",
        "f468.txt",
        "f469.txt",
        "f470.txt",
        "f471.txt",
        "f472.txt",
        "f473.txt",
        "f474.txt",
        "f475.txt",
        "f476.txt",
        "f477.txt",
        "f478.txt",
        "f479.txt",
        "f480.txt",
        "f481.txt",
        "f482.txt",
        "f483.txt",
        "f484.txt",
        "f485.txt",
        "f486.txt",
        "f487.txt",
        "f488.txt",
        "f489.txt",
        "f490.txt",
        "f491.txt",
        "f492.txt",
        "f493.txt",
        "f494.txt",
        "f495.txt",
        "f496.txt",
        "f497.txt",
        "f498.txt",
        "f499.txt"};

    pid_t pkt_gen, r1, r2, r3, r4, l1, l2, l3, l4, l5, l6, l7, l8, l9, l10;

    l1 = fork();          // process of link1, que1->que2
    if(l1 == 0){
        while(1){
            char *b;
            b = (char*)malloc(4200*sizeof(char));
            int pres = -1;
            while(pres < 0){
                pres = que1.AttachQueue(tQueueKey1);
                pres = que1.ReadDataFromQueueHead(b, 4200);
            }
//            usleep(1);
            res = -1;
            while(res < 0){
                res = que2.AttachQueue(tQueueKey2);
                res = que2.PushDataIntoQueueBack(b, strlen(b));
            }
            free(b);
            b = NULL;
        }
        return 0;
    }
    l2 = fork();          // process of link2, que4->que3
    if(l2 == 0){
/*        char *d;        // measure net throughput
        d = (char*)malloc(4200*sizeof(char));
        int response = -1;
        while(response < 0){
            response = que4.AttachQueue(tQueueKey4);
            response = que4.ReadDataFromQueueHead(d, 4200);
        }
        struct timeval through_start;
        gettimeofday(&through_start,NULL);
        unsigned long tp_start = 1000000 * (through_start.tv_sec)+ through_start.tv_usec;
        response = -1;
        while(response < 0){
            response = que3.AttachQueue(tQueueKey3);
            response = que3.PushDataIntoQueueBack(d, strlen(d));
        }
        int i=1;
        while(i<20000){
            char *b;
            b = (char*)malloc(4200*sizeof(char));
            int pres = -1;
            while(pres < 0){
                pres = que4.AttachQueue(tQueueKey4);
                pres = que4.ReadDataFromQueueHead(b, 4200);
            }
//            usleep(1);
            res = -1;
            while(res < 0){
                res = que3.AttachQueue(tQueueKey3);
                res = que3.PushDataIntoQueueBack(b, strlen(b));
            }
            free(b);
            b = NULL;
            i++;
        }
        struct timeval through_end;
        gettimeofday(&through_end,NULL);
        unsigned long tp_end = 1000000 * (through_end.tv_sec)+ through_end.tv_usec;
        unsigned long tp = tp_end - tp_start;
        printf("throughput %lu %d\n", tp, i);*/
        while(1){
            char *b;
            b = (char*)malloc(4200*sizeof(char));
            int pres = -1;
            while(pres < 0){
                pres = que4.AttachQueue(tQueueKey4);
                pres = que4.ReadDataFromQueueHead(b, 4200);
            }
//            usleep(1);
            res = -1;
            while(res < 0){
                res = que3.AttachQueue(tQueueKey3);
                res = que3.PushDataIntoQueueBack(b, strlen(b));
            }
            free(b);
            b = NULL;
        }
        return 0;
    }
    l3 = fork();          // process of link3, que5->que6
    if(l3 == 0){
        while(1){
            char *b;
            b = (char*)malloc(4200*sizeof(char));
            int pres = -1;
            while(pres < 0){
                pres = que5.AttachQueue(tQueueKey5);
                pres = que5.ReadDataFromQueueHead(b, 4200);
            }
//            usleep(1);
            res = -1;
            while(res < 0){
                res = que6.AttachQueue(tQueueKey6);
                res = que6.PushDataIntoQueueBack(b, strlen(b));
            }
            free(b);
            b = NULL;
        }
        return 0;
    }
    l4 = fork();          // process of link4, que8->que7
    if(l4 == 0){
        while(1){
            char *b;
            b = (char*)malloc(4200*sizeof(char));
            int pres = -1;
            while(pres < 0){
                pres = que8.AttachQueue(tQueueKey8);
                pres = que8.ReadDataFromQueueHead(b, 4200);
            }
//            usleep(1);
            res = -1;
            while(res < 0){
                res = que7.AttachQueue(tQueueKey7);
                res = que7.PushDataIntoQueueBack(b, strlen(b));
            }
            free(b);
            b = NULL;
        }
        return 0;
    }
    l5 = fork();          // process of link5, que9->que10
    if(l5 == 0){
        while(1){
            char *b;
            b = (char*)malloc(4200*sizeof(char));
            int pres = -1;
            while(pres < 0){
                pres = que9.AttachQueue(tQueueKey9); 
                pres = que9.ReadDataFromQueueHead(b, 4200);
            }
//            usleep(1);
            res = -1;
            while(res < 0){
                res = que10.AttachQueue(tQueueKey10);
                res = que10.PushDataIntoQueueBack(b, strlen(b));
            }
            free(b);
            b = NULL;
        }
        return 0;
    }
    l6 = fork();          // process of link6, que12->que7
    if(l6 == 0){
        while(1){
            char *b;
            b = (char*)malloc(4200*sizeof(char));
            int pres = -1;
            while(pres < 0){
                pres = que12.AttachQueue(tQueueKey12);
                pres = que12.ReadDataFromQueueHead(b, 4200);
            }
//            usleep(1);
            res = -1;
            while(res < 0){
                res = que7.AttachQueue(tQueueKey7);
                res = que7.PushDataIntoQueueBack(b, strlen(b));
            }
            free(b);
            b = NULL;
        }
        return 0;
    }
    l7 = fork();          // process of link7, que13->que14
    if(l7 == 0){
        while(1){
            char *b;
            b = (char*)malloc(4200*sizeof(char));
            int pres = -1;
            while(pres < 0){
                pres = que13.AttachQueue(tQueueKey13);
                pres = que13.ReadDataFromQueueHead(b, 4200);
            }
//            usleep(1);
            res = -1;
            while(res < 0){
                res = que14.AttachQueue(tQueueKey14);
                res = que14.PushDataIntoQueueBack(b, strlen(b));
            }
            free(b);
            b = NULL;
        }
        return 0;
    }
    l8 = fork();          // process of link8, que16->que15
    if(l8 == 0){
        while(1){
            char *b;
            b = (char*)malloc(4200*sizeof(char));
            int pres = -1;
            while(pres < 0){
                pres = que16.AttachQueue(tQueueKey16);
                pres = que16.ReadDataFromQueueHead(b, 4200);
            }
//            usleep(1);
            res = -1;
            while(res < 0){
                res = que15.AttachQueue(tQueueKey15);
                res = que15.PushDataIntoQueueBack(b, strlen(b));
            }
            free(b);
            b = NULL;
        }
        return 0;
    }
    l9 = fork();          // process of link9, que17->que14
    if(l9 == 0){
        while(1){
            char *b;
            b = (char*)malloc(4200*sizeof(char));
            int pres = -1;
            while(pres < 0){
                pres = que17.AttachQueue(tQueueKey17);
                pres = que17.ReadDataFromQueueHead(b, 4200);
            }
//            usleep(1);
            res = -1;
            while(res < 0){
                res = que14.AttachQueue(tQueueKey14);
                res = que14.PushDataIntoQueueBack(b, strlen(b));
            }
            free(b);
            b = NULL;
        }
        return 0;
    }
    l10 = fork();          // process of link10, que20->que19
    if(l10 == 0){
        while(1){
            char *b;
            b = (char*)malloc(4200*sizeof(char));
            int pres = -1;
            while(pres < 0){
                pres = que20.AttachQueue(tQueueKey20);
                pres = que20.ReadDataFromQueueHead(b, 4200);
            }
//            usleep(1);
            res = -1;
            while(res < 0){
                res = que19.AttachQueue(tQueueKey19);
                res = que19.PushDataIntoQueueBack(b, strlen(b));
            }
            free(b);
            b = NULL;
        }
        return 0;
    }

    r1 = fork();          // process of router r1
    if(r1 == 0){
        for(int i=0; i<200; i++){
            url_file[url[i]] = f[i];     // Insert some strings
        }
        // initialize fib table
        for(int i=0;i<50;i++){
            fib[url[i]] = 1;
        }
        for(int i=100;i<150;i++){
            fib[url[i]] = 1;
        }
        for(int i=200;i<250;i++){
            fib[url[i]] = 1;
        }
        for(int i=300;i<325;i++){
            fib[url[i]] = 1;
        }
        for(int i=350;i<400;i++){
            fib[url[i]] = 1;
        }
        for(int i=450;i<475;i++){
            fib[url[i]] = 1;
        }
        for(int i=50;i<100;i++){
            fib[url[i]] = 2;
        }
        for(int i=150;i<200;i++){
            fib[url[i]] = 2;
        }
        for(int i=250;i<300;i++){
            fib[url[i]] = 2;
        }
        for(int i=325;i<350;i++){
            fib[url[i]] = 2;
        }
        for(int i=400;i<450;i++){
            fib[url[i]] = 2;
        }
        for(int i=475;i<500;i++){
            fib[url[i]] = 2;
        }
        // initialize inside_que
        struct inside_que *riq;
        riq = in_queInit();
        if(riq ==  NULL){
            fprintf (stderr, "main: Queue Init failed.\n");
            exit (1);
        }
        // initialize some parameters
        struct para_intrstin p1;
        struct para_intrstout p2;
        struct para_data p3;
        p1.shm1 = &que2;
        p1.queKey1 = tQueueKey2;
        p1.iq = riq;
        p1.shm2 = &que4;
        p1.queKey2 = tQueueKey4;
        p2.shm1 = &que5;
        p2.queKey1 = tQueueKey5;
        p2.shm2 = &que9;
        p2.queKey2 = tQueueKey9;
        p2.iq = riq;
        p3.shm1 = &que7;
        p3.queKey1 = tQueueKey7;
        p3.shm2 = &que4;
        p3.queKey2 = tQueueKey4;

        pthread_mutex_t shmlock;
        pthread_mutex_init(&shmlock, NULL);
        p1.quelock = &shmlock;
        p3.quelock = &shmlock;

        struct para_measure p4;
        pthread_mutex_t loadquelock;
        pthread_mutex_init(&loadquelock, NULL);
        p1.quemeasure_lock = &loadquelock;
        p4.shm = &que2;
        p4.queKey = tQueueKey2;
        p4.quemeasure_lock = &loadquelock;

        // create threads
        pthread_t cs_fun, fib_fun, data_fun, load_fun;
        pthread_create(&cs_fun, NULL, cs_action, &p1);
        pthread_create(&fib_fun, NULL, pit_fib, &p2);
        pthread_create(&data_fun, NULL, data_pro, &p3);
//        pthread_create(&load_fun, NULL, load_measure, &p4);
        pthread_join(cs_fun, NULL);
        pthread_join(fib_fun, NULL);
        pthread_join(data_fun, NULL);
//        pthread_join(load_fun, NULL);
        in_queDelete(riq);
        return 0;
    }

    r2 = fork();          // process of router r2
    if(r2 == 0){
        // Insert some strings
        for(int i=50; i<100; i++){
            url_file[url[i]] = f[i];
        }
        for(int i=200; i<250; i++){
            url_file[url[i]] = f[i];
        }
        for(int i=300; i<350; i++){
            url_file[url[i]] = f[i];
        }
        // initialize fib table
        for(int i=0;i<500;i++){
            fib[url[i]] = 2;
        }
        // initialize inside_que
        struct inside_que *riq;
        riq = in_queInit();
        if(riq ==  NULL){
            fprintf (stderr, "main: Queue Init failed.\n");
            exit (1);
        }
        // initialize some parameters
        struct para_intrstin p1;
        struct para_intrstout p2;
        struct para_data p3;
        p1.shm1 = &que6;
        p1.queKey1 = tQueueKey6;
        p1.iq = riq;
        p1.shm2 = &que8;
        p1.queKey2 = tQueueKey8;
        p2.shm1 = &que;
        p2.queKey1 = tQueueKey;
        p2.shm2 = &que13;
        p2.queKey2 = tQueueKey13;
        p2.iq = riq;
        p3.shm1 = &que15;
        p3.queKey1 = tQueueKey15;
        p3.shm2 = &que8;
        p3.queKey2 = tQueueKey8;

        pthread_mutex_t shmlock;
        pthread_mutex_init(&shmlock, NULL);
        p1.quelock = &shmlock;
        p3.quelock = &shmlock;

        struct para_measure p4;
        pthread_mutex_t loadquelock;
        pthread_mutex_init(&loadquelock, NULL);
        p1.quemeasure_lock = &loadquelock;
        p4.shm = &que6;
        p4.queKey = tQueueKey6;
        p4.quemeasure_lock = &loadquelock;

        // create threads
        pthread_t cs_fun, fib_fun, data_fun, load_fun;
        pthread_create(&cs_fun, NULL, cs_action, &p1);
        pthread_create(&fib_fun, NULL, pit_fib, &p2);
        pthread_create(&data_fun, NULL, data_pro, &p3);
//        pthread_create(&load_fun, NULL, load_measure, &p4);
        pthread_join(cs_fun, NULL);
        pthread_join(fib_fun, NULL);
        pthread_join(data_fun, NULL);
//        pthread_join(load_fun, NULL);
        in_queDelete(riq);
        return 0;
    }

    r3 = fork();          // process of router r3
    if(r3 == 0){
        // Insert some strings
        for(int i=100; i<150; i++){
            url_file[url[i]] = f[i];
        }
        for(int i=250; i<300; i++){
            url_file[url[i]] = f[i];
        }
        for(int i=350; i<400; i++){
            url_file[url[i]] = f[i];
        }
        // initialize fib table
        for(int i=0;i<500;i++){
            fib[url[i]] = 1;
        }
        // initialize inside_que
        struct inside_que *riq;
        riq = in_queInit();
        if(riq ==  NULL){
            fprintf (stderr, "main: Queue Init failed.\n");
            exit (1);
        }
        // initialize some parameters
        struct para_intrstin p1;
        struct para_intrstout p2;
        struct para_data p3;
        p1.shm1 = &que10;
        p1.queKey1 = tQueueKey10;
        p1.iq = riq;
        p1.shm2 = &que12;
        p1.queKey2 = tQueueKey12;
        p2.shm1 = &que17;
        p2.queKey1 = tQueueKey17;
        p2.shm2 = &que;
        p2.queKey2 = tQueueKey;
        p2.iq = riq;
        p3.shm1 = &que19;
        p3.queKey1 = tQueueKey19;
        p3.shm2 = &que12;
        p3.queKey2 = tQueueKey12;

        pthread_mutex_t shmlock;
        pthread_mutex_init(&shmlock, NULL);
        p1.quelock = &shmlock;
        p3.quelock = &shmlock;

        struct para_measure p4;
        pthread_mutex_t loadquelock;
        pthread_mutex_init(&loadquelock, NULL);
        p1.quemeasure_lock = &loadquelock;
        p4.shm = &que10;
        p4.queKey = tQueueKey10;
        p4.quemeasure_lock = &loadquelock;

        // create threads
        pthread_t cs_fun, fib_fun, data_fun, load_fun;
        pthread_create(&cs_fun, NULL, cs_action, &p1);
        pthread_create(&fib_fun, NULL, pit_fib, &p2);
        pthread_create(&data_fun, NULL, data_pro, &p3);
//        pthread_create(&load_fun, NULL, load_measure, &p4);
        pthread_join(cs_fun, NULL);
        pthread_join(fib_fun, NULL);
        pthread_join(data_fun, NULL);
//        pthread_join(load_fun, NULL);
        in_queDelete(riq);
        return 0;
    }

    r4 = fork();          // process of router r4
    if(r4 == 0){
        // Insert some strings
        for(int i=0; i<500; i++){
            url_file[url[i]] = f[i];
        }
        // initialize fib table
        for(int i=0;i<500;i++){
            fib[url[i]] = 1;
        }
        // initialize inside_que
        struct inside_que *riq;
        riq = in_queInit();
        if(riq ==  NULL){
            fprintf (stderr, "main: Queue Init failed.\n");
            exit (1);
        }
        // initialize some parameters
        struct para_intrstin p1;
        struct para_intrstout p2;
        p1.shm1 = &que14;
        p1.queKey1 = tQueueKey14;
        p1.iq = riq;
        p1.shm2 = &que16;
        p1.queKey2 = tQueueKey16;
        p2.shm1 = &que;
        p2.queKey1 = tQueueKey;
        p2.shm2 = &que;
        p2.queKey2 = tQueueKey;
        p2.iq = riq;

        pthread_mutex_t shmlock;
        pthread_mutex_init(&shmlock, NULL);
        p1.quelock = &shmlock;

        struct para_measure p4;
        pthread_mutex_t loadquelock;
        pthread_mutex_init(&loadquelock, NULL);
        p1.quemeasure_lock = &loadquelock;
        p4.shm = &que14;
        p4.queKey = tQueueKey14;
        p4.quemeasure_lock = &loadquelock;

        // create threads
        pthread_t cs_fun, fib_fun, load_fun;
        pthread_create(&cs_fun, NULL, cs_action, &p1);
        pthread_create(&fib_fun, NULL, pit_fib, &p2);
//        pthread_create(&load_fun, NULL, load_measure, &p4);
        pthread_join(cs_fun, NULL);
        pthread_join(fib_fun, NULL);
//        pthread_join(load_fun, NULL);
        in_queDelete(riq);
        return 0;
    }

    pkt_gen = fork();        // process of interest packet generator
    if(pkt_gen == 0){
        sleep(1);
        printf("into pkt_gen\n");
        // initialize memory-mapped files
        char *memory = NULL;
        int fd;
        int file_length = 0;
        fd = open("interest_pkt.txt", O_RDONLY);
        if(fd < 0){
            perror("Failed to open interest_pkt.txt!");
            return -1;
        }
        file_length = lseek(fd, 0, SEEK_END);
        memory = static_cast<char*>(mmap(NULL, file_length, PROT_READ, MAP_SHARED, fd, 0));
        int mem_len = strlen(memory);
        string strSend;
        res = que1.AttachQueue(tQueueKey1);

        int count = 0;  // interest packet counting
        struct timeval arrive;
        gettimeofday(&arrive,NULL);
        for(int i = 0; i<mem_len; i++){
            if(memory[i] != 10){
                char c = memory[i];
                strSend.append(1, c);
            }else{
                struct timeval start;
                gettimeofday(&start,NULL);
                unsigned long st = 1000000 * (start.tv_sec)+ start.tv_usec;
                stringstream stream;
                string apd, scount;
                stream.clear(); stream.str(""); stream << st; stream >> apd;
                stream.clear(); stream.str(""); stream << count; stream >> scount;
                apd = "\\" + apd + "\\" + scount + "\\0\\0";
                strSend.append(apd);
                res = -1;
                while(res < 0){
                    res = que1.PushDataIntoQueueBack((char *)strSend.data(), strSend.length());
                }
                strSend.clear();
                count++;
                for(int sleeping=0;sleeping<100;sleeping++){       // delay, control the rate
                }
            }
        }
        struct timeval afinish;
        gettimeofday(&afinish,NULL);
        unsigned long t = 1000000 * (afinish.tv_sec-arrive.tv_sec)+ afinish.tv_usec-arrive.tv_usec;
        printf("pkt_gen TIME: %lu us, num: %d\n", t, count);
        return 0;
    }
    waitpid(pkt_gen, NULL, 0);
    waitpid(r1, NULL, 0);
    waitpid(r2, NULL, 0);
    waitpid(r3, NULL, 0);
    waitpid(r4, NULL, 0);
    waitpid(l1, NULL, 0);
    waitpid(l2, NULL, 0);
    waitpid(l3, NULL, 0);
    waitpid(l4, NULL, 0);
    waitpid(l5, NULL, 0);
    waitpid(l6, NULL, 0);
    waitpid(l7, NULL, 0);
    waitpid(l8, NULL, 0);
    waitpid(l9, NULL, 0);
    waitpid(l10, NULL, 0);
	return 0;
}

// router's neighbor_que load measuring function
void *load_measure(void *q){
    struct para_measure *para;
    para = (struct para_measure *)q;
    for(int i=0; i<20000; i++){
        pthread_mutex_lock(para->quemeasure_lock);
        int res=-1;
        int num;
        res = para->shm->AttachQueue(para->queKey);
        num = para->shm->GetInQueuePkgNum();
        pthread_mutex_unlock(para->quemeasure_lock);
        printf("quekey %d %d\n", para->queKey, num);
        usleep(300);
    }
    return 0;
}

// cs thread function
void *cs_action(void *q){
    struct para_intrstin *para;
    para = (struct para_intrstin *)q;
    int res;
    int cur_que;
    char *file_name;
    int io_file;
    off_t offset = 0;
    int flength = 0;
    map<string, string>::iterator iter;
    
    while(1){
        string req;
        char *b;
        b = (char*)malloc(100*sizeof(char));
        int pres = -1;
        while(pres < 0){       // read interest packet from neighbor_que
            pthread_mutex_lock(para->quemeasure_lock);
            pres = para->shm1->AttachQueue(para->queKey1);
            pres = para->shm1->ReadDataFromQueueHead(b, 1024);
            pthread_mutex_unlock(para->quemeasure_lock);
        }
        req = b;
        
        unsigned long r_start;
        struct timeval r_srt;
        gettimeofday(&r_srt,NULL);
        r_start = 1000000 * (r_srt.tv_sec)+ r_srt.tv_usec;
        
        // parse head of interest packet
        std::vector<std::string> parts;
        split(req, '\\', parts);
        stringstream stream;
        int segID, pid, flag;
        unsigned long pkt_start, pkt_end;
        stream.clear(); stream.str(""); stream << parts[1];  stream >> segID;
        stream.clear(); stream.str(""); stream << parts[2];  stream >> pkt_start;
        stream.clear(); stream.str(""); stream << parts[3];  stream >> pid;
        stream.clear(); stream.str(""); stream << parts[4];  stream >> flag;

        // search hash index
        iter = url_file.find(parts[0]);
        if(iter != url_file.end()){
            // if content in CS
            file_name = (char *)url_file[parts[0]].data();
            io_file = open(file_name, O_RDONLY);
            if(io_file < 0){
                perror("Failed to open file!");
                return (NULL);
            }
            flength = lseek(io_file, 0, SEEK_END);
            int out = segID*SEG_SIZE;
            if(out <= flength){
                offset = lseek(io_file, out, SEEK_SET);
                res = read(io_file, &pbuf[pid][0], SEG_SIZE);     // initiate blocking io command
                if(res < 0){
                    perror("fail to read");
                    return (NULL);
                }
            }else{
                printf("Bad segID! Not send req!\n");
                continue;
            }
            char data_return[SEG_SIZE];
            for(int i=0;i<SEG_SIZE;i++){
                data_return[i] = pbuf[pid][i];
            }
            int r = -1;
            pthread_mutex_lock(para->quelock);
            while(r < 0){
                r = para->shm2->AttachQueue(para->queKey2);
                if(r < 0){
                    printf("call back error\n");
                }
                r = para->shm2->PushDataIntoQueueBack(data_return, strlen(data_return));   // return data packet
                if(r < 0){
                    printf("call back pushque error\n");
                }
            }
            pthread_mutex_unlock(para->quelock);
            struct timeval par_end;
            gettimeofday(&par_end,NULL);
            pkt_end = 1000000 * (par_end.tv_sec)+ par_end.tv_usec;
            unsigned long p_latency = pkt_end - pkt_start;
            printf("pid %d %lu\n", pid, p_latency);
        }else{
            // if content not in CS
            char url_in[URLSIZE];
            strcpy(url_in, parts[0].c_str());
            pthread_mutex_lock(para->iq->mut);
            while(para->iq->full){
                pthread_cond_wait(para->iq->notFull, para->iq->mut);  // inside_que full, wait cond_signal
            }
            in_queAdd(para->iq, url_in, segID, pkt_start, pid, r_start, flag);     // add to inside_que
            pthread_cond_signal(para->iq->notEmpty);
            pthread_mutex_unlock(para->iq->mut);
        }
    }
    return (NULL);
}

// pit/fib thread funtion
void *pit_fib(void *q){
    struct para_intrstout *para;
    para = (struct para_intrstout *)q;
    int res;
    while(1){
        pthread_mutex_lock(para->iq->mut);
        while(para->iq->empty){
            pthread_cond_wait(para->iq->notEmpty, para->iq->mut);  // inside_que empty, wait cond_signal
        }
        struct pkt_head head;
        in_queDel(para->iq, &head);       // read from inside_que
        pthread_cond_signal(para->iq->notFull);
        pthread_mutex_unlock(para->iq->mut);
        // encapsulate interest packet
        string out;
        stringstream stream;
        string url_name, segID, pkt_start, pid, flag;
        stream.clear(); stream.str(""); stream << head.url_name; stream >> url_name;
        stream.clear(); stream.str(""); stream << head.segID; stream >> segID;
        stream.clear(); stream.str(""); stream << head.pkt_start; stream >> pkt_start;
        stream.clear(); stream.str(""); stream << head.pid; stream >> pid;
        stream.clear(); stream.str(""); stream << head.flag; stream >> flag;
        out = out + url_name +"\\" + segID + "\\" + pkt_start + "\\" + pid + "\\" + flag + "\\0";
        char *value = (char*)out.data();
        // search fib table
        int port = fib[url_name];
        unsigned long r_end, r_latency;
        struct timeval r_srt;
        switch(port){
            case 1:     // port 1
                res = para->shm1->AttachQueue(para->queKey1);
                res = -1;
                while(res < 0){
                    res = para->shm1->PushDataIntoQueueBack(value, strlen(value));
                }
                gettimeofday(&r_srt,NULL);
                r_end = 1000000 * (r_srt.tv_sec)+ r_srt.tv_usec;
                r_latency = r_end - head.r_start;
//                printf("r%d %d %lu %d\n", para->queKey1, para->queKey2, r_latency, head.flag);
                break;
            case 2:     // port 2
                res = para->shm2->AttachQueue(para->queKey2);
                res = -1;
                while(res < 0){
                    res = para->shm2->PushDataIntoQueueBack(value, strlen(value));
                }
                gettimeofday(&r_srt,NULL);
                r_end = 1000000 * (r_srt.tv_sec)+ r_srt.tv_usec;
                r_latency = r_end - head.r_start;
//                printf("r%d %d %lu %d\n", para->queKey1, para->queKey2, r_latency, head.flag);
                break;
            default:  // error
                printf("out error!\n");
                break;
        }
    }
    return (NULL);
}

// data processing thread function
void *data_pro(void *q){
    struct para_data *para;
    para = (struct para_data *)q;
    int pres;
    while(1){
        string req;
        char *b;
        b = (char*)malloc(4200*sizeof(char));
        pres = -1;
        while(pres < 0){          // read data packet from neighbor_que
            pres = para->shm1->AttachQueue(para->queKey1);
            pres = para->shm1->ReadDataFromQueueHead(b, 4200);
        }
        req = b;
//        usleep(100);
        char *value = (char*)req.data();
        pres = -1;
        pthread_mutex_lock(para->quelock);
        while(pres < 0){          // push data packet into neighbor_que
            pres = para->shm2->AttachQueue(para->queKey2);
            pres = para->shm2->PushDataIntoQueueBack(value, strlen(value));
            if(pres < 0){
                printf("data_pro push error %d quekey %d\n", pres, para->queKey2);
            }
        }
        pthread_mutex_unlock(para->quelock);
    }
	return (NULL);
}

struct inside_que *in_queInit(void){
    struct inside_que *q;
    q = (struct inside_que *)malloc (sizeof (struct inside_que));
    if (q == NULL) return (NULL);
    q->empty = 1;
    q->full = 0;
    q->head = 0;
    q->tail = 0;
    q->mut = (pthread_mutex_t *) malloc (sizeof (pthread_mutex_t));
    pthread_mutex_init (q->mut, NULL);
    q->notFull = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
    pthread_cond_init (q->notFull, NULL);
    q->notEmpty = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
    pthread_cond_init (q->notEmpty, NULL);
    return (q);
}
void in_queDelete(struct inside_que *q){
    pthread_mutex_destroy (q->mut);
    free (q->mut);	
    pthread_cond_destroy (q->notFull);
    free (q->notFull);
    pthread_cond_destroy (q->notEmpty);
    free (q->notEmpty);
    free (q);
}
void in_queAdd(struct inside_que *q, char url_in[], int ID_in, unsigned long pkt_start, int pid, unsigned long r_start, int flag){
    strcpy(q->pkt_arr[q->tail].url_name, url_in);
    q->pkt_arr[q->tail].segID = ID_in;
    q->pkt_arr[q->tail].pkt_start = pkt_start;
    q->pkt_arr[q->tail].pid = pid;
    q->pkt_arr[q->tail].r_start = r_start;
    q->pkt_arr[q->tail].flag = flag;
    q->tail++;
    if(q->tail == QUEUESIZE)
        q->tail = 0;
    if(q->tail == q->head)
        q->full = 1;
    q->empty = 0;
    return;
}
void in_queDel(struct inside_que *q, struct pkt_head *out){
    strcpy((*out).url_name, q->pkt_arr[q->head].url_name);
    (*out).segID = q->pkt_arr[q->head].segID;
    (*out).pkt_start = q->pkt_arr[q->head].pkt_start;
    (*out).pid = q->pkt_arr[q->head].pid;
    (*out).r_start = q->pkt_arr[q->head].r_start;
    (*out).flag = q->pkt_arr[q->head].flag;
    q->head++;
    if(q->head == QUEUESIZE)
        q->head = 0;
    if(q->head == q->tail)
        q->empty = 1;
    q->full = 0;
    return;
}

void split(const std::string &s, char delim, std::vector<std::string> &elems){
    std::stringstream ss;
    ss.str(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
}
