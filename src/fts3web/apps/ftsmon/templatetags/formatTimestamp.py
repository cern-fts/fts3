from django import template
import datetime
import types

register = template.Library()

@register.simple_tag
def formatTimestamp(timestamp):
    if timestamp is None or timestamp == -1:
        return "N/A"
    elif isinstance(timestamp, (int, long, float)):
        try:
            dt = datetime.datetime.fromtimestamp(timestamp)
            return dt.ctime()
        except:
            return "Invalid value: %s", str(timestamp)
    else:
        return "Invalid type: %s (%s)" % (type(timestamp).__name__, str(timestamp))
