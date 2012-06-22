

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <stdlib.h>
#include <stdarg.h>
#include "cgsi_plugin.h"
#include "globus_gsi_system_config.h"
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
        return "Out of memory";

    if(!ctx->error_message)
        return "No error";

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
        message = #exc " received from the service"; \
    glite_delegation_set_error(ctx, "%s: %s", method, message); \
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
        if (soap->fault->detail) {
        	SOAP_ENV__Detail *detail = soap->fault->detail;
            decode_exception(ctx, detail, method);
        }
        /* Look for a SOAP 1.2 fault */
        if (soap->fault->SOAP_ENV__Detail) {
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
            glite_delegation_set_error(ctx, "%s: SOAP fault: %s - %s (%s)", method, *code,
                *string, *detail);
        else
            glite_delegation_set_error(ctx, "%s: SOAP fault: %s - %s", method, *code,
                *string);
    }

    soap_end(soap);
}

glite_delegation_ctx *glite_delegation_new(const char *endpoint)
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
            sd_type = GLITE_DELEGATION_SD_TYPE;
        
        ctx->endpoint = glite_discover_endpoint(sd_type, endpoint, &error);

        if (!ctx->endpoint)
        {
            glite_delegation_set_error(ctx, "glite_delegation: service discovery error %s", error);
            free(error);
            return ctx;
        }
    }
    else 
    {
        ctx->endpoint = strdup(endpoint);
        if (!ctx->endpoint)
        {
            glite_delegation_set_error(ctx, "glite_delegation: out of memory");
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

    if (ret)
    {
        glite_delegation_set_error(ctx, "Failed to initialize the GSI plugin");
        return ctx;
    }

    /* Namespace setup should happen after CGSI plugin initialization */
    if ( soap_set_namespaces(ctx->soap, delegation_namespaces) )
    {
        _fault_to_error(ctx, "Setting SOAP namespaces");
        return ctx;
    }

    return ctx;
}

void glite_delegation_free(glite_delegation_ctx *ctx)
{
    if(!ctx)
        return;

    free(ctx->endpoint);
    free(ctx->error_message);
    if(ctx->soap) 
    {
        soap_destroy(ctx->soap);
        soap_end(ctx->soap);
        free(ctx->soap);
    }
    free(ctx);
}

const char *glite_delegation_get_endpoint(glite_delegation_ctx *ctx)
{
    if(!ctx)
        return NULL;

    return ctx->endpoint;
}

int glite_delegation_delegate(glite_delegation_ctx *ctx, 
    const char *delegationID, int expiration, int force)
{
    char *sdelegationID = "", *localproxy, *certreq, *certtxt, *scerttxt;
    
    struct delegation__getProxyReqResponse get_resp;
    struct delegation__renewProxyReqResponse renew_resp;
    struct delegation__getTerminationTimeResponse term_resp;
    struct delegation__putProxyResponse put_resp;
    time_t term_time;
    int ret;

    if(!ctx)
        return -1;
    
    if (NULL == (localproxy = getenv("X509_USER_PROXY")))
    {
        if (GLOBUS_GSI_SYSCONFIG_GET_PROXY_FILENAME(&localproxy,
            GLOBUS_PROXY_FILE_INPUT))
        {
                glite_delegation_set_error(ctx, "glite_delegation_dowork: unable to get"
                " user proxy filename!");
            return -1;
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
            glite_delegation_set_error(ctx, "glite_delegation_dowork: soap_strdup()"
                           " of delegationID failed!");
            return -1;
        }
    }

    /* using certreq as a marker */
    certreq = NULL;
    if (force) 
    {
        /* force the renewal of the proxy */
        ret = soap_call_delegation__renewProxyReq(ctx->soap, ctx->endpoint, NULL, std::string(sdelegationID), renew_resp);
        if (SOAP_OK != ret)
        {
            _fault_to_error(ctx, __func__);
            return -1;
        }
        certreq = (char *) std::string(renew_resp._renewProxyReqReturn).c_str();
    }

    /* if it was forced and failed, or if it was not forced at all */
    if (NULL == certreq) 
    {
        /* there was no proxy, or not forcing -- the normal path */
        ret = soap_call_delegation__getProxyReq(ctx->soap, ctx->endpoint, NULL, std::string(sdelegationID), get_resp);
        if (SOAP_OK != ret)
        {
            _fault_to_error(ctx, __func__);
            return -1;
        }
        certreq = (char*) std::string(get_resp._getProxyReqReturn).c_str();
    }

    /* generating a certificate from the request */
    ret = GRSTx509MakeProxyCert(&certtxt, stderr, certreq, 
        localproxy, localproxy, expiration);
    if (ret != GRST_RET_OK)
    {
        glite_delegation_set_error(ctx, "glite_delegation_delegate: "
                                   "GRSTx509MakeProxyCert call failed");
        return -1;
    }

    scerttxt = soap_strdup(ctx->soap, certtxt);
    if (!scerttxt)
    {
        glite_delegation_set_error(ctx, "glite_delegation_delegate: soap_strdup()"
                                   " of delegationID failed!");
        return -1;
    }

    if (SOAP_OK != soap_call_delegation__putProxy(ctx->soap, ctx->endpoint, NULL, std::string(sdelegationID), scerttxt, put_resp))
    {
            _fault_to_error(ctx, __func__);
            return -1;
    }

    return 0;
}


int glite_delegation_info(glite_delegation_ctx *ctx,
    const char *delegationID, time_t *expiration)
{
    char *sdelegationID = "";
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
            glite_delegation_set_error(ctx, "glite_delegation_info: soap_strdup()"
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

int glite_delegation_destroy(glite_delegation_ctx *ctx, const char *delegationID)
{
    char *sdelegationID = "";
    struct delegation__destroyResponse dest_resp;

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
            glite_delegation_set_error(ctx, "glite_delegation_destroy: soap_strdup()"
                           " of delegationID failed!");
            return -1;
        }
    }
    
    if (SOAP_OK != soap_call_delegation__destroy(ctx->soap, ctx->endpoint, NULL, std::string(sdelegationID), dest_resp))
    {
            _fault_to_error(ctx, __func__);
            return -1;
    }

    return 0;
}

char *glite_delegation_getVersion(glite_delegation_ctx *ctx)
{
    struct delegation__getVersionResponse resp;
    char *version;

    if(!ctx)
        return NULL;

    /* error is already set */
    if (!ctx->soap) 
        return NULL;

    if (SOAP_OK != soap_call_delegation__getVersion(ctx->soap, ctx->endpoint, NULL, resp))
    {
            _fault_to_error(ctx, __func__);
            return NULL;
    }

    if (!resp.getVersionReturn.empty())
    {
        glite_delegation_set_error(ctx, "%s: service sent empty version", __func__);
        soap_end(ctx->soap);
        return NULL;
    }

    version = strdup( (const char*) std::string(resp.getVersionReturn).c_str());
    soap_end(ctx->soap);
    return version;
}

char *glite_delegation_getInterfaceVersion(glite_delegation_ctx *ctx)
{
    struct delegation__getInterfaceVersionResponse resp;
    char *version;

    if(!ctx)
        return NULL;

    /* error is already set */
    if (!ctx->soap) 
        return NULL;

    if (SOAP_OK != soap_call_delegation__getInterfaceVersion(ctx->soap, ctx->endpoint, NULL, resp))
    {
            _fault_to_error(ctx, __func__);
            return NULL;
    }

    if (!resp.getInterfaceVersionReturn.empty())
    {
        glite_delegation_set_error(ctx, "%s: service sent empty version", __func__);
        soap_end(ctx->soap);
        return NULL;
    }

    version = strdup( (const char*) std::string(resp.getInterfaceVersionReturn).c_str());
    soap_end(ctx->soap);
    return version;
}

char *glite_delegation_getServiceMetadata(glite_delegation_ctx *ctx,
    const char *key)
{
    struct delegation__getServiceMetadataResponse resp;
    char *req_key, *value;

    if (!key) 
    {
        glite_delegation_set_error(ctx, "%s: key must not be NULL", __func__);
        return NULL;
    }

    if(!ctx)
        return NULL;

    /* error is already set */
    if (!ctx->soap) 
        return NULL;

    /* Turning 'const char *' into 'char *' */
    req_key = soap_strdup(ctx->soap, key);
    if (!req_key)
    {
        glite_delegation_set_error(ctx, "%s: out of memory", __func__);
        return NULL;
    }

    if (SOAP_OK != soap_call_delegation__getServiceMetadata(ctx->soap, ctx->endpoint, NULL, req_key, resp))
    {
            _fault_to_error(ctx, __func__);
            return NULL;
    }

    if (!resp._getServiceMetadataReturn.empty())
    {
        glite_delegation_set_error(ctx, "%s: service had no value for key '%s'", 
                __func__, key);
        soap_end(ctx->soap);
        return NULL;
    }

    value= strdup((const char*) std::string(resp._getServiceMetadataReturn).c_str());
    soap_end(ctx->soap);
    return value;
}

