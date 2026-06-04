#include <Arduino.h>
#include <M5Unified.h>
#include <M5GFX.h>

// 干支表
const char *TIANGAN[10] = {"甲","乙","丙","丁","戊","己","庚","辛","壬","癸"};
const char *DIZHI[12] = {"子","丑","寅","卯","辰","巳","午","未","申","酉","戌","亥"};

// 初始化座标系
    int ori_x = 0;
    int ori_y = 0;
    int scaling = 10; // 大小倍率(0-100)
    int width = (M5.Display.getRotation() == 0) ? 540 : 960;
    int height = (M5.Display.getRotation() == 0) ? 960 : 540;
    int b_width = (int)(width * scaling / 100);
    int b_height = (int)(height * scaling / 100);

// 初始化时间
    struct TimeInfo {
        int year;
        int month;
        int day;
        int hour;
        int minute;
        int second;
        const char *weekday;
    };

    TimeInfo nowTime;   
    unsigned long lastTick = 0;

    TimeInfo getMockTime() {
        TimeInfo t;
        t.year = 2026;
        t.month = 6;
        t.day = 3;
        t.hour = 16;
        t.minute = 0;
        t.second = 0;
        t.weekday = "星期日";
        return t;
    }

    struct ganzhiTimeInfo {
        char year;
        char month;
        char day;
        char hour;
        char quarter;
    };

    ganzhiTimeInfo nowGanzhiTime;


char gregorianToGanzhi(TimeInfo t, ganzhiTimeInfo& ganzhi) {

}

int getDaysInMonth(int year, int month) {
    static const int day[] = {
        0,31,28,31,30,31,30,31,31,30,31,30,31
    };

    if (month == 2 && ((year % 4 == 0 && year % 100 != 0) ||
        (year % 400 == 0))) {
            return 29;
        }
    
        return day[month];
}

void tickTime(TimeInfo& t) {
    t.second++;
    
    if (t.second >=60) {
        t.second = 0;
        t.minute++;
    }

    if (t.minute >= 60) {
        t.minute = 0;
        t.hour++;
    }

    if (t.hour >= 24) {
        t.hour = 0;
        t.day++;
    }

    if (t.day > getDaysInMonth(t.year, t.month)) {
        t.day = 1;
        t.month++;
    }

    if (t.month > 12) {
        t.month = 1;
        t.year++;
    }
}

void drawCalendarPage (const TimeInfo& t) {
    M5.Display.setTextColor(BLACK, WHITE);

    // 日期
    M5.Display.setCursor(20, 40);
    M5.Display.printf("共和%04d年%02d月%02d日\n", t.year + 841, t.month, t.day);
    // 星期
    M5.Display.setCursor(20, 120);
    M5.Display.println(t.weekday);
    // 时间
    M5.Display.setCursor(20, 180);
    M5.Display.printf("%02d:%02d:%02d\n", t.hour, t.minute, t.second);
}

void setup() {
    // 初始化整机
    auto cfg = M5.config();
    M5.begin(cfg);

    // 设置屏幕方向
    M5.Display.setRotation(1);
    // 设置刷新模式
    M5.Display.setEpdMode(epd_mode_t::epd_fastest);
    M5.Display.fillScreen(WHITE);
    // 设置中文字体
    M5.Display.setFont(&fonts::efontCN_16);
    // 设置字号
    M5.Display.setTextSize(4);

    nowTime = getMockTime();
    drawCalendarPage(nowTime);

    lastTick = millis();
}

void loop() {
    unsigned long currentMillis = millis();

    if (currentMillis - lastTick >= 1000) {
        lastTick += 1000;
        tickTime(nowTime);
        drawCalendarPage(nowTime);
    }
}