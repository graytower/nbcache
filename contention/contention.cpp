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
#include <eio.h>
#include <map>
#include "shmqueue.h"
#include "bloom_filter.hpp"

using namespace std;

#define FILE_NUM 100   // total number of files
#define FILE_NUM_A 500 // total number of files
#define URLSIZE 45     // size of url
#define QUEUESIZE 50000  // max length of inside_que
#define REQ_NUM 5001     // total number of requests
#define SEG_SIZE 4096    // data segment(4096Byte)
#define IO_QUE 64       // IO que length threshold

map<string, string> url_file;  // url-file hash table in content store
char pbuf[REQ_NUM][SEG_SIZE];
char dbuf[REQ_NUM][SEG_SIZE];
map<string, int> fib;         // fib table
map<string, int> file_id;     // file-id hash table
int rdpids[REQ_NUM-1];        // interest packet id
int wrpids[REQ_NUM-1];        // data packet id

struct pkt_head{          // interest packet head
    char url_name[URLSIZE];
    int segID;
    unsigned long pkt_start;
    int pid;
    unsigned long r_start;
};
struct inside_que{    // inside_que in one router for multi-threaded communication
    struct pkt_head pkt_arr[QUEUESIZE];
    int head, tail;
    int full, empty;
    pthread_mutex_t *mut;
    pthread_cond_t *notFull, *notEmpty;
};
struct parsePara{     // parameter for cs thread
    bloom_filter *filter;
    QueueOper *shm;
    key_t queKey;
    struct inside_que *iq;
};
struct outPara{       // parameter for pit/fib thread
    QueueOper *shm1;
    key_t queKey1;      // port 1
    QueueOper *shm2;
    key_t queKey2;      // port 2
    struct inside_que *iq;
};
struct dataPara{     // parameter for data processing thread
    bloom_filter *filter;
    QueueOper *shm1;
    key_t queKey1;      // port 1
    QueueOper *shm2;
    key_t queKey2;      // port 2
    QueueOper *shm3;
    key_t queKey3;      // port 3
};
struct eio_data{   // parameter for aio command
    int *pid;
    int fid;
};
struct eio_data rdedata[REQ_NUM-1];
struct eio_data wredata[REQ_NUM-1];

void *parseReq(void *args);   // cs thread function
void *outReq(void *args);     // pit/fib thread function
void *cacheRes(void *args);   // data processing thread function
void *checkIO(void *args);    // aio polling funtion
void *monQue(void *args);     // IO que length monitoring function

// some functions for inside_que
struct inside_que *in_queInit(void);
void in_queDelete(struct inside_que *q);
void in_queAdd(struct inside_que *q, char url_in[], int ID_in, unsigned long pkt_start, int pid, unsigned long r_start);
void in_queDel(struct inside_que *q, struct pkt_head *out);

void split(const std::string &s, char delim, std::vector<std::string> &elems);

// some functions for libeio
void want_poll(void);
void done_poll(void);
int call_back(eio_req *req);
int call_back_data(eio_req *req);

