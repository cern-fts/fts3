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
#include <libsoup/soup-uri.h>


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
    SoupURI* soupUri = soup_uri_new(uri.c_str());

    u0.Protocol = strptr2str(soup_uri_get_scheme(soupUri));
    u0.Host = strptr2str(soup_uri_get_host(soupUri));
    u0.Port = soup_uri_get_port(soupUri);
    u0.Path = strptr2str(soup_uri_get_path(soupUri));
    u0.QueryString = strptr2str(soup_uri_get_query(soupUri));

    soup_uri_free(soupUri);
    return u0;
}
