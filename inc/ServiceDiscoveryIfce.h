/*                                                                -*- mode: c; c-basic-offset: 8;  -*-
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
 */

/*
 ============================================================================
 External API which is used by gfal and FTS
 ============================================================================
 */


#ifndef SERVICE_DISCOVERY_H
#define SERVICE_DISCOVERY_H

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <lber.h>
#include <ldap.h>
#include <glib-2.0/glib.h>
#include <assert.h>
#include <sys/types.h>
#include <getopt.h>
#include <unistd.h>


#ifdef __cplusplus 
extern "C" {
#endif


    /* GLOBAL DEFINES *************************************************************/

#define SDStatus_SUCCESS 0
#define SDStatus_FAILURE 1

    /* Name of the environment variable holding the default VO */
#define SDEnv_VO     "GLITE_SD_VO"
    /* Name of the environment variable holding the default site */
#define SDEnv_SITE   "GLITE_SD_SITE"


    extern char *sbasedn;


    /* GLOBAL TYPE DEFINITIONS  ***************************************************/

    /**
     * The SDServiceData structure holds a single service data item as a
     * keyword value pair.
     */

    typedef struct {
        /** Keyword (not empty and not NULL). */
        char *key;
        /** Value (not NULL but may be empty). */
        char *value;
    } SDServiceData;

    /**
     * The SDServiceDataList structure holds an array of SDServiceData items
     * and a count.
     */

    typedef struct {
        /** The plugin that allocated the structure. The application should not touch it. */
        void * const _owner;
        /** The number of data items (may be zero). */
        int numItems;
        /** Array of data items (NULL if and only if numItems is zero). */
        SDServiceData *items;
    } SDServiceDataList;

    /**
     * The SDVOList structure holds an array of VO names and a count.
     */

    typedef struct {
        /** The number of VO names (may be zero). */
        int numNames;
        /** Array of VO names (NULL if and only if numVOs is zero). */
        char **names;
    } SDVOList;

    /**
     * The SDService structure holds the basic details about a GLite service. More
     * details can be obtained from a SDServiceDetails structure (see below).
     */

    typedef struct {
        /** The plugin that allocated the structure. The application should not touch it. */
        void * const _owner;
        /** Unique service name. */
        char *name;
        /** The type of service. */
        char *type;
        /** The endpoint used to contact the service. */
        char *endpoint;
        /** The service version. */
        char *version;
    } SDService;

    /**
     * The SDServiceList structure holds a list of Services and a count.
     */

    typedef struct {
        /** The plugin that allocated the structure. The application should not touch it. */
        void * const _owner;
        /** The number of services (may be zero). */
        int numServices;
        /** Array of services (NULL if and only if numServices is zero). */
        SDService **services;
    } SDServiceList;

    /**
     * The SDServiceDetails structure holds full details about a GLite service,
     * including those returned in the SDService structure.
     */

    typedef struct {
        /** The plugin that allocated the structure. The application should not touch it. */
        void * const _owner;
        /** See description in SDService. */
        char *name;
        /** See description in SDService. */
        char *type;
        /** See description in SDService. */
        char *endpoint;
        /** See description in SDService. */
        char *version;
        /** The name of the site that hosts the service. */
        char *site;
        /** The URL of the WSDL for the service (NULL if it is not
         a Web Service). */
        char *wsdl;
        /** An administration contact e-mail address. */
        char *administration;
        /** The list of VOs supported by this service. */
        SDVOList *vos;
        /** A list of associated services. */
        SDServiceList *associatedServices;
        /** A list of service data (keyword/value pairs). */
        SDServiceDataList *data;

        /*GFAL */

    } SDServiceDetails;

    /**
     * The SDServiceDetailsList structure holds a list of Service details and a count.
     */

    typedef struct {
        /** The plugin that allocated the structure. The application should not touch it. */
        void * const _owner;
        /** The number of service details (may be zero). */
        int numServiceDetails;
        /** Array of service datils (NULL if and only if numServiceDatils is zero). */
        SDServiceDetails **servicedetails;
    } SDServiceDetailsList;

    /**
     * The SDException structure holds the status of a call to this Service
     * Discovery API.
     */

    typedef struct {
        /** API call status. Will be SDStatus_SUCCESS (guaranteed to be zero)
         on success, or some other value on error. */
        int status;
        /** Reason for failure. Will be NULL if and only if status is
         SDStatus_SUCCESS, but may be an empty string. */
        char *reason;
    } SDException;

    /**********************************************************************
     * Prototypes
     */


    /* PROTOTYPE FUNCTIONS ***********************************************************/

    /**
     * The SD_getService function returns basic details about the requested
     * service.
     *
     * You can dispose of any data returned by calling SD_freeService.
     * You can dispose of exception data (on failure) by calling SD_freeException.
     *
     * @param  serviceName        Unique name of service.
     * @param  exception          If not NULL, receives status of API call.
     *
     * @return Basic service details, or NULL if service cannot be found or if the
     *         API call fails.
     */
    SDService *SD_getService(const char *serviceName, SDException *exception);

    /**
     * The SD_getServiceDetails function returns full details about the requested
     * service.
     *
     * You can dispose of any data returned by calling SD_freeServiceDetails.
     * You can dispose of exception data (on failure) by calling SD_freeException.
     *
     * @param  serviceName        Unique name of service.
     * @param  exception          If not NULL, receives status of API call.
     *
     * @return Full service details, or NULL if service cannot be found or if the
     *         API call fails.
     */
    SDServiceList *SD_listServices(const char *type, const char *site, const SDVOList *vos, SDException *exception);




    SDServiceDetails *SD_getServiceDetails(const char *serviceName,
            SDException *exception);

    /**
     * The SD_getServiceData function returns all service keyword/value data for
     * the requested service.
     *
     * You can dispose of any data returned by calling SD_freeServiceData.
     * You can dispose of exception data (on failure) by calling SD_freeException.
     *
     * @param  serviceName        Unique name of service.
     * @param  exception          If not NULL, receives status of API call.
     *
     * @return Service data list or NULL if the API call fails. Service data list
     *         will be empty if the service cannot be found, or doesn't have any
     *         keyword/value data.
     */

    SDServiceDataList *SD_getServiceData(const char *serviceName,
            SDException *exception);

    /**
     * The SD_getServiceDataItem function returns the value of the requested
     * service parameter.
     *
     * You can dispose of any data returned by calling free().
     * You can dispose of exception data (on failure) by calling SD_freeException.
     *
     * @param  serviceName        Unique name of service.
     * @param  key                Parameter name.
     * @param  exception          If not NULL, receives status of API call.
     *
     * @return Parameter value or NULL if the service cannot be found, doesn't
     *         contain the requested data, or if the API call fails.
     */

    char *SD_getServiceDataItem(const char *serviceName, const char *key,
            SDException *exception);

    /**
     * The SD_getServiceSite function returns the name of the site where a service
     * runs.
     *
     * You can dispose of any data returned by calling free().
     * You can dispose of exception data (on failure) by calling SD_freeException.
     *
     * @param  serviceName        Unique name of service.
     * @param  exception          If not NULL, receives status of API call.
     *
     * @return Site name or NULL if the service cannot be found, or if the API call
     *         fails.
     */

    char *SD_getServiceSite(const char *serviceName, SDException *exception);

    /**
     * The SD_getServiceWSDL function returns the URL to the service WSDL (if any).
     *
     * You can dispose of any data returned by calling free().
     * You can dispose of exception data (on failure) by calling SD_freeException.
     *
     * @param  serviceName        Unique name of service.
     * @param  exception          If not NULL, receives status of API call.
     *
     * @return WSDL string or NULL if the service cannot be found, is not a Web
     *         Service, or if the API call fails.
     */

    char *SD_getServiceWSDL(const char *serviceName, SDException *exception);

    /**
     * The SD_listAssociatedServices function returns a list of services that are
     * are associated with the requested service and that match the specified type,
     * site and VOs (services with no VO affiliation match any VO specified by the
     * user).
     *
     * You can dispose of any data returned by calling SD_freeServiceList.
     * You can dispose of exception data (on failure) by calling SD_freeException.
     *
     * @param  serviceName        Name of service with which others are associated.
     * @param  type               Type of services required (NULL => any).
     * @param  site               Site of services required (NULL => any).
     * @param  vos                List of VOs from which services may come
     *                            (NULL => any).
     * @param  exception          If not NULL, receives status of API call.
     *
     * @return List of matching services or NULL if the API call fails. List will
     *         be empty if no services are found.
     */

    SDServiceList *SD_listAssociatedServices(const char *serviceName,
            const char *type, const char *site, const SDVOList *vos,
            SDException *exception);


    /**
     * The SD_listServicesByData function returns a list of services that match
     * the specified keyword/value data, type, site and VOs (services with no VO
     * affiliation match any VO specified by the user).
     *
     * You can dispose of any data returned by calling SD_freeServiceList.
     * You can dispose of exception data (on failure) by calling SD_freeException.
     *
     * @param  data               List of keyword/value data to match.
     * @param  type               Type of services required (NULL => any).
     * @param  site               Site of services required (NULL => any).
     * @param  vos                List of VOs from which services may come
     *                            (NULL => any).
     * @param  exception          If not NULL, receives status of API call.
     *
     * @return List of matching services or NULL if the API call fails. List will
     *         be empty if no services are found.
     */

    SDServiceList *SD_listServicesByData(const SDServiceDataList *data,
            const char *type, const char *site, const SDVOList *vos,
            SDException *exception);

    /**
     * The SD_listServicesByHost function returns a list of services that
     * match the specified type, the given host and port and VOs (services
     * with no VO affiliation match any VO specified by the user).
     *
     * The host:port is looked up in the service endpoint and a string match is
     * applied in the query (this is more efficient than performing this search
     * on the client side)
     *
     * You can dispose of any data returned by calling SD_freeServiceList.
     * You can dispose of exception data (on failure) by calling SD_freeException.
     *
     * @param  type               Type of services required (NULL => any).
     * @param  host               Host name of services required (NULL => any).
     * @param  vos                List of VOs from which services may come
     *                            (NULL => any).
     * @param  exception          If not NULL, receives status of API call.
     *
     * @return List of matching services or NULL if the API call fails. List will
     *         be empty if no services are found.
     */

    SDServiceList *SD_listServicesByHost(const char *type, const char *host,
            const SDVOList *vos, SDException *exception);

    /**
     * Frees all memory associated with an SDServiceDataList structure.
     *
     * @param  serviceDataList    Structure to free (if NULL, does nothing).
     *
     * @return Nothing.
     */


    void SD_freeException(SDException *exception);
    void SD_freeServiceDetails(SDServiceDetails *serviceDetails);
    void SD_freeServiceList(SDServiceList *serviceList);
    void SD_freeService(SDService *service);
    void SD_freeServiceDataList(SDServiceDataList *serviceDataList);



    /* GFAL ONLY API */
    int sd_get_seap_info(const char *host, char ***access_protocol, int **port, char *errbuf, int errbufsz);
    int sd_get_se_types_and_endpoints(const char *host, char ***se_types, char ***se_endpoints, char *errbuf, int errbufsz);
    int sd_get_storage_path(const char *host, const char *spacetokendesc, char **sa_path, char **sa_root, char *errbuf, int errbufsz);
    int sd_get_lfc_endpoint(char **lfc_endpoint, char *errbuf, int errbufsz);
    int sd_get_ce_ap(const char *host, char **ce_ap, char *errbuf, int errbufsz);
    int sd_is_online(char *sa, char *errbuf, int errbufsz);
    void set_gfal_vo(char *vo);
    void set_gfal_fqan(char **fqan, int fqan_size);

    /*RLS is not used anymore, leave the API as placeholder for future use*/
    int sd_get_rls_endpoints(char **lrc_endpoint, char **rmc_endpoint, char *vo);


#ifdef __cplusplus
}
#endif


#endif /* SERVICE_DISCOVERY_H */

