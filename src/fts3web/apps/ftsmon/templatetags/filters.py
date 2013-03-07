from django import template

register = template.Library()

@register.filter
def partition(dictlist, key):
    array = []
    for item in dictlist:
        array.append(item[str(key)])
    return array


@register.simple_tag(takes_context = True)
def with_query(context, arg_name, arg_value):
	request = context['request']
	pre_existing = {}
	pre_existing.update(request.GET.iteritems())
	
	pre_existing[arg_name] = arg_value
	
	return '&'.join(map(lambda (x,y): "%s=%s" % (x, y), pre_existing.iteritems()))
