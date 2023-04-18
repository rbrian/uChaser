#ifndef _SETTING_H_
#define _SETTING_H_
#include <WString.h>

class Setting
{
private:
    // todo: I don't think these are necessary?
    // int d_foo;
    // String d_ssid = "uChaser Receiver";
    // String d_password = "";
    // int d_num_avg_samples = 5;

protected:
    Setting();

public:
    Setting(Setting &other) = delete;
    void operator=(const Setting &) = delete;

    static Setting *GetInstance();

    // these settings are stored in the JSON file.  Default values to be overidden by JSON values
    String ssid = "uChaser Transmitter";
    String password = "";
    String mac_address_string = "";
    uint8_t mac_address[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    int interval_milliseconds = 1000;

    void restoreDefaults();
    bool readJSON();
    bool writeJSON();
    void macStrToArr(const String &macString, uint8_t (&macArray)[6]);
    // void *macStrToArr(const String& macString, uint8_t (&macArray)[6]);
    String macIntArrToStr(const uint8_t *macArray);
};

#endif