#include "../includes/ConfigLookup.hpp"

// "/" matchea cualquier path. Para el resto, requestPath debe empezar exactamente
// con locationPath, y lo que sigue (si algo sigue) debe empezar en un '/', para que
// "/uploads" no matchee "/uploadsExtra"
static bool pathMatchesLocation(const std::string &requestPath, const std::string &locationPath)
{
    if (locationPath == "/")
        return (true);
    if (requestPath.compare(0, locationPath.size(), locationPath) != 0)
        return (false);
    if (requestPath.size() == locationPath.size())
        return (true);
    if (locationPath[locationPath.size() - 1] == '/')
        return (true);
    return (requestPath[locationPath.size()] == '/');
}

const LocationConfig *findLocation(const ServerConfig &server, const std::string &path)
{
    const LocationConfig *best = NULL;
    size_t bestLen = 0;

    for (size_t i = 0; i < server.locations.size(); ++i)
    {
        const LocationConfig &loc = server.locations[i];
        if (pathMatchesLocation(path, loc.path) && loc.path.size() > bestLen)
        {
            best = &server.locations[i];
            bestLen = loc.path.size();
        }
    }
    return (best);
}
