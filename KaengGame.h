// KaengGame.h
#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <random>
#include <sstream>
#include <chrono>
#include <memory>

enum class Suit { SPADES, HEARTS, DIAMONDS, CLUBS };
enum class Rank { ACE = 1, TWO = 2, THREE = 3, FOUR = 4, FIVE = 5, SIX = 6, SEVEN = 7, EIGHT = 8, NINE = 9, TEN = 10, JACK = 11, QUEEN = 12, KING = 13 };

class Card {
public:
    Rank rank; Suit suit;
    Card(Rank r, Suit s) : rank(r), suit(s) {}
    int getValue() const { return (rank >= Rank::TEN) ? 10 : static_cast<int>(rank); }
    std::string toString() const {
        std::string rStr, sStr;
        if (rank == Rank::TEN) rStr = "T"; else if (rank == Rank::JACK) rStr = "J";
        else if (rank == Rank::QUEEN) rStr = "Q"; else if (rank == Rank::KING) rStr = "K";
        else if (rank == Rank::ACE) rStr = "A"; else rStr = std::to_string(static_cast<int>(rank));
        if (suit == Suit::SPADES) sStr = "♠️"; else if (suit == Suit::HEARTS) sStr = "💗";
        else if (suit == Suit::DIAMONDS) sStr = "♦️"; else sStr = "♣️";
        return sStr + rStr;
    }
};

class Player {
public:
    std::string name;
    std::vector<Card> hand;
    int chips = 0; 
    std::string lastAction = "รอเริ่มเกม";
    std::chrono::time_point<std::chrono::system_clock> lastSeen; // เก็บเวลาติดต่อล่าสุด

    Player(std::string n) : name(n) {
        lastSeen = std::chrono::system_clock::now(); // ตั้งค่าเวลาเริ่มต้นตอนเข้าห้อง
    }
    int getScore() const { int s=0; for(auto& c:hand) s+=c.getValue(); return s; }
    
    std::string toJSON(bool reveal, const std::string& reqUser) const {
        std::string j = "{\"name\":\"" + name + "\",\"chips\":" + std::to_string(chips) + ",";
        bool canSee = reveal || (name == reqUser);
        j += "\"score\":" + std::to_string(canSee ? getScore() : 0) + ",";
        j += "\"last_action\":\"" + lastAction + "\",\"cards\":[";
        for (size_t i=0; i<hand.size(); ++i) {
            j += "\"" + (canSee ? hand[i].toString() : "คว่ำ") + "\"" + (i < hand.size()-1 ? "," : "");
        }
        return j + "]}";
    }
};

// --- Dealer System ---
class Dealer {
protected:
    std::string name;
    int id;
public:
    Dealer(int id, std::string name) : id(id), name(name) {}
    virtual ~Dealer() = default;
    std::string getName() const { return name; }
    int getId() const { return id; }
    virtual Card drawCard(std::vector<Card>& deck) {
        Card c = deck.back(); 
        deck.pop_back(); 
        return c; 
    }
};

class RandomDealer : public Dealer {
public:
    RandomDealer(int id, std::string name) : Dealer(id, name) {}
};

class HighCardDealer : public Dealer {
public:
    HighCardDealer(int id, std::string name) : Dealer(id, name) {}
    Card drawCard(std::vector<Card>& deck) override {
        int r = rand() % 100;
        if (r < 70) { 
            for (auto it = deck.begin(); it != deck.end(); ++it) {
                if (it->rank >= Rank::TEN) {
                    Card c = *it; deck.erase(it); return c;
                }
            }
        }
        Card c = deck.back(); deck.pop_back(); return c;
    }
};

class LowCardDealer : public Dealer {
public:
    LowCardDealer(int id, std::string name) : Dealer(id, name) {}
    Card drawCard(std::vector<Card>& deck) override {
        int r = rand() % 100;
        if (r < 70) { 
            for (auto it = deck.begin(); it != deck.end(); ++it) {
                if (it->rank <= Rank::FIVE) {
                    Card c = *it; deck.erase(it); return c;
                }
            }
        }
        Card c = deck.back(); deck.pop_back(); return c;
    }
};

// --- Main Game Logic ---
class KaengGame {
public:
    std::vector<Player> players;
    std::vector<Card> deck, discardPile;
    std::vector<std::string> gameLog;
    std::shared_ptr<Dealer> currentDealer;
    
