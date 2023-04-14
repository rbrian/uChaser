#ifndef _SETTING_H_
#define _SETTING_H_
#include <WString.h>

class Setting
{
private:
    int d_foo;
    String d_ssid = "uChaser";
    String d_password = "";

protected:
    Setting();

public:
    Setting(Setting &other) = delete;
    void operator=(const Setting &) = delete;

    static Setting *GetInstance();

    // these settings are stored in the JSON file.  Default values
    String ssid = "uChaser";
    String password = "";

    void reset();
    void resetWifi();

    void writeJSON();
};

#endif