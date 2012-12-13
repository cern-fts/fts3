import os
import tempfile
os.environ['MPLCONFIGDIR'] = tempfile.mkdtemp()

from matplotlib.backends.backend_agg import FigureCanvasAgg as FigureCanvas
from matplotlib.figure import Figure


from django.http import HttpResponse


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
                labels = argv.split(',')
            elif arg == 'v':
                values = argv.split(',')
            elif arg == 'c':
                colors = argv.split(',')
                
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
