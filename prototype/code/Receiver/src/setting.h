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
    String ssid = "uChaser Receiver";
    String password = "";
    int num_avg_samples = 5;
    int distance_between_receivers_mm = 1000;
    String output_method = "UART";

    void restoreDefaults();
    bool readJSON();
    bool writeJSON();
};

#endif