int main(){
    // create some neighbor_ques for multi-process communication
    int res = 1;
    QueueOper que = QueueOper();
    key_t tQueueKey = 9;
    res = que.CreateQueue(tQueueKey);
    cout<<"CreateQueue "<<res<<endl;
    QueueOper que1 = QueueOper();    
    key_t tQueueKey1 = 1;
    res = que1.CreateQueue(tQueueKey1);
    cout<<"CreateQueue1 "<<res<<endl;
    QueueOper que2 = QueueOper();
    key_t tQueueKey2 = 2;
    res = que2.CreateQueue(tQueueKey2);
    cout<<"CreateQueue2 "<<res<<endl;
    
    for(int i=0;i<REQ_NUM-1;i++){
        rdpids[i]=i;
    }
    for(int i=0;i<REQ_NUM-1;i++){
        wrpids[i]=i+5000;
    }
    
    // initialize all urls
    string url[FILE_NUM_A] = {
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
    string f[FILE_NUM_A] = {
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
    
    for(int i=0;i<FILE_NUM;i++){
        file_id[f[i]] = i;
    }

    pid_t pro_interest, pro_data, r1;
    
    r1 = fork();        // process of router r1
    if(r1 == 0){
        // initialize bloom filter
        bloom_parameters parameters;
        // How many elements roughly do we expect to insert?
        parameters.projected_element_count = 100;
        // Maximum tolerable false positive probability? (0,1)
        parameters.false_positive_probability = 0.000001; // 1 in 1000000
        if (!parameters){
            cout<<"Error - Invalid set of bloom filter parameters!"<<endl;
            return 1;
        }
        parameters.compute_optimal_parameters();
        // Instantiate Bloom Filter
        bloom_filter filter(parameters);
        // Insert some strings
        for(int i=0; i<100; i++){
            filter.insert(url[i]);
            url_file[url[i]] = f[i];
        }
        // initialize fib table
        for(int i=0;i<100;i++){
            fib[url[i]] = 1;
        }
        // initialize eio
        cout<<"r1:eio_init()"<<endl;
        if(eio_init(want_poll, done_poll)) abort();
        // initialize inside_que
        struct inside_que *r1iq;
        r1iq = in_queInit();
        if(r1iq ==  NULL){
            fprintf (stderr, "main: Queue Init failed.\n");
            exit (1);
        }
        // initialize some parameters
        struct parsePara r1p1;
        struct outPara r1p2;
        struct dataPara r1p3;
        r1p1.filter = &filter;
        r1p1.shm = &que1;
        r1p1.queKey = tQueueKey1;
        r1p1.iq = r1iq;
        
        r1p2.shm1 = &que;
        r1p2.queKey1 = tQueueKey;
        r1p2.shm2 = &que;
        r1p2.queKey2 = tQueueKey;
        r1p2.iq = r1iq;
        
        r1p3.filter = &filter;
        r1p3.shm1 = &que2;
        r1p3.queKey1 = tQueueKey2;
        r1p3.shm2 = &que;
        r1p3.queKey2 = tQueueKey;
        r1p3.shm3 = &que;
        r1p3.queKey3 = tQueueKey;
        
        // create threads
        pthread_t r1_par, r1_out, r1_data, monitor;
        pthread_create(&r1_par, NULL, parseReq, &r1p1);
        pthread_create(&r1_out, NULL, outReq, &r1p2);
        pthread_create(&r1_data, NULL, cacheRes, &r1p3);
        pthread_create(&monitor, NULL, monQue, NULL);
//        pthread_create(&check_io, NULL, checkIO, NULL);
        pthread_join(r1_par, NULL);
        pthread_join(r1_out, NULL);
        pthread_join(r1_data, NULL);
        pthread_join(monitor, NULL);
//        pthread_join(check_io, NULL);
        in_queDelete(r1iq);
        return 0;
    }
    
    pro_interest = fork();        // process of interest producing
    if(pro_interest == 0){
        sleep(1);
        // initialize memory-mapped files
        char *memory = NULL;
        int fd;
        int file_length = 0;
        fd = open("con_interest_pkt.txt", O_RDONLY);
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
                apd = "\\" + apd + "\\" + scount + "\\0";
                strSend.append(apd);
                res = que1.PushDataIntoQueueBack((char *)strSend.data(), strSend.length());
                strSend.clear();
                count++;
                usleep(20);      // delay, control the rate
            }
        }
        struct timeval afinish;
        gettimeofday(&afinish,NULL);
        unsigned long t = 1000000 * (afinish.tv_sec-arrive.tv_sec)+ afinish.tv_usec-arrive.tv_usec;
        printf("pro_interest TIME %lu us, num %d\n", t, count);
        return 0;
    }
    
    pro_data = fork();        // process of data producing
    if(pro_data == 0){
        sleep(1);
        cout<<"into pro_data"<<endl;
        // initialize memory-mapped files
        char *memory = NULL;
        int fd;
        int file_length = 0;
        fd = open("con_data_pkt.txt", O_RDONLY);
        if(fd < 0){
            perror("Failed to open data_pkt.txt!");
            return -1;
        }
        file_length = lseek(fd, 0, SEEK_END);
        memory = static_cast<char*>(mmap(NULL, file_length, PROT_READ, MAP_SHARED, fd, 0));
        
        int mem_len = strlen(memory);
        string strSend;
        res = que2.AttachQueue(tQueueKey2);

        int count = 0;  // data packet counting
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
                apd = "\\" + apd + "\\" + scount + "\\0";
                strSend.append(apd);
                res = que2.PushDataIntoQueueBack((char *)strSend.data(), strSend.length());
                strSend.clear();
                count++;
                usleep(1);      // delay, control the rate
            }
        }
        struct timeval afinish;
        gettimeofday(&afinish,NULL);
        unsigned long t = 1000000 * (afinish.tv_sec-arrive.tv_sec)+ afinish.tv_usec-arrive.tv_usec;
        printf("pro_data TIME %lu us, num %d\n", t, count);
        return 0;
    }
    
    waitpid(pro_interest, NULL, 0);
    waitpid(pro_data, NULL, 0);
    waitpid(r1, NULL, 0);
    
    res = que.DestroyQueue();
    cout<<"DestroyQueue "<<res<<endl;    
    res = que1.DestroyQueue();
    cout<<"DestroyQueue1 "<<res<<endl;
    res = que2.DestroyQueue();
    cout<<"DestroyQueue2 "<<res<<endl;
    return 0;
}

