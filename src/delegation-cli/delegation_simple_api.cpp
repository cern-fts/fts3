/*
 *	Copyright notice:
 *	Copyright © Members of the EMI Collaboration, 2010.
 *
 *	See www.eu-emi.eu for details on the copyright holders
 *
 *	Licensed under the Apache License, Version 2.0 (the "License");
 *	you may not use this file except in compliance with the License.
 *	You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 *	Unless required by applicable law or agreed to in writing, software
 *	distributed under the License is distributed on an "AS IS" BASIS,
 *	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *	See the License for the specific language governing permissions and
 *	limitations under the License.
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <stdlib.h>
#include <stdarg.h>
#include "globus_gsi_system_config.h"
#include "cgsi_plugin.h"
#include "gridsite.h"
#include "delegation.nsmap"
#include "delegation-simple.h"
#include <sstream>
#include "ServiceDiscoveryIfce.h"
#include "util.h"

struct _glite_delegation_ctx
{
    struct soap             *soap;
    char                    *endpoint;
    char                    *error_message;
    int                     error;
    char                    *proxy;
};

#define HTTP_PREFIX     "http:"
/* Check if this is a http:// URL */
static int is_http(const char *url)
{
    return url && !strncmp(url, HTTP_PREFIX, strlen(HTTP_PREFIX));
}

#define HTTPS_PREFIX    "https:"
/* Check if this is a https:// URL */
static int is_https(const char *url)
{
    return url && !strncmp(url, HTTPS_PREFIX, strlen(HTTPS_PREFIX));
}

#define HTTPG_PREFIX    "httpg:"
/* Check if this is a httpg:// URL */
static int is_httpg(const char *url)
{
    return url && !strncmp(url, HTTPG_PREFIX, strlen(HTTPG_PREFIX));
}

static void glite_delegation_set_error(glite_delegation_ctx *ctx, char *fmt, ...)
{
    va_list ap;
    if (ctx->error_message)
        free(ctx->error_message);

    va_start(ap, fmt);
    ctx->error_message = g_strdup_vprintf(fmt, ap);
    va_end(ap);
}


char *glite_delegation_get_error(glite_delegation_ctx *ctx)
{
    if(!ctx)
        return (char *) "Out of memory";

    if(!ctx->error_message)
        return (char *) "No error";

    return ctx->error_message;
}

static void decode_exception(glite_delegation_ctx *ctx,
                             struct SOAP_ENV__Detail *detail, const char *method)
{
    char *message;

    if (!detail)
        return;

