/*
 * Copyright (c) Members of the EGEE Collaboration. 2004.
 * See http://www.eu-egee.org/partners/ for details on the copyright holders.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *  GLite Utilities - Misc. utilities
 *
 *  Authors: Gabor Gombas <Gabor.Gombas@cern.ch>
 *  Version info: $Id: util.c,v 1.15 2007/06/06 09:05:48 szamsu Exp $
 *  Release: $Name:  $
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "util.h"
#include "ServiceDiscoveryIfce.h"

#include <gridsite.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include "globus_gsi_system_config.h"


#include <stdlib.h>
#include <string.h>
#include <glib.h>

typedef struct version_struct
{
    int major;
    int minor;
    int patch;
} version_t;


/**********************************************************************
 * Internal function
 */
void SD_I_freeVOList(SDVOList *vos)
{
    int i;

    if (vos)
        {
            if (vos->names)
                {
                    for (i = 0; i < vos->numNames; ++i)
                        {
                            if (vos->names[i])
                                {
                                    free(vos->names[i]);
                                }
                        }
                    free(vos->names);
                }
            free(vos);
        }
}


static void fill_version(const char * version, version_t * v)
{
    char * token = NULL;
    char * tmp   = NULL;
    v->major = 0;
    v->minor = 0;
    v->patch = 0;
    if((NULL != version) && (strlen(version) > 0))
        {
            tmp = strdup(version);
            token = strtok(tmp,".");
            if(NULL != token)
                {
                    v->major = atoi(token);
                    token = strtok(NULL,".");
                    if(NULL != token)
                        {
                            v->minor = atoi(token);
                            token = strtok(NULL,".");
                            if(NULL != token)
                                {
                                    v->patch = atoi(token);
                                }
                        }
                }
        }
    if(tmp)
        {
            free(tmp);
        }
}


const SDService * select_service_by_version(const SDServiceList * list, const char * version)
{
    int i;
    const SDService * service = NULL;

    version_t* srv_versions = NULL;
    version_t  req_version;
    int index = -1;
    if ((NULL != list) && (list->numServices > 0))
        {
            if(NULL == version)
                {
                    // No version specified: take the first entry in the list
                    service = list->services[0];
                }
            else
                {
                    // Create the Version List
                    fill_version(version,&req_version);
                    srv_versions = (version_t*)calloc(sizeof(version_t), static_cast<size_t>(list->numServices));
                    if (srv_versions == NULL)
                        return NULL;

                    for(i = 0; i < list->numServices; ++i)
                        {
                            fill_version(list->services[i]->version,&srv_versions[i]);
                        }

                    // Check if there is a service that match the exact version
                    for(i = 0; i < list->numServices; ++i)
                        {
                            // Check if the major version is the same
                            if(req_version.major == srv_versions[i].major)
                                {
                                    if(srv_versions && (req_version.minor == srv_versions[i].minor) &&
                                            (req_version.patch == srv_versions[i].patch))
                                        {
                                            // exact match: choose it
                                            index = i;
                                            break;
                                        }
                                    else
                                        {
                                            // Check if this version is "closer" (same
                                            // major number and greatest minor number)
                                            if((-1 == index) ||
                                                    (srv_versions[i].minor > srv_versions[index].minor) ||
                                                    ((srv_versions[i].minor == srv_versions[index].minor) &&
                                                     (srv_versions[i].patch > srv_versions[index].patch)))
                                                {
                                                    index = i;
                                                }
                                        }
                                }
                        }
                    if(-1 != index)
                        {
                            service = list->services[index];
                        }
                    else
                        {
                            // Fall-back soluton: take the first
                            service = list->services[0];
                        }
                }
        }
    if(NULL != srv_versions)
        {
            free(srv_versions);
        }
    return service;
}

/**********************************************************************
 * Utilities
 */

