from django import template
import settings

register = template.Library()

def _urlHref(baseUrl, path):
    return """<li><a href="%(base)s%(path)s">%(path)s</a></li>""" % {'base': baseUrl,
                                                                     'path': path}
@register.simple_tag
def urlLog(transfer):
    baseUrl = settings.LOG_BASE_URL.replace('%(host)', transfer.transferHost).strip('/')
    block = "<ul>\n" + _urlHref(baseUrl, transfer.log_file)
    if transfer.log_debug:
        block += _urlHref(baseUrl, transfer.log_file + ".debug")
    block += "\n</ul>"
    return block