void *monQue(void *q){
    sleep(1);

    for(int i=0; i<10000; i++){
        usleep(1);
        int nr = eio_nr();
        int nw = eio_nw();
        printf("%d %d\n", nr, nw);
    }

    return NULL;
}

// cs thread function
void *parseReq(void *q){
    cout<<"into parse"<<endl;
    struct parsePara *para;
    para = (struct parsePara *)q;
    int res;
    char *file_name;
    int io_file;
    int flength = 0;
    res = para->shm->AttachQueue(para->queKey);
    int cur_que;
    
    while(1){
        string req;
        char *b;
        b = (char*)malloc(512*sizeof(char));
        int pres = -1;
        while(pres < 0){      // read interest packet from neighbor_que
            pres = para->shm->ReadDataFromQueueHead(b, 1024);
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
        int segID, pid;
        unsigned long pkt_start;
        stream.clear(); stream.str(""); stream << parts[1];  stream >> segID;
        stream.clear(); stream.str(""); stream << parts[2];  stream >> pkt_start;
        stream.clear(); stream.str(""); stream << parts[3];  stream >> pid;
        // search bloom filter
        res = para->filter->contains(parts[0]);
        if(res == 1){
            // if content in CS, check IO que length vs threshold
            cur_que = eio_nready();
            if(cur_que < IO_QUE){
                file_name = (char *)url_file[parts[0]].data();
                io_file = open(file_name, O_RDONLY);
                if(io_file < 0){
                    perror("Failed to open file!");
                    cout<<pid<<" "<<parts[0]<<" "<<para->queKey<<endl;
                    return (NULL);
                }
                flength = lseek(io_file, 0, SEEK_END);
                int offset = segID*SEG_SIZE;
                if(offset <= flength){
                    rdedata[pid].pid = &rdpids[pid];
                    rdedata[pid].fid = file_id[file_name];
                    // initiate aio command
                    eio_read(io_file, &pbuf[pid][0], SEG_SIZE, offset, 0, NULL, &(rdedata[pid]));
//                    printf("%d %lu\n", rdpids[pid], r_start);
                }else{
                    cout<<"Bad segID! Not send req!"<<endl;
                }
            }else{
                char url_in[URLSIZE];
                strcpy(url_in, parts[0].c_str());
                pthread_mutex_lock(para->iq->mut);
                while(para->iq->full){
                    pthread_cond_wait(para->iq->notFull, para->iq->mut);  // inside_que full, wait cond_signal
                }
                in_queAdd(para->iq, url_in, segID, pkt_start, pid, r_start);      // add to inside_que
                pthread_cond_signal(para->iq->notEmpty);
                pthread_mutex_unlock(para->iq->mut);
            }
        }else{
            // if content not in CS
            char url_in[URLSIZE];
            strcpy(url_in, parts[0].c_str());
            pthread_mutex_lock(para->iq->mut);
            while(para->iq->full){
                pthread_cond_wait(para->iq->notFull, para->iq->mut);  // inside_que full, wait cond_signal
            }
            in_queAdd(para->iq, url_in, segID, pkt_start, pid, r_start);      // add to inside_que
            pthread_cond_signal(para->iq->notEmpty);
            pthread_mutex_unlock(para->iq->mut);
        }
    }
    return (NULL);
}

// pit/fib thread funtion
void *outReq(void *q){
    printf("into pit_fib\n");
    struct outPara *para;
    para = (struct outPara *)q;
    struct pkt_head head;
    int res;
    while(1){
        pthread_mutex_lock(para->iq->mut);
        while(para->iq->empty){
            pthread_cond_wait(para->iq->notEmpty, para->iq->mut);  // inside_que empty, wait cond_signal
        }
        in_queDel(para->iq, &head);      // read from inside_que
        pthread_cond_signal(para->iq->notFull);
        pthread_mutex_unlock(para->iq->mut);
        // encapsulate interest packet
        string out;
        stringstream stream;
        string url_name, segID, pkt_start, pid;
        stream.clear(); stream.str(""); stream << head.url_name; stream >> url_name;
        stream.clear(); stream.str(""); stream << head.segID; stream >> segID;
        stream.clear(); stream.str(""); stream << head.pkt_start; stream >> pkt_start;
        stream.clear(); stream.str(""); stream << head.pid; stream >> pid;
        out = out + url_name +"\\" + segID + "\\" + pkt_start + "\\" + pid + "\\0";
        // search fib table
        char *value = (char*)out.data();
        int port = fib[url_name];
        unsigned long r_end, r_latency;
        struct timeval r_srt;
        switch(port){
            case 1:  // port 1
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
            case 2:  // port 2
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
void *cacheRes(void *q){
    cout<<"into cacheRes"<<endl;
    struct dataPara *para;
    para = (struct dataPara *)q;
    int res;
    char *file_name;
    int io_file;
    res = para->shm1->AttachQueue(para->queKey1);
    int copynum;
    int cur_que;
    while(1){
        string respon;
        char *b;
        b = (char*)malloc(5000*sizeof(char));
        int pres = -1;
        while(pres < 0){      // read data packet from neighbor_que
            pres = para->shm1->ReadDataFromQueueHead(b, 5000);
        }
        respon = b;
        unsigned long r_start;
        struct timeval r_srt;
        gettimeofday(&r_srt,NULL);
        r_start = 1000000 * (r_srt.tv_sec)+ r_srt.tv_usec;
        // parse data packet
        std::vector<std::string> parts;
        split(respon, '\\', parts);
        stringstream stream;
        int segID, pid;
        unsigned long pkt_start;
        stream.clear(); stream.str(""); stream << parts[1];  stream >> segID;
        stream.clear(); stream.str(""); stream << parts[3];  stream >> pkt_start;
        stream.clear(); stream.str(""); stream << parts[4];  stream >> pid;
        
        cur_que = eio_nready();   // check IO que length vs threshold
        if(cur_que < IO_QUE){
            int offset = segID*SEG_SIZE;
            file_name = (char *)url_file[parts[0]].data();
            io_file = open(file_name, O_WRONLY|O_CREAT, S_IRWXU);
            if(io_file < 0){
                perror("Failed to open file!");
                return (NULL);
            }
            copynum = parts[2].copy(&dbuf[pid][0], SEG_SIZE);
            wredata[pid].pid = &wrpids[pid];
            wredata[pid].fid = file_id[file_name];
            // initiate aio command
            eio_write(io_file, &dbuf[pid][0], SEG_SIZE, offset, 0, NULL, &(wredata[pid]));
//            printf("%d %lu\n", wrpids[pid], r_start);
        }
    }
    return (NULL);
}

void want_poll(){
//    cout<<"want_poll()"<<endl;
}
void done_poll(){
//    cout<<"done_poll()"<<endl;
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
void in_queAdd(struct inside_que *q, char url_in[], int ID_in, unsigned long pkt_start, int pid, unsigned long r_start){
    strcpy(q->pkt_arr[q->tail].url_name, url_in);
    q->pkt_arr[q->tail].segID = ID_in;
    q->pkt_arr[q->tail].pkt_start = pkt_start;
    q->pkt_arr[q->tail].pid = pid;
    q->pkt_arr[q->tail].r_start = r_start;
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