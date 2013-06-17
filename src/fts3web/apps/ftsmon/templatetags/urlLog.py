from django import template
import settings

register = template.Library()

def _transferLogEntry(baseUrl, path):
    return """<li><a href="%(base)s%(path)s">%(path)s</a></li>""" % {'base': baseUrl,
                                                                     'path': path}
@register.simple_tag
def urlTransferLog(transfer):
    if transfer.log_file:
        baseUrl = settings.LOG_BASE_URL.replace('%(host)', transfer.transferHost).strip('/')
        block = "<ul>\n" + _transferLogEntry(baseUrl, transfer.log_file)
        if transfer.log_debug:
            block += _transferLogEntry(baseUrl, transfer.log_file + ".debug")
        block += "\n</ul>"
        return block
    else:
        return "None"



@register.simple_tag
def urlServerLog(server):
    baseUrl = settings.LOG_BASE_URL.replace('%(host)', server['hostname']).strip('/')
    return baseUrl + '/var/log/fts3/fts3server.log'