    int currentTurn = 0, pot = 0;
    bool isConfigured = false;
    int startingChips = 1000, betPerGame = 50, flowFee = 20, penaltyFee = 30;
    int flowPayerIdx = -1; 
    bool hasDrawn = false;
    std::string status = "waiting", message = "รอหัวหน้าห้องตั้งค่าโต๊ะ...";
    std::string hostName = "";
    int roundCount = 0;

    std::chrono::time_point<std::chrono::system_clock> turnEndTime;
    int turnDuration = 20;

    KaengGame() {
        currentDealer = std::make_shared<RandomDealer>(7, "Puppies");
    }

    void addLog(std::string text) {
        gameLog.push_back(text);
        if(gameLog.size() > 12) gameLog.erase(gameLog.begin());
    }

    void resetAll() {
        players.clear(); deck.clear(); discardPile.clear(); gameLog.clear();
        currentTurn = 0; pot = 0; isConfigured = false; status = "waiting";
        message = "ห้องถูกยุบแล้ว รอหัวหน้าห้องตั้งค่าใหม่...";
        hostName = ""; roundCount = 0; flowPayerIdx = -1;
        addLog("--- ระบบถูกยุบโต๊ะโดยหัวหน้าห้อง ---");
    }

    void join(std::string name) {
        for (auto& p : players) if (p.name == name) return;
        if (status == "waiting" && players.size() < 5) {
            if (players.empty()) { hostName = name; isConfigured = false; }
            Player newP(name);
            newP.chips = isConfigured ? startingChips : 0; 
            players.push_back(newP);
            message = name + " เข้าร่วมโต๊ะ!";
        }
    }

    void setDealer(int type) {
        switch(type) {
            case 1: currentDealer = std::make_shared<HighCardDealer>(1, "AJ. Tee"); break;
            case 2: currentDealer = std::make_shared<RandomDealer>(2, "AJ. Ton"); break;
            case 3: currentDealer = std::make_shared<LowCardDealer>(3, "AJ. Blinkemon"); break;
            case 4: currentDealer = std::make_shared<HighCardDealer>(4, "SIA NICEZAZA"); break;
            case 5: currentDealer = std::make_shared<RandomDealer>(5, "SingTohhh"); break;
            case 6: currentDealer = std::make_shared<HighCardDealer>(6, "Illectrica"); break;
            case 7: currentDealer = std::make_shared<RandomDealer>(7, "Puppies"); break;
            case 8: currentDealer = std::make_shared<LowCardDealer>(8, "AshiLnwZA"); break;
        }
    }

    void configureRoom(int chips, int bet, int flow, int pen, int dealerType) {
        startingChips = chips; betPerGame = bet; flowFee = flow; penaltyFee = pen; 
        setDealer(dealerType); 
        isConfigured = true;
        for (auto& p : players) p.chips = startingChips;
        message = "ตั้งค่าห้องสำเร็จ รอคนอื่นพร้อมแล้วกดเริ่มเกม";
        addLog("⚙️ อัปเดตกติกาเกมโดย " + hostName);
    }

    void initDeck() {
        deck.clear();
        for (int s=0; s<4; ++s) for (int r=1; r<=13; ++r) deck.push_back(Card(static_cast<Rank>(r), static_cast<Suit>(s)));
        std::random_device rd; std::mt19937 g(rd()); std::shuffle(deck.begin(), deck.end(), g);
    }

    Card drawCard() { 
        if (deck.empty()) initDeck(); 
        return currentDealer->drawCard(deck); 
    }

    void resetTimer() { turnEndTime = std::chrono::system_clock::now() + std::chrono::seconds(turnDuration); }

    void start(std::string reqUser) {
        if (status != "waiting" && status != "game_over") return;
        if (reqUser != hostName || !isConfigured) return;
        if (players.size() < 2) { message = "ต้องมีอย่างน้อย 2 คน"; return; }
        
        roundCount++;
        initDeck(); discardPile.clear();
        pot = players.size() * betPerGame;
        for (auto& p : players) { p.chips -= betPerGame; p.hand.clear(); p.lastAction = ""; }
        for (int i=0; i<5; ++i) for (auto& p : players) p.hand.push_back(drawCard());
        discardPile.push_back(drawCard());
        
        status = "playing"; currentTurn = 0; hasDrawn = false; flowPayerIdx = -1;
        message = "เริ่มตาที่ " + std::to_string(roundCount) + "! ตาของ " + players[currentTurn].name;
        addLog("=====================");
        addLog("🃏 เริ่มเกมตาที่ " + std::to_string(roundCount) + " (แจกโดย " + currentDealer->getName() + ")");
        resetTimer();
    }

