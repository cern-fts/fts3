from django import template

register = template.Library()

@register.filter
def partition(dictlist, key):
    array = []
    for item in dictlist:
        array.append(item[str(key)])
    return array