char *glite_discover_service_by_version(const char *type, const char *name, const char * version, char **error)
{
    SDServiceList *list;
    SDService *service;
    SDException exc;
    SDVOList *vos;
    GString *str;
    char *result;

    /** VO detection - check the SD_VO environment variable first */
    vos = check_vo_env();

    /** VO detection - if not defined yet, try the VOMS proxy */
    if (vos == NULL)
        vos = check_voms_proxy();

    str = g_string_new("");

    if (name)
        {
            // Get service by name
            service = SD_getService(name, &exc);
            if (service && strcasecmp(service->type, type))
                {
                    // Service exists, but the type is wrong: chek if there's
                    // an associated service of the given type
                    SD_freeService(service);
                    list = SD_listAssociatedServices(name, type, NULL,
                                                     vos, &exc);
                    if ((NULL != list) && (list->numServices > 0))
                        {
                            // select service by version
                            service = SD_getService(select_service_by_version(list,version)->name, &exc);
                            SD_freeServiceList(list);
                        }
                    else
                        {
                            // No associated service found
                            SD_freeServiceList(list);
                            SD_freeException(&exc);
                            //exc.reason = NULL;
                        }
                }
            else
                {
                    SD_freeException(&exc);
                    //exc.reason = NULL;
                }
            if (!service)
                {
                    // name is not a service name: check if it's a site name
                    list = SD_listServices(type, name, vos, &exc);
                    if ((NULL != list) && (list->numServices > 0))
                        {
                            // select service by version
                            service = SD_getService(select_service_by_version(list,version)->name, &exc);
                            SD_freeServiceList(list);
                        }
                    else
                        {
                            // No service found
                            SD_freeServiceList(list);
                            SD_freeException(&exc);
                            //exc.reason = NULL;
                        }
                }
            if (!service)
                {
                    // name is neither a site name: check it it's an host name
                    list = SD_listServicesByHost(type, name, vos, &exc);
                    if ((NULL != list) && (list->numServices > 0))
                        {
                            // select service by version
                            service = SD_getService(select_service_by_version(list,version)->name, &exc);
                            SD_freeServiceList(list);
                        }
                    else
                        {
                            // No service found
                            SD_freeServiceList(list);
                            SD_freeException(&exc);
                            //exc.reason = NULL;
                        }
                }

            if (!service)
                {
                    g_string_append_printf(str, "Failed to look up "
                                           "%s: not a service, site or host name.",name);
                    if (error)
                        *error = g_string_free(str, FALSE);
                    else
                        g_string_free(str, TRUE);
                    free(vos);
                    SD_freeException(&exc);
                    return NULL;
                }
            result = strdup(service->name);
            SD_freeService(service);
        }
    else
        {
            list = SD_listServices(type, NULL, vos, &exc);
            if (!list || !list->numServices)
                {
                    if (exc.status == SDStatus_SUCCESS)
                        g_string_append_printf(str, "No services of "
                                               "type %s were found.", type);
                    else
                        g_string_append_printf(str, "Locating services "
                                               "of type %s have failed: %s.", type,
                                               exc.reason);
                    SD_freeServiceList(list);
                    SD_freeException(&exc);
                    if (error)
                        *error = g_string_free(str, FALSE);
                    else
                        g_string_free(str, TRUE);
                    return NULL;
                }
            // select service by version
            result = strdup(select_service_by_version(list,version)->name);
            SD_freeServiceList(list);
        }

    g_string_free(str, TRUE);
    free(vos);
    if (error)
        *error = NULL;
    return result;
}


char *glite_discover_endpoint_by_version(const char *type, const char *name, const char * version, char **error)
{
    char *srvname, *endpoint;
    SDService *service;
    SDException exc;

    srvname = glite_discover_service_by_version(type, name, version, error);
    if (!srvname)
        return NULL;

    service = SD_getService(srvname, &exc);
    if (!service)
        {
            GString *str;

            str = g_string_new("");
            g_string_append_printf(str, "Service discovery lookup failed "
                                   "for %s: %s", srvname, exc.reason);
            SD_freeException(&exc);
            free(srvname);
            return NULL;
        }

    free(srvname);
    endpoint = strdup(service->endpoint);
    SD_freeService(service);
    return endpoint;
}

char *glite_discover_endpoint(const char *type, const char *name, char **error)
{
    return glite_discover_endpoint_by_version(type,name,NULL,error);
}


/* Build an SDVOList from the list of VOs in the VOMS proxy certificate */

