from django import template
import settings

register = template.Library()

@register.simple_tag
def getSetting(value):
    return getattr(settings, value)
