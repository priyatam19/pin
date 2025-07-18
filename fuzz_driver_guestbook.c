#include <stdio.h>
#include <stdlib.h>
#include <string>
#include "app/src/db.hpp"
#include "app/src/tpl/filter.hpp"
#include "crow/http_request.h"
#include "crow/query_string.h"

// Database setup with a dummy row
Db* initialize_db() {
    Db* db = new Db();
    if (!db->db) {
        fprintf(stderr, "Cannot open database\n");
        delete db;
        return nullptr;
    }

    // Insert a dummy row so the query has something to work with
    DbStatement stmt = db->prepare("INSERT INTO guests (name, email, message) VALUES (?, ?, ?)");
    if (!stmt.stmt) {
        fprintf(stderr, "Cannot prepare insert statement\n");
        delete db;
        return nullptr;
    }

    sqlite3_bind_text(stmt.stmt, 1, "Dummy", 5, SQLITE_STATIC);
    sqlite3_bind_text(stmt.stmt, 2, "dummy@example.com", 17, SQLITE_STATIC);
    sqlite3_bind_text(stmt.stmt, 3, "Dummy message", 13, SQLITE_STATIC);

    if (sqlite3_step(stmt.stmt) != SQLITE_DONE) {
        fprintf(stderr, "Insert failed: %s\n", sqlite3_errmsg(db->db));
        delete db;
        return nullptr;
    }

    return db;
}

// Simulate the /filter route
void process_filter_route(Db* db, const char* input, size_t input_len) {
    // Fixed where param to match the dummy row, fuzzed order_by param
    std::string query_str = "where[name]=Dummy"; // Match the inserted row
    if (input_len > 0) {
        query_str += "&order_by=";
        query_str.append(input, input_len);
    }

    printf("Query string: %s\n", query_str.c_str());

    // Mock the request
    crow::request req;
    req.url_params = crow::query_string(query_str, false);

    // Let filter.cpp process the request
    tpl::Filter filter(*db, req.url_params.get_dict("where"), req.url_params.get_dict("order_by"));
    crow::response resp = filter.render();

    printf("Response code: %d\n", resp.code);
    printf("Response body: %s\n", resp.body.c_str());
}

int main(int argc, char** argv) {
    printf("Testing guestbook v0.8.1\n");

    // Set up the database
    Db* db = initialize_db();
    if (!db) {
        return 1;
    }

    // Buffer for AFL++ input
    char input[4096];
    size_t input_size = fread(input, 1, sizeof(input), stdin);
    if (input_size <= 0) {
        printf("No input from AFL++\n");
        delete db;
        return 1;
    }

    // Feed the entire fuzzed input to order_by
    process_filter_route(db, input, input_size);

    delete db;
    return 0;
}