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
 *  gLite configuration query functions
 *
 *  Authors: Gabor Gombas <Gabor.Gombas@cern.ch>
 *  Version info: $Id: cfg.c,v 1.1 2005/06/08 14:03:34 rbritoda Exp $
 *  Release: $Name:  $
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "util.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <glib.h>

/**********************************************************************
 * Constants
 */

#ifndef G_OS_WIN32
#define DEFAULT_GLITE_LOCATION      "/opt/glite"
#define DEFAULT_GLITE_CONF      "/etc/glite.conf"
#else
/* XXX Check the values */
#define DEFAULT_GLITE_LOCATION      "c:\\opt\\glite"
#define DEFAULT_GLITE_CONF      "c:\\etc\\glite.conf"
#endif


/**********************************************************************
 * Global variables
 */

/* Parsed contents of /etc/glite.conf */
static GHashTable *glite_conf;


/**********************************************************************
 * Parsing glite.conf
 */

/* Parse /etc/glite.conf and populate the glite_conf hash table */
static void parse_glite_conf(void)
{
    static int initialized;
    char buf[1024], *p;
    const char *env;
    GString *path;
    FILE *fp;

    /* Try to parse /etc/glite.conf only once */
    if (initialized)
        return;
    initialized = 1;

    if (!glite_conf)
        glite_conf = g_hash_table_new_full(g_str_hash, g_str_equal,
                                           g_free, g_free);

    /* If GLITE_LOCATION is already set, use that */
    env = getenv(GLITE_LOCATION);
    if (env)
        {
            path = g_string_new(env);
            g_string_append(path, G_DIR_SEPARATOR_S "etc"
                            G_DIR_SEPARATOR_S "glite.conf");
        }
    else
        {
            const char *home;

            /* If GLITE_LOCATION is not set, try $HOME/.glite.conf first.
             * If that does not exist, fall back to /etc/glite.conf */
            home = g_get_home_dir();
            path = g_string_new(home);
            g_string_append(path, G_DIR_SEPARATOR_S ".glite.conf");
            if (access(path->str, R_OK))
                g_string_assign(path, DEFAULT_GLITE_CONF);
        }

    fp = fopen(path->str, "r");
    if (!fp)
        {
            /* XXX Info message that there is no glite.conf */
            g_string_free(path, 1);
            return;
        }
    /* XXX Info message that we are using glite.conf */

    while (fgets(buf, sizeof(buf), fp))
        {
            /* Skip comment lines */
            if (*buf == '#')
                continue;

            p = strchr(buf, '=');
            if (!p)
                continue; /* XXX Report the error */

            *p++ = '\0';
            g_hash_table_insert(glite_conf, g_strdup(buf), g_strdup(p));
        }
    fclose(fp);
    g_string_free(path, 1);
}

const char *glite_conf_value(const char *key)
{
    const char *result;

    parse_glite_conf();

    /* XXX Add support for registry lookup on Windows */

    /* Let the environment values override glite.conf */
    result = getenv(key);
    if (!result)
        result = (const char *) g_hash_table_lookup(glite_conf, key);
    return result;
}

const char *glite_location(void)
{
    const char *path;

    path = glite_conf_value(GLITE_LOCATION);
    if (!path)
        path = DEFAULT_GLITE_LOCATION;
    return path;
}
