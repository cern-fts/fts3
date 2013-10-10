# Copyright notice:
# Copyright (C) Members of the EMI Collaboration, 2010.
#
# See www.eu-emi.eu for details on the copyright holders
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import colorsys
import os
import tempfile
os.environ['MPLCONFIGDIR'] = tempfile.mkdtemp()

from matplotlib.backends.backend_agg import FigureCanvasAgg as FigureCanvas
from matplotlib.figure import Figure
from matplotlib.font_manager import FontProperties
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


def _tetrad(h, s, v):
    t = [((0.25 * x + h), s, v) for x in range(4)]
    return t


def _generateColors(number):
    if number <= 0:
        return []
    
    # Starting hue
    hStart = 0.61
    
    # Append tetrads displacing the start depending on how many
    # elements we have
    dispNumber = number / 4
    if number % 4 > 0:
        dispNumber = dispNumber + 1
    dispRange  = 0.25
    dispStep   = dispRange / float(dispNumber)
    HSV_tuples = []
    for i in range(dispNumber):
        h = hStart + dispStep * i
        HSV_tuples.extend(_tetrad(h, 0.75, 0.85))
    
    RGB_tuples = map(lambda hsv: colorsys.hsv_to_rgb(*hsv), HSV_tuples)
    RGB_255_tuples = map(lambda rgb: (rgb[0] * 255, rgb[1] * 255, rgb[2] * 255), RGB_tuples)
    WEB_tuples = map(lambda rgb: "%02X%02X%02X" % rgb, RGB_255_tuples)
    
    return WEB_tuples


def pie(httpRequest):
    try:
        labels = []
        values = []
        colors = None
        title  = None
        
        for (arg, argv) in httpRequest.GET.iteritems():
            if arg == 't':
                title = argv
            elif arg == 'l':
               labels = strToArray(argv)
            elif arg == 'v':
                values = strToArray(argv)
            elif arg == 'c':
                cx = strToArray(argv)
                if len(cx):
                    colors = cx
                    
        # Truncate values if there are more
        if len(labels) < len(values):
            values = values[:len(labels)]
            
        # Extend if there are less
        if len(labels) > len(values):
            values.extend([values[-1]] * (len(labels) - len(values)))
        
        # Generate color list if not specified
        if not colors:
            colors = _generateColors(len(labels))

        # Append # if the color is a full RGB                    
        for c in range(len(colors)):
            if len(colors[c]) > 1:
                colors[c] = '#' + colors[c]
                
        if not values:
            return error(httpRequest, 'No values')
        
        if sum(map(int, values)) == 0:
            return error(httpRequest, 'Total is 0')
                
        fig = Figure(figsize = (6,3))
        canvas = FigureCanvas(fig)
        
        ax = fig.add_subplot(1,2,1)
        (patches, texts, auto) = ax.pie(values, labels=None, colors=colors, autopct='%1.1f%%')
        if title:
            ax.set_title(title)
            
        for p in patches:
            p.set_edgecolor('white')
        
        ax2 = fig.add_subplot(1,2,2)
        fontP = FontProperties()
        fontP.set_size('small')
        ax2.legend(patches, labels, loc='center left', prop = fontP)
        ax2.set_axis_off()

        response = HttpResponse(content_type = 'image/png')
        fig.savefig(response, format='png', bbox_inches = 'tight', transparent = True)
                
        return response
    except Exception, e:
        return error(httpRequest, str(e))
