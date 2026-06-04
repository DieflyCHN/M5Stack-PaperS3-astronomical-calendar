#include <Arduino.h>
#include <M5Unified.h>
#include <M5GFX.h>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <cmath>


constexpr int LINEHEIGHT = 40;
constexpr int REPUBLIC_OFFSET = 841;

int gregorianToRepublicanYear(int gregorianYear) {
    return gregorianYear + REPUBLIC_OFFSET;
}

/* 
    Unix 时间部分
*/
struct UnixClock {
    int64_t unixUTC;    // UTC 秒数，唯一事实来源
    uint32_t lastMillis;// 上次校准 millis()
    bool valid;         // 当前时间是否可信
};

void tickUnixClock(UnixClock& u) {
    if (!u.valid) return;

    uint32_t now = millis();

    while ((uint32_t)(now - u.lastMillis) >= 1000) {
        u.unixUTC++;
        u.lastMillis += 1000;
    }
}

void setUnixClock(UnixClock& u, int64_t unixTimeUTC) {
    u.unixUTC = unixTimeUTC;
    u.lastMillis = millis();
    u.valid = true;
}

int64_t getUnixTimeUTC(const UnixClock& u) {
    return u.unixUTC;
}
// 初始化 UnixTime
UnixClock clockUnix = {0,0,false};

/*
    Gregorian 时间部分
*/
struct GregorianTime {
    int year;
    int month;
    int day;

    int hour;
    int minute;
    int second;
    
    int weekday;
};

GregorianTime unixToGregorian(int64_t unixUTC) {
    time_t now = unixUTC;

    struct tm t;
    gmtime_r(&now, &t);

    GregorianTime g;
    g.year = t.tm_year + 1900;
    g.month = t.tm_mon + 1;
    g.day = t.tm_mday;

    g.hour = t.tm_hour;
    g.minute = t.tm_min;
    g.second = t.tm_sec;

    g.weekday = t.tm_wday;

    return g;
}

void setupTimezoneBerlin() {
    setenv("TZ", "CET-1CEST,M3.5.0/2,M10.5.0/3", 1);
    tzset();
}

int64_t gregorianToUnixUTC(
    int year,
    int month,
    int day,
    int hour,
    int minute,
    int second)
{
    struct tm t = {};

    t.tm_year = year - 1900;
    t.tm_mon  = month - 1;
    t.tm_mday = day;

    t.tm_hour = hour;
    t.tm_min  = minute;
    t.tm_sec  = second;

    setenv("TZ", "UTC0", 1);
    tzset();
    int64_t result = (int64_t)mktime(&t);
    setupTimezoneBerlin();   // 恢复全局时区
    return result;
}

const char *weekdayStrCHN[] = {"星期日","星期一","星期二","星期三",
    "星期四","星期五","星期六"};
// 初始化 GregorianTime
GregorianTime utcGregorian;

/*
    本地时间部分
*/
int64_t unixToBerlinUnixLocal(int64_t unixUTC) {
    time_t now = unixUTC;

    struct tm utcTm;
    struct tm berTm;

    gmtime_r(&now, &utcTm);
    localtime_r(&now, &berTm);

    int offset =
        (berTm.tm_hour - utcTm.tm_hour) * 3600 +
        (berTm.tm_min  - utcTm.tm_min ) * 60 +
        (berTm.tm_sec  - utcTm.tm_sec );

    int dayDiff = berTm.tm_yday - utcTm.tm_yday;

    if (dayDiff > 1) dayDiff = -1;
    if (dayDiff < -1) dayDiff = 1;

    offset += dayDiff * 86400;

    return unixUTC + offset;
}

int64_t unixToBeijingUnixLocal(int64_t unixUTC) {
    return unixUTC + 8 * 3600;
}
// 初始化 GregorianLocalTime
GregorianTime berGregorian;
GregorianTime beiGregorian;

/*
    JulianDate部分
*/
double unixToJD(int64_t unixUTC) {
    return unixUTC / 86400.0 + 2440587.5;
}

int64_t unixToJDN(int64_t unixUTC) {
    return (int64_t)floor(unixToJD(unixUTC) + 0.5);
}
// 初始化 JulianDate
double jd;

/*
    编译时间写入
*/
constexpr int BUILD_TIME_COMPENSATION_SECONDS = 20;
int64_t getBuildUnixUTC() {
    struct tm t = {};

    t.tm_year = BUILD_YEAR - 1900;
    t.tm_mon  = BUILD_MONTH - 1;
    t.tm_mday = BUILD_DAY;

    t.tm_hour = BUILD_HOUR;
    t.tm_min  = BUILD_MINUTE;
    t.tm_sec  = BUILD_SECOND + BUILD_TIME_COMPENSATION_SECONDS;

    t.tm_isdst = -1; // 玄学开启夏令时检测

    return mktime(&t);
}

