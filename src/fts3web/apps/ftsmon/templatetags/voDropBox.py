from django import template
from fts3.models import Job

register = template.Library()

@register.simple_tag(takes_context=True)
def  voDropBox(context):
    request = context['request']
    if 'vo' in request.GET:
        selectedVO = request.GET['vo']
    else:
        selectedVO = None
    
    html  = '<div id="voDropBox">\n'
    html += '<form method="get" action="">'
    html += '<label for="voDropbox">Filter by VO: </label>'
    html += '<select id="voDropbox" name="vo" onchange="javascript:this.form.submit();">\n'
    html += '<option value="">- All -</option>'
    
    for v in Job.objects.values('vo_name').distinct().order_by('vo_name'):
        vo = v['vo_name']
        if selectedVO == vo:
            html += '<option selected="selected">%s</option>' % vo
        else:
            html += '<option>%s</option>' % vo

    html += '</select>\n'
    html += '</form>'
    html += '</div>\n'
    return html;
