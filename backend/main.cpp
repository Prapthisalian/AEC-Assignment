#include "crow.h"
#include "crow/middlewares/cors.h"
#include <iostream>
#include <vector>
#include <string>
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include <mutex>
#include <sqlite3.h>

using json = nlohmann::json;

// --- Database Helper ---
sqlite3* db;

void init_db() {
    int rc = sqlite3_open("mental_health.db", &db);
    if (rc) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
        return;
    } else {
        std::cout << "Opened database successfully" << std::endl;
    }

    const char* sql_users = 
        "CREATE TABLE IF NOT EXISTS users ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "name TEXT NOT NULL,"
        "email TEXT UNIQUE NOT NULL,"
        "password_hash TEXT NOT NULL,"
        "created_at DATETIME DEFAULT CURRENT_TIMESTAMP);"
        ;
    
    char* zErrMsg = 0;
    rc = sqlite3_exec(db, sql_users, 0, 0, &zErrMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error (users): " << zErrMsg << std::endl;
        sqlite3_free(zErrMsg);
    }
    
    const char* sql_diary = 
        "CREATE TABLE IF NOT EXISTS diary_entries ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "user_id INTEGER NOT NULL,"
        "date TEXT NOT NULL,"
        "diary_text TEXT NOT NULL,"
        "mood_score INTEGER,"
        "ai_response_json TEXT,"
        "FOREIGN KEY(user_id) REFERENCES users(id));";

    rc = sqlite3_exec(db, sql_diary, 0, 0, &zErrMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error (diary): " << zErrMsg << std::endl;
        sqlite3_free(zErrMsg);
    }
}

// --- Gemini API Helper ---

std::string call_gemini_api(const std::string& text) {
    // REPLACE WITH YOUR ACTUAL API KEY
    std::string api_key = "AIzaSyARu1HcNU_x-6jjoh1Mqb13MgR8nE3qRKc"; 
    // Using gemini-2.0-flash as confirmed in models list
    std::string url = "https://generativelanguage.googleapis.com/v1beta/models/gemini-2.5-flash:generateContent?key=" + api_key;

    json payload = {
        {"contents", {{
            {"parts", {{
                {"text", "Analyze the mood of the following diary entry and provide a short, supportive, and empathetic response (max 2 sentences). Entry: " + text}
            }}}
        }}}
    };

    cpr::Response r = cpr::Post(cpr::Url{url},
                                cpr::Header{{"Content-Type", "application/json"}},
                                cpr::Body{payload.dump()});
                                std::cout << "Response: " << r.text << std::endl;

    if (r.status_code == 200) {
        try {
            auto response_json = json::parse(r.text);
            return response_json["candidates"][0]["content"]["parts"][0]["text"];
        } catch (...) {
            return "Error parsing AI response.";
        }
    } else if (r.status_code == 503) {
        return "The AI model is currently overloaded. Please try again in a moment.";
    } else {
        return "Error calling Gemini API: " + std::to_string(r.status_code);
    }
}

// --- Main ---

