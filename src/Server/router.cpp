#include <AtlasHTTP/Server/router.hpp>

bool HTTP::Router::match_path(const std::string& route_path, const std::string& req_path,
                              std::unordered_map<std::string, std::string>& params) {
    size_t i = 0;
    size_t j = 0;

    while (i < route_path.size() && j < req_path.size()) {
        while (i < route_path.size() && route_path[i] == '/') i++;
        while (j < req_path.size() && req_path[j] == '/') j++;

        if (i >= route_path.size() && j >= req_path.size()) {
            return true;
        }

        const size_t i_start = i;
        while (i < route_path.size() && route_path[i] != '/') i++;
        const std::string route_part = route_path.substr(i_start, i - i_start);

        const size_t j_start = j;
        while (j < req_path.size() && req_path[j] != '/') j++;
        const std::string req_part = req_path.substr(j_start, j - j_start);

        if (!route_part.empty() && route_part[0] == ':') {
            params[route_part.substr(1)] = req_part;
        } else if (route_part != req_part) {
            return false;
        }
    }

    return i >= route_path.size() && j >= req_path.size();
}

HTTP::Response HTTP::Router::dispatch(const std::vector<HTTP::Route>& routes, HTTP::Request& req) {
    HTTP::Response res;
    //res.status = 404;

    for (const auto& route : routes) {
        if (route.method != req.route.method) {
            continue;
        }

        std::unordered_map<std::string, std::string> params;
        if (!match_path(route.path, req.route.path, params)) {
            continue;
        }

        req.params = params;

        try {
            res = route.handler(req);
        } catch (...) {
            res.status = 500;
            res.body = "Internal Server Error";
        }
        break;
    }

    return res;
}
