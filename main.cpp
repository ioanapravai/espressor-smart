#include <iostream>
#include <typeinfo>

#include <pistache/net.h>
#include <pistache/http.h>
#include <pistache/peer.h>
#include <pistache/http_headers.h>
#include <pistache/cookie.h>
#include <pistache/router.h>
#include <pistache/endpoint.h>
#include <pistache/common.h>

#include <signal.h>

using namespace std;
using namespace Pistache;

// This is just a helper function to preety-print the Cookies that one of the enpoints shall receive.
void printCookies(const Http::Request &req) {
    auto cookies = req.cookies();
    std::cout << "Cookies: [" << std::endl;
    const std::string indent(4, ' ');
    for (const auto &c: cookies) {
        std::cout << indent << c.name << " = " << c.value << std::endl;
    }
    std::cout << "]" << std::endl;
}

// Some generic namespace, with a simple function we could use to test the creation of the endpoints.
namespace Generic {
    // practic dirijeaza pornirea serverului
    void handleReady(const Rest::Request &, Http::ResponseWriter response) {
        response.send(Http::Code::Ok, "1");
    }

}

class EspressorEndPoint {

public:
    explicit EspressorEndPoint(Address address)
            : httpEndpoint(std::make_shared<Http::Endpoint>(address)) {}

    void init(size_t thr = 2) {
        auto opts = Http::Endpoint::options().threads(static_cast<int>(thr));
        httpEndpoint->init(opts);
        // Server routes are loaded up
        setupRoutes();
    }

    void start() {
        httpEndpoint->setHandler(router.handler());
        httpEndpoint->serveThreaded();
    }

    void stop() {
        httpEndpoint->shutdown();
    }

private:
    void setupRoutes() {
        using namespace Rest;
        Routes::Get(router, "/ready", Routes::bind(&Generic::handleReady));
        Routes::Get(router, "/auth", Routes::bind(&EspressorEndPoint::doAuth, this));

        /// sugar/size
        Routes::Post(router, "/settings/:settingName/:value", Routes::bind(&EspressorEndPoint::setSetting, this));
        Routes::Get(router, "/settings/:settingName/", Routes::bind(&EspressorEndPoint::getSetting, this));

        /// type/americano/
        Routes::Post(router, "/type/:typeName/", Routes::bind(&EspressorEndPoint::setType, this));
        Routes::Get(router, "/type/", Routes::bind(&EspressorEndPoint::getType, this));
    }

    void doAuth(const Rest::Request &request, Http::ResponseWriter response) {
        // Function that prints cookies
        printCookies(request);
        // In the response object, it adds a cookie regarding the communications language.
        response.cookies().add(Http::Cookie("lang", "en-US"));
        // Send the response
        response.send(Http::Code::Ok);
    }

    /// setting
    // Endpoint to configure one of the Espressor's settings.
    void setSetting(const Rest::Request &request, Http::ResponseWriter response) {
        auto settingName = request.param(":settingName").as<std::string>();

        if (settingName != "sugar" || settingName != "size") {
            response.send(Http::Code::Not_Found, "parameter is not valid!");
            return;
        }

        // This is a guard that prevents editing the same value by two concurent threads.
        Guard guard(espressorLock);

        int value;
        if (request.hasParam(":value")) {
            auto val = request.param(":value");
            value = val.as<int>();
        }

        // Setting the microwave's setting to value
        int setResponse;

        if (settingName == "sugar")
            setResponse = espr.setSugar(value);
        else if (settingName == "size")
            setResponse = espr.setSize(value);

        /// convertim value inainte
        std::string value_str = std::to_string(value);
        // Sending some confirmation or error response.
        if (setResponse == 1) {
            response.send(Http::Code::Ok, settingName + " was set to " + value_str);
        } else {
            response.send(Http::Code::Not_Found,
                          settingName + " was not found and or '" + value_str + "' was not a valid value ");
        }
    }

    // Endpoint to get one of the Espressor's settings.
    void getSetting(const Rest::Request &request, Http::ResponseWriter response) {
        auto settingName = request.param(":settingName").as<std::string>();

        if (settingName != "sugar" || settingName != "size") {
            response.send(Http::Code::Not_Found, "parameter is not valid!");
            return;
        }

        /// verificam optiunea
        int getResponse;
        if (settingName == "sugar")
            getResponse = espr.getSugar();
        else
            getResponse = espr.getSize();

        /// blocam
        Guard guard(espressorLock);
        std::string getResponseStr = std::to_string(getResponse); /// ptr a-l putea afisa trb sa fie string

        if (getResponse != -1) {
            response.send(Http::Code::Ok, settingName + " is " + getResponseStr);
        } else {
            response.send(Http::Code::Not_Found, settingName + " was not found!");
        }
    }

