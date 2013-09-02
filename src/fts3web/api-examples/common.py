import os
import pycurl
import urllib
import urlparse
from StringIO import StringIO


def _set_x509_creds(handle):
    """
    Set the certificate if available
    """
    if 'X509_USER_CERT' in os.environ:
        handle.setopt(pycurl.SSLCERT, os.environ['X509_USER_CERT'])
    if 'X509_USER_KEY' in os.environ:
        handle.setopt(pycurl.SSLKEY, os.environ['X509_USER_KEY'])


def _build_full_url(url, args):
    """
    Update url with the given query parameters that are passed by args
    """
    parsed = urlparse.urlparse(url)
    query = urlparse.parse_qs(parsed.query)
    for a in args:
        if args[a]:
            query[a] = args[a]
    query_str = urllib.urlencode(query)
    new_url = (parsed.scheme, parsed.netloc, parsed.path, parsed.params, query_str, parsed.fragment)
    return urlparse.urlunparse(new_url)


def get_url(url, **kwargs):
    """
    Get the content from an URL, and expand the extra arguments as query parameters
    """
    handle = pycurl.Curl()
    
    handle.setopt(pycurl.SSL_VERIFYPEER, False)
    _set_x509_creds(handle)
    
    buffer = StringIO()
    handle.setopt(pycurl.WRITEFUNCTION, buffer.write)
    
    handle.setopt(pycurl.FOLLOWLOCATION, True)
    handle.setopt(pycurl.URL, _build_full_url(url, kwargs))
    handle.perform()
    
    return buffer.getvalue()
