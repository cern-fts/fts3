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
 *  GLite Utilities
 *
 *  Authors: Gabor Gombas <Gabor.Gombas@cern.ch>
 *  Version info: $Id: glite-util.h,v 1.8 2006/08/22 08:29:29 badino Exp $
 *  Release: $Name:  $
 *
 *
 */

#ifndef GLITE_UTIL_H
#define GLITE_UTIL_H

#ifdef __cplusplus
extern "C" {
#endif

/* Common URI schemes. */
#define GLITE_URI_LFN			"lfn"
#define GLITE_URI_GUID			"guid"
#define GLITE_URI_SRM			"srm"
#define GLITE_URI_HTTP			"http"
#define GLITE_URI_HTTPS			"https"
#define GLITE_URI_HTTPG			"httpg"

/*
 * Names of the environment variables that define the location of gLite
 * components.
 */
#define GLITE_LOCATION			"GLITE_LOCATION"
#define GLITE_LOCATION_VAR		"GLITE_LOCATION_VAR"
#define GLITE_LOCATION_LOG		"GLITE_LOCATION_LOG"
#define GLITE_TMP			"GLITE_TMP"

#include "ServiceDiscoveryIfce.h"

/**
 * Broken up URI
 */
typedef struct _glite_uri			glite_uri;

/**
 * \brief The broken up URI structure.
 *
 * The following combinations are valid:
 *  - hierarchical:
 *    - &lt;scheme&gt;://&lt;path&gt;
 *    - Non-LFN URIs: &lt;scheme&gt;://[&lt;endpoint&gt;]&lt;path&gt;[?&lt;query&gt;]
 *    - LFN URIs: &lt;scheme&gt;://&lt;endpoint&gt;?lfn=&lt;path&gt;
 *  - non-hierarchical: &lt;scheme&gt;:&lt;path&gt;
 */
struct _glite_uri
{
	int				hierarchical; /**< specifies wheter the URI is hierarhical or not. */
	char				*scheme; /**< the URI scheme */
	char				*endpoint; /**< the authority */
	char				*path; /**< the path part of the URI */
	char				*query; /**< the query part of the URI */
};

/**
 * \defgroup glite_util GLITE Utils
 * @{
 */

/** 
 * \brief Retrieve the value of "key" from either the environment or from
 * glite.conf. 
 *
 * @param key [IN] the key of which the value we are looking for.
 *
 * @return the associated value.
 * 
 */
const char *glite_conf_value(const char *key);

/**
 * \brief Get the value of GLITE_LOCATION. 
 *
 * @return the value of GLITE_LOCATION. Never returns NULL.
 */
const char *glite_location(void);

/** 
 * \brief Get the value of GLITE_LOCATION_VAR.
 *
 * @return the value of GLITE_LOCATION_VAR. 
 */
const char *glite_location_var(void);

/**
 * \brief Get the directory where logfiles should be stored.
 * 
 * @return the directory where logfiles should be stored.
 */   
const char *glite_location_log(void);

/** 
 * \brief Get the value of GLITE_TMP.
 *
 * @return the value of GLITE_TMP. Never returns NULL.
 */
const char *glite_tmp(void);

/** 
 * Get the value of any glite package.
 *
 * @return the value of GLITE_<pkg>_<var> from either the environment,
 * or from glite.conf. Returns NULL if the variable is not defined.
 */
const char *glite_pkg_var(const char *pkg, const char *var);

/**
 * \brief Get the full path of a configuration file.
 *
 * The function will first try
 * to look under $HOME/.glite/etc if look_in_home is TRUE, then under
 * $GLITE_LOCATION_VAR/etc, then under $GLITE_LOCATION/etc.
 *
 * @return the path of the first existing configuration file.
 * If no configuration file was found, NULL is returned.
 */
char *glite_config_file(const char *file, int look_in_home);

/**
 * \brief Parse an URI.
 *
 * Parses a string and creates the URI struct.
 * @param uri_str the URI to be parsed
 *
 * @return the properly parsed uri.
 */
glite_uri *glite_uri_new(const char *uri_str);

/** 
 * \brief Destroy a parsed URI.
 *
 * @param uri the existing URI.
 */
void glite_uri_free(glite_uri *uri);

/**
 * \brief Utility function: free a char *[] array.
 *
 * @param nitems number of items to be freed.
 * @param array the container.
 */
void glite_freeStringArray(int nitems, char *array[]);

/**
 * \brief Look up a service with a given type using Service Discovery.
 *
 * Checks wheter an appropriate service exists, and gives back its name.
 * @param type [IN] the type we are interested in. 
 * @param name [IN] the name of the service, or the service site or host name.
 * @param error [OUT] an array for the error messages.
 *
 * @return 
 */
char *glite_discover_service(const char *type, const char *name, char **error);
                                                                                                                                                      
                                                                                                                                                      
/**
 * \brief Look up a service with a given type using Service Discovery, using the
 * version as hint.
 * 
 * Checks wheter an appropriate service exists, and gives back its name. In case
 * multiple results are returned, select the one with the closest match with
 * the given interface version. The rule applied to find the closest is:
 * 1. same version 
 * 2. same major version number, greater minor 
 * 3. same major and minor, greater patch 
 * 4.(fallback) the first result returned.
 * @param type [IN] the type we are interested in. 
 * @param name [IN] the name of the service, or the service site or host name.
 * @param version [IN] the service interface version hint.
 * @param error [OUT] an array for the error messages.
 *
 * @return 
 */
char *glite_discover_service_by_version(const char *type, const char *name, const char * version, char **error);

/**
 * \brief Look up a service endpoint.
 *
 * Checks wheter an appropriate service exists, and gives back its endpoint.
 * @param type [IN] the type we are interested in. 
 * @param name [IN] the name of the service.
 * @param error [OUT] an array for the error messages.
 *
 * @return 
 */
char *glite_discover_endpoint(const char *type, const char *name, char **error);

/**
 * \brief Look up a service endpoint, using the
 * version as hint.
 *
 * Checks wheter an appropriate service exists, using the version as hint, and gives back its endpoint.
 * @param type [IN] the type we are interested in. 
 * @param name [IN] the name of the service, or it's site or host name
 * @param version [IN] the service interface version hint.
 * @param error [OUT] an array for the error messages.
 *
 * @return 
 */
char *glite_discover_endpoint_by_version(const char *type, const char *name, const char * version, char **error);

/**
 * \brief Query version of a service.
 * Checks wheter an appropriate service exists, and gives back its version.
 * @param type [IN] the type we are interested in. 
 * @param name [IN] the name of the service.
 * @param error [OUT] an array for the error messages.
 *
 * return the version of the service.
 * If the requested service doesn't exist, NULL is the returned.
 */
char *glite_discover_version(const char *type, const char *name, char **error);

/**
 * \brief Get a service which matches given type and data list
 * Gets a service which matches given type and data list
 * @param type [IN] Type of the service
 * @param datas [IN] List of requested datas
 * @param error [OUT] an array of the error messages.
 *
 * return the name of the service
 * If the requested service doesn't exist, NULL is the returned.
 */
char *glite_discover_service_by_data(const char *type, const SDServiceDataList *datas, char **error);

/**
 * \brief Compare service and tool interface versions.
 * Checks if service is usable.
 * @param client_ver [IN] version of tool interface.
 * @param service_ver [IN] version of service interface to use.
 *
 * return 0 if serive servsion doesn't make it possible to use the service
 */
int glite_check_versions(const char *client_ver, const char *service_ver);

/**
 * \brief Get details about a service
 * Checks wheter an appropriate service exists, and gives back its details.
 * @param type [IN] the type we are interested in. 
 * @param name [IN] the name of the service.
 * @param error [OUT] an array for the error messages.
 *
 * return the details about the service.
 * If the requested service doesn't exist, NULL is the returned.
 */
SDServiceDetails *glite_discover_getservicedetails(const char *type, const char *name, char **error);

/**
 * \brief Get details about a service, using the service interfcae version as hint.
 * Checks wheter an appropriate service exists, using the version as hint, and gives back its details.
 * @param type [IN] the type we are interested in. 
 * @param name [IN] the name of the service, or it's site or host name
 * @param version [IN] the service interface version hint.
 * @param error [OUT] an array for the error messages.
 *
 * return the details about the service.
 * If the requested service doesn't exist, NULL is the returned.
 */
SDServiceDetails *glite_discover_getservicedetails_by_version(const char *type, const char *name, const char *version, char **error);

/**
 * \brief Select the entry from a list using the version as hint.
 *
 * Select the entry that best matches the given version. If no match is found, the one with the 
 * same major number and greater minor number is returned. As fallback solution, the first
 * entry is returned.
 *
 * @return 
 */
const SDService * select_service_by_version(const SDServiceList * list, const char * version);

/**
 * \brief Returns the client VO list taken from a VOMS proxy.
 *
 * Checks the user's VOMS certificate (if available) and retrieves the list of VOs of the client.
 *
 * @return 
 */
SDVOList *check_voms_proxy(void);

/**
 * \brief Returns the client VO list taken from an environment variable.
 *
 * Checks the environment variable SD_VO taking the list of VOs from it.
 *
 * @return
 */
SDVOList *check_vo_env(void);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* GLITE_UTIL_H */