int main() {
    init_db();

    crow::App<crow::CORSHandler> app;

    // Enable CORS
    auto& cors = app.get_middleware<crow::CORSHandler>();
    cors.global()
        .headers("Content-Type", "Authorization")
        .methods("POST"_method, "GET"_method)
        .origin("*");

// --- Routes ---

    // 1. POST /signup
    CROW_ROUTE(app, "/signup").methods("POST"_method)([](const crow::request& req) {
        auto x = json::parse(req.body);
        std::string name = x["name"];
        std::string email = x["email"];
        std::string password = x["password"];
        
        // Store password directly (not recommended for production)
        std::string sql = "INSERT INTO users (name, email, password_hash) VALUES ('" + name + "', '" + email + "', '" + password + "');";
        
        char* zErrMsg = 0;
        int rc = sqlite3_exec(db, sql.c_str(), 0, 0, &zErrMsg);
        if (rc != SQLITE_OK) {
            std::string err = zErrMsg;
            sqlite3_free(zErrMsg);
            return crow::response(400, "Error creating user: " + err);
        }
        
        return crow::response(200, "User created");
    });

    // 2. POST /login
    CROW_ROUTE(app, "/login").methods("POST"_method)([](const crow::request& req) {
        auto x = json::parse(req.body);
        std::string email = x["email"];
        std::string password = x["password"];
        
        // Check password directly (not recommended for production)
        std::string sql = "SELECT id, name, email FROM users WHERE email='" + email + "' AND password_hash='" + password + "';";
        
        sqlite3_stmt* stmt;
        int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0);
        
        if (rc != SQLITE_OK) {
            return crow::response(500, "Database error");
        }
        
        rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {
            int user_id = sqlite3_column_int(stmt, 0);
            std::string name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            std::string user_email = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            
            json resp;
            resp["user_id"] = user_id;
            resp["name"] = name;
            resp["email"] = user_email;
            resp["message"] = "Login successful";
            
            sqlite3_finalize(stmt);
            return crow::response(resp.dump());
        } else {
            sqlite3_finalize(stmt);
            return crow::response(401, "Invalid credentials");
        }
    });

    // 3. POST /diary
    CROW_ROUTE(app, "/diary").methods("POST"_method)([](const crow::request& req) {
        auto x = json::parse(req.body);
        int user_id = x["user_id"];
        std::string date = x["date"];
        std::string text = x["text"];
        
        // 1. Call Gemini for analysis
        // Update prompt to ask for JSON
        std::string api_key = "AIzaSyARu1HcNU_x-6jjoh1Mqb13MgR8nE3qRKc"; 
        std::string url = "https://generativelanguage.googleapis.com/v1beta/models/gemini-2.5-flash:generateContent?key=" + api_key;

        json payload = {
            {"contents", {{
                {"parts", {{
                    {"text", "Analyze the diary entry. Return a JSON object with two fields: 'score' (an integer from 1 to 10 where 1 is extremely negative/depressed and 10 is extremely positive/happy) and 'response' (a short, empathetic text response). Entry: " + text}
                }}}
            }}},
            {"generationConfig", {
                {"responseMimeType", "application/json"}
            }}
        };

        cpr::Response r = cpr::Post(cpr::Url{url},
                                    cpr::Header{{"Content-Type", "application/json"}},
                                    cpr::Body{payload.dump()});
        
        int mood_score = 5;
        std::string ai_text = "Thank you for sharing.";

        if (r.status_code == 200) {
            try {
                auto response_json = json::parse(r.text);
                std::string raw_text = response_json["candidates"][0]["content"]["parts"][0]["text"];
                
                auto ai_data = json::parse(raw_text);
                if (ai_data.contains("score")) mood_score = ai_data["score"];
                if (ai_data.contains("response")) ai_text = ai_data["response"];
                
            } catch (std::exception& e) {
                std::cout << "Error parsing AI response: " << e.what() << std::endl;
            }
        }
        
        // 2. Save to DB
        const char* sql = "INSERT INTO diary_entries (user_id, date, diary_text, mood_score, ai_response_json) VALUES (?, ?, ?, ?, ?);";
        sqlite3_stmt* stmt;
        sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
        
        sqlite3_bind_int(stmt, 1, user_id);
        sqlite3_bind_text(stmt, 2, date.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, text.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 4, mood_score); 
        sqlite3_bind_text(stmt, 5, ai_text.c_str(), -1, SQLITE_STATIC);
        
        int rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        
        if (rc != SQLITE_DONE) {
             return crow::response(500, "Error saving entry");
        }
        
        json result = {{"analysis", ai_text}, {"score", mood_score}};
        return crow::response(result.dump());
    });
    
    CROW_ROUTE(app, "/test")([]() {
        return "Backend is working!";
    });

    // 4. GET /diary/history
    CROW_ROUTE(app, "/diary/history")([](const crow::request& req) {
        char* user_id_str = req.url_params.get("user_id");
        if (!user_id_str) return crow::response(400, "Missing user_id");
        
        std::string sql = "SELECT id, date, diary_text, mood_score, ai_response_json FROM diary_entries WHERE user_id=" + std::string(user_id_str) + " ORDER BY id DESC;";
        
        sqlite3_stmt* stmt;
        int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0);
        
        json history = json::array();
        
        while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
            int entry_id = sqlite3_column_int(stmt, 0);
            std::string date = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            std::string text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            int score = sqlite3_column_int(stmt, 3);
            std::string ai = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
            
            history.push_back({
                {"id", entry_id},
                {"date", date},
                {"text", text},
                {"score", score},
                {"analysis", ai}
            });
        }
        
        sqlite3_finalize(stmt);
        return crow::response(history.dump());
    });

    // DELETE /diary/entry - Delete a diary entry
    CROW_ROUTE(app, "/diary/entry").methods("DELETE"_method)([](const crow::request& req) {
        auto x = json::parse(req.body);
        
        if (!x.contains("entry_id") || !x.contains("user_id")) {
            return crow::response(400, "Missing entry_id or user_id");
        }
        
        int entry_id = x["entry_id"];
        int user_id = x["user_id"];
        
        // Verify the entry belongs to the user before deleting
        std::string verify_sql = "SELECT user_id FROM diary_entries WHERE id=?;";
        sqlite3_stmt* stmt;
        int rc = sqlite3_prepare_v2(db, verify_sql.c_str(), -1, &stmt, 0);
        sqlite3_bind_int(stmt, 1, entry_id);
        
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            int owner_id = sqlite3_column_int(stmt, 0);
            sqlite3_finalize(stmt);
            
            if (owner_id != user_id) {
                return crow::response(403, "Not authorized to delete this entry");
            }
        } else {
            sqlite3_finalize(stmt);
            return crow::response(404, "Entry not found");
        }
        
        // Delete the entry
        std::string delete_sql = "DELETE FROM diary_entries WHERE id=?;";
        rc = sqlite3_prepare_v2(db, delete_sql.c_str(), -1, &stmt, 0);
        sqlite3_bind_int(stmt, 1, entry_id);
        
        rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        
        if (rc == SQLITE_DONE) {
            return crow::response(200, "Entry deleted successfully");
        } else {
            return crow::response(500, "Error deleting entry");
        }
    });

    // 5. GET /user/stats - Calculate streak and total entries
    CROW_ROUTE(app, "/user/stats")([](const crow::request& req) {
        char* user_id_str = req.url_params.get("user_id");
        if (!user_id_str) return crow::response(400, "Missing user_id");
        
        int user_id = std::atoi(user_id_str);
        
        // Get all unique dates for this user, sorted descending
        std::string sql = "SELECT DISTINCT date FROM diary_entries WHERE user_id=? ORDER BY date DESC;";
        sqlite3_stmt* stmt;
        int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0);
        sqlite3_bind_int(stmt, 1, user_id);
        
        std::vector<std::string> dates;
        while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
            std::string date = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            dates.push_back(date);
        }
        sqlite3_finalize(stmt);
        
        // Calculate total entries
        int total_entries = 0;
        std::string count_sql = "SELECT COUNT(*) FROM diary_entries WHERE user_id=?;";
        rc = sqlite3_prepare_v2(db, count_sql.c_str(), -1, &stmt, 0);
        sqlite3_bind_int(stmt, 1, user_id);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            total_entries = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
        
        // Calculate streak
        int streak = 0;
        if (dates.size() > 0) {
            // Get today's date (YYYY-MM-DD format)
            time_t now = time(0);
            tm* ltm = localtime(&now);
            char today[11];
            snprintf(today, sizeof(today), "%04d-%02d-%02d", 
                     1900 + ltm->tm_year, 1 + ltm->tm_mon, ltm->tm_mday);
            
            // Get yesterday's date
            time_t yesterday_time = now - 86400;
            tm* ytm = localtime(&yesterday_time);
            char yesterday[11];
            snprintf(yesterday, sizeof(yesterday), "%04d-%02d-%02d",
                     1900 + ytm->tm_year, 1 + ytm->tm_mon, ytm->tm_mday);
            
            // Check if latest entry is today or yesterday
            if (dates[0] == std::string(today) || dates[0] == std::string(yesterday)) {
                // Start from the appropriate date
                time_t check_time = (dates[0] == std::string(today)) ? now : yesterday_time;
                
                for (size_t i = 0; i < dates.size(); i++) {
                    tm* ctm = localtime(&check_time);
                    char check_date[11];
                    snprintf(check_date, sizeof(check_date), "%04d-%02d-%02d",
                             1900 + ctm->tm_year, 1 + ctm->tm_mon, ctm->tm_mday);
                    
                    if (dates[i] == std::string(check_date)) {
                        streak++;
                        check_time -= 86400; // Move back one day
                    } else {
                        break;
                    }
                }
            }
        }
        
        json result = {
            {"streak", streak},
            {"total_entries", total_entries}
        };
        
        return crow::response(result.dump());
    });
    // --- Static File Serving ---
    
    CROW_ROUTE(app, "/")([](const crow::request& req, crow::response& res){
        res.set_static_file_info("frontend/index.html");
        res.end();
    });
       CROW_ROUTE(app, "/gets")([](const crow::request& req, crow::response& res){
        sqlite3_stmt* stmt;
        int rc = sqlite3_prepare_v2(db, "SELECT * FROM users", -1, &stmt, 0);
        std::cout << "SELECT * FROM users" << std::endl;
        
        while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
            std::cout << "ID: " << sqlite3_column_int(stmt, 0) << std::endl;
            std::cout << "Name: " << reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)) << std::endl;
            std::cout << "Email: " << reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)) << std::endl;
            std::cout << "Password: " << reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)) << std::endl;
        }
        sqlite3_finalize(stmt);
        res.end();
    });

    CROW_ROUTE(app, "/<string>")([](const crow::request& req, crow::response& res, std::string path){
        if (path.find("..") != std::string::npos) {
            res.code = 403;
            res.end();
            return;
        }
        res.set_static_file_info("frontend/" + path); 
        res.end();
    });

    std::cout << "Server starting..." << std::endl;
    std::cout << "Backend API available at: http://localhost:18080" << std::endl;
    
    app.port(18080).multithreaded().run();
    
    sqlite3_close(db);
}