    void draw(std::string reqUser) {
        if (status != "playing" || hasDrawn || players[currentTurn].name != reqUser) return;
        players[currentTurn].hand.push_back(drawCard());
        hasDrawn = true;
        players[currentTurn].lastAction = "จั่วไพ่";
        message = reqUser + " จั่วไพ่";
        addLog("📥 " + message);
    }

    void discard(std::string reqUser, std::vector<int> indices) {
        if (status != "playing" || players[currentTurn].name != reqUser || indices.empty()) return;
        Rank firstRank = players[currentTurn].hand[indices[0]].rank;
        for (int idx : indices) if (players[currentTurn].hand[idx].rank != firstRank) return;

        bool isFlow = false;
        int numCards = indices.size();

        if (!hasDrawn && !discardPile.empty()) {
            if (firstRank == discardPile.back().rank) {
                isFlow = true;
                if (flowPayerIdx != -1 && flowPayerIdx != currentTurn) {
                    int totalFlow = flowFee * numCards;
                    players[flowPayerIdx].chips -= totalFlow;
                    players[currentTurn].chips += totalFlow;
                    message = reqUser + " ไหล " + std::to_string(numCards) + " ใบ! ได้เงินจาก " + players[flowPayerIdx].name;
                    addLog("🌊 " + message);
                } else { addLog("🌊 " + reqUser + " ไหลไพ่ " + std::to_string(numCards) + " ใบ"); }
            } else return;
        }

        std::sort(indices.rbegin(), indices.rend());
        for (int idx : indices) {
            discardPile.push_back(players[currentTurn].hand[idx]);
            players[currentTurn].hand.erase(players[currentTurn].hand.begin() + idx);
        }
        
        if (!isFlow) {
            flowPayerIdx = currentTurn; 
            message = reqUser + " ทิ้งไพ่ " + std::to_string(numCards) + " ใบ";
            addLog("📤 " + message);
        }
        
        players[currentTurn].lastAction = isFlow ? "ไหลไพ่" : "ทิ้งไพ่";
        hasDrawn = false; currentTurn = (currentTurn + 1) % players.size(); resetTimer();
    }

    void callKaeng(std::string reqUser) {
        if (status != "playing" || hasDrawn || players[currentTurn].name != reqUser) return;
        status = "game_over";
        int minScore = players[currentTurn].getScore(); 
        
        std::vector<int> winnersIdx;
        for (size_t i = 0; i < players.size(); ++i) {
            if (players[i].getScore() < minScore) {
                minScore = players[i].getScore();
                winnersIdx.clear(); winnersIdx.push_back(i);
            } else if (players[i].getScore() == minScore) {
                winnersIdx.push_back(i);
            }
        }

        bool callerWins = false;
        for(int idx : winnersIdx) if(idx == currentTurn) callerWins = true;

        if (callerWins) {
            int splitPot = pot / winnersIdx.size();
            for(int idx : winnersIdx) players[idx].chips += splitPot;
            message = reqUser + " แคงสำเร็จ! ชนะด้วยแต้ม " + std::to_string(minScore);
            addLog("🏆 " + reqUser + " แคงชนะ (" + std::to_string(minScore) + " แต้ม)");
        } else {
            int totalPenalty = 0;
            for (size_t i = 0; i < players.size(); ++i) {
                if (i != currentTurn) {
                    players[currentTurn].chips -= betPerGame;
                    players[i].chips += betPerGame;
                    totalPenalty += betPerGame;
                }
            }
            int splitPot = pot / winnersIdx.size();
            for(int idx : winnersIdx) players[idx].chips += splitPot;
            message = reqUser + " โดนแหก! จ่ายรอบวงรวม " + std::to_string(totalPenalty);
            addLog("💥 " + reqUser + " โดนแหก! จ่ายรอบวง");
        }
        pot = 0;

        std::string kickedMsg = "";
        for (auto it = players.begin(); it != players.end(); ) {
            if (it->chips <= 0) { kickedMsg += it->name + " "; it = players.erase(it); }
            else { ++it; }
        }

        if (!kickedMsg.empty()) {
            addLog("💸 " + kickedMsg + "หมดตัว! ถูกเตะออกจากโต๊ะ");
            bool hostExists = false;
            for(auto& p : players) if(p.name == hostName) hostExists = true;
            if(!hostExists && !players.empty()) hostName = players[0].name;
        }
    }