/*
    干支部分
*/
const char *TIANGAN[10] = {"甲","乙","丙","丁","戊",
    "己","庚","辛","壬","癸"};
const char *DIZHI[12] = {"子","丑","寅","卯",
    "辰","巳","午","未",
    "申","酉","戌","亥"};

struct Ganzhi {
    int index;
    const char *gan;
    const char *zhi;
};
// 儒略日數（JDN）计算日柱
Ganzhi jdnToDayGanzhi(int64_t jdn) {
    int index = (int)((jdn + 49) % 60); // 神奇对应，既定经验，-11或+49

    if (index < 0) {
        index += 60;
    }

    Ganzhi rz;
    rz.index = index;
    rz.gan = TIANGAN[index % 10];
    rz.zhi = DIZHI[index % 12];

    return rz;
}
// 计算时柱及时辰
const char *KE[4] = {
    "初","一","二","三"
};
struct ChineseHour {
    const char *hourGan;
    const char *hourZhi;
    const char *chuZheng;   // T = 正，F = 初
    const char *ke;
};

ChineseHour dayGanzhiToHourGanzhi(const Ganzhi& dayGanzhi, int hour, int minute) {
    int zhiIndex = ((hour + 1) / 2) % 12;
    int ganIndex = ((dayGanzhi.index % 5) * 2 + zhiIndex) % 10;
    
    ChineseHour sz;
        sz.hourGan = TIANGAN[ganIndex];
        sz.hourZhi = DIZHI[zhiIndex];
        sz.chuZheng = (hour % 2 == 0)? "正":"初";
        sz.ke = KE[minute / 15];

    return sz;
}
/*
    RTC
*/
bool saveUnixClockToRTC(const UnixClock& clock) {
    if (!clock.valid) return false;
    if (!M5.Rtc.isEnabled()) return false;

    time_t now = (time_t)clock.unixUTC;
    struct tm t;
    gmtime_r(&now, &t);   // RTC 内部统一存 UTC

    m5::rtc_datetime_t rtc;
    rtc.date.year  = t.tm_year + 1900;
    rtc.date.month = t.tm_mon + 1;
    rtc.date.date  = t.tm_mday;
    rtc.date.weekDay = t.tm_wday;

    rtc.time.hours   = t.tm_hour;
    rtc.time.minutes = t.tm_min;
    rtc.time.seconds = t.tm_sec;

    M5.Rtc.setDateTime(rtc);
    return true;
}
bool loadUnixClockFromRTC(UnixClock& clock) {
    if (!M5.Rtc.isEnabled()) return false;

    auto rtc = M5.Rtc.getDateTime();

    int year  = rtc.date.year;
    int month = rtc.date.month;
    int day   = rtc.date.date;

    if (year < 2024 || year > 2100) return false;
    if (month < 1 || month > 12) return false;
    if (day < 1 || day > 31) return false;

    struct tm t = {};

    t.tm_year = year - 1900;
    t.tm_mon  = month - 1;
    t.tm_mday = day;

    t.tm_hour = rtc.time.hours;
    t.tm_min  = rtc.time.minutes;
    t.tm_sec  = rtc.time.seconds;

    // 因为 RTC 里存的是 UTC，所以不能用 mktime()
    int64_t unixUTC = gregorianToUnixUTC(
        year,
        month,
        day,
        rtc.time.hours,
        rtc.time.minutes,
        rtc.time.seconds
    );

    setUnixClock(clock, unixUTC);
    return true;
}
uint32_t lastRTCSaveMillis = 0;
constexpr uint32_t RTC_SAVE_INTERVAL_MS = 60000;
constexpr bool FORCE_RESET_RTC_FROM_BUILD_TIME = false; // 手动操作是否写入 Build 时间，使用后再改回
const char* timeSource = "UNKNOWN";
// --------------------------------------------------------------------------------
void setup() {
    // 初始化整机
    auto cfg = M5.config();
    M5.begin(cfg);
    M5.Rtc.begin();

    // 设置屏幕方向
    M5.Display.setRotation(0);
    // 初始化座标系
    int ori_x = 0;
    int ori_y = 0;
    int scaling = 5; // 大小倍率(0-100)
    int width = (M5.Display.getRotation() == 0) ? 540 : 960;
    int height = (M5.Display.getRotation() == 0) ? 960 : 540;
    int b_width = (int)(width * scaling / 100);
    int b_height = (int)(height * scaling / 100);
    // 设置刷新模式
    M5.Display.setEpdMode(epd_mode_t::epd_fastest);
    M5.Display.fillScreen(WHITE);
    // 设置中文字体
    M5.Display.setFont(&fonts::efontCN_16);
    // 设置字号
    M5.Display.setTextSize(2.5);
    // 设置时区
    setupTimezoneBerlin();

    if (FORCE_RESET_RTC_FROM_BUILD_TIME) {
        setUnixClock(clockUnix, getBuildUnixUTC());
        saveUnixClockToRTC(clockUnix);
        timeSource = "FORCE BUILD";
    } else if (loadUnixClockFromRTC(clockUnix)) {
        timeSource = "RTC";
    } else {
        setUnixClock(clockUnix, getBuildUnixUTC());
        saveUnixClockToRTC(clockUnix);
        timeSource = "BUILD";
    }
}
// --------------------------------------------------------------------------------
void loop() {
    // 初始化时间
    tickUnixClock(clockUnix);
    uint32_t currentMillis = millis();

    if (clockUnix.valid &&
        (uint32_t)(currentMillis - lastRTCSaveMillis) >= RTC_SAVE_INTERVAL_MS) {
        saveUnixClockToRTC(clockUnix);
        lastRTCSaveMillis = currentMillis;
    }

    int64_t nowUTC = getUnixTimeUTC(clockUnix);
    int64_t nowBER = unixToBerlinUnixLocal(nowUTC);
    int64_t nowBEI = unixToBeijingUnixLocal(nowUTC);

    utcGregorian = unixToGregorian(nowUTC);
    berGregorian = unixToGregorian(nowBER);
    beiGregorian = unixToGregorian(nowBEI);

    jd = unixToJD(nowUTC);
    double mjd = jd - 2400000.5;

    int64_t berJDN = unixToJDN(nowBER);
    int64_t beiJDN = unixToJDN(nowBEI);
    Ganzhi berDayGanzhi = jdnToDayGanzhi(berJDN);
    Ganzhi beiDayGanzhi = jdnToDayGanzhi(beiJDN);
    ChineseHour berShichen = dayGanzhiToHourGanzhi(berDayGanzhi, berGregorian.hour, berGregorian.minute);
    ChineseHour beiShichen = dayGanzhiToHourGanzhi(beiDayGanzhi, beiGregorian.hour, beiGregorian.minute);

    // 显示 unixUTC
    M5.Display.setCursor(0,LINEHEIGHT*0);
    M5.Display.printf("unixUTC:%lld",(long long)clockUnix.unixUTC);

    // 显示 JD 时间
    M5.Display.setCursor(0,LINEHEIGHT*1);
    M5.Display.printf("JD:%f\nMJD:%f", jd, mjd);

    // 显示 UTC 时间
    M5.Display.setCursor(0,LINEHEIGHT*3);
    M5.Display.printf("UTC:%04d.%02d.%02d  %02d:%02d:%02d  %1d",
        utcGregorian.year,utcGregorian.month,utcGregorian.day,
        utcGregorian.hour,utcGregorian.minute,utcGregorian.second,
        ((utcGregorian.weekday == 0) ? 7 : utcGregorian.weekday));

    // 显示纪年
    M5.Display.setCursor(0,LINEHEIGHT*4);
        // 采用本地时间，避免潜在的跨年问题
    M5.Display.printf("共和%04d年  公元%04d年",
        gregorianToRepublicanYear(berGregorian.year),
        berGregorian.year
    );

    // 显示 柏林 时间
    M5.Display.setCursor(0,LINEHEIGHT*6);
    M5.Display.printf("BER:%04d.%02d.%02d  %02d:%02d:%02d  %1d",
        berGregorian.year,berGregorian.month,berGregorian.day,
        berGregorian.hour,berGregorian.minute,berGregorian.second,
        ((berGregorian.weekday == 0) ? 7 : berGregorian.weekday));  
    // 显示柏林干支（四柱八字）
    M5.Display.setCursor(0,LINEHEIGHT*7);
    M5.Display.printf("某某 某某 %s%s %s%s%s%s刻",
        berDayGanzhi.gan, berDayGanzhi.zhi,
        berShichen.hourGan,berShichen.hourZhi,berShichen.chuZheng,berShichen.ke);

    // 显示 北京 时间
    M5.Display.setCursor(0,LINEHEIGHT*9);
    M5.Display.printf("BEI:%04d.%02d.%02d  %02d:%02d:%02d  %1d",
        beiGregorian.year,beiGregorian.month,beiGregorian.day,
        beiGregorian.hour,beiGregorian.minute,beiGregorian.second,
        ((beiGregorian.weekday == 0) ? 7 : beiGregorian.weekday));
    // 显示柏林干支（四柱八字）
    M5.Display.setCursor(0,LINEHEIGHT*10);
    M5.Display.printf("某某 某某 %s%s %s%s%s%s刻",
        beiDayGanzhi.gan, beiDayGanzhi.zhi,
        beiShichen.hourGan,beiShichen.hourZhi,beiShichen.chuZheng,beiShichen.ke);
    // 显示校时来源
    M5.Display.setCursor(0,LINEHEIGHT*11);
    M5.Display.printf("SRC:%s",timeSource);
}