SDVOList *check_voms_proxy(void)
{
    STACK_OF(X509_INFO) *info;
    globus_result_t result;
    STACK_OF(X509) *chain;
    int i, first;
    SDVOList *volist;
    char *proxy_name;
    X509 *proxy;
    FILE *fp;

    result = GLOBUS_GSI_SYSCONFIG_GET_PROXY_FILENAME(&proxy_name,
             GLOBUS_PROXY_FILE_INPUT);
    if (result)
        return NULL;

    fp = fopen(proxy_name, "r");
    if (!fp)
        {
            free(proxy_name);
            return NULL;
        }

    /* Read the X509 structures from the proxy file */
    info = PEM_X509_INFO_read(fp, NULL, NULL, NULL);
    free(proxy_name);
    fclose(fp);
    if (!info)
        return NULL;

    /* Allocate the stack for the certificate chain */
    chain = sk_X509_new_null();
    if (!chain)
        {
            sk_X509_INFO_free(info);
            return NULL;
        }

    /* Extract the proxy certificate and build the cert chain */
    first = 1;
    proxy = NULL;
    while (sk_X509_INFO_num(info))
        {
            X509_INFO *elem;

            elem = sk_X509_INFO_shift(info);
            if (elem->x509)
                {
                    /* The first cert is the proxy itself, the rest
                     * is the CA chain */
                    if (first)
                        {
                            X509_set_subject_name(elem->x509, X509_get_issuer_name(elem->x509));
                            proxy = elem->x509;
                            first = 0;
                        }
                    sk_X509_push(chain, elem->x509);
                    elem->x509 = NULL;
                }
            X509_INFO_free(elem);
        }
    sk_X509_INFO_free(info);

    if (!sk_X509_num(chain) || !proxy)
        {
            sk_X509_free(chain);
            X509_free(proxy);
            return NULL;
        }

    const size_t credlen = 512;	/* Credential size */
    const int maxcreds = 10;	/* Maximum credential number */
    int lastcred = -1;		/* Initial "last" credential */
    char *creds, *vomsdir;

    if (NULL == (vomsdir = getenv("X509_VOMS_DIR")))
        {
            vomsdir = (char *) "/etc/grid-security/vomsdir";
        }

    if (NULL == (creds = (char *)malloc(maxcreds*(credlen+1))))
        {
            sk_X509_free(chain);
            X509_free(proxy);
            return NULL;
        }

    GRSTx509GetVomsCreds(&lastcred, maxcreds, credlen, creds, proxy, chain,
                         vomsdir);

    sk_X509_free(chain);
    X509_free(proxy);
    if (lastcred < 0)
        {
            free(creds);
            return NULL;
        }

    volist = (SDVOList*) calloc(1, sizeof(*volist));
    if (!volist)
        {
            free(creds);
            return NULL;
        }

    for (i = 0; i < lastcred+1; i++)
        {
            char **tmp, *p1, *p2, *act_cred;

            act_cred = &creds[i * static_cast<int>(credlen+1)];

            /* Search VO name
               act_cred is in form of '.../<voname>/...' */
            if (NULL == (p1 = strstr(act_cred, "/")))
                continue;
            p2 = ++p1;
            while (*p2 && *p2 != '/')
                p2++;
            if (*p2)
                *p2 = '\0';

            tmp = (char**) realloc(volist->names, static_cast<size_t>(volist->numNames + 1) *
                                   sizeof(*volist->names));
            if (!tmp)
                {
                    free(creds);
                    SD_I_freeVOList(volist);
                    return NULL;
                }
            tmp[volist->numNames] = strdup(p1);
            if (!tmp[volist->numNames])
                {
                    free(creds);
                    SD_I_freeVOList(volist);
                    return NULL;
                }
            volist->numNames++;
            volist->names = tmp;
        }
    free(creds);

    return volist;
}


/* Convert the value of the SD_VO env. variable to an SDVOList */
SDVOList *check_vo_env(void)
{
    SDVOList *volist;
    const char *vo;

    vo = getenv(SDEnv_VO);
    if (!vo)
        return NULL;

    volist = (SDVOList*) calloc(1, sizeof(*volist));
    if (!volist)
        return NULL;

    volist->names = (char**) malloc(sizeof(char *));
    if (!volist->names)
        {
            free(volist);
            return NULL;
        }

    volist->numNames = 1;
    volist->names[0] = strdup(vo);
    if (!volist->names[0])
        {
            free(volist->names);
            free(volist);
            return NULL;
        }
    return volist;
}