    /// type/americano
    void setType(const Rest::Request &request, Http::ResponseWriter response) {
        auto typeName = request.param(":typeName").as<std::string>();

        Guard guard(espressorLock);
        int value;

        if (typeName == "") {
            response.send(Http::Code::Not_Found, "type is not valid!");
            return;
        }

        int setResponse = espr.setType(typeName);

        if (setResponse == 0) {
            response.send(Http::Code::Not_Found, "value is not valid!");
        } else {
            response.send(Http::Code::Ok, typeName + " was set!");
        }

    }

    void getType(const Rest::Request &request, Http::ResponseWriter response) {
        auto typeName = request.param(":typeName").as<std::string>();

        Guard guard(espressorLock);
        string getResponse = espr.getType();

        if (getResponse == "") {
            response.send(Http::Code::Not_Found, "coffee type is not valid!");
            return;
        } else {
            response.send(Http::Code::Ok, "Selected coffee: " + typeName);
        }

    }

    class Espressor {
    public:
        explicit Espressor() {}

        /// sugar
        int setSugar(int value) {
            if (value < 1 || value > 5)
                return 0;
            sugarSetting.value = value;
            return 1;
        }

        int getSugar() {
            return sugarSetting.value;
        }

        /// size
        int setSize(int value) {
            if (value < 1 || value > 3)
                return 0;
            sizeSetting.value = value;
            return 1;
        }

        int getSize() {
            return sizeSetting.value;
        }

        /// type
        int setType(string typeName) {
            //americano, cappuccino, latte_machiato, mocha

            if (typeName == "americano") {
                coffeeType = americano;
                return 1;
            }

            if (typeName == "cappuccino") {
                coffeeType = cappuccino;
                return 1;
            }

            if (typeName == "latte_machiato") {
                coffeeType = latte_machiato;
                return 1;
            }

            if (typeName == "mocha") {
                coffeeType = mocha;
                return 1;
            }
            return 0;
        }

        string getType() {
            switch (coffeeType) {
                case americano:
                    return "americano";
                case cappuccino:
                    return "cappuccino";
                case latte_machiato:
                    return "latte_machiato";
                case mocha:
                    return "mocha";
                default:
                    return "";
            }
        }

    private:
        /*consideram deocamdata ca esspressorul are trei setari:
            - setarea pentru nivelul de zahar: sugarSetting, cu valori intregi in intervalul [1, 5]
            - setarea pentru marimea cafelei: sizeSetting, cu valori intregi in intervalul [1, 3]
            -setarea tipului de cafea: [americano, cappuccino, latte machiato, mocha]
        */
        /// definirea setarilor
        struct setting {
            string name;
            int value;
        } sugarSetting, sizeSetting;

        enum type {
            americano = 1, cappuccino = 2, latte_machiato = 3, mocha = 4
        } coffeeType;


    };

    /// blocam celelalte optiuni
    using Lock = std::mutex;
    using Guard = std::lock_guard<Lock>;
    Lock espressorLock;

    // Instance of the microwave model
    Espressor espr;

    std::shared_ptr<Http::Endpoint> httpEndpoint;
    Rest::Router router;
};


int main(int argc, char *argv[]) {
    // This code is needed for gracefull shutdown of the server when no longer needed.
    sigset_t signals;
    if (sigemptyset(&signals) != 0
        || sigaddset(&signals, SIGTERM) != 0
        || sigaddset(&signals, SIGINT) != 0
        || sigaddset(&signals, SIGHUP) != 0
        || pthread_sigmask(SIG_BLOCK, &signals, nullptr) != 0) {
        perror("install signal handler failed");
        return 1;
    }

    // Set a port on which your server to communicate
    Port port(9080);

    // Number of threads used by the server
    int thr = 2;

    if (argc >= 2) {
        port = static_cast<uint16_t>(std::stol(argv[1]));

        if (argc == 3)
            thr = std::stoi(argv[2]);
    }

    Address addr(Ipv4::any(), port);

    cout << "Cores = " << hardware_concurrency() << endl;
    cout << "Using " << thr << " threads" << endl;

    // Instance of the class that defines what the server can do.
    EspressorEndPoint stats(addr);

    // Initialize and start the server
    stats.init(thr);
    stats.start();


    // Code that waits for the shutdown sinal for the server
    int signal = 0;
    int status = sigwait(&signals, &signal);
    if (status == 0)
    {
        std::cout << "received signal " << signal << std::endl;
    }
    else
    {
        std::cerr << "sigwait returns " << status << std::endl;
    }

    stats.stop();
}