#include "httplib.h"
#include "KaengGame.h"
#include <iostream>
#include <sstream>
#include <vector>
#include <string>

// สร้างตัวแปรห้องเกมแค่ตัวเดียว ใช้ร่วมกันทั้งเซิร์ฟเวอร์
KaengGame game;

int main() {
    httplib::Server svr;
    
    // ชี้ไปที่โฟลเดอร์ปัจจุบัน เพื่อให้โหลด index.html ได้
    svr.set_mount_point("/", "./");

    // 1. เข้าห้อง
    svr.Get(R"(/api/login/(.*))", [&](const httplib::Request& req, httplib::Response& res) {
        std::string user = req.matches[1];
        game.join(user); 
        res.set_content(game.getJSON(user), "application/json");
    });

    // 2. ตั้งค่าห้อง (โดยหัวหน้าห้อง)
    svr.Get(R"(/api/setup_room/(.*)/(\d+)/(\d+)/(\d+)/(\d+)/(\d+))", [&](const httplib::Request& req, httplib::Response& res) {
        std::string user = req.matches[1];
        if (user == game.hostName) {

            game.configureRoom(
                std::stoi(req.matches[2]), //  1: Starting Chips
                std::stoi(req.matches[3]), //  2: Bet
                std::stoi(req.matches[4]), //  3: ค่าไหล
                std::stoi(req.matches[5]), //  4: ค่าปรับเวลาหมด
                std::stoi(req.matches[6])  //  5: Dealer ID
            );
        }
        res.set_content(game.getJSON(user), "application/json");
    });
    
    // ยุบโต๊ะ/รีเกม
    svr.Get(R"(/api/reset/(.*))", [&](const httplib::Request& req, httplib::Response& res) {
        if (req.matches[1] == game.hostName) {
            game.resetAll();
        }
        res.set_content(game.getJSON(req.matches[1]), "application/json");
    });

    //ดึงข้อมูล/สถานะเกมให้หน้าเว็บอัปเดตแบบ Real-tim
    svr.Get(R"(/api/state/(.*))", [&](const httplib::Request& req, httplib::Response& res) {
        res.set_content(game.getJSON(req.matches[1]), "application/json");
    });

    //เริ่มเกม
    svr.Get(R"(/api/start/(.*))", [&](const auto& req, auto& res) { 
        game.start(req.matches[1]); 
        res.set_content(game.getJSON(req.matches[1]), "application/json"); 
    });
    
    // จั่วไพ่
    svr.Get(R"(/api/draw/(.*))", [&](const auto& req, auto& res) { 
        game.draw(req.matches[1]); 
        res.set_content(game.getJSON(req.matches[1]), "application/json"); 
    });
    
    //แคง
    svr.Get(R"(/api/kaeng/(.*))", [&](const auto& req, auto& res) { 
        game.callKaeng(req.matches[1]); 
        res.set_content(game.getJSON(req.matches[1]), "application/json"); 
    });
    
    //ทิ้งไพ่
    svr.Get(R"(/api/discard/(.*)/(.*))", [&](const httplib::Request& req, httplib::Response& res) {
        std::string user = req.matches[1]; 
        std::string cardsStr = req.matches[2];
        
        std::stringstream ss(cardsStr); 
        std::vector<int> idx; 
        std::string item;
        while (std::getline(ss, item, ',')) {
            idx.push_back(std::stoi(item));
        }
        
        game.discard(user, idx);
        res.set_content(game.getJSON(user), "application/json");
    });

    std::cout << ">>> Kaeng API Server is Running! <<<\n";
    std::cout << "Listening on http://0.0.0.0:18080\n";
    
    // รันเซิร์ฟเวอร์
    svr.listen("0.0.0.0", 18080);
}