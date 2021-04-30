#define CURL_STATICLIB
#include <curl\curl.h>
#include <fstream>
#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

//using curl and rapidJson
//curl installed from https://stackoverflow.com/questions/53861300/how-do-you-properly-install-libcurl-for-use-in-visual-studio-2017
//rapidJson installed via NuGet


static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
std::string getNwsWeatherData(double lat, double lng);
std::string makeCurlrequest(std::string url);

struct dateTime {
    int year = 0;
    int month = 0;
    int day = 0;
    int hour = 0;
    int minute = 0;
    int second = 0;
    std::string timezone = "";
};
struct forecastHour {
    int number = -1;
    std::string name = "";
    dateTime startTime = dateTime();
    dateTime endTime = dateTime();
    bool isDaytime = false;
    int temperature = -1;
    std::string temperatureUnit = "";
    std::string windSpeed = "";
    std::string windDirection = "";
    std::string iconURI = "";
    std::string shortForecast = "";
    std::string detailedForecast = "";
};
struct coordinatePoint {
    double lat = -1.0;
    double lng = -1.0;
};
struct elevation {
    double altitude = 0.0;
    std::string units = "";
};
struct weatherForecast {
    std::vector<coordinatePoint> coordinates;
    dateTime timeUpdated = dateTime();
    dateTime timeGenerated = dateTime();
    dateTime validTimes = dateTime();
    elevation altitude = elevation();
    std::vector<forecastHour> forecastHours;

};


//these have to go after their return struct declaration
dateTime parseDateTime(std::string str);
weatherForecast parseWeatherJson(std::string strIn);

//---------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------
//----------------FILL IN YOUR EMAIL ON LINE 195 BEFORE MAKING A REQUEST----------------------------
//---------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------

int main(void)
{
    //std::string data = getNwsWeatherData(40.7128, 74.0060);
    //weatherForecast wf = parseWeatherJson(data);
    

    return 0;
}


weatherForecast parseWeatherJson(std::string strIn) {

        const char* json = strIn.c_str(); //string to char array
        rapidjson::Document doc;
        doc.Parse(json); // parse the string


        //get time updated
        rapidjson::Value& properties = doc;
        dateTime timeUpdated = parseDateTime(properties["updated"].GetString());
        std::string units = properties["units"].GetString();
        dateTime timeGenerated = parseDateTime(properties["generatedAt"].GetString());
        dateTime updateTime = parseDateTime(properties["updateTime"].GetString());
        dateTime validTimes = parseDateTime(properties["validTimes"].GetString());
        elevation ele = elevation();
        ele.altitude = properties["elevation"]["value"].GetDouble();
        ele.units = properties["elevation"]["unitCode"].GetString();

        //get forecast
        rapidjson::Value& JsonForcastHours = doc["periods"];
        assert(JsonForcastHours.IsArray());
        std::vector<forecastHour> forecastHourArray;
        for (int i = 0; i < JsonForcastHours.Size(); i++) {
            forecastHour fh = forecastHour();

            //TODO - Check if any of these are null first
            fh.number = JsonForcastHours[i]["number"].GetInt();
            fh.name = JsonForcastHours[i]["name"].GetString();
            fh.startTime = parseDateTime(JsonForcastHours[i]["startTime"].GetString());
            fh.endTime = parseDateTime(JsonForcastHours[i]["endTime"].GetString());
            fh.isDaytime = JsonForcastHours[i]["isDaytime"].GetBool();
            fh.temperature = JsonForcastHours[i]["temperature"].GetInt();
            fh.temperatureUnit = JsonForcastHours[i]["temperatureUnit"].GetString();
            fh.windSpeed = JsonForcastHours[i]["windSpeed"].GetString();
            fh.windDirection = JsonForcastHours[i]["windDirection"].GetString();
            fh.iconURI = JsonForcastHours[i]["icon"].GetString();
            fh.shortForecast = JsonForcastHours[i]["shortForecast"].GetString();
            fh.detailedForecast = JsonForcastHours[i]["detailedForecast"].GetString();

            forecastHourArray.push_back(fh);
        }

        //now we have an array of forecastHours with the current forecast

        //put everything together
        weatherForecast wf = weatherForecast();
        wf.timeUpdated = timeUpdated;
        wf.timeGenerated = timeGenerated;
        wf.validTimes = validTimes;
        wf.altitude = ele;
        wf.forecastHours = forecastHourArray;

        return wf;
}

