from django import template
import datetime
import types

register = template.Library()

@register.simple_tag
def filesize(size):
    if size is None:
        return 'Unknown'
    fsize = float(size)
    
    for suffix in ['bytes', 'KiB', 'MiB', 'GiB', 'TiB']:
        if fsize < 1024:
            return "%.2f %s" % (fsize, suffix)
        fsize = fsize / 1024.0
    
    return "%.2f PiB" % fsize