    void checkTimeout() {
        if (status != "playing") return;
        auto now = std::chrono::system_clock::now();
        if (now >= turnEndTime) {
            players[currentTurn].chips -= penaltyFee;
            pot += penaltyFee;
            message = players[currentTurn].name + " หมดเวลา! โดนปรับเข้ากองกลาง";
            addLog("⏰ " + message);

            if (!hasDrawn) players[currentTurn].hand.push_back(drawCard());
            
            discardPile.push_back(players[currentTurn].hand.back());
            players[currentTurn].hand.pop_back();
            players[currentTurn].lastAction = "หมดเวลา";

            flowPayerIdx = currentTurn; 
            hasDrawn = false; currentTurn = (currentTurn + 1) % players.size(); resetTimer();
        }
    }

    // ฟังก์ชันจัดการคนออฟไลน์
    void cleanupInactivePlayers() {
        auto now = std::chrono::system_clock::now();
        bool playerRemoved = false;
        
        for (auto it = players.begin(); it != players.end(); ) {
            if (std::chrono::duration_cast<std::chrono::seconds>(now - it->lastSeen).count() > 10) {
                addLog("🔌 " + it->name + " ขาดการเชื่อมต่อ (เตะออก)");
                it = players.erase(it); 
                playerRemoved = true;
            } else {
                ++it;
            }
        }

        if (playerRemoved) {
            if (players.empty()) {
                resetAll(); 
            } else {
                if (currentTurn >= players.size()) currentTurn = 0;
                
                bool hostExists = false;
                for(auto& p : players) if(p.name == hostName) hostExists = true;
                if(!hostExists && !players.empty()) hostName = players[0].name;

                if (status == "playing" && players.size() < 2) {
                    status = "waiting";
                    message = "คนออกกลางคัน ผู้เล่นไม่พอ ยกเลิกเกม!";
                    addLog("🛑 ยกเลิกเกม (ผู้เล่นไม่พอ)");
                }
            }
        }
    }

    std::string getJSON(std::string reqUser) {
        // อัปเดตเวลาให้กับคนที่ขอข้อมูล
        for (auto& p : players) {
            if (p.name == reqUser) {
                p.lastSeen = std::chrono::system_clock::now();
                break;
            }
        }

        cleanupInactivePlayers(); // เรียกกวาดล้างคนออฟไลน์
        
        checkTimeout();
        auto now = std::chrono::system_clock::now();
        int timeLeft = std::max(0, (int)std::chrono::duration_cast<std::chrono::seconds>(turnEndTime - now).count());

        std::string j = "{\"status\":\"" + status + "\",\"message\":\"" + message + "\",";
        j += "\"is_configured\":" + std::string(isConfigured ? "true" : "false") + ",";
        j += "\"pot\":" + std::to_string(pot) + ",\"deck_count\":" + std::to_string(deck.size()) + ",";
        j += "\"host\":\"" + hostName + "\",";
        j += "\"dealer_id\":" + std::to_string(currentDealer->getId()) + ",\"dealer_name\":\"" + currentDealer->getName() + "\",";
        j += "\"bet\":" + std::to_string(betPerGame) + ",\"flow\":" + std::to_string(flowFee) + ",\"penalty\":" + std::to_string(penaltyFee) + ",";
        j += "\"current_turn\":" + std::to_string(currentTurn) + ",\"time_left\":" + std::to_string(timeLeft) + ",\"has_drawn\":" + (hasDrawn ? "true" : "false") + ",";
        j += "\"log\":[";
        for (size_t i=0; i<gameLog.size(); ++i) j += "\"" + gameLog[i] + "\"" + (i<gameLog.size()-1 ? "," : "");
        j += "],";
        j += "\"discard_top\":\"" + (discardPile.empty() ? "" : discardPile.back().toString()) + "\",\"players\":[";
        for (size_t i=0; i<players.size(); ++i) j += players[i].toJSON(status=="game_over", reqUser) + (i<players.size()-1 ? "," : "");
        return j + "]}";
    }
};