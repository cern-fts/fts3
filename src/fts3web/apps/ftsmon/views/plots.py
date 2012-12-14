import os
import tempfile
os.environ['MPLCONFIGDIR'] = tempfile.mkdtemp()

from matplotlib.backends.backend_agg import FigureCanvasAgg as FigureCanvas
from matplotlib.figure import Figure
from django.http import HttpResponse



def strToArray(str):
    a = []
    for e in str.split(','):
        if e:
            a.append(e)
    return a



def error(httpRequest, msg = 'Error on plotting. Probably wrong query format.'):
    fig = Figure(figsize = (3, 3))
    canvas = FigureCanvas(fig)
    
    ax = fig.add_subplot(111)
    ax.text(0.5 , 0.5, msg,
            horizontalalignment='center',
            verticalalignment='center',
            transform = ax.transAxes)
    ax.set_axis_off()
    
    response = HttpResponse(content_type = 'image/png')
    fig.savefig(response, format='png')
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
               labels = strToArray(argv)
            elif arg == 'v':
                values = strToArray(argv)
            elif arg == 'c':
                colors = strToArray(argv)
                
        if not values:
            return error(httpRequest, 'No values')
        
        if sum(map(int, values)) == 0:
            return error(httpRequest, 'Total is 0')
                
        fig = Figure(figsize = (3,3))
        canvas = FigureCanvas(fig)
        
        ax = fig.add_subplot(111)
        ax.pie(values, labels = labels, colors = colors, autopct='%1.1f%%')
        if title:
            ax.set_title(title)
                
        response = HttpResponse(content_type = 'image/png')
        fig.savefig(response, format='png')
                
        return response
    except Exception, e:
        return error(httpRequest, str(e))
