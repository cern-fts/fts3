import os
from decorator import decorator
from django.core.exceptions import PermissionDenied


@decorator
def require_certificate(f, *args, **kwargs):
    dn = os.environ.get('SSL_CLIENT_S_DN', None)
    if not dn:
        raise PermissionDenied()
    return f(*args, **kwargs)