std::string getNwsWeatherData(double lat, double lng) {
    // https://www.weather.gov/documentation/services-web-api

    //https://api.weather.gov/points/{latitude},{longitude}
    std::string apiUrl = "https://api.weather.gov/points/";
    apiUrl += std::to_string(lat);
    apiUrl += ",";
    apiUrl += std::to_string(lng);
    std::string firstRequest = makeCurlrequest(apiUrl);
    std::ofstream fileOut("requestOne.json");
    fileOut << firstRequest;
    fileOut.close();

    //make the first request and parse it
    //TODO - make sure the page is valid (check status code before parsing any farther)
    const char* json = firstRequest.c_str();
    rapidjson::Document doc;
    doc.Parse(json);
    rapidjson::Value& main = doc;
    std::string forecastHourly = main["forecastHourly"].GetString(); //get the URL we want

    std::string secondRequest = makeCurlrequest(forecastHourly); //make that request

    std::ofstream fileOut2("requestTwo.json"); // save it to a file
    fileOut2 << secondRequest;
    fileOut2.close();

    //this is the string we want, return it
    return secondRequest;
}

std::string makeCurlrequest(std::string url) {
    //most of this taken from https://curl.se/libcurl/c/httpcustomheader.html

    CURL* curl;
    CURLcode res;

    curl = curl_easy_init();
    if (curl) {
        struct curl_slist* chunk = NULL;

        //10s timeout
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);

        // Follow HTTP redirects if necessary.
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        //this goes before?
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);

        //tell it we want json
        chunk = curl_slist_append(chunk, "Accept: application/ld+json");

        //firefox user agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:88.0) Gecko/20100101 Firefox/88.0
        //chunk = curl_slist_append(chunk, "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:88.0) Gecko/20100101 Firefox/88.0")


        //---------------------------------------------------------------------------------------------------
        //---------------------------------------------------------------------------------------------------
        //-------------------------------FILL THIS IN BEFORE MAKING A REQUEST--------------------------------
        //---------------------------------------------------------------------------------------------------
        //---------------------------------------------------------------------------------------------------
        chunk = curl_slist_append(chunk, "User-Agent: (noWebsite.com, your_email_here@fake_email.com)"); 

           /* set our custom set of headers */
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str()); 
        //curl_easy_setopt(curl, CURLOPT_URL, "https://api.weather.gov"); //test URL
        // curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L); // just to show output
        std::string weatherJson = "";
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &weatherJson);
        res = curl_easy_perform(curl);
        /* Check for errors */
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }
        std::cout << weatherJson;
        std::ofstream fileOut("curlJson.json");
        fileOut << weatherJson;
        fileOut.close();


        /* always cleanup */
        curl_easy_cleanup(curl);

        /* free the custom headers */
        curl_slist_free_all(chunk);

        return weatherJson;
    }
    return "curl failed!";
}

dateTime parseDateTime(std::string str) {
    //2021-04-29T15:00:00-06:00
    /*
    * Year - 0-3
    * month - 5-6
    * day - 8-9
    * hour - 11-12
    * minute - 14-15
    * second - 16-17
    * offset - 19-24
    */
    dateTime dt;
    try {
        dt.year = std::stoi(str.substr(0, 4));
        dt.month = std::stoi(str.substr(5, 2));
        dt.day = std::stoi(str.substr(8, 2));
        dt.hour = std::stoi(str.substr(11, 2));
        dt.minute = std::stoi(str.substr(14, 2));
        dt.second = std::stoi(str.substr(17, 2));
        
    }
    catch (int ex) {
        std::cout << "exception while parsing dateTime: " << ex << std::endl;
    }
    try {
        if (str.length() >= 18) {
            dt.timezone = str.substr(19);
        }
        
    }
    catch (int e) {
        std::cout << "exception while parsing dateTime: " << e << std::endl;
    }
    return dt;
}

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

