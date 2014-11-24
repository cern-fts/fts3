/* Copyright @ Members of the EMI Collaboration, 2010.
See www.eu-emi.eu for details on the copyright holders.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License. */

#include "parse_url.h"

// Yep, really...
#if SOUP_VERSION_MAJOR == 2 && SOUP_VERSION_MINOR <= 2
extern "C" {
#endif
#include <libsoup/soup-uri.h>
#if SOUP_VERSION_MAJOR == 2 && SOUP_VERSION_MINOR <= 2
}
#endif

static std::string strptr2str(const char* ptr)
{
    if (ptr)
        return std::string(ptr);
    else
        return std::string();
}


Uri Uri::Parse(const std::string &uri)
{
    Uri u0;

/* Sadly, we need to do some preprocessor here because the Soup version in EL5 is 2.2
 * and it doesn't work quite the same
 */
#if SOUP_VERSION_MAJOR == 2 && SOUP_VERSION_MINOR <= 2
    SoupUri* soupUri = soup_uri_new(uri.c_str());

    u0.Protocol = strptr2str(g_quark_to_string(soupUri->protocol));
    u0.Host = strptr2str(soupUri->host);
    u0.Port = soupUri->port;
    u0.Path = strptr2str(soupUri->path);
    u0.QueryString = strptr2str(soupUri->query);

    soup_uri_free(soupUri);
/** For EL6 or greater */
#else
    SoupURI* soupUri = soup_uri_new(uri.c_str());

    u0.Protocol = strptr2str(soup_uri_get_scheme(soupUri));
    u0.Host = strptr2str(soup_uri_get_host(soupUri));
    u0.Port = soup_uri_get_port(soupUri);
    u0.Path = strptr2str(soup_uri_get_path(soupUri));
    u0.QueryString = strptr2str(soup_uri_get_query(soupUri));

    soup_uri_free(soupUri);
#endif

    return u0;
}