    // TODO remove this macro!!!

#define SET_EXCEPTION(exc) \
    message = const_cast<char*>(((struct _delegation__ ## exc *)detail->fault)->msg->c_str()); \
    if (!message) \
        message = (char *) #exc " received from the service"; \
    glite_delegation_set_error(ctx, (char *) "%s: %s", method, message); \
    ctx->error = 1;

    switch (detail->__type)
        {
        case SOAP_TYPE__delegation__DelegationException:
            SET_EXCEPTION(DelegationException);
            break;
        default:
            /* Let the generic error decoding handle this */
            break;
        }
#undef SET_EXCEPTION
}

static void _fault_to_error(glite_delegation_ctx *ctx, const char *method)
{
    const char **code, **string, **detail;
    struct soap *soap = ctx->soap;

    soap_set_fault(soap);

    if (soap->fault)
        {
            /* Look for a SOAP 1.1 fault */
            if (soap->fault->detail)
                {
                    SOAP_ENV__Detail *detail = soap->fault->detail;
                    decode_exception(ctx, detail, method);
                }
            /* Look for a SOAP 1.2 fault */
            if (soap->fault->SOAP_ENV__Detail)
                {
                    SOAP_ENV__Detail *detail = soap->fault->SOAP_ENV__Detail;
                    decode_exception(ctx, detail, method);
                }
        }

    /* If we did not manage to decode the exception, try generic error
    * decoding */
    if (!ctx->error)
        {
            code = soap_faultcode(soap);
            string = soap_faultstring(soap);
            detail = soap_faultdetail(soap);

            /* If the SOAP 1.1 detail is empty, try the SOAP 1.2 detail */
            if (!detail && soap->fault && soap->fault->SOAP_ENV__Detail)
                detail = (const char **)&soap->fault->SOAP_ENV__Detail->__any;

            /* Provide default messages */
            if (!code || !*code)
                {
                    code = (const char**) alloca(sizeof(*code));
                    *code = "(SOAP fault code missing)";
                }
            if (!string || !*string)
                {
                    string = (const char**) alloca(sizeof(*string));
                    *string = "(SOAP fault string missing)";
                }

            if (detail && *detail)
                glite_delegation_set_error(ctx, (char *) "%s: SOAP fault: %s - %s (%s)", method, *code,
                                           *string, *detail);
            else
                glite_delegation_set_error(ctx, (char *) "%s: SOAP fault: %s - %s", method, *code,
                                           *string);
        }

    soap_end(soap);
}

glite_delegation_ctx *glite_delegation_new(const char *endpoint, const char *proxy)
{
    int ret;
    glite_delegation_ctx *ctx;

    ctx = (glite_delegation_ctx *) calloc(sizeof(*ctx), 1);
    if(!ctx)
        return NULL;


    if( (! is_http(endpoint)) && (! is_https(endpoint)) && (! is_httpg(endpoint)) )
        {
            char *error;

            char *sd_type = getenv(GLITE_DELEGATION_SD_ENV);
            if(!sd_type)
                sd_type = (char *) GLITE_DELEGATION_SD_TYPE;

            ctx->endpoint = glite_discover_endpoint(sd_type, endpoint, &error);

            if (!ctx->endpoint)
                {
                    glite_delegation_set_error(ctx, (char *) "glite_delegation: service discovery error %s", (char *) error);
                    free(error);
                    return ctx;
                }
        }
    else
        {
            ctx->endpoint = strdup(endpoint);
            if (!ctx->endpoint)
                {
                    glite_delegation_set_error(ctx, (char *) "glite_delegation: out of memory");
                    return ctx;
                }
        }

    ctx->soap = soap_new();
    /* Register the CGSI plugin if secure communication is requested */
    if (is_https(ctx->endpoint))
        ret = soap_cgsi_init(ctx->soap,
                             CGSI_OPT_DISABLE_NAME_CHECK |
                             CGSI_OPT_SSL_COMPATIBLE);
    else if (is_httpg(ctx->endpoint))
        ret = soap_cgsi_init(ctx->soap,
                             CGSI_OPT_DISABLE_NAME_CHECK);
    else
        ret = 0;

    /* Setup credentials */
    if (ret == 0 && proxy)
        {
            if (cgsi_plugin_set_credentials(ctx->soap, 0, proxy, proxy) < 0) {
                _fault_to_error(ctx, "Setting credentials");
                return ctx;
            }
        }

    if (ret)
        {
            glite_delegation_set_error(ctx, (char *) "Failed to initialize the GSI plugin");
            return ctx;
        }

    /* Namespace setup should happen after CGSI plugin initialization */
    if ( soap_set_namespaces(ctx->soap, delegation_namespaces) )
        {
            _fault_to_error(ctx, "Setting SOAP namespaces");
            return ctx;
        }

    if (proxy)
        ctx->proxy = strdup(proxy);
    else
        ctx->proxy = NULL;

    return ctx;
}

void glite_delegation_free(glite_delegation_ctx *ctx)
{
    if(!ctx)
        return;

    free(ctx->endpoint);
    free(ctx->error_message);
    free(ctx->proxy);
    if(ctx->soap)
        {
            soap_destroy(ctx->soap);
            soap_end(ctx->soap);
            free(ctx->soap);
        }
    free(ctx);
}

int glite_delegation_delegate(glite_delegation_ctx *ctx,
                              const char *delegationID,
                              int expiration, int force)
{
    char *sdelegationID = (char *) "", *localproxy, *certtxt, *scerttxt;
    std::string certreq;

    struct delegation__getProxyReqResponse get_resp;
    struct delegation__renewProxyReqResponse renew_resp;
    //struct delegation__getTerminationTimeResponse term_resp;
    struct delegation__putProxyResponse put_resp;
    //time_t term_time;
    int ret;

    if(!ctx)
        return -1;

    if (ctx->proxy) {
        localproxy = ctx->proxy;
    }
    else {
        localproxy = getenv("X509_USER_PROXY");
        if (!localproxy)
            {
                if (GLOBUS_GSI_SYSCONFIG_GET_PROXY_FILENAME(&localproxy,
                        GLOBUS_PROXY_FILE_INPUT))
                    {
                        glite_delegation_set_error(ctx, (char *) "glite_delegation_dowork: unable to get"
                                                   " user proxy filename!");
                        return -1;
                    }
            }
    }

    /* error is already set */
    if (!ctx->soap)
        return -1;

    if (delegationID)
        {
            sdelegationID = soap_strdup(ctx->soap, delegationID);
            if (!sdelegationID)
                {
                    glite_delegation_set_error(ctx, (char *) "glite_delegation_dowork: soap_strdup()"
                                               " of delegationID failed!");
                    return -1;
                }
        }

    /* using certreq as a marker */
    if (force)
        {
            /* force the renewal of the proxy */
            ret = soap_call_delegation__renewProxyReq(ctx->soap, ctx->endpoint, NULL, std::string(sdelegationID), renew_resp);
            if (SOAP_OK != ret)
                {
                    _fault_to_error(ctx, __func__);
                    return -1;
                }
            certreq = renew_resp._renewProxyReqReturn;
        }

    /* if it was forced and failed, or if it was not forced at all */
    if (certreq.empty())
        {
            /* there was no proxy, or not forcing -- the normal path */
            ret = soap_call_delegation__getProxyReq(ctx->soap, ctx->endpoint, NULL, sdelegationID, get_resp);
            if (SOAP_OK != ret)
                {
                    _fault_to_error(ctx, __func__);
                    return -1;
                }
            certreq = get_resp._getProxyReqReturn;
        }

    /* generating a certificate from the request */
    if(!certreq.empty())
        {
            ret = GRSTx509MakeProxyCert(&certtxt, stderr, (char*)certreq.c_str(),
                                        localproxy, localproxy, expiration);
        }
    else
        {
            glite_delegation_set_error(ctx, (char *) "glite_delegation_delegate: "
                                       "GRSTx509MakeProxyCert call failed");
            return -1;

        }
    if (ret != GRST_RET_OK)
        {
            glite_delegation_set_error(ctx, (char *) "glite_delegation_delegate: "
                                       "GRSTx509MakeProxyCert call failed");
            return -1;
        }

    scerttxt = soap_strdup(ctx->soap, certtxt);
    if (!scerttxt)
        {
            glite_delegation_set_error(ctx, (char *) "glite_delegation_delegate: soap_strdup()"
                                       " of delegationID failed!");
            return -1;
        }

    if (SOAP_OK != soap_call_delegation__putProxy(ctx->soap, ctx->endpoint, NULL, sdelegationID, scerttxt, put_resp))
        {
            _fault_to_error(ctx, __func__);
            return -1;
        }

    return 0;
}


int glite_delegation_info(glite_delegation_ctx *ctx,
                          const char *delegationID, time_t *expiration)
{
    char *sdelegationID = (char *) "";
    struct delegation__getTerminationTimeResponse resp;

    if(!ctx)
        return -1;

    /* error is already set */
    if (!ctx->soap)
        return -1;

    if (delegationID)
        {
            sdelegationID = soap_strdup(ctx->soap, delegationID);
            if (!sdelegationID)
                {
                    glite_delegation_set_error(ctx, (char *)"glite_delegation_info: soap_strdup()"
                                               " of delegationID failed!");
                    return -1;
                }
        }

    if (SOAP_OK != soap_call_delegation__getTerminationTime(ctx->soap, ctx->endpoint, NULL, std::string(sdelegationID), resp))
        {
            _fault_to_error(ctx, __func__);
            return -1;
        }

    *expiration = resp._getTerminationTimeReturn;

    return 0;
}
