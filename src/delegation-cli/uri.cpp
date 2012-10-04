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
 *  GLite Utilities - URI parsing
 *
 *  Authors: Gabor Gombas <Gabor.Gombas@cern.ch>
 *  Version info: $Id: uri.c,v 1.1 2005/06/08 14:03:34 rbritoda Exp $
 *  Release: $Name:  $
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "util.h"
#include <string.h>
#include <glib.h>

/**********************************************************************
 * URI parsing
 */

/* Handle lfn://<host><path>?lfn=<lfn>, so that uri->endpoint will contain
 * <host><path>, and uri->path will contain <lfn> */
static void fixup_lfn_uri(glite_uri *uri)
{
	char *p, *path, *start;
	int len;

	if (!uri->endpoint || !uri->query)
		return;

	if (!strncasecmp(uri->query, "lfn=", 4))
		start = uri->query;
	else
		start = strstr(uri->query, "&lfn=");
	if (!start)
		/* No lfn parameter */
		return;

	/* Concatenate the old endpoint and path */
	if (uri->path)
	{
		if (uri->endpoint)
		{
			uri->endpoint = (char *) g_realloc(uri->endpoint,
				strlen(uri->endpoint) + strlen(uri->path) + 1);
			strcat(uri->endpoint, uri->path);
			g_free(uri->path);
		}
		else
			uri->endpoint = uri->path;
		uri->path = NULL;
	}

	/* Look for other query parts */
	path = strchr(start, '=') + 1;
	p = strchr(path, '&');
	if (p)
		len = p - path;
	else
		len = strlen(path);
	uri->path = g_strndup(path, static_cast<gsize>(len));

	/* Remove the "lfn=..." part from the query */
	memmove(start, path + len, strlen(path) - static_cast<size_t>(len));
	if (!*uri->query)
	{
		/* If nothing remains, set uri->query to NULL */
		g_free(uri->query);
		uri->query = NULL;
	}

	/* XXX Perform URL-decoding for uri->path? */
}

glite_uri *glite_uri_new(const char *uri_str)
{
	glite_uri *uri;
	char *p;

	uri = g_new0(glite_uri, 1);

	/* Look for the scheme */
	p = (char *) strchr(uri_str, ':');
	if (p)
	{
		uri->scheme = g_ascii_strdown(uri_str, p - uri_str);
		uri_str = p + 1;

		/* If "//" follows the ':', then this is a hierarchical URI */
		if (uri_str[0] == '/' && uri_str[1] == '/')
		{
			uri->hierarchical = 1;
			uri_str += 2;
		}
	}

	/* Look for a host specification. It is only valid after a scheme */
	if (uri->scheme)
	{
		p = (char *) strchr(uri_str, '/');
		if (p && p != uri_str)
		{
			/* <scheme>://<host><path> */
			uri->endpoint = g_strndup(uri_str, static_cast<gsize>(p - uri_str));
			/* The '/' is part of <path> */
			uri_str = p;
		}
	}

	/* Look for a query component, but only if we have <scheme> and
	 * <host> */
	if (uri->scheme && uri->endpoint && (p = (char *) strchr(uri_str, '?')))
	{
		uri->path = g_strndup(uri_str, static_cast<gsize>(p - uri_str));
		uri->query = g_strdup(p + 1);
	}
	else
		uri->path = g_strdup(uri_str);

	/* LFN-specific fixup */
	if (uri->scheme && !strcmp(uri->scheme, GLITE_URI_LFN))
		fixup_lfn_uri(uri);

	return uri;
}

void glite_uri_free(glite_uri *uri)
{
	if (!uri)
		return;

	g_free(uri->scheme);
	g_free(uri->endpoint);
	g_free(uri->path);
	g_free(uri);
}
