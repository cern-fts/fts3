import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as pyplot
from django.http import HttpResponse


def error(httpRequest, msg = 'Error on plotting. Probably wrong query format.'):
    fig = pyplot.figure(1, figsize=(3,3))
    pyplot.clf()
    
    ax = fig.add_axes([0, 0, 1, 1])
    pyplot.text(0.5 , 0.5, msg,
                horizontalalignment='center',
                verticalalignment='center',
                transform = ax.transAxes)
    ax.set_axis_off()
    
    response = HttpResponse(content_type = 'image/png')
    pyplot.savefig(response, format='png')
    return response


def pie(httpRequest):
    try:   
        labels = []
        values = []
        colors = ['b', 'g', 'r', 'c', 'm', 'y', 'k', 'w']
        title  = None
        
        for (arg, argv) in httpRequest.GET.iteritems():
            if arg == 't':
                title = argv
            elif arg == 'l':
                labels = argv.split(',')
            elif arg == 'v':
                values = argv.split(',')
            elif arg == 'c':
                colors = argv.split(',')
        
        pyplot.figure(1, figsize=(3,3))
        pyplot.clf()    
        pyplot.pie(values, labels = labels, colors = colors, autopct='%1.1f%%')
        if title:
            pyplot.title(title)
        
        response = HttpResponse(content_type = 'image/png')
        pyplot.savefig(response, format='png')        
        return response
    except Exception, e:
        return error(httpRequest, str(e))
