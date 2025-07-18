#include <crow.h>
#include <crow/middlewares/cookie_parser.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

#include "db.hpp"
#include "session.hpp"
#include "template_finder.hpp"
#include "util_rand.hpp"
#include "tpl/index.hpp"
#include "tpl/info.hpp"
#include "tpl/filter.hpp"

static const char* SESSION_COOKIE_NAME = "guestbook_session";

/* ------------------------------------------------------------------ */
/*  Helpers                                                           */
/* ------------------------------------------------------------------ */
static std::string percent_encode(const std::string& in)
{
    static const char hex[] = "0123456789ABCDEF";
    std::string out;
    out.reserve(in.size() * 3);
    for (unsigned char c : in) {
        if ((c >= 'A' && c <= 'Z') ||
            (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') ||
            c == '-' || c == '_' || c == '.' || c == '~') {
            out.push_back(c);
        } else {
            out.push_back('%');
            out.push_back(hex[c >> 4]);
            out.push_back(hex[c & 0x0F]);
        }
    }
    return out;
}

static std::string read_stdin()
{
    std::string data;
    constexpr size_t kMaxLen = 8192;
    data.reserve(kMaxLen);
    char buf[1024];
    size_t n;
    while ((n = ::fread(buf, 1, sizeof(buf), stdin)) > 0 && data.size() < kMaxLen)
        data.append(buf, buf + n);
    return data;
}

/*  The real Crow server stores one context object per-middleware per
 *  request.  For fuzzing we don’t care about the actual cookie data,
 *  we just need a *non-null* pointer for CookieParser to cast to.     */
static void attach_cookie_ctx(crow::request& r)
{
    static crow::CookieParser::context dummy{};
    r.middleware_context = &dummy;           // <- one pointer is enough
}

/* ------------------------------------------------------------------ */
/*  App setup                                                         */
/* ------------------------------------------------------------------ */
using GBApp = crow::App<crow::CookieParser>;

static GBApp& get_app(Db& db)
{
    static GBApp app;
    GBApp* const app_ptr = &app;  
    static bool init = false;
    if (init) return app;
    init = true;

    std::string tmpl = template_finder::find_template_path();
    crow::mustache::set_global_base(tmpl);

/* -- Routes -------------------------------------------------------- */

    CROW_ROUTE(app, "/")([&](const crow::request& req) {
        auto& ctx = app.get_context<crow::CookieParser>(req);
        Session sess{ctx.get_cookie(SESSION_COOKIE_NAME)};

        std::string csrf = util_rand::rand_long_word();
        sess.put_field("csrf_token", csrf);
        ctx.set_cookie(SESSION_COOKIE_NAME, sess.serialize()).httponly();

        tpl::Index tpl{db};
        tpl.csrf_token = csrf;
        return tpl.render();
    });

    CROW_ROUTE(app, "/info")([] {
        tpl::Info tpl;
        return tpl.render();
    });

    CROW_ROUTE(app, "/filter")([&](const crow::request& req) {
        tpl::Filter::query_dict where = req.url_params.get_dict("where");
        tpl::Filter::query_dict order_by = req.url_params.get_dict("order_by");
        tpl::Filter tpl{db, where, order_by};
        return tpl.render();
    });

CROW_ROUTE(app, "/sign").methods("POST"_method)
([&db, app_ptr](const crow::request& req, crow::response& resp) {
    /* use *app_ptr instead of app */
    auto& ctx  = app_ptr->template get_context<crow::CookieParser>(req);
    Session ses{ctx.get_cookie(SESSION_COOKIE_NAME)};

    if (!ses.has_field("csrf_token"))            { resp.code = 403; resp.end(); return; }
    if (req.body.size() > 4096)                  { resp.code = 400; resp.end(); return; }

    auto params     = req.get_body_params();
    const char* tok = params.get("csrf_token");
    if (!tok || ses.get_field("csrf_token").value() != tok) {
        resp.code = 403; resp.end(); return;
    }

    DbStatement stmt = db.prepare(
        "INSERT INTO guests (name,email,message) VALUES (?,?,?)");
    sqlite3_bind_text(stmt.stmt, 1, params.get("name"),    -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt.stmt, 2, params.get("email"),   -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt.stmt, 3, params.get("message"), -1, SQLITE_STATIC);

    resp.code = (sqlite3_step(stmt.stmt) == SQLITE_DONE) ? 303 : 500;
    resp.end();
});
    CROW_ROUTE(app, "/schema")([&db](const crow::request&, crow::response& resp) {
        std::ostringstream buf;
        buf << "<!doctype html><html><body><table>";
        DbStatement stmt = db.prepare("SELECT name,type,sql FROM sqlite_master");
        while (sqlite3_step(stmt.stmt) == SQLITE_ROW) {
            buf << "<tr><td>"  << sqlite3_column_text(stmt.stmt, 0)
                << "</td><td>" << sqlite3_column_text(stmt.stmt, 1)
                << "</td><td><pre>" << sqlite3_column_text(stmt.stmt, 2)
                << "</pre></td></tr>";
        }
        buf << "</table></body></html>";
        resp.set_header("Content-Type", "text/html");
        resp.write(buf.str());
        resp.end();
    });

    return app;
}

/* ------------------------------------------------------------------ */
/*  Fuzz driver                                                       */
/* ------------------------------------------------------------------ */
static void fuzz_one_input(const std::string& data, GBApp& app, Db& db)
{
    if (data.empty()) return;

    uint8_t selector   = static_cast<uint8_t>(data[0]);
    std::string payload = data.substr(1);

    crow::request  req;
    crow::response res;

    switch (selector) {
        case 0: req.method = crow::HTTPMethod::Get;  req.url = "/";            break;
        case 1: req.method = crow::HTTPMethod::Get;  req.url = "/info";        break;
        case 2:
            req.method   = crow::HTTPMethod::Get;
            req.url      = "/filter";
            req.raw_url  = "/filter?" + payload;
            req.url_params = crow::query_string(payload, false);
            break;
        case 3: {
            /* Simulate first visit to obtain CSRF cookie */
            crow::request  reqIndex;
            crow::response resIndex;
            reqIndex.url = "/";
            attach_cookie_ctx(reqIndex);            // <<<<<<<<<< fixed line
            app.handle_full(reqIndex, resIndex);

            /* Parse the freshly-set cookie */
            std::string set_cookie = resIndex.get_header_value("Set-Cookie");
            std::string cookie_kv  = set_cookie.substr(0, set_cookie.find(';'));
            std::string cookie_val = cookie_kv.substr(cookie_kv.find('=') + 1);

            Session sess(cookie_val);
            std::string csrf = sess.get_field("csrf_token").value_or("invalid");

            /* Split payload name|email|message */
            std::string name, email, message;
            size_t p1 = payload.find('|');
            size_t p2 = payload.find('|', p1 == std::string::npos ? payload.size() : p1 + 1);
            if (p1 != std::string::npos) {
                name = payload.substr(0, p1);
                if (p2 != std::string::npos) {
                    email   = payload.substr(p1 + 1, p2 - p1 - 1);
                    message = payload.substr(p2 + 1);
                } else {
                    email = payload.substr(p1 + 1);
                }
            } else {
                name = payload;
            }

            std::string body =
                "csrf_token=" + percent_encode(csrf) +
                "&name="      + percent_encode(name) +
                "&email="     + percent_encode(email) +
                "&message="   + percent_encode(message);

            req.method = crow::HTTPMethod::Post;
            req.url    = "/sign";
            req.body   = body;
            req.add_header("Content-Type",
                           "application/x-www-form-urlencoded");
            req.add_header("Cookie", cookie_kv);
            break;
        }
        default:
            req.method = crow::HTTPMethod::Get;
            req.url    = "/schema";
            break;
    }

    attach_cookie_ctx(req);                  // <<<<<<<<<< fixed line
    app.handle_full(req, res);
    std::fprintf(stderr, "[RES] code=%d body=%zu\n", res.code, res.body.size());
}

/* ------------------------------------------------------------------ */
int main()
{
    Db db;
    sqlite3_step(db.prepare(
        "INSERT INTO guests (name,email,message) VALUES ('fuzz','fuzz','fuzz')"
    ).stmt);

    GBApp& app = get_app(db);
    
    app.validate();

#ifdef __AFL_INIT
    __AFL_INIT();
#endif
    while (__AFL_LOOP(1000)) {
        std::string input = read_stdin();
        fuzz_one_input(input, app, db);
    }
    return 0;
